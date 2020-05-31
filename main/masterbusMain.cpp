#include "driver/spi_master.h"
#include <SPI.h>
#include <GPIO.h>

#include <freertos/task.h>
#include <esp_log.h>

extern "C" {
#include <i2c_driver.h>
#include <axp20x.h>
}

#include <mcp_can_dfs.h>

#define SPI_CLK_PIN 25
#define SPI_MASTER_OUT_SLAVE_IN_PIN 14
#define SPI_MASTER_IN_SLAVE_OUT_PIN 13
#define SPI_CS_MCP2515RX_PIN 2

#define tag "KOLBANSAMPLE"

void initPowerSupply(){
	i2c_driver_t i2cDriver;
	i2c_driver_init(&i2cDriver, 21, 22);
	AXP20X_t axp192;
	AXP20X_begin(&axp192, 0x34);
	AXP20X_adc1Enable(&axp192, 0xff, true);
	AXP20X_enableCoulombCounter(&axp192, true);

	AXP20X_setDCDC1Voltage(&axp192, 3300);
	AXP20X_setDCDC2Voltage(&axp192, 3300);
	const int GPS_POWER_CHANNEL=3;
	AXP20X_setPowerOutPut(&axp192, GPS_POWER_CHANNEL, false);
}

#include <driver/spi_master.h>
void kolbanSample(){
	ESP_LOGD(tag, ">> test_spi_task");
	spi_bus_config_t bus_config;
	bus_config.flags=0;
	bus_config.intr_flags=0;
	bus_config.sclk_io_num = SPI_CLK_PIN; // CLK
	bus_config.mosi_io_num = SPI_MASTER_OUT_SLAVE_IN_PIN; // MOSI
	bus_config.miso_io_num = SPI_MASTER_IN_SLAVE_OUT_PIN; // MISO
	bus_config.quadwp_io_num = -1; // Not used
	bus_config.quadhd_io_num = -1; // Not used
	ESP_LOGI(tag, "... Initializing bus.");
	ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &bus_config, 0));
	spi_device_handle_t handle;
	spi_device_interface_config_t dev_config;
	dev_config.address_bits = 0;
	dev_config.command_bits = 0;
	dev_config.dummy_bits = 0;
	dev_config.mode = 0;
	dev_config.duty_cycle_pos = 0;
	dev_config.cs_ena_posttrans = 0;
	dev_config.cs_ena_pretrans = 0;
	dev_config.clock_speed_hz = 10000;
	dev_config.spics_io_num = SPI_CS_MCP2515RX_PIN;
	dev_config.flags = 0;
	dev_config.queue_size = 1;
	dev_config.pre_cb = NULL;
	dev_config.post_cb = NULL;
	ESP_LOGI(tag, "... Adding device bus.");
	ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &dev_config, &handle));

	while(true){
		char data[3];
		spi_transaction_t
		trans_desc;
		trans_desc.addr = 0;
		trans_desc.cmd= 0;
		trans_desc.flags = 0;
		trans_desc.length = 3 * 8;
		trans_desc.rxlength = 0;
		trans_desc.tx_buffer = data;
		trans_desc.rx_buffer = data;
		data[0] = 0x12;
		data[1] = 0x34;
		data[2] = 0x56;
		ESP_LOGI(tag, "... Transmitting.");
		ESP_ERROR_CHECK(spi_device_transmit(handle, &trans_desc));

		vTaskDelay(100);
	}
	ESP_LOGI(tag, "... Removing device.");
	ESP_ERROR_CHECK(spi_bus_remove_device(handle));
	ESP_LOGI(tag, "... Freeing bus.");
	ESP_ERROR_CHECK(spi_bus_free(HSPI_HOST));
	ESP_LOGD(tag, "<< test_spi_task");
	vTaskDelete(NULL);
}

#define MCP_READ_STATUS 0xA0
#define MCP_RX_STATUS   0xB0
#define MCP_RESET       0xC0

