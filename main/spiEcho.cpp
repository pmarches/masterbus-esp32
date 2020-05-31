#include "driver/spi_master.h"
#include <SPI.h>
#include <GPIO.h>

#include <freertos/task.h>
#include <esp_log.h>

#define SPI_CLK_PIN 25
#define SPI_MASTER_OUT_SLAVE_IN_PIN 14
#define SPI_MASTER_IN_SLAVE_OUT_PIN 13
#define SPI_CS_MCP2515RX_PIN 2

void configureOutputPin(gpio_num_t pin){
	gpio_pad_select_gpio(pin);
	ESP32CPP::GPIO::setOutput(pin);
}

SPI spi;

#define OP_WRITE 0x02
#define OP_READ 0x03
#define OP_MODIFY_BITS 0x05
#define OP_RESET 0b11000000

#define REG_BFPCTRL                0x0c
#define REG_TXRTSCTRL              0x0d

#define REG_CANSTAT                0x0e
#define REG_CANCTRL                0x0f

#define REG_CNF3                   0x28
#define REG_CNF2                   0x29
#define REG_CNF1                   0x2a

#define REG_CANINTE                0x2b
#define REG_CANINTF                0x2c

#define FLAG_RXnIE(n)              (0x01 << n)
#define FLAG_RXnIF(n)              (0x01 << n)
#define FLAG_TXnIF(n)              (0x04 << n)

#define REG_RXFnSIDH(n)            (0x00 + (n * 4))
#define REG_RXFnSIDL(n)            (0x01 + (n * 4))
#define REG_RXFnEID8(n)            (0x02 + (n * 4))
#define REG_RXFnEID0(n)            (0x03 + (n * 4))

#define REG_TX_ERROR_COUNTER 0x1C
#define REG_RX_ERROR_COUNTER 0x1D

#define REG_RXMnSIDH(n)            (0x20 + (n * 0x04))
#define REG_RXMnSIDL(n)            (0x21 + (n * 0x04))
#define REG_RXMnEID8(n)            (0x22 + (n * 0x04))
#define REG_RXMnEID0(n)            (0x23 + (n * 0x04))

#define REG_TXBnCTRL(n)            (0x30 + (n * 0x10))
#define REG_TXBnSIDH(n)            (0x31 + (n * 0x10))
#define REG_TXBnSIDL(n)            (0x32 + (n * 0x10))
#define REG_TXBnEID8(n)            (0x33 + (n * 0x10))
#define REG_TXBnEID0(n)            (0x34 + (n * 0x10))
#define REG_TXBnDLC(n)             (0x35 + (n * 0x10))
#define REG_TXBnD0(n)              (0x36 + (n * 0x10))

#define REG_RXBnCTRL(n)            (0x60 + (n * 0x10))
#define REG_RXBnSIDH(n)            (0x61 + (n * 0x10))
#define REG_RXBnSIDL(n)            (0x62 + (n * 0x10))
#define REG_RXBnEID8(n)            (0x63 + (n * 0x10))
#define REG_RXBnEID0(n)            (0x64 + (n * 0x10))
#define REG_RXBnDLC(n)             (0x65 + (n * 0x10))
#define REG_RXBnD0(n)              (0x66 + (n * 0x10))

#define FLAG_IDE                   0x08
#define FLAG_SRR                   0x10
#define FLAG_RTR                   0x40
#define FLAG_EXIDE                 0x08

#define FLAG_RXM0                  0x20
#define FLAG_RXM1                  0x40

