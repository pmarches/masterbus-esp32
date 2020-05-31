#ifndef MAIN_INCLUDE_MVPARSER_H_
#define MAIN_INCLUDE_MVPARSER_H_

#include <string>
#include <map>

class MastervoltPacket {
public:
	MastervoltPacket(): canId(0), dataType(0), attributeId(0), valueType(UNKNOWN){
	}
//	uint8_t getGroupNumber();
//	uint8_t getItemNumber();

	std::string getLabel();
	float getFloatValue();
	uint32_t getIntValue();
	std::string getUnit();

	uint32_t canId;
	uint8_t dataType;
	uint8_t attributeId;
	enum MastervoltPacketType {
		UNKNOWN,
		REQUEST,
		FLOAT,
		LABEL,
	} valueType;

	union {
		float floatValue;
	};
	std::string labelContent;
	std::string unparsed;
};


class MvParser {
	static const std::map<uint16_t, MastervoltPacket::MastervoltPacketType> attributeToTypeMap;
	static std::map<uint16_t, MastervoltPacket::MastervoltPacketType> populateAttributeIdToTypeMap();
public:
	bool parse(uint32_t canId, std::string& stringToParse, MastervoltPacket* mvPacket);
	bool parseString();
	bool parseUnknownAttribute();
	float parseValueAsFloat();
	std::string stringToParse;
	MastervoltPacket* mvPacket;
};

class MastervoltDictionary {
	std::map<uint16_t, std::string> attributeIdToLabelMap;
	MastervoltDictionary();
	static MastervoltDictionary* instance;
public:
	static MastervoltDictionary* getInstance() { if(instance==nullptr) instance=new MastervoltDictionary(); return instance;}
	std::string resolveCanIdToLabel(long int canId);
	std::string resolveAttributeIdToLabel(uint16_t attId);
};


#endif /* MAIN_INCLUDE_MVPARSER_H_ */
