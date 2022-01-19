#include "mvParser.hpp"

#include <esp_log.h>
#include <memory.h>

#include <sstream>
#include <iomanip>
#include <iterator>
#include <math.h>

#include <mastervoltMessage.hpp>

uint16_t MvParser::getShortAttributeFromPayload(const std::string &mbPayloadToParse) {
  return *(uint16_t*) ((mbPayloadToParse.c_str()));
}

float MvParser::getFloatFromPayloadAtOffset(const std::string &mbPayloadToParse, const uint8_t offset) {
  return *(float*) ((mbPayloadToParse.c_str() + offset));
}

void MvParser::convertMastervoltFloatToTime(float floatValue, MastervoltMessageTime* timeMsg){
  uint32_t secondElapsedSinceMidnight=floatValue;
  timeMsg->hour=secondElapsedSinceMidnight/60/60;
  secondElapsedSinceMidnight-=(timeMsg->hour*60*60);
  timeMsg->minute=secondElapsedSinceMidnight/60;
  secondElapsedSinceMidnight-=(timeMsg->minute*60);
  timeMsg->second=secondElapsedSinceMidnight;
}

void MvParser::convertMastervoltFloatToDate(float floatTimestamp, MastervoltMessageDate* dateMsg){
  uint32_t nbDaysZeroBC=floatTimestamp;
  ESP_LOGI(__FUNCTION__, "nbDaysZeroBC %d", nbDaysZeroBC);

  dateMsg->day=(nbDaysZeroBC%32);
  nbDaysZeroBC-=dateMsg->day;
  float yearAndFraction=nbDaysZeroBC/(32.0*13); //32 days *13 months=416
  dateMsg->year=yearAndFraction;
  yearAndFraction-=dateMsg->year;
  dateMsg->month=round(yearAndFraction*13);
}

MastervoltMessage* MvParser::parseDCShuntResponseItemAndFloat(uint32_t stdCanbusId, uint32_t extCanbusId, const std::string& mbPayloadToParse){
  ESP_LOGI(__FUNCTION__, "stdCanbusId=0x%x extCanbusId=0x%x", stdCanbusId, extCanbusId);
  ESP_LOG_BUFFER_HEXDUMP(__FUNCTION__, mbPayloadToParse.c_str(), mbPayloadToParse.size(), ESP_LOG_DEBUG);

  if(mbPayloadToParse.size()!=6){
    ESP_LOGE(__FUNCTION__, "mbPayloadToParse.size() needs at least 6 bytes, but has only %d", mbPayloadToParse.size());
    return NULL;
  }

  uint16_t attribute=getShortAttributeFromPayload(mbPayloadToParse);
  if(0x09==attribute){
    float floatValue = getFloatFromPayloadAtOffset(mbPayloadToParse, 2);
    MastervoltMessageTime* timeMsg=new MastervoltMessageTime(extCanbusId, stdCanbusId, attribute);
    convertMastervoltFloatToTime(floatValue, timeMsg);
    return timeMsg;
  }
  else if(0x0a==attribute){
    float floatValue = getFloatFromPayloadAtOffset(mbPayloadToParse, 2);
    MastervoltMessageDate* dateMsg=new MastervoltMessageDate(extCanbusId, stdCanbusId, attribute);
    convertMastervoltFloatToDate(floatValue, dateMsg);
    return dateMsg;
  }
  else{
    float floatValue = getFloatFromPayloadAtOffset(mbPayloadToParse, 2);
    MastervoltMessageFloat* floatMsg=new MastervoltMessageFloat(extCanbusId, stdCanbusId, attribute, floatValue);
    return floatMsg;
  }
}