void makeSimple1HzTest(gpio_num_t testPin){
	gpio_pad_select_gpio(testPin);
	ESP32CPP::GPIO::setOutput(testPin);
	while(true){
		ESP32CPP::GPIO::high(testPin);
		vTaskDelay(1);
		ESP32CPP::GPIO::low(testPin);
		vTaskDelay(1);
	}
}

void prepareMCPRead(uint8_t* canBusDataToSend, uint32_t* canBusDataToSendLen){
	*canBusDataToSend=0b00001010;
	*canBusDataToSendLen=1;
}

void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc=(int)t->user;
	ESP_LOGD(__FUNCTION__, "DC=%d", dc);
    gpio_set_level((gpio_num_t) SPI_CS_MCP2515RX_PIN, dc);
}

spi_device_handle_t spiDeviceHandle;
void initSPI(){
    esp_err_t ret;
    spi_bus_config_t buscfg={
        .mosi_io_num=SPI_MASTER_OUT_SLAVE_IN_PIN,
        .miso_io_num=SPI_MASTER_IN_SLAVE_OUT_PIN,
        .sclk_io_num=SPI_CLK_PIN,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=320*2+8,
		.flags=0,
		.intr_flags=0,
    };
    spi_device_interface_config_t devcfg={
    		.command_bits=0,
			.address_bits=0,
			.dummy_bits=0,
			.mode=0,
			.duty_cycle_pos=0,
			.cs_ena_pretrans=0,
			.cs_ena_posttrans=0,
        .clock_speed_hz=10*1000*1000,           //Clock out at 10 MHz
		.input_delay_ns=0,
        .spics_io_num=(gpio_num_t) SPI_CS_MCP2515RX_PIN,               //CS pin
		.flags=0,
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
        .pre_cb=lcd_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
		.post_cb=0,
    };
    //Initialize the SPI bus
    ret=spi_bus_initialize(HSPI_HOST, &buscfg, HSPI_HOST);
    ESP_ERROR_CHECK(ret);
    //Attach the LCD to the SPI bus
    ret=spi_bus_add_device(HSPI_HOST, &devcfg, &spiDeviceHandle);
    ESP_ERROR_CHECK(ret);
    ESP_LOGD(__FUNCTION__, "SPI Device handle is %p", spiDeviceHandle);
}

void spiTransmit(uint8_t* spiDataToSend, uint32_t spiDataToSendLen){
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=8*spiDataToSendLen;                     //Command is 8 bits
    t.tx_buffer=spiDataToSend;               //The data is the cmd itself

    t.rxlength=t.length;
    t.rx_buffer=spiDataToSend;
    t.user=(void*)0;                //D/C needs to be set to 0
    ret=spi_device_polling_transmit(spiDeviceHandle, &t);  //Transmit!
    vTaskDelay(50);
    assert(ret==ESP_OK);            //Should have had no issues.
}

#define MCP_WRITE       0x02
#define MCP_READ        0x03
#define MCP_BITMOD      0x05
#define MCP_LOAD_TX0    0x40
#define MCP_LOAD_TX1    0x42
#define MCP_LOAD_TX2    0x44

SPI spi;

