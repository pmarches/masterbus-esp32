// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef ARDUINO_ARCH_ESP32

#include "MCP2515.h"
#include "driver/spi_master.h"
#include <esp_log.h>
#include <freertos/task.h>

#define REG_BFPCTRL                0x0c
#define REG_TXRTSCTRL              0x0d
#define REG_CANSTAT 0xe

#define REG_CANCTRL                0x0f

#define REG_TEC 0x1c
#define REG_REC 0x1d
#define REG_EFLG 0x2d

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

MCP2515Class::MCP2515Class(SPIDevice* spi, gpio_num_t chipSelectPin, gpio_num_t interruptPin, uint32_t clockFrequency) :
  CANControllerClass(),
//  _spiSettings(10E6, MSBFIRST, SPI_MODE0),
  spi(spi),
  _csPin(chipSelectPin),
  _intPin(interruptPin),
  _clockFrequency(clockFrequency),
  _operationalMode(MCP2515Class::LISTEN_ONLY_MODE)
{
}

MCP2515Class::~MCP2515Class()
{
}

int MCP2515Class::begin(long baudRate)
{
	ESP_LOGD(__FUNCTION__, "Setting baudrate at %ld", baudRate);
  CANControllerClass::begin(baudRate);

  //  spi.begin();
  reset();

  if(setMode(CONFIGURATION_MODE)!=0){
	  ESP_LOGE(__FUNCTION__, "Failed to go to configuration mode");
	  return 1;
  }

  const struct {
    long clockFrequency;
    long baudRate;
    uint8_t cnf[3];
  } CNF_MAPPER[] = {
    {  (long)8E6, (long)1000E3, { 0x00, 0x80, 0x00 } },
    {  (long)8E6,  (long)500E3, { 0x00, 0x90, 0x02 } },
    {  (long)8E6,  (long)250E3, { 0x00, 0xb1, 0x05 } },
    {  (long)8E6,  (long)200E3, { 0x00, 0xb4, 0x06 } },
    {  (long)8E6,  (long)125E3, { 0x01, 0xb1, 0x05 } },
    {  (long)8E6,  (long)100E3, { 0x01, 0xb4, 0x06 } },
    {  (long)8E6,   (long)80E3, { 0x01, 0xbf, 0x07 } },
    {  (long)8E6,   (long)50E3, { 0x03, 0xb4, 0x06 } },
    {  (long)8E6,   (long)40E3, { 0x03, 0xbf, 0x07 } },
    {  (long)8E6,   (long)20E3, { 0x07, 0xbf, 0x07 } },
    {  (long)8E6,   (long)10E3, { 0x0f, 0xbf, 0x07 } },
    {  (long)8E6,    (long)5E3, { 0x1f, 0xbf, 0x07 } },

    { (long)16E6, (long)1000E3, { 0x00, 0xd0, 0x82 } },
    { (long)16E6,  (long)500E3, { 0x00, 0xf0, 0x86 } },
    { (long)16E6,  (long)250E3, { 0x41, 0xf1, 0x85 } },
    { (long)16E6,  (long)200E3, { 0x01, 0xfa, 0x87 } },
    { (long)16E6,  (long)125E3, { 0x03, 0xf0, 0x86 } },
    { (long)16E6,  (long)100E3, { 0x03, 0xfa, 0x87 } },
    { (long)16E6,   (long)80E3, { 0x03, 0xff, 0x87 } },
    { (long)16E6,   (long)50E3, { 0x07, 0xfa, 0x87 } },
    { (long)16E6,   (long)40E3, { 0x07, 0xff, 0x87 } },
    { (long)16E6,   (long)20E3, { 0x0f, 0xff, 0x87 } },
    { (long)16E6,   (long)10E3, { 0x1f, 0xff, 0x87 } },
    { (long)16E6,    (long)5E3, { 0x3f, 0xff, 0x87 } },
  };

  const uint8_t* cnf = NULL;

  for (unsigned int i = 0; i < (sizeof(CNF_MAPPER) / sizeof(CNF_MAPPER[0])); i++) {
    if (CNF_MAPPER[i].clockFrequency == _clockFrequency && CNF_MAPPER[i].baudRate == baudRate) {
      cnf = CNF_MAPPER[i].cnf;
      break;
    }
  }

  if (cnf == NULL) {
	  ESP_LOGE(__FUNCTION__, "Failed to configure for clock freq %ld and baudrate %ld", _clockFrequency, baudRate);
    return 1;
  }

  writeRegister(REG_CNF1, cnf[0]);
  writeRegister(REG_CNF2, cnf[1]);
  writeRegister(REG_CNF3, cnf[2]);

  writeRegister(REG_CANINTE, FLAG_RXnIE(1) | FLAG_RXnIE(0));
  writeRegister(REG_BFPCTRL, 0x00);
  writeRegister(REG_TXRTSCTRL, 0x00);
  writeRegister(REG_RXBnCTRL(0), FLAG_RXM1 | FLAG_RXM0);
  writeRegister(REG_RXBnCTRL(1), FLAG_RXM1 | FLAG_RXM0);

  if (setMode(_operationalMode)) {
	  ESP_LOGE(__FUNCTION__, "Failed to go to mode %x", _operationalMode);
    return 1;
  }

  return 0;
}