MastervoltMessage* MvParser::parseMasscombiResponseItemAndFloat(uint32_t stdCanbusId, uint32_t extCanbusId, const std::string& mbPayloadToParse){
  ESP_LOGI(__FUNCTION__, "stdCanbusId=0x%x extCanbusId=0x%x", stdCanbusId, extCanbusId);

  if(mbPayloadToParse.size()!=6){
    ESP_LOGE(__FUNCTION__, "mbPayloadToParse.size() needs at least 6 bytes, but has only %d", mbPayloadToParse.size());
    return NULL;
  }
  ESP_LOG_BUFFER_HEXDUMP(__FUNCTION__, mbPayloadToParse.c_str(), mbPayloadToParse.size(), ESP_LOG_DEBUG);

  uint16_t attribute = getShortAttributeFromPayload(mbPayloadToParse);
  float floatValue = getFloatFromPayloadAtOffset(mbPayloadToParse, 2);
  MastervoltMessageFloat* floatMsg=new MastervoltMessageFloat(extCanbusId, stdCanbusId, attribute, floatValue);
  return floatMsg;
}

MastervoltMessage* MvParser::parseStdCanbusId(uint32_t stdCanbusId, uint32_t extCanbusId, const std::string& mbPayloadToParse){
  ESP_LOGI(__FUNCTION__, "stdCanbusId=0x%x extCanbusId=0x%x", stdCanbusId, extCanbusId);
  if(0x21b==stdCanbusId){
    return parseDCShuntResponseItemAndFloat(stdCanbusId, extCanbusId, mbPayloadToParse);
  }
  if(0x020e==stdCanbusId){
    return parseMasscombiResponseItemAndFloat(stdCanbusId, extCanbusId, mbPayloadToParse);
  }
  ESP_LOGE(__FUNCTION__, "Don't know how to parse stdCanbusId=0x%x extCanbusId=0x%x", stdCanbusId, extCanbusId);
  return NULL;
}

MastervoltMessage* MvParser::parse(uint32_t stdCanbusId, uint32_t extCanbusId, const std::string& mbPayloadToParse){
	ESP_LOGI(__FUNCTION__, "stdCanbusId=0x%x extCanbusId=0x%x", stdCanbusId, extCanbusId);
	this->stringToParse=mbPayloadToParse;
	ESP_LOG_BUFFER_HEXDUMP(__FUNCTION__, mbPayloadToParse.c_str(), mbPayloadToParse.size(), ESP_LOG_INFO);
	if(mbPayloadToParse.size()==0){
		return new MastervoltMessageUnknown(extCanbusId, stdCanbusId, 0, mbPayloadToParse);
	}
	uint8_t attributeId=mbPayloadToParse[0];
//	MastervoltAttributeKind::MastervoltDataType dataType=(MastervoltAttributeKind::MastervoltDataType) mbPayloadToParse[1];
//	ESP_LOGI(__FUNCTION__, "attributeId=0x%x dataType=0x%x", attributeId, dataType);

//	if(isRequest){
//		return new MastervoltMessageRequest(deviceUniqueId, deviceKindId, attributeId);
//	}
	const MastervoltAttributeKind* resolvedAttribute=MastervoltDictionary::getInstance()->resolveAttribute(stdCanbusId, attributeId);
	ESP_LOGI(__FUNCTION__, "resolvedAttribute is %s", resolvedAttribute->toString().c_str());

	if(resolvedAttribute->dataType==MastervoltAttributeKind::MastervoltDataType::LABEL_DT){
		uint8_t segmentNumber=mbPayloadToParse[3];
		uint8_t numberOfCharsInPacket=mbPayloadToParse.size()-4;
		return new MastervoltMessageLabel(extCanbusId, stdCanbusId, attributeId, segmentNumber, mbPayloadToParse.substr(4, numberOfCharsInPacket));
	}
	else if(resolvedAttribute->dataType==MastervoltAttributeKind::MastervoltDataType::DATE_DT){
		MastervoltMessageDate* dateMsg=new MastervoltMessageDate(extCanbusId, stdCanbusId, attributeId);
		uint32_t nbDaysZeroBC=parseValueAsFloat();
		ESP_LOGI(__FUNCTION__, "nbDaysZeroBC %d", nbDaysZeroBC);

		//Day of month = float%32, Year=float/32/13, Month=decimal remainder of year*13
		//840509.00==30/06/2020
		//840528.00==16/06/2020
		//840944.00==16/05/2021
		//840943.00==15/06/2021
		//840527.00==15/06/2020
		//840559.00==15/07/2020
		//840591.00==15/08/2020
		//832207.00==15/06/2000 : The masterview interface does not allow for less than year 2000
		//832034.00==02/01/2000 : The interface fails to show the date if we set the date to 01/01/2000. This is probably the default

		dateMsg->day=(nbDaysZeroBC%32)+1;
		nbDaysZeroBC-=dateMsg->day;
		float yearAndFraction=nbDaysZeroBC/416.0; //32 days *13 months=416
		dateMsg->year=yearAndFraction;
		yearAndFraction-=dateMsg->year;
		dateMsg->month=round(yearAndFraction*13);
		return dateMsg;
	}
	else if(resolvedAttribute->dataType==MastervoltAttributeKind::MastervoltDataType::TIME_DT){
		MastervoltMessageTime* timeMsg=new MastervoltMessageTime(extCanbusId, stdCanbusId, attributeId);
		uint32_t secondElapsedSinceMidnight=parseValueAsFloat();
		timeMsg->hour=secondElapsedSinceMidnight/60/60;
		secondElapsedSinceMidnight-=(timeMsg->hour*60*60);
		timeMsg->minute=secondElapsedSinceMidnight/60;
		secondElapsedSinceMidnight-=(timeMsg->minute*60);
		timeMsg->second=secondElapsedSinceMidnight;
		return timeMsg;
	}
	else if(resolvedAttribute->dataType==MastervoltAttributeKind::MastervoltDataType::SIMPLE_FLOAT_DT){
		return new MastervoltMessageFloat(extCanbusId, stdCanbusId, attributeId, parseValueAsFloat());
	}

	return new MastervoltMessageUnknown(extCanbusId, stdCanbusId, attributeId, mbPayloadToParse);
}


