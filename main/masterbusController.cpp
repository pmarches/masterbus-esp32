#include <MCP2515.h>
#include <freertos/task.h>
#include <masterbusController.h>
#include <arpa/inet.h>

void configureOutputPin(gpio_num_t pin){
  ESP_LOGD(__FUNCTION__, "pin=%d", pin);
	gpio_pad_select_gpio(pin);
	gpio_set_direction(pin, GPIO_MODE_OUTPUT);
}

#define REG_CANSTAT 0xe
#define REG_CANCTRL                0x0f
#define REG_TEC 0x1c
#define REG_REC 0x1d
#define REG_EFLG 0x2d
#define REG_CANINTE                0x2b
#define REG_CANINTF                0x2c

long int WELLKNOWN_BAUDRATE[]={1000000, 500000, 250000, 200000, 150000, 125000, 100000, 80000, 50000, 40000, 20000, 10000, 5000};
long autoDetectBaudRate(MCP2515Class& mcp2515){
	uint8_t nbWellKnownBaudRate=sizeof(WELLKNOWN_BAUDRATE)/sizeof(long int);
	for(int i=0; i<nbWellKnownBaudRate; i++){
		uint8_t nbValidPackets=0;
		long baudRate=WELLKNOWN_BAUDRATE[i];
		mcp2515.begin(baudRate);
		while(true){
			mcp2515.dumpRegisterState();
			bool messageHasError=(mcp2515.getAndClearRegister(REG_CANINTF)&0x80)!=0;
			mcp2515.getAndClearRegister(REG_EFLG);
			mcp2515.getAndClearRegister(REG_CANSTAT);
			if(messageHasError){
				ESP_LOGE(__FUNCTION__, "The baudrate %ld is not correct, trying something else", baudRate);
				break;
			}
			else{
			  CANBusPacket frame;
				nbValidPackets+=mcp2515.parsePacket(&frame);
				if(nbValidPackets>3){
					ESP_LOGD(__FUNCTION__, "Got enough good packets to determine the baud rate is %ld", baudRate);
					return baudRate;
				}
			}
//			vTaskDelay(300);
		}
	}

	ESP_LOGE(__FUNCTION__, "Failed to autodetect the baudrate");
	return 0;
}

MasterbusController::MasterbusController(MCP2515Class* mcp2515): mcp2515(mcp2515), keepPumping(false), taskPumpMToQueueHandle(NULL) {
	pumpQueue=xQueueCreate(10, sizeof(CANBusPacket));
}

MasterbusController::~MasterbusController(){
	vQueueDelete(pumpQueue);
}

#include <memory.h>

void MasterbusController::hexdumpCanBusPacket(CANBusPacket& packet){
  uint8_t virtualDumpBytes[16];
  memset(virtualDumpBytes, 0, sizeof(virtualDumpBytes));
  //CANBus fields are not aligned on byte boundaries, making the hexdump non-trivial. So, I will instead dump the bytes as a follows:
  //Request flag (either RTR or SRR) (1 byte)
  //11-Bit identifier + 18-bit identifier (4 bytes)
  //DLC (payload length) (1 byte)
  //payload (variable number of bytes, maximum 8)
  virtualDumpBytes[0]=packet.isRequest;
  *(uint32_t*) (virtualDumpBytes+1) = packet.canId;
  virtualDumpBytes[5]=packet.dataLen;
  memcpy(virtualDumpBytes+6, packet.data, packet.dataLen);

  uint8_t virtualDumpBytesLen=8+packet.dataLen;
  ESP_LOG_BUFFER_HEXDUMP("CANBUS_HEXDUMP", virtualDumpBytes, virtualDumpBytesLen, ESP_LOG_DEBUG);
  for(uint32_t i=0; i<virtualDumpBytesLen; i++){
    if(i==0){
      printf("%04X ", i);
    }
    else if(i%16 ==0){
      printf("\n%04X ", i);
    }
    printf("%02X ", virtualDumpBytes[i]);
  }
  printf("\n%04X\n", ((virtualDumpBytesLen/16)+1)*16);
}


