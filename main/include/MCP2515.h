// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef ARDUINO_ARCH_ESP32

#ifndef MCP2515_H
#define MCP2515_H

#include <SPIBus.h>

#include "CANController.h"
#include <freertos/semphr.h>

#define MCP2515_DEFAULT_CLOCK_FREQUENCY 16e6

#if defined(ARDUINO_ARCH_SAMD) && defined(USB_VID) && (USB_VID == 0x2341) && !defined(ARDUINO_SAMD_ZERO)
// Arduino MKR board: MKR CAN shield CS is pin 3, INT is pin 7
#define MCP2515_DEFAULT_CS_PIN          3
#define MCP2515_DEFAULT_INT_PIN         7
#else
#define MCP2515_DEFAULT_CS_PIN          10
#define MCP2515_DEFAULT_INT_PIN         2
#endif

class MCP2515Class : public CANControllerClass {

public:
  enum {
	NORMAL_MODE=0x00,
	SLEEP_MODE=0x20,
	LOOPBACK_MODE=0x40,
	LISTEN_ONLY_MODE=0x60,
	CONFIGURATION_MODE=0x80,
  };

  MCP2515Class(SPIDevice* spi, gpio_num_t chipSelectPin, gpio_num_t interruptPin, uint32_t clockFrequency);
  virtual ~MCP2515Class();

  virtual int begin(long baudRate);
  virtual void end();

  virtual int endPacket();

  virtual int parsePacket();

  using CANControllerClass::filter;
  virtual int filter(int id, int mask);
  using CANControllerClass::filterExtended;
  virtual int filterExtended(long id, long mask);
  void clearAllFilters();

  int setMode(uint8_t newMode);
  void setOperationalMode(uint8_t newMode) { _operationalMode=newMode; }

  void setPins(int cs = MCP2515_DEFAULT_CS_PIN, int irq = MCP2515_DEFAULT_INT_PIN);
  void setSPIFrequency(uint32_t frequency);
  void setClockFrequency(long clockFrequency);

//  void dumpRegisters(Stream& out);
  void dumpRegisterState();
  uint8_t getAndClearRegister(uint8_t registerToGetAndClear);
  bool getAndClearRxOverflow();

  static void handleInterrupt(void* thisObj);
  void attachInterrupt(gpio_isr_t canbusInterruptHandler);

private:
  void reset();


  uint8_t readRegister(uint8_t address);
  void modifyRegister(uint8_t address, uint8_t mask, uint8_t value);
  void writeRegister(uint8_t address, uint8_t value);

private:
//  SPISettings _spiSettings;
  SPIDevice* spi;
  gpio_num_t _csPin;
  gpio_num_t _intPin;
  long _clockFrequency;
  uint8_t _operationalMode;
  SemaphoreHandle_t msgPumpSemaphore;
};

#endif

#endif
