#ifndef COMPONENTS_MASTERBUS_ESP32_MAIN_INCLUDE_MASTERVOLTMESSAGE_HPP_
#define COMPONENTS_MASTERBUS_ESP32_MAIN_INCLUDE_MASTERVOLTMESSAGE_HPP_

#include <string>

class MastervoltMessage {
public:
	uint32_t deviceUniqueId;
	uint32_t deviceKindId;
	uint16_t attributeId;
	enum MastervoltMessageType {
		UNKNOWN,
		REQUEST,
		FLOAT,
		LABEL,
		DATE,
		TIME
	};
	MastervoltMessageType type;

	union {
    float floatValue;
	  struct {
      uint16_t day;
      uint16_t month;
      uint16_t year;
	  };
	  struct {
	    uint8_t hour;
	    uint8_t minute;
	    uint8_t second;
	  };
	  struct {
	    uint16_t segmentNumber;
	    std::string* label;
	  };
	} value;

	MastervoltMessage(MastervoltMessageType type, uint32_t deviceUniqueId, uint32_t deviceKindId, uint8_t attributeId) :
		deviceUniqueId(deviceUniqueId),
		deviceKindId(deviceKindId),
		attributeId(attributeId),
		type(type) {}
	virtual ~MastervoltMessage() {}
	virtual std::string toString() const;
};


#endif /* COMPONENTS_MASTERBUS_ESP32_MAIN_INCLUDE_MASTERVOLTMESSAGE_HPP_ */