void MCP2515Class::end()
{
//  spi.end();

  CANControllerClass::end();
}

int MCP2515Class::endPacket()
{
  if (!CANControllerClass::endPacket()) {
	  ESP_LOGE(__FUNCTION__, "CANControllerClass::endPacket failed");
    return 0;
  }

  int n = 0; //TX buffer number

  if (_txExtended) {
    writeRegister(REG_TXBnSIDH(n), _txId >> 21);
    writeRegister(REG_TXBnSIDL(n), (((_txId >> 18) & 0x07) << 5) | FLAG_EXIDE | ((_txId >> 16) & 0x03));
    writeRegister(REG_TXBnEID8(n), (_txId >> 8) & 0xff);
    writeRegister(REG_TXBnEID0(n), _txId & 0xff);
  } else {
    writeRegister(REG_TXBnSIDH(n), _txId >> 3);
    writeRegister(REG_TXBnSIDL(n), _txId << 5);
    writeRegister(REG_TXBnEID8(n), 0x00);
    writeRegister(REG_TXBnEID0(n), 0x00);
  }

  if (_txRtr) {
    writeRegister(REG_TXBnDLC(n), 0x40 | _txLength);
  } else {
    writeRegister(REG_TXBnDLC(n), _txLength);
//    ESP_LOGD(__FUNCTION__, "_txLength=%d", _txLength);

    for (int i = 0; i < _txLength; i++) {
//	ESP_LOGD(__FUNCTION__, "_txData[i]=%x", _txData[i]);
      writeRegister(REG_TXBnD0(n) + i, _txData[i]);
    }
  }

  writeRegister(REG_TXBnCTRL(n), 0x08);

  while (readRegister(REG_TXBnCTRL(n)) & 0x08) {
    if (readRegister(REG_TXBnCTRL(n)) & 0x10) {
      // abort
    	ESP_LOGE(__FUNCTION__, "Aborting");
      modifyRegister(REG_CANCTRL, 0x10, 0x10);
    }

//    yield(); //FIXME Stream
  }

  modifyRegister(REG_CANINTF, FLAG_TXnIF(n), 0x00);

  return (readRegister(REG_TXBnCTRL(n)) & 0x70) ? 0 : 1;
}

