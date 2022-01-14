#ifndef MAIN_INCLUDE_MASTERBUSCONTROLLER_H_
#define MAIN_INCLUDE_MASTERBUSCONTROLLER_H_

#include <cstddef>
#include <stdint.h>
#include <MCP2515.h>

#include <sstream>
#include <iomanip>

class CANBusPacket {
public:
	uint32_t canId;
	uint32_t stdCanbusId;
	uint32_t extCanbusId;
	uint8_t data[16];
	uint32_t dataLen;
	bool isDirty;
	bool isRequest;
	CANBusPacket();
	~CANBusPacket();
	std::string valueToHexString();
	std::string getData();
};

class MasterbusController {
public:
	MasterbusController(MCP2515Class* mcp2515);
	virtual ~MasterbusController();
	int configure(int operationalMode);
	void send(uint32_t canId, uint8_t* canDataToSend, uint32_t canDataLenToSend);
	bool readPacket(CANBusPacket& packet);
	MCP2515Class* mcp2515;
	QueueHandle_t pumpQueue;
	static void pumpCanbusToQueue(void* thisObj);
	void startCANBusPump();
	void stopCANBusPump();
	bool isCANBusPumping();
	bool keepPumping;
	xTaskHandle taskPumpMToQueueHandle;
	void hexdumpCanBusPacket(CANBusPacket& canbusPacket);
};

#endif /* MAIN_INCLUDE_MASTERBUSCONTROLLER_H_ */