void echoOneLineOverSPI(){
	char c;
	ESP32CPP::GPIO::low((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Select slave
	for (const char *p = "SPI TEST! We are online over SPI\n\r" ; (c = *p); p++)  {
		char returnChr = spi.transferByte(c);
		printf("%c", returnChr); // MOSI MISO Link sends echo back
	}
	ESP32CPP::GPIO::high((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Release slave
}

class MCP2515OverSPI {
public:
	uint8_t* spiSequence;
	enum MODE_OF_OPERATION {
		CONFIGURATION_MODE=0b10000000,
		SLEEP_MODE=0b00100000,
		LOOPBACK_MODE=0b01000000,
		LISTEN_ONLY_MODE=0b01100000,
		NORMAL_MODE=0b00000000,
	};

	MCP2515OverSPI(){
		spiSequence=(uint8_t*)heap_caps_malloc(10, MALLOC_CAP_DMA);
	}

	uint8_t init(MODE_OF_OPERATION finalMode){
		reset();
		setMode(CONFIGURATION_MODE);
		disableClockOutput();

//		uint8_t defaultState=readRegister(REG_CANCTRL); //FIXME This check needs work.
//		if(defaultState!=0x80){
//			ESP_LOGE(__FUNCTION__, "After reset the MCP2515 has an unexpected default state of 0x%x", defaultState);
//			return ESP_FAIL;
//		}
//		ESP_LOGD(__FUNCTION__, "After reset the MCP2515 has correct state of 0x%x", defaultState);

		//Select baud rate here...
//		uint8_t cnf[]={ 0x03, 0xb4, 0x06 };
//		writeRegister(REG_CNF1, cnf[0]);
//		writeRegister(REG_CNF2, cnf[1]);
//		writeRegister(REG_CNF3, cnf[2]);

		writeRegister(REG_CANINTE, FLAG_RXnIE(1) | FLAG_RXnIE(0));
		writeRegister(REG_BFPCTRL, 0x00);
		writeRegister(REG_TXRTSCTRL, 0x00);
		writeRegister(REG_RXBnCTRL(0), FLAG_RXM1 | FLAG_RXM0);
		writeRegister(REG_RXBnCTRL(1), FLAG_RXM1 | FLAG_RXM0);

		setMode(finalMode);
		uint8_t defaultState=readRegister(REG_CANCTRL);
		if (defaultState != finalMode) { //FIXME This check needs work, some other bits might be set
			ESP_LOGE(__FUNCTION__, "After configuration, the MCP2515 has an unexpected state of 0x%x", defaultState);
			return ESP_FAIL;
		}

		return ESP_OK;
	}

	void reset(){
		ESP_LOGD(__FUNCTION__, "Sending MCP2515 reset");
		ESP32CPP::GPIO::low((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Select slave
//		uint8_t spiSequence[]={OP_RESET};
		spiSequence[0]=OP_RESET;
		spi.transfer(spiSequence, sizeof(spiSequence));
		vTaskDelay(10);
		ESP32CPP::GPIO::high((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Release slave
	}

	void disableClockOutput(){
		writeBitsToRegister(REG_CANCTRL, 0b111, 0);
	}
	/**
	 * The mask byte determines which bits in the register will be allowed to change. A ‘1’ in the mask byte will allow a bit in the register to change, while a ‘0’ will not.
	 */
	void writeBitsToRegister(uint8_t registerAddressToWrite, uint8_t changeMask, uint8_t newBits){
		ESP32CPP::GPIO::low((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Select slave
//		uint8_t spiSequence[]={OP_MODIFY_BITS, registerAddressToWrite, changeMask, newBits};
		spiSequence[0]=OP_MODIFY_BITS;
		spiSequence[1]=registerAddressToWrite;
		spiSequence[2]=changeMask;
		spiSequence[3]=newBits;
		spi.transfer(spiSequence, 4);
		ESP32CPP::GPIO::high((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Release slave
	}


	void writeRegister(uint8_t registerAddressToWrite, uint8_t newValue){
		ESP32CPP::GPIO::low((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Select slave
//		uint8_t spiSequence[]={OP_WRITE, registerAddressToWrite, newValue};
		spiSequence[0]=OP_WRITE;
		spiSequence[1]=registerAddressToWrite;
		spiSequence[2]=newValue;
		spi.transfer(spiSequence, sizeof(spiSequence));
		ESP32CPP::GPIO::high((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Release slave
	}

	uint8_t readRegister(uint8_t registerAddressToRead){
		ESP32CPP::GPIO::low((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Select slave
//		uint8_t spiSequence[]={OP_READ, registerAddressToRead, 0xFF}; //The third byte is where the response will end up
		spiSequence[0]=OP_READ; spiSequence[1]=registerAddressToRead; spiSequence[2]=0xFF;

		spi.transfer(spiSequence, 3);
		ESP32CPP::GPIO::high((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Release slave
		return spiSequence[2];
	}

	void setMode(uint8_t newMode){
		writeBitsToRegister(REG_CANCTRL, 0b11100000, newMode);
		ESP_LOGD(__FUNCTION__, "new CANCTRL state is %x", readRegister(REG_CANCTRL));
	}

	void readStatus(){
		uint8_t status=readRegister(REG_CANSTAT);
		ESP_LOGD(__FUNCTION__, "Status %x", status);
	}

	void readCANInterruptFlag(){
		uint8_t status=readRegister(REG_CANINTF);
		ESP_LOGD(__FUNCTION__, "Status %x", status);
		ESP_LOGD(__FUNCTION__, "	Message Error Interrupt Flag bit: %d", status&0b10000000);
		ESP_LOGD(__FUNCTION__, "	Wake up Interrupt Flag bit: %d", status&0b01000000);
		ESP_LOGD(__FUNCTION__, "	Error interrupt Flag bit: %d", status&0b00100000);
		ESP_LOGD(__FUNCTION__, "	Transmit Buffer 2 Empty interrupt Flag bit: %d", status&0b00010000);
		ESP_LOGD(__FUNCTION__, "	Transmit Buffer 1 Empty interrupt Flag bit: %d", status&0b00001000);
		ESP_LOGD(__FUNCTION__, "	Transmit Buffer 0 Empty interrupt Flag bit: %d", status&0b00000100);
		ESP_LOGD(__FUNCTION__, "	Receive Buffer 1 Empty interrupt Flag bit: %d", status&0b00000010);
		ESP_LOGD(__FUNCTION__, "	Receive Buffer 0 Empty interrupt Flag bit: %d", status&0b00000001);
	}

	void requestToSendBuffer(uint8_t bufferNumberToSend){
		ESP32CPP::GPIO::low((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Select slave
		bufferNumberToSend|=0b1000000;
		spi.transfer(&bufferNumberToSend, 1);
		ESP32CPP::GPIO::high((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Release slave
	}

	void send(uint8_t* canDataToSend, uint8_t canDataLen){

		requestToSendBuffer(0);
	}

	void readErrorCounters(){
		printf("RxError: %d  TxError: %d\n", readRegister(REG_RX_ERROR_COUNTER), readRegister(REG_TX_ERROR_COUNTER));
	}

	void clearAllFilters(){
		writeBitsToRegister(REG_RXBnCTRL(0), 0b01100000, 0xFF); //Accept all messages for RX buffer 0
		writeBitsToRegister(REG_RXBnCTRL(1), 0b01100000, 0xFF); //Accept all messages for RX buffer 1
	}
};

extern "C" void app_main(void) {
	//	initPowerSupply();
	configureOutputPin((gpio_num_t)SPI_CLK_PIN);
	configureOutputPin((gpio_num_t)SPI_MASTER_OUT_SLAVE_IN_PIN);
	ESP32CPP::GPIO::setInput((gpio_num_t)SPI_MASTER_IN_SLAVE_OUT_PIN);
	configureOutputPin((gpio_num_t)SPI_CS_MCP2515RX_PIN);

	ESP32CPP::GPIO::setOutput((gpio_num_t)SPI_CS_MCP2515RX_PIN);
	ESP32CPP::GPIO::high((gpio_num_t)SPI_CS_MCP2515RX_PIN); // Release slave
	ESP_LOGD(__FUNCTION__, "Ready to SPI");

	spi.setHost(HSPI_HOST);
	spi.init(SPI_MASTER_OUT_SLAVE_IN_PIN, SPI_MASTER_IN_SLAVE_OUT_PIN, SPI_CLK_PIN, SPI_CS_MCP2515RX_PIN);

	MCP2515OverSPI mcp2515;
	mcp2515.init(MCP2515OverSPI::LISTEN_ONLY_MODE);

	while(true){
//		echoOneLineOverSPI();
		mcp2515.clearAllFilters();
		mcp2515.send("Hello", 5);
		mcp2515.readCANInterruptFlag();
		mcp2515.readErrorCounters();
		vTaskDelay(100);
	}
}