int MCP2515Class::parsePacket()
{
  int whichRxBuffer;

  uint8_t intf = readRegister(REG_CANINTF);
//  ESP_LOGD(__FUNCTION__, "intf=%x", intf);

  if (intf & FLAG_RXnIF(0)) {
    whichRxBuffer = 0;
  } else if (intf & FLAG_RXnIF(1)) {
    whichRxBuffer = 1;
  } else {
    _rxId = -1;
    _rxExtended = false;
    _rxRtr = false;
    _rxLength = 0;
//    ESP_LOGD(__FUNCTION__, "RX in neither buffers");
    return 0;
  }

  _rxExtended = (readRegister(REG_RXBnSIDL(whichRxBuffer)) & FLAG_IDE) ? true : false;

  uint32_t idA = ((readRegister(REG_RXBnSIDH(whichRxBuffer)) << 3) & 0x07f8) | ((readRegister(REG_RXBnSIDL(whichRxBuffer)) >> 5) & 0x07);
  if (_rxExtended) {
    uint32_t idB = (((uint32_t)(readRegister(REG_RXBnSIDL(whichRxBuffer)) & 0x03) << 16) & 0x30000) | ((readRegister(REG_RXBnEID8(whichRxBuffer)) << 8) & 0xff00) | readRegister(REG_RXBnEID0(whichRxBuffer));

    _rxId = (idA << 18) | idB;
    _rxRtr = (readRegister(REG_RXBnDLC(whichRxBuffer)) & FLAG_RTR) ? true : false;
  } else {
    _rxId = idA;
    _rxRtr = (readRegister(REG_RXBnSIDL(whichRxBuffer)) & FLAG_SRR) ? true : false;
  }
  _rxDlc = readRegister(REG_RXBnDLC(whichRxBuffer)) & 0x0f;
  _rxIndex = 0;

  if (_rxRtr) {
    _rxLength = 0;
  } else {
    _rxLength = _rxDlc;

    for (int i = 0; i < _rxLength; i++) {
      _rxData[i] = readRegister(REG_RXBnD0(whichRxBuffer) + i);
    }
  }

//  ESP_LOGD(__FUNCTION__, "_rxExtended=%x", _rxExtended);
//  ESP_LOGD(__FUNCTION__, "_rxId=%ld", _rxId);
//  ESP_LOGD(__FUNCTION__, "_rxRtr=%x", _rxRtr);
//  ESP_LOGD(__FUNCTION__, "_rxDlc=%x", _rxDlc);
//  ESP_LOGD(__FUNCTION__, "_rxIndex=%x", _rxIndex);

  modifyRegister(REG_CANINTF, FLAG_RXnIF(whichRxBuffer), 0x00);

  return _rxDlc;
}

void MCP2515Class::onReceive(void(*callback)(int))
{
  CANControllerClass::onReceive(callback);

  gpio_pad_select_gpio(_intPin);
  gpio_set_direction(_intPin, GPIO_MODE_INPUT);
#ifdef MCP2515INTERRUPT_EN
  if (callback) {
    spi.usingInterrupt(digitalPinToInterrupt(_intPin));
    attachInterrupt(digitalPinToInterrupt(_intPin), MCP2515Class::onInterrupt, LOW);
  } else {
    detachInterrupt(digitalPinToInterrupt(_intPin));
#ifdef SPI_HAS_NOTUSINGINTERRUPT
    spi.notUsingInterrupt(digitalPinToInterrupt(_intPin));
#endif
  }
#else
	#warning "INTERRUPT DISABLED"
#endif
}

void MCP2515Class::clearAllFilters(){
	modifyRegister(REG_RXBnCTRL(0), 0b01100000, 0xFF); //Accept all messages for RX buffer 0
	modifyRegister(REG_RXBnCTRL(1), 0b01100000, 0xFF); //Accept all messages for RX buffer 1
}

int MCP2515Class::filter(int id, int mask)
{
  id &= 0x7ff;
  mask &= 0x7ff;

  if (setMode(CONFIGURATION_MODE)) {
    return 1;
  }

  for (int n = 0; n < 2; n++) {
    // standard only
    writeRegister(REG_RXBnCTRL(n), FLAG_RXM0);
    writeRegister(REG_RXBnCTRL(n), FLAG_RXM0);

    writeRegister(REG_RXMnSIDH(n), mask >> 3);
    writeRegister(REG_RXMnSIDL(n), mask << 5);
    writeRegister(REG_RXMnEID8(n), 0);
    writeRegister(REG_RXMnEID0(n), 0);
  }

  for (int n = 0; n < 6; n++) {
    writeRegister(REG_RXFnSIDH(n), id >> 3);
    writeRegister(REG_RXFnSIDL(n), id << 5);
    writeRegister(REG_RXFnEID8(n), 0);
    writeRegister(REG_RXFnEID0(n), 0);
  }

  if (setMode(_operationalMode)) {
    return 1;
  }

  return 0;
}

