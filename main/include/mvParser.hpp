#ifndef MAIN_INCLUDE_MVPARSER_H_
#define MAIN_INCLUDE_MVPARSER_H_

#include <mastervoltMessage.hpp>
#include <mvDictionary.hpp>
#include <string>

class MvParser {
public:
	MastervoltMessage* parse(uint32_t stdCanbusId, uint32_t extCanbusId, const std::string& canbusPayloadToParse);
	std::string parseString();
	float parseValueAsFloat();
	std::string stringToParse;
};


#endif /* MAIN_INCLUDE_MVPARSER_H_ */
