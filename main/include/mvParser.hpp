#ifndef MAIN_INCLUDE_MVPARSER_H_
#define MAIN_INCLUDE_MVPARSER_H_

#include <mastervoltMessage.hpp>
#include <mvDictionary.hpp>
#include <string>

class MvParser {
public:
	MastervoltMessage* parse(uint32_t stdCanbusId, uint32_t extCanbusId, const std::string& canbusPayloadToParse);
  MastervoltMessage* parseMasscombiResponseItemAndFloat(uint32_t stdCanbusId, uint32_t extCanbusId, const std::string& canbusPayloadToParse);
  MastervoltMessage* parseDCShuntResponseItemAndFloat(uint32_t stdCanbusId, uint32_t extCanbusId, const std::string& canbusPayloadToParse);
  MastervoltMessage* parseStdCanbusId(uint32_t stdCanbusId, uint32_t extCanbusId, const std::string& canbusPayloadToParse);
	std::string parseString();
	float parseValueAsFloat();
	std::string stringToParse;

	static void convertMastervoltFloatToTime(float floatValue, MastervoltMessage* timeMsg);
	static void convertMastervoltFloatToDate(float floatValue, MastervoltMessage* dateMsg);
private:
  uint16_t getShortAttributeFromPayload(const std::string &mbPayloadToParse);
  float getFloatFromPayloadAtOffset(const std::string &mbPayloadToParse, const uint8_t offset);
};


#endif /* MAIN_INCLUDE_MVPARSER_H_ */