void mcp2515_setRegister(const uint8_t address, const uint8_t value) {
	ESP32CPP::GPIO::low((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Release slave
    spi.transferByte(MCP_WRITE);
    spi.transferByte(address);
    spi.transferByte(value);
	ESP32CPP::GPIO::high((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Release slave
}

void mcp2515_initCANBuffers(void) {
    uint8_t i, a1, a2, a3;

    a1 = MCP_TXB0CTRL;
    a2 = MCP_TXB1CTRL;
    a3 = MCP_TXB2CTRL;
    for (i = 0; i < 14; i++) {                       // in-buffer loop
        mcp2515_setRegister(a1, 0);
        mcp2515_setRegister(a2, 0);
        mcp2515_setRegister(a3, 0);
        a1++;
        a2++;
        a3++;
    }
    mcp2515_setRegister(MCP_RXB0CTRL, 0);
    mcp2515_setRegister(MCP_RXB1CTRL, 0);
}

uint8_t mcp2515_readRegister(const uint8_t address) {
    uint8_t ret;

	ESP32CPP::GPIO::low((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Release slave
    spi.transferByte(MCP_READ);
    spi.transferByte(address);
    ret = spi.transferByte(0x00);
	ESP32CPP::GPIO::high((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Release slave

    return ret;
}

uint8_t getMode() {
    return mcp2515_readRegister(MCP_CANSTAT) & MODE_MASK;
}

void configureOutputPin(gpio_num_t pin){
	gpio_pad_select_gpio(pin);
	ESP32CPP::GPIO::setOutput(pin);
}

void loopOnCS(){
	while(true){
		ESP_LOGD(__FUNCTION__, "Going low");
		ESP32CPP::GPIO::low((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Select slave
		vTaskDelay(50);
		ESP_LOGD(__FUNCTION__, "Going high");
		ESP32CPP::GPIO::high((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Release slave
		vTaskDelay(50);
	}
}

extern "C" void app_main(void)
{
#if 0
//	kolbanSample();
	makeSimple1HzTest((gpio_num_t) SPI_MASTER_IN_SLAVE_OUT_PIN);
#else
//	initPowerSupply();
	configureOutputPin((gpio_num_t)SPI_CLK_PIN);
	configureOutputPin((gpio_num_t)SPI_MASTER_OUT_SLAVE_IN_PIN);
//	ESP32CPP::GPIO::setInput((gpio_num_t)SPI_MASTER_IN_SLAVE_OUT_PIN);
	configureOutputPin((gpio_num_t)SPI_CS_MCP2515RX_PIN);

	ESP32CPP::GPIO::setOutput((gpio_num_t)SPI_CS_MCP2515RX_PIN);
	ESP32CPP::GPIO::high((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Release slave
	ESP_LOGD(__FUNCTION__, "Ready to SPI");

	uint32_t canBusDataToSendLen=200;
	uint8_t* canBusDataToSend=(uint8_t*)malloc(canBusDataToSendLen);
	memset(canBusDataToSend, 0, canBusDataToSendLen);
	canBusDataToSend[0]=0x0;
	canBusDataToSend[1]=0xff;
#if 0
	initSPI();
	canBusDataToSend[0]=MCP_RESET;
	canBusDataToSendLen=1;
	spiTransmit(canBusDataToSend, canBusDataToSendLen);
#else
	initPowerSupply();
	spi.setHost(HSPI_HOST);
	spi.init(SPI_MASTER_OUT_SLAVE_IN_PIN, SPI_MASTER_IN_SLAVE_OUT_PIN, SPI_CLK_PIN, SPI_CS_MCP2515RX_PIN);

	ESP32CPP::GPIO::low((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Select slave
	spi.transferByte(MCP_RESET);
	ESP32CPP::GPIO::high((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Release slave

	while(true){
		ESP32CPP::GPIO::low((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Select slave
		spi.transfer(canBusDataToSend, 2);
//		spi.transferByte(0xff);
		ESP32CPP::GPIO::high((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Release slave
		vTaskDelay(1);
	}
//	mcp2515_initCANBuffers();
#endif

	while(true){
//		ESP_LOGD(__FUNCTION__, "Got mode :%d ", getMode());
#if 1
		canBusDataToSendLen=200;
//		prepareMCPRead(canBusDataToSend, &canBusDataToSendLen);
		ESP_LOGD(__FUNCTION__, "Sending to slave this:");
        ESP_LOG_BUFFER_HEX_LEVEL(__FUNCTION__, canBusDataToSend, canBusDataToSendLen, ESP_LOG_DEBUG);
		ESP32CPP::GPIO::low((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Select slave
#if 0
		spiTransmit(canBusDataToSend, canBusDataToSendLen);
#else
		spi.transfer(canBusDataToSend, canBusDataToSendLen);
#endif
		ESP_LOGD(__FUNCTION__, "got this in response");
        ESP_LOG_BUFFER_HEX_LEVEL(__FUNCTION__, canBusDataToSend, canBusDataToSendLen, ESP_LOG_DEBUG);
		ESP32CPP::GPIO::high((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Release slave
#endif
		vTaskDelay(300);
	}
#endif
}

