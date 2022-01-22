#ifndef MAIN_INCLUDE_MASTERBUSCONTROLLER_H_
#define MAIN_INCLUDE_MASTERBUSCONTROLLER_H_

#include <cstddef>
#include <stdint.h>
#include <MCP2515.h>

#include <sstream>
#include <iomanip>

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
