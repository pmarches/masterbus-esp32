#include <unity.h>
#include <stdio.h>
#include <MCP2515.h>
#include <cstring>

#include <esp_log.h>

#define SPI_CLK_PIN (gpio_num_t)13
#define SPI_MASTER_OUT_SLAVE_IN_PIN (gpio_num_t) 5
#define SPI_MASTER_IN_SLAVE_OUT_PIN (gpio_num_t) 4
#define SPI_CS_PIN (gpio_num_t) 3
#define SPI_INT_PIN (gpio_num_t)14

void configurePinAsOutput(gpio_num_t pin);

#include <eventCounter.h>
#include <map>
std::map<uint32_t, EventCounter*> msgkindToEventCounters;

SPIBus spiBus(SPI2_HOST);
TEST_CASE("Test canbus controller mcp2515", "[mcp2515]") {
//  esp_log_level_set("parsePacket", ESP_LOG_WARN);
  ESP_LOGD(__FUNCTION__, "For whaterever reason, I cannot reset the gpio pins twice in the unit test...");
  spiBus.init(SPI_MASTER_OUT_SLAVE_IN_PIN, SPI_MASTER_IN_SLAVE_OUT_PIN, SPI_CLK_PIN, SPI_INT_PIN);

  SPIDevice* spiDevice=new SPIDevice(&spiBus, SPI_CS_PIN);
  ESP_ERROR_CHECK(spiDevice->init());
  MCP2515Class* mcp2515=new MCP2515Class(spiDevice, SPI_CS_PIN, SPI_INT_PIN, 8E6);
  mcp2515->setOperationalMode(MCP2515Class::NORMAL_MODE);
  mcp2515->clearAllFilters();

#if 0
  //Apply filter on RXF0-5
  //Message will be found in RXB0-5 ?
  mcp2515->filtersAcceptCanId(0x83AF412);
#endif

  mcp2515->attachInterrupt(MCP2515Class::handleInterrupt);
  TEST_ASSERT_EQUAL(0, mcp2515->begin(250000));

  CANBusPacket packet;
  for(uint32_t i=0;; i++){
    if(mcp2515->parsePacket(&packet)==false){
//      ESP_LOGD(__FUNCTION__, "No frame");
    }
    else {
      if(packet.dataLen==0){
        ESP_LOGW(__FUNCTION__, "Got empty frame");
      }
      if(0xFFFFFFFF==packet.canId){
        ESP_LOGW(__FUNCTION__, "Got a invalid packet. Maybe the MCP2515 is not initialized properly?");
      }

      EventCounter* eventCounter=msgkindToEventCounters[packet.canId];
      if(nullptr == eventCounter){
        eventCounter=new EventCounter();
        msgkindToEventCounters[packet.canId]=eventCounter;
      }
      eventCounter->increment();
  //    mcp2515->dumpRegisterState();
      ESP_LOGD(__FUNCTION__, "packet.canId=0x%X => packet.stdCanbusId=0x%X packet.extCanbusId=0x%X", packet.canId, packet.stdCanbusId, packet.extCanbusId);
      ESP_LOGD(__FUNCTION__, "%s", eventCounter->toString().c_str());
      if(mcp2515->getAndClearRxOverflow()){
        ESP_LOGW(__FUNCTION__, "Canbus RX overflow occured. We have lost some messages");
      }

      if(i%30==0){
        mcp2515->dumpRegisterState();
      }
    }
  }
  delete mcp2515;

  delete spiDevice;

  ESP_ERROR_CHECK(spiBus.deInit());
}

#if 0
TEST_CASE("Test reset pin", "[mcp2515]") {
  gpio_set_direction(SPI_CS_PIN, GPIO_MODE_OUTPUT);
}
#endif