float MvParser::parseValueAsFloat(){
	float* valueAsFloat=(float*) (stringToParse.c_str()+2);
	return *valueAsFloat;
}

MastervoltDeviceKind::MastervoltDeviceKind(uint32_t deviceKind, std::string textDescription) : deviceKind(deviceKind), textDescription(textDescription) {
}

std::string MastervoltDeviceKind::toString() const {
	std::stringstream ss;
	ss<< "deviceKind=0x"<< std::setw(2) << std::setfill('0') << std::hex << std::uppercase<<deviceKind;
	return ss.str();
}

MastervoltAttributeKind::MastervoltAttributeKind(uint16_t attributeKind, std::string textDescription, MastervoltEncoding encoding, MastervoltDataType dataType):
		parent(nullptr), attributeKind(attributeKind), textDescription(textDescription), encoding(encoding), dataType(dataType) {
}

void MastervoltDeviceKind::addAttribute(uint16_t attributeKind, std::string textDescription, MastervoltAttributeKind::MastervoltEncoding encoding, MastervoltAttributeKind::MastervoltDataType dataType){
	MastervoltAttributeKind* att=new MastervoltAttributeKind(this, attributeKind, textDescription, encoding, dataType);
	attributes.insert(std::pair<uint16_t, MastervoltAttributeKind*>(att->attributeKind, att));
}

std::string MastervoltAttributeKind::toString() const {
	std::stringstream ss;
	ss<< textDescription
			<<" : encoding="<<encoding
			<< std::setw(2) << std::setfill('0') << std::hex <<" dataType=0x"<<dataType
			<< std::setw(2) << std::setfill('0') << std::hex <<" attributeKind=0x"<<attributeKind;
	return ss.str();
}

