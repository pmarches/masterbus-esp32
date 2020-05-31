#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
//#include "cmd_decl.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <masterbusController.h>
#include <masterbusConsole.h>

extern "C" {
	#include <axp20x.h>
	#include <i2c_driver.h>
}

int hex2int(char input)
{
  if(input >= '0' && input <= '9')
    return input - '0';
  if(input >= 'A' && input <= 'F')
    return input - 'A' + 10;
  if(input >= 'a' && input <= 'f')
    return input - 'a' + 10;

  return 0;
}

void registerCanRx();
void registerCanSend();
void registerCanlisten();
void registerCanMonitor();
void registerCanFwd();
void registerMastervoltParse();
void registerMvMonitor();

MasterbusController* g_mbctl[2];

static void initialize_console(void) {
    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    const uart_config_t uart_config = {
            .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
			.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
			.rx_flow_ctrl_thresh = 0,
            .use_ref_tick = true,
    };
    ESP_ERROR_CHECK( uart_param_config((uart_port_t) CONFIG_ESP_CONSOLE_UART_NUM, &uart_config) );

    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK( uart_driver_install((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM,
            256, 0, 0, NULL, 0) );

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {
            .max_cmdline_length = 256,
            .max_cmdline_args = 8,
            .hint_color = 0,
			.hint_bold = 0,
    };
    ESP_ERROR_CHECK( esp_console_init(&console_config) );

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(100);
}

SPIBus spi(HSPI_HOST);
AXP20X_t axp192;

void initPowerSupply(){
	i2c_driver_t i2cDriver;
	i2c_driver_init(&i2cDriver, 21, 22);
	AXP20X_begin(&axp192, 0x34);
	AXP20X_adc1Enable(&axp192, 0xff, true);
	AXP20X_enableCoulombCounter(&axp192, true);

	AXP20X_setDCDC1Voltage(&axp192, 3300);
	AXP20X_setDCDC2Voltage(&axp192, 3300);
	AXP20X_setDCDC3Voltage(&axp192, 3300);
	ESP_ERROR_CHECK(AXP20X_setPowerOutPut(&axp192, 1, true));
	ESP_ERROR_CHECK(AXP20X_setPowerOutPut(&axp192, 2, true));
	ESP_ERROR_CHECK(AXP20X_setPowerOutPut(&axp192, 3, true));
}

#define SPI_CLK_PIN (gpio_num_t)25
#define SPI_MASTER_OUT_SLAVE_IN_PIN (gpio_num_t)14
#define SPI_MASTER_IN_SLAVE_OUT_PIN (gpio_num_t)13
#define SPI_CS_MCP2515RX_PIN (gpio_num_t)2
#define SPI_CS_MCP2515TX_PIN (gpio_num_t)23
void initializeMasterbus(){
	initPowerSupply();

	spi.init(SPI_MASTER_OUT_SLAVE_IN_PIN, SPI_MASTER_IN_SLAVE_OUT_PIN, SPI_CLK_PIN);
	SPIDevice* spiRx=new SPIDevice(&spi, SPI_CS_MCP2515RX_PIN);
	spiRx->init();
	MCP2515Class* mcp2515Rx=new MCP2515Class(spiRx, SPI_CS_MCP2515RX_PIN, (gpio_num_t) 33, 8E6);
	g_mbctl[0]=new MasterbusController(mcp2515Rx);
    g_mbctl[0]->configure(MCP2515Class::NORMAL_MODE);

	SPIDevice* spiTx=new SPIDevice(&spi, SPI_CS_MCP2515TX_PIN);
	spiTx->init();
	MCP2515Class* mcp2515Tx=new MCP2515Class(spiTx, SPI_CS_MCP2515TX_PIN, (gpio_num_t) 32, 8E6);
	g_mbctl[1]=new MasterbusController(mcp2515Tx);
    g_mbctl[1]->configure(MCP2515Class::NORMAL_MODE);
}

extern "C" void app_main(void) {
	esp_log_level_set("spi_master", ESP_LOG_ERROR);
    initialize_console();

    const char* prompt = LOG_COLOR_I "masterbus> " LOG_RESET_COLOR;
    /* Figure out if the terminal supports escape sequences */
    int probe_status = linenoiseProbe();
    if (probe_status) { /* zero indicates success */
        printf("\n"
               "Your terminal application does not support escape sequences.\n"
               "Line editing and history features are disabled.\n"
               "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
        /* Since the terminal doesn't support escape sequences,
         * don't use color codes in the prompt.
         */
        prompt = "masterbus> ";
    }
    registerCanSend();
    registerCanlisten();
    registerCanMonitor();
    registerCanRx();
    registerCanFwd();
    registerMastervoltParse();
    registerMvMonitor();

    initializeMasterbus();

    while(true) {
        /* Get a line using linenoise.
         * The line is returned when ENTER is pressed.
         */
        char* line = linenoise(prompt);
        if (line == NULL) { /* Ignore empty lines */
            continue;
        }
        /* Add the command to the history */
        linenoiseHistoryAdd(line);

        /* Try to run the command */
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        } else if (err == ESP_ERR_INVALID_ARG) {
            // command was empty
        } else if (err == ESP_OK && ret != ESP_OK) {
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
        } else if (err != ESP_OK) {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }
        /* linenoise allocates line buffer on the heap, so need to free it */
        linenoiseFree(line);
    }

}
