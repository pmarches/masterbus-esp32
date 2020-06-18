#ifndef MAIN_INCLUDE_MVPARSER_H_
#define MAIN_INCLUDE_MVPARSER_H_

#include <string>
#include <map>
#include <mastervoltMessage.h>

class MastervoltPacket;
class MastervoltDeviceKind;

class MastervoltAttributeKind {
public:
	enum MastervoltEncoding {
		FLOAT_ENCODING,
		STRING_ENCODING,
	};
	enum MastervoltDataType {
		UNKNOWN_DT,
		SIMPLE_FLOAT_DT,
		DATE_DT,
		TIME_DT,
		TEXT_DT,
		BYTES_DT,
		REQUEST_DT,
		LABEL_DT
	};
	MastervoltAttributeKind(const MastervoltDeviceKind* parent, uint16_t attributeKind, std::string textDescription, MastervoltEncoding encoding, MastervoltDataType dataType)
		: MastervoltAttributeKind(attributeKind, textDescription, encoding, dataType){
	}
	MastervoltAttributeKind(uint16_t attributeKind, std::string textDescription, MastervoltEncoding encoding, MastervoltDataType dataType);
	MastervoltDeviceKind* parent;
	uint16_t attributeKind;
	std::string textDescription;
	MastervoltEncoding encoding; //How is the underlying data encoded? With floats? Strings?
	MastervoltDataType dataType; //What does the data look like? A date? A float? Text?

	std::string packetValueToString(MastervoltPacket& packet) const;
	std::string toString() const;
};

class MastervoltDeviceKind {
public:
	MastervoltDeviceKind(uint32_t deviceKind);
	void addAttribute(MastervoltAttributeKind att);
	std::string toString() const;
	uint32_t deviceKind;
	std::map<uint16_t, MastervoltAttributeKind> attributeKind;
};

class MastervoltDictionary {
protected:
	MastervoltDictionary();
	static MastervoltDictionary* instance;
	std::map<uint32_t, MastervoltDeviceKind> deviceKind;
	const MastervoltDeviceKind UNKNOWN_DEVICE_KIND;
	const MastervoltAttributeKind UNKNOWN_ATTRIBUTE_KIND;
public:
	static const uint32_t DCSHUNT_KIND;
	static const uint32_t INVERTER_KIND;
	static const uint32_t BATTERY_CHARGER_KIND;
	static const uint32_t MASTERVIEW_KIND;

	static const uint32_t DCSHUNT_CANID;
	static const uint16_t DCSHUNT_BATTERY_PERCENTAGE;
	static const uint16_t DCSHUNT_BATTERY_VOLTS;
	static const uint16_t DCSHUNT_BATTERY_AMPS;
	static const uint16_t DCSHUNT_BATTERY_AMPS_CONSUMED;
	static const uint16_t DCSHUNT_BATTERY_TEMPERATURE;
	static const uint16_t DCSHUNT_DATE;
	static const uint16_t DCSHUNT_TIME;

	static const uint32_t INVERTER_CANID;
	static const uint16_t INVERTER_DC_VOLTAGE_IN;
	static const uint16_t INVERTER_DC_AMPS_IN;
	static const uint16_t INVERTER_AC_AMPS_OUT;
	static const uint16_t INVERTER_STATE;

	static const uint16_t DCSHUNT_DAYOFMONTH;
	static const uint16_t DCSHUNT_MONTHOFYEAR;
	static const uint16_t DCSHUNT_YEAROFCALENDAR;


	static MastervoltDictionary* getInstance() { if(instance==nullptr) instance=new MastervoltDictionary(); return instance;}
	const MastervoltAttributeKind& resolveAttribute(uint32_t canId, uint16_t attributeId);
};

class MastervoltPacket {
public:
	MastervoltPacket(): canId(0), attributeId(0), dataType(0), valueType(UNKNOWN), floatValue(0){
	}
//	uint8_t getGroupNumber();
//	uint8_t getItemNumber();

	std::string getLabel();
	float getFloatValue();

	uint32_t canId;
	uint8_t attributeId; //What is the value about? Battery volts? Battery Amps? Time? Date? These attribute Id overlap across different device type

	uint8_t dataType;

	enum MastervoltPacketType {
		UNKNOWN,
		REQUEST,
		FLOAT,
		LABEL,
	} valueType;

	float floatValue;
	std::string labelContent;
	std::string unparsed;

	bool isRequest(){ return (canId&0x10000000) != 0; }
	uint32_t getDeviceUniqueId() { return canId&0x7FFF0000; }
	void dump();
};


class MvParser {
	static std::map<uint16_t, MastervoltPacket::MastervoltPacketType> populateAttributeIdToTypeMap();
public:
	bool parse(uint32_t canId, std::string& stringToParse, MastervoltPacket* mvPacket);
	bool parseString();
	bool parseUnknownAttribute();
	float parseValueAsFloat();
	std::string stringToParse;
	MastervoltPacket* mvPacket;
};


#endif /* MAIN_INCLUDE_MVPARSER_H_ */
