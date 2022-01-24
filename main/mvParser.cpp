#include <memory.h>
#include <sstream>
#include <iomanip>
#include <iterator>
#include <math.h>

#include <esp_log.h>

#include <mvParser.hpp>
#include <mastervoltMessage.hpp>

uint16_t MvParser::getShortAttributeFromPayload(const std::string &mbPayloadToParse) {
  return *(uint16_t*) ((mbPayloadToParse.c_str()));
}

float MvParser::getFloatFromPayloadAtOffset(const std::string &mbPayloadToParse, const uint8_t offset) {
  return *(float*) ((mbPayloadToParse.c_str() + offset));
}

void MvParser::convertMastervoltFloatToTime(float floatValue, MastervoltMessage* timeMsg){
  uint32_t secondElapsedSinceMidnight=floatValue;
  timeMsg->value.hour=secondElapsedSinceMidnight/60/60;
  secondElapsedSinceMidnight-=(timeMsg->value.hour*60*60);
  timeMsg->value.minute=secondElapsedSinceMidnight/60;
  secondElapsedSinceMidnight-=(timeMsg->value.minute*60);
  timeMsg->value.second=secondElapsedSinceMidnight;
}

void MvParser::convertMastervoltFloatToDate(float floatTimestamp, MastervoltMessage* dateMsg){
  uint32_t nbDaysZeroBC=floatTimestamp;
  ESP_LOGI(__FUNCTION__, "nbDaysZeroBC %d", nbDaysZeroBC);

  dateMsg->value.day=(nbDaysZeroBC%32);
  nbDaysZeroBC-=dateMsg->value.day;
  float yearAndFraction=nbDaysZeroBC/(32.0*13); //32 days *13 months=416
  dateMsg->value.year=yearAndFraction;
  yearAndFraction-=dateMsg->value.year;
  dateMsg->value.month=round(yearAndFraction*13);
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
    MastervoltMessage* timeMsg=new MastervoltMessage(MastervoltMessage::TIME, extCanbusId, stdCanbusId, attribute);
    convertMastervoltFloatToTime(floatValue, timeMsg);
    return timeMsg;
  }
  else if(0x0a==attribute){
    float floatValue = getFloatFromPayloadAtOffset(mbPayloadToParse, 2);
    MastervoltMessage* dateMsg=new MastervoltMessage(MastervoltMessage::DATE, extCanbusId, stdCanbusId, attribute);
    convertMastervoltFloatToDate(floatValue, dateMsg);
    return dateMsg;
  }
  else{
    MastervoltMessage* floatMsg=new MastervoltMessage(MastervoltMessage::FLOAT, extCanbusId, stdCanbusId, attribute);
    floatMsg->value.floatValue = getFloatFromPayloadAtOffset(mbPayloadToParse, 2);
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
  MastervoltMessage* floatMsg=new MastervoltMessage(MastervoltMessage::FLOAT, extCanbusId, stdCanbusId, attribute);
  floatMsg->value.floatValue = getFloatFromPayloadAtOffset(mbPayloadToParse, 2);
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

float MvParser::parseValueAsFloat(){
	float* valueAsFloat=(float*) (stringToParse.c_str()+2);
	return *valueAsFloat;
}

