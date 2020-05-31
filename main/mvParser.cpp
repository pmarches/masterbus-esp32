#include <mvParser.h>
#include <esp_log.h>
#include <memory.h>

#include <sstream>
#include <iomanip>


const std::map<uint16_t, MastervoltPacket::MastervoltPacketType> MvParser::attributeToTypeMap = {
	{0x00, MastervoltPacket::MastervoltPacketType::FLOAT},
	{0x02, MastervoltPacket::MastervoltPacketType::FLOAT},
	{0x03, MastervoltPacket::MastervoltPacketType::FLOAT},
	{0x05, MastervoltPacket::MastervoltPacketType::FLOAT},
	{0x06, MastervoltPacket::MastervoltPacketType::FLOAT},
	{0x07, MastervoltPacket::MastervoltPacketType::FLOAT},
	{0x0b, MastervoltPacket::MastervoltPacketType::FLOAT},
	{0x14, MastervoltPacket::MastervoltPacketType::FLOAT},
};


bool MvParser::parse(uint32_t canId, std::string& stringToParse, MastervoltPacket* mvPacket){
	this->stringToParse=stringToParse;
	this->mvPacket=mvPacket;
	this->mvPacket->canId=canId;

	ESP_LOG_BUFFER_HEXDUMP(__FUNCTION__, stringToParse.c_str(), stringToParse.size(), ESP_LOG_DEBUG);
	if(stringToParse.size()==0){
		return false;
	}

	this->mvPacket->dataType=stringToParse[0];
	this->mvPacket->attributeId=stringToParse[1];

	if(stringToParse.size()==2){
		this->mvPacket->valueType=MastervoltPacket::REQUEST;
		return true;
	}

	if(this->mvPacket->dataType==0x30){
		this->mvPacket->valueType=MastervoltPacket::LABEL;
		return parseString();
	}
	else if(attributeToTypeMap.find(mvPacket->attributeId)!=attributeToTypeMap.end()){
		this->mvPacket->valueType=MastervoltPacket::FLOAT;
		this->mvPacket->floatValue=parseValueAsFloat();
	}
	else {
		std::string attributeLabel=MastervoltDictionary::getInstance()->resolveAttributeIdToLabel(this->mvPacket->attributeId);
		mvPacket->unparsed=stringToParse;
		ESP_LOGW(__FUNCTION__, "UKNX	%s	0x%x	0x%x", attributeLabel.c_str(), this->mvPacket->canId, this->mvPacket->attributeId);
	}

	return true;
}


float MvParser::parseValueAsFloat(){
	float* valueAsFloat=(float*) (stringToParse.c_str()+2);
	return *valueAsFloat;
}

bool MvParser::parseString(){
	uint8_t segmentNumber=stringToParse[3];

	uint8_t numberOfCharsInPacket=stringToParse.size()-4;
	this->mvPacket->labelContent=stringToParse.substr(4, numberOfCharsInPacket);
	return true;
}


MastervoltDictionary* MastervoltDictionary::instance=nullptr;
MastervoltDictionary::MastervoltDictionary(){
//	attributeIdToLabelMap[0x00]="batteryPercentage";
//	attributeIdToLabelMap[0x02]="batteryCurrent";
//	attributeIdToLabelMap[0x03]="batteryAmpHourConsumed";
//	attributeIdToLabelMap[0x05]="batteryTemperature";
//	attributeIdToLabelMap[0x06]="massCombiDCVoltageIn";
//	attributeIdToLabelMap[0x07]="massCombiDCCurrentIn";
//	attributeIdToLabelMap[0x0b]="massCombiACAmpsOut";
//	attributeIdToLabelMap[0x14]="massCombiInverterState?";
}

std::string MastervoltDictionary::resolveCanIdToLabel(long int canId){
//	if(canId==0x86f1297){
//		return "dcShunt";
//	}
//	else if(canId==0x083af412){
//		return "massCombi";
//	}
//	else if(canId==0x063af412){
//		return "massCombiPowerState";
//	}
//	else
	{
		std::stringstream ss;
		ss << "0x";
		ss << std::hex;
		ss << std::setw(8) << std::setfill('0') << canId;
		return ss.str();
	}
}

std::string MastervoltDictionary::resolveAttributeIdToLabel(uint16_t attId){
	auto attFound=attributeIdToLabelMap.find(attId);
	if(attFound!=attributeIdToLabelMap.end()){
		return attFound->second;
	}
	else {
		std::stringstream ss;
		ss << "0x";
		ss << std::hex;
		ss << std::setw(2) << std::setfill('0') << attId;
		return ss.str();
	}
}
