#ifndef MAIN_INCLUDE_MASTERBUSCONTROLLER_H_
#define MAIN_INCLUDE_MASTERBUSCONTROLLER_H_

#include <cstddef>
#include <stdint.h>
#include <MCP2515.h>

#include <sstream>
#include <iomanip>

class CANBusPacket {
public:
	long int canId;
	uint8_t* data;
	uint32_t dataLen;
	bool isDirty;
	CANBusPacket(): canId(0), data(0), dataLen(0), isDirty(false){
	}
	std::string valueToHexString();
	std::string getData();
};

class MasterbusController {
public:
	MasterbusController(MCP2515Class* mcp2515);
	void configure(int operationalMode);
	void send(long int canId, uint8_t* canDataToSend, uint32_t canDataLenToSend);
	bool readPacket(CANBusPacket& packet);
	MCP2515Class* mcp2515;
};

#endif /* MAIN_INCLUDE_MASTERBUSCONTROLLER_H_ */
