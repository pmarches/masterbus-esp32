#include <MCP2515.h>
#include <freertos/task.h>
#include <masterbusController.h>

void configureOutputPin(gpio_num_t pin){
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
				nbValidPackets+=mcp2515.parsePacket();
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

MasterbusController::MasterbusController(MCP2515Class* mcp2515): mcp2515(mcp2515), keepPumping(false) {
	pumpQueue=xQueueCreate(10, sizeof(CANBusPacket));
}

MasterbusController::~MasterbusController(){
	vQueueDelete(pumpQueue);
}

bool MasterbusController::readPacket(CANBusPacket& packet){
	if(mcp2515->getAndClearRxOverflow()){
		ESP_LOGW(__FUNCTION__, "Canbus RX overflow occured. We have lost some messages");
	}
	bool hasPacket=mcp2515->parsePacket();
	if(hasPacket){
		if(mcp2515->packetDlc()==0){
			ESP_LOGW(__FUNCTION__, "These packets are sent by the masterview, every 30 seconds. They request data to keep coming. But how is the data selected as we change the pages?");
		}

//		mcp2515->dumpRegisterState();
		packet.canId=mcp2515->packetId();
		packet.stdCanbusId= (packet.canId&0xFFFC0000)>>18; //This might be a deviceKind Id
		packet.extCanbusId= (packet.canId&0x0003FFFF);     //This might be a device unique ID

		packet.dataLen=mcp2515->packetDlc();
		for(int i=0; i<packet.dataLen; i++){
			packet.data[i]=mcp2515->read();
		}
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
			  ESP_LOGD(__FUNCTION__, "RX Packet, forward it to the queue");
			  if(xQueueSendToBack(thisObj->pumpQueue, &canbusPacketToQueue, 0)!=pdTRUE){
				  ESP_LOGE(__FUNCTION__, "Failed to enqueue canbus packet. Maybe queue is full?");
			  }
		  }
	  }

	  vTaskDelete(NULL);
}

void MasterbusController::startCANBusPump(){
	this->keepPumping=true;
	xTaskCreate(MasterbusController::pumpCanbusToQueue, "pumpCanbusToQueue", 2048, this, 5, &taskPumpMToQueueHandle);
}

void MasterbusController::stopCANBusPump(){
	this->keepPumping=false;
//	vTaskDelete(taskPumpMToQueueHandle);
}

void MasterbusController::send(long int canId, uint8_t* canDataToSend, uint32_t canDataLenToSend){
	mcp2515->getAndClearRegister(REG_CANINTF);

	ESP_LOGW(__FUNCTION__, "Sending %d bytes to 0x%lx", canDataLenToSend, canId);
	ESP_LOG_BUFFER_HEXDUMP("  canDataToSend ", canDataToSend, canDataLenToSend, ESP_LOG_DEBUG);
	mcp2515->beginExtendedPacket(canId, canDataLenToSend, true);

	mcp2515->write(canDataToSend, canDataLenToSend);
	if(mcp2515->endPacket()==0){
		ESP_LOGE(__FUNCTION__, "Failed to endPacket()");
		mcp2515->dumpRegisterState();
	}

//	vTaskDelay(15);
}

void MasterbusController::configure(int operationalMode) {
	mcp2515->setOperationalMode(operationalMode);
	mcp2515->clearAllFilters();
	long masterBusBaudRate=250000;
#if AUTO_DETECT_BAUDRATE
	masterBusBaudRate=autoDetectBaudRate(mcp2515Rx);
	if(masterBusBaudRate==0){
		return;
	}
#endif
	mcp2515->begin(masterBusBaudRate);

	mcp2515->attachInterrupt(MCP2515Class::handleInterrupt);
}

CANBusPacket::CANBusPacket() : 	canId(0), stdCanbusId(0), extCanbusId(0), dataLen(0), isDirty(false){
}

CANBusPacket::~CANBusPacket() {
}

std::string CANBusPacket::getData(){
	std::string dataStr;
	for(int i=0; i<dataLen; i++){
		dataStr.push_back(data[i]);
	}
	return dataStr;
}

std::string CANBusPacket::valueToHexString(){
	std::stringstream ss;
	ss << std::hex;
	for( int i(0) ; i < dataLen; ++i ){
		ss << std::setw(2) << std::setfill('0') << (int)data[i];
	}

	return ss.str();
}
