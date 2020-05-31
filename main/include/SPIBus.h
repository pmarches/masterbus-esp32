#ifndef MAIN_INCLUDE_SPIBUS_H_
#define MAIN_INCLUDE_SPIBUS_H_

#include <driver/spi_master.h>
#include <esp_log.h>
#include "sdkconfig.h"
#include <driver/gpio.h>

class SPIBus {
public:
	SPIBus(spi_host_device_t host);
	~SPIBus();
	esp_err_t init(gpio_num_t mosiPin, gpio_num_t misoPin, gpio_num_t clkPin);
//	uint8_t transferByte(uint8_t value);
private:
  spi_host_device_t   m_host;
  gpio_num_t mosiPin;
  gpio_num_t misoPin;
  gpio_num_t clkPin;
  friend class SPIDevice;
};


class SPIDevice {
	SPIBus* spiBus;
	gpio_num_t csPin;
	spi_device_handle_t m_handle;
public:
	SPIDevice(SPIBus* spiBus, gpio_num_t csPin);
	~SPIDevice();
	esp_err_t init();
	void transfer(uint8_t* data, size_t dataLen);
};



#endif /* MAIN_INCLUDE_SPIBUS_H_ */