int MCP2515Class::filterExtended(long id, long mask)
{
  id &= 0x1FFFFFFF;
  mask &= 0x1FFFFFFF;

  if (setMode(CONFIGURATION_MODE)) {
    return 1;
  }

  for (int n = 0; n < 2; n++) {
    // extended only
    writeRegister(REG_RXBnCTRL(n), FLAG_RXM1);
    writeRegister(REG_RXBnCTRL(n), FLAG_RXM1);

    writeRegister(REG_RXMnSIDH(n), mask >> 21);
    writeRegister(REG_RXMnSIDL(n), (((mask >> 18) & 0x03) << 5) | FLAG_EXIDE | ((mask >> 16) & 0x03));
    writeRegister(REG_RXMnEID8(n), (mask >> 8) & 0xff);
    writeRegister(REG_RXMnEID0(n), mask & 0xff);
  }

  for (int n = 0; n < 6; n++) {
    writeRegister(REG_RXFnSIDH(n), id >> 21);
    writeRegister(REG_RXFnSIDL(n), (((id >> 18) & 0x03) << 5) | FLAG_EXIDE | ((id >> 16) & 0x03));
    writeRegister(REG_RXFnEID8(n), (id >> 8) & 0xff);
    writeRegister(REG_RXFnEID0(n), id & 0xff);
  }

  if (setMode(_operationalMode)) {
    return 1;
  }

  return 0;
}

int MCP2515Class::setMode(uint8_t newMode)
{
	ESP_LOGD(__FUNCTION__, "readRegister(REG_CANCTRL)=%x", readRegister(REG_CANCTRL));
	uint8_t modeMask=0b11100000;
  modifyRegister(REG_CANCTRL, modeMask, newMode);
  if ((readRegister(REG_CANCTRL)&modeMask) != newMode) {
	ESP_LOGE(__FUNCTION__, "Failed to go into mode %x", newMode);
    return 1;
  }

  return 0;
}

void MCP2515Class::setPins(int cs, int irq)
{
  _csPin = (gpio_num_t) cs;
  _intPin = (gpio_num_t) irq;
}

void MCP2515Class::setSPIFrequency(uint32_t frequency)
{
//  _spiSettings = SPISettings(frequency, MSBFIRST, SPI_MODE0);
}

void MCP2515Class::setClockFrequency(long clockFrequency)
{
  _clockFrequency = clockFrequency;
}

//void MCP2515Class::dumpRegisters(Stream& out)
//{
//  for (int i = 0; i < 128; i++) {
//    byte b = readRegister(i);
//
//    out.print("0x");
//    if (i < 16) {
//      out.print('0');
//    }
//    out.print(i, HEX);
//    out.print(": 0x");
//    if (b < 16) {
//      out.print('0');
//    }
//    out.println(b, HEX);
//  }
//}

void MCP2515Class::reset()
{
	ESP_LOGD(__FUNCTION__, "Begin reset");
  //spi.beginTransaction(_spiSettings);
	gpio_set_level(_csPin, 0); //Select Slave


  uint8_t spiBytes[]={0xc0};
  spi->transfer(spiBytes, 1);
  gpio_set_level(_csPin, 1);// Release slave
  //spi.endTransaction();

//  delayMicroseconds(10);
  vTaskDelay(5);
	ESP_LOGD(__FUNCTION__, "Done reset");
}

void MCP2515Class::handleInterrupt()
{
  if (readRegister(REG_CANINTF) == 0) {
    return;
  }

  while (parsePacket()) {
    _onReceive(available());
  }
}

uint8_t MCP2515Class::readRegister(uint8_t address)
{
  //spi.beginTransaction(_spiSettings);
  gpio_set_level(_csPin, 0); //Select Slave
  uint8_t spiBytes[]={0x03, address, 0x00};
  spi->transfer(spiBytes, 3);
  gpio_set_level(_csPin, 1);// Release slave
  //spi.endTransaction();

  return spiBytes[2];
}

