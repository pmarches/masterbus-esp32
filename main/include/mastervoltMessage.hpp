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

	MastervoltMessage(uint32_t deviceUniqueId, uint32_t deviceKindId, uint8_t attributeId) :
		deviceUniqueId(deviceUniqueId),
		deviceKindId(deviceKindId),
		attributeId(attributeId),
		type(UNKNOWN) {}
	virtual ~MastervoltMessage() {}
	virtual std::string toString() const;
};

class MastervoltMessageUnknown : public MastervoltMessage {
public:
	std::string unknownBytes;
	MastervoltMessageUnknown(uint32_t deviceUniqueId, uint32_t deviceKindId, uint8_t attributeId, std::string unknownBytes):
		MastervoltMessage(deviceUniqueId, deviceKindId, attributeId),
		unknownBytes(unknownBytes) {
		type=UNKNOWN;
	}
	virtual ~MastervoltMessageUnknown() {}
	std::string toString() const;
};


class MastervoltMessageRequest : public MastervoltMessage {
public:
	MastervoltMessageRequest(uint32_t deviceUniqueId, uint32_t deviceKindId, uint8_t attributeId):
		MastervoltMessage(deviceUniqueId, deviceKindId, attributeId){
		type=REQUEST;
	}
	virtual ~MastervoltMessageRequest() {}
};

class MastervoltMessageFloat : public MastervoltMessage {
public:
	MastervoltMessageFloat(uint32_t deviceUniqueId, uint32_t deviceKindId, uint8_t attributeId, float floatValue):
		MastervoltMessage(deviceUniqueId, deviceKindId, attributeId),
		floatValue(floatValue) {
		type=FLOAT;
	}
	virtual ~MastervoltMessageFloat() {}
	float floatValue;
	float getFloatValue();
	std::string toString() const;
};

class MastervoltMessageDate : public MastervoltMessage {
public:
	uint8_t day;
	uint8_t month;
	uint8_t year;

	MastervoltMessageDate(uint32_t deviceUniqueId, uint32_t deviceKindId, uint8_t attributeId):
		MastervoltMessage(deviceUniqueId, deviceKindId, attributeId),
		day(0),
		month(0),
		year(0) {
		type=DATE;
	}
	virtual ~MastervoltMessageDate() {}
	void toTm(struct tm& destinationTm);
};

class MastervoltMessageTime : public MastervoltMessage {
public:
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	MastervoltMessageTime(uint32_t deviceUniqueId, uint32_t deviceKindId, uint8_t attributeId):
		MastervoltMessage(deviceUniqueId, deviceKindId, attributeId),
		hour(0),
		minute(0),
		second(0) {
		type=DATE;
	}
	virtual ~MastervoltMessageTime() {}
	void toTm(struct tm& destinationTm);
};

class MastervoltMessageLabel : public MastervoltMessage {
public:
	MastervoltMessageLabel(uint32_t deviceUniqueId, uint32_t deviceKindId, uint8_t attributeId, uint8_t segmentNumber, std::string label):
		MastervoltMessage(deviceUniqueId, deviceKindId, attributeId),
		segmentNumber(segmentNumber),
		label(label) {
		type=LABEL;
	}
	virtual ~MastervoltMessageLabel() {}
	uint16_t segmentNumber;
	std::string label;

	std::string toString() const;
};



#endif /* COMPONENTS_MASTERBUS_ESP32_MAIN_INCLUDE_MASTERVOLTMESSAGE_HPP_ */