bool MasterbusController::readPacket(CANBusPacket& packet){
#ifdef ENABLING_THIS_SEEMS_TO_BE_CAUSING_TOO_MUCH_DELAYS
	if(mcp2515->getAndClearRxOverflow()){
		ESP_LOGW(__FUNCTION__, "Canbus RX overflow occured. We have lost some messages");
	}
#endif

	bool hasPacket=mcp2515->parsePacket(&packet);
	if(hasPacket){
		if(packet.dataLen==0){
			ESP_LOGW(__FUNCTION__, "These empty packets are sent by the masterview, every 30 seconds. They request data to keep coming. But how is the data selected as we change the pages?");
		}
		if(0xFFFFFF1F==packet.canId){
      ESP_LOGW(__FUNCTION__, "Got a invalid packet. Maybe the MCP2515 is not initialized properly?");
		  return false;
		}

//		mcp2515->dumpRegisterState();
	}
//	ESP_LOGD(__FUNCTION__, "Tried to read canpacket %d", hasPacket);
	return hasPacket;
}

void MasterbusController::pumpCanbusToQueue(void* thisObjPtr){
	  ESP_LOGI(__FUNCTION__, "Begin %p", thisObjPtr);
	  MasterbusController* thisObj=(MasterbusController*) thisObjPtr;

	  CANBusPacket canbusPacketToQueue;
	  while (thisObj->keepPumping) {
//		  ESP_LOGD(__FUNCTION__, "Waitingfor a canbus packet");
		  if(thisObj->readPacket(canbusPacketToQueue)){
			  ESP_LOGD(__FUNCTION__, "RX Packet, forward it to the queue. dataLen=%d", canbusPacketToQueue.dataLen);
			  if(xQueueSendToBack(thisObj->pumpQueue, &canbusPacketToQueue, 0)!=pdTRUE){
				  ESP_LOGE(__FUNCTION__, "Failed to enqueue canbus packet. Maybe queue is full?");
			  }
		  }
	  }

	  vTaskDelete(NULL);
}

void MasterbusController::startCANBusPump(){
	this->keepPumping=true;
	xTaskCreatePinnedToCore(MasterbusController::pumpCanbusToQueue, "pumpCanbusToQueue", 4096, this, 5, &taskPumpMToQueueHandle, 0);
}

bool MasterbusController::isCANBusPumping(){
	return taskPumpMToQueueHandle!=NULL;
}

void MasterbusController::stopCANBusPump(){
	this->keepPumping=false;
//	vTaskDelete(taskPumpMToQueueHandle);
}

void MasterbusController::send(uint32_t canId, uint8_t* canDataToSend, uint32_t canDataLenToSend){
	mcp2515->getAndClearRegister(REG_CANINTF);

	ESP_LOGW(__FUNCTION__, "Sending %d bytes to 0x%x", canDataLenToSend, canId);
	ESP_LOG_BUFFER_HEXDUMP("  canDataToSend ", canDataToSend, canDataLenToSend, ESP_LOG_DEBUG);

	CANBusPacket frameToSend;
	frameToSend.canId=canId;
	frameToSend.isRequest=false;
	frameToSend.dataLen=canDataLenToSend;
	memcpy(frameToSend.data, canDataToSend, canDataLenToSend);

	if(mcp2515->write(&frameToSend)){
		ESP_LOGE(__FUNCTION__, "Failed to write(CANBusFrame)");
		mcp2515->dumpRegisterState();
	}

//	vTaskDelay(15);
}

int MasterbusController::configure(int operationalMode) {
	mcp2515->setOperationalMode(operationalMode);
	mcp2515->clearAllFilters();
	long masterBusBaudRate=250000;
#if AUTO_DETECT_BAUDRATE
	masterBusBaudRate=autoDetectBaudRate(mcp2515Rx);
	if(masterBusBaudRate==0){
		return;
	}
#endif
	if(0!=mcp2515->begin(masterBusBaudRate)){
	  return ESP_FAIL;
	}

	mcp2515->attachInterrupt(MCP2515Class::handleInterrupt);
  ESP_LOGI(__FUNCTION__, "MCP2515 configured successfully");
	return ESP_OK;
}