void MCP2515Class::modifyRegister(uint8_t address, uint8_t mask, uint8_t value)
{
  //spi.beginTransaction(_spiSettings);
  gpio_set_level(_csPin, 0); //Select Slave
  uint8_t spiBytes[]={0x05, address, mask, value};
  spi->transfer(spiBytes, 4);
  gpio_set_level(_csPin, 1);// Release slave
  //spi.endTransaction();
}

void MCP2515Class::writeRegister(uint8_t address, uint8_t value)
{
  //spi.beginTransaction(_spiSettings);
  gpio_set_level(_csPin, 0); //Select Slave
  uint8_t spiBytes[]={0x02, address, value};
  spi->transfer(spiBytes, 3);
  gpio_set_level(_csPin, 1);// Release slave
  //spi.endTransaction();
}

#ifdef MCP2515INTERRUPT_EN
void MCP2515Class::onInterrupt()
{
  CAN.handleInterrupt();
}
#endif

uint8_t MCP2515Class::getAndClearRegister(uint8_t registerToGetAndClear){
	uint8_t status=readRegister(registerToGetAndClear);
	writeRegister(registerToGetAndClear, 0);
	return status;
}

void MCP2515Class::dumpRegisterState(){
	ESP_LOGD(__FUNCTION__, "-------------BEGIN-------------");
	uint8_t status=readRegister(REG_CANINTF);
	ESP_LOGD(__FUNCTION__, "CANINTF %x", status);
	ESP_LOGD(__FUNCTION__, "	Message Error Interrupt Flag bit: %d", status&0b10000000);
	ESP_LOGD(__FUNCTION__, "	Wake up Interrupt Flag bit: %d", status&0b01000000);
	ESP_LOGD(__FUNCTION__, "	Error interrupt Flag bit: %d", status&0b00100000);
	ESP_LOGD(__FUNCTION__, "	Transmit Buffer 2 Empty interrupt Flag bit: %d", status&0b00010000);
	ESP_LOGD(__FUNCTION__, "	Transmit Buffer 1 Empty interrupt Flag bit: %d", status&0b00001000);
	ESP_LOGD(__FUNCTION__, "	Transmit Buffer 0 Empty interrupt Flag bit: %d", status&0b00000100);
	ESP_LOGD(__FUNCTION__, "	Receive Buffer 1 Empty interrupt Flag bit: %d", status&0b00000010);
	ESP_LOGD(__FUNCTION__, "	Receive Buffer 0 Empty interrupt Flag bit: %d", status&0b00000001);

	ESP_LOGD(__FUNCTION__, "TEC TX Error Counter %d", readRegister(REG_TEC));
	ESP_LOGD(__FUNCTION__, "REC RX Error Counter %d", readRegister(REG_REC));


	status=readRegister(REG_EFLG);
	ESP_LOGD(__FUNCTION__, "EFLG: %x", status);
	ESP_LOGD(__FUNCTION__, "	Receive Buffer 1 Overflow Flag bit : %d", status&0x80);
	ESP_LOGD(__FUNCTION__, "	Receive Buffer 0 Overflow Flag bit : %d", status&0x40);
	ESP_LOGD(__FUNCTION__, "	Bus-Off Error Flag bit : %d", status&0x20);
	ESP_LOGD(__FUNCTION__, "	Transmit Error-Passive Flag bit : %d", status&0x10);
	ESP_LOGD(__FUNCTION__, "	Receive Error-Passive Flag bit : %d", status&0x08);
	ESP_LOGD(__FUNCTION__, "	Transmit Error Warning Flag bit : %d", status&0x04);
	ESP_LOGD(__FUNCTION__, "	Receive Error Warning Flag bit : %d", status&0x02);
	ESP_LOGD(__FUNCTION__, "	Error Warning Flag bit: %d", status&0x01);

	status=readRegister(REG_CANSTAT);
	ESP_LOGD(__FUNCTION__, "CANSTAT: %x", status);

	status=readRegister(REG_TXBnCTRL(0));
	ESP_LOGD(__FUNCTION__, "REG_TXBnCTRL(0): %x", status);
}

#endif
