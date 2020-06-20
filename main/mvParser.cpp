#include "mvParser.hpp"

#include <esp_log.h>
#include <mastervoltMessage.hpp>
#include <memory.h>

#include <sstream>
#include <iomanip>
#include <iterator>


MastervoltMessage* MvParser::parse(uint32_t canId, std::string& stringToParse){
	ESP_LOGI(__FUNCTION__, "canId=%x", canId);
	this->stringToParse=stringToParse;
	ESP_LOG_BUFFER_HEXDUMP(__FUNCTION__, stringToParse.c_str(), stringToParse.size(), ESP_LOG_INFO);
	uint32_t deviceUniqueId= (canId&0x7FFF0000)>>16;
	uint32_t deviceKindId=(canId&0x0000FFFF);
	bool isRequest=(canId&0x10000000)!=0;
	ESP_LOGI(__FUNCTION__, "deviceUniqueId=%x deviceKindId=%x isRequest=%d", deviceUniqueId, deviceKindId, isRequest);
	if(stringToParse.size()==0){
		return new MastervoltMessageUnknown(deviceUniqueId, deviceKindId, 0, stringToParse);
	}
	uint8_t attributeId=stringToParse[0];
	MastervoltAttributeKind::MastervoltDataType dataType=(MastervoltAttributeKind::MastervoltDataType) stringToParse[1];
	ESP_LOGI(__FUNCTION__, "attributeId=0x%x dataType=0x%x", attributeId, dataType);

	if(isRequest){
		return new MastervoltMessageRequest(deviceUniqueId, deviceKindId, attributeId);
	}
	const MastervoltAttributeKind* resolvedAttribute=MastervoltDictionary::getInstance()->resolveAttribute(deviceKindId, attributeId);
	ESP_LOGI(__FUNCTION__, "resolvedAttribute is %s", resolvedAttribute->toString().c_str());

	if(dataType==0x00){
		if(resolvedAttribute->dataType==MastervoltAttributeKind::DATE_DT){
			MastervoltMessageDate* dateMsg=new MastervoltMessageDate(deviceUniqueId, deviceKindId, attributeId);
			uint32_t nbDaysZeroBC=parseValueAsFloat();
			dateMsg->day=nbDaysZeroBC%32;
			nbDaysZeroBC-=dateMsg->day;
			float yearAndFraction=nbDaysZeroBC/416.0; //32 days *13 months=416
			dateMsg->year=yearAndFraction;
			yearAndFraction-=dateMsg->year;
			dateMsg->month=yearAndFraction*13;
			return dateMsg;
		}
		else if(resolvedAttribute->dataType==MastervoltAttributeKind::DATE_DT){
			MastervoltMessageTime* timeMsg=new MastervoltMessageTime(deviceUniqueId, deviceKindId, attributeId);
			//Day of month = float%32, Year=float/32/13, Month=decimal remainder of year*13
			//840528.00==16/06/2020
			//840944.00==16/05/2021
			//840943.00==15/06/2021
			//840527.00==15/06/2020
			//840559.00==15/07/2020
			//840591.00==15/08/2020
			//832207.00==15/06/2000 : The masterview interface does not allow for less than year 2000
			//832034.00==02/01/2000 : The interface fails to show the date if we set the date to 01/01/2000. This is probably the default

			uint32_t secondElapsedSinceMidnight=parseValueAsFloat();
			timeMsg->hour=secondElapsedSinceMidnight/60/60;
			secondElapsedSinceMidnight-=(timeMsg->hour*60*60);
			timeMsg->minute=secondElapsedSinceMidnight/60;
			secondElapsedSinceMidnight-=(timeMsg->minute*60);
			timeMsg->second=secondElapsedSinceMidnight;
			return timeMsg;
		}
		return new MastervoltMessageFloat(deviceUniqueId, deviceKindId, attributeId, parseValueAsFloat());
	}
	else if(attributeId==0x30){
		uint8_t segmentNumber=stringToParse[3];
		uint8_t numberOfCharsInPacket=stringToParse.size()-4;
		return new MastervoltMessageLabel(deviceUniqueId, deviceKindId, attributeId, segmentNumber, stringToParse.substr(4, numberOfCharsInPacket));
	}

	return new MastervoltMessageUnknown(deviceUniqueId, deviceKindId, attributeId, stringToParse);
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

MastervoltDictionary* MastervoltDictionary::instance=nullptr;

//TODO These values are dependant on the device type. The device type is encoded in the CANId, but some bits are unique per device.
//TODO Refactor these attributes into a per device organization
const uint32_t MastervoltDictionary::DCSHUNT_CANID=0x086f1297;
const uint16_t MastervoltDictionary::DCSHUNT_BATTERY_PERCENTAGE=0x00;
const uint16_t MastervoltDictionary::DCSHUNT_BATTERY_VOLTS=0x01;
const uint16_t MastervoltDictionary::DCSHUNT_BATTERY_AMPS=0x02;
const uint16_t MastervoltDictionary::DCSHUNT_BATTERY_AMPS_CONSUMED=0x03;
const uint16_t MastervoltDictionary::DCSHUNT_BATTERY_TEMPERATURE=0x05;
const uint16_t MastervoltDictionary::DCSHUNT_DATE=0x0a; //The number of days since day 0
const uint16_t MastervoltDictionary::DCSHUNT_TIME=0x09; //The float represents the number of seconds elapsed since 00:00:00

//0x0bef1297 Is the canid for configuring the date/time. Perhaps the masterview id?
const uint16_t MastervoltDictionary::DCSHUNT_DAYOFMONTH=0x13;
const uint16_t MastervoltDictionary::DCSHUNT_MONTHOFYEAR=0x14;
const uint16_t MastervoltDictionary::DCSHUNT_YEAROFCALENDAR=0x15;


const uint32_t MastervoltDictionary::INVERTER_CANID=0x083af412;
const uint16_t MastervoltDictionary::INVERTER_DC_VOLTAGE_IN=0x06;
const uint16_t MastervoltDictionary::INVERTER_DC_AMPS_IN=0x07;
const uint16_t MastervoltDictionary::INVERTER_AC_AMPS_OUT=0x0b;
const uint16_t MastervoltDictionary::INVERTER_STATE=0x14; //1.0=On 0.0=Off
const uint16_t MastervoltDictionary::INVERTER_POWER_STATE=0x38; //1.0=On 0.0=Off

const uint32_t MastervoltDictionary::DCSHUNT_KIND=0x1297;
const uint32_t MastervoltDictionary::INVERTER_KIND=0xf412;
const uint32_t MastervoltDictionary::BATTERY_CHARGER_KIND=666;
const uint32_t MastervoltDictionary::MASTERVIEW_KIND=666;

MastervoltDictionary::MastervoltDictionary() :
		UNKNOWN_DEVICE_KIND(0, "UnknownDevice"),
		UNKNOWN_ATTRIBUTE_KIND(&UNKNOWN_DEVICE_KIND, MastervoltAttributeKind::UNKNOWN_DT, "Unknown", MastervoltAttributeKind::STRING_ENCODING, MastervoltAttributeKind::BYTES_DT) {
	MastervoltDeviceKind* dcShuntDeviceKind=new MastervoltDeviceKind(DCSHUNT_KIND, "DCShuntDevice");
	deviceKind.insert(std::pair<uint32_t, MastervoltDeviceKind*>(dcShuntDeviceKind->deviceKind, dcShuntDeviceKind));
	dcShuntDeviceKind->addAttribute(DCSHUNT_BATTERY_PERCENTAGE, "BatteryPercentage", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);
	dcShuntDeviceKind->addAttribute(DCSHUNT_BATTERY_VOLTS, "BatteryVolts", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);
	dcShuntDeviceKind->addAttribute(DCSHUNT_BATTERY_AMPS, "BatteryAmps", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);
	dcShuntDeviceKind->addAttribute(DCSHUNT_BATTERY_AMPS_CONSUMED, "BatteryAmpsConsumed", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);
	dcShuntDeviceKind->addAttribute(DCSHUNT_BATTERY_TEMPERATURE, "BatteryTemperature", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);
	dcShuntDeviceKind->addAttribute(DCSHUNT_DATE, "Date", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::DATE_DT);
	dcShuntDeviceKind->addAttribute(DCSHUNT_TIME, "Time", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::TIME_DT);

	MastervoltDeviceKind* inverterDeviceKind=new MastervoltDeviceKind(INVERTER_KIND, "InverterDevice");
	deviceKind.insert(std::pair<uint32_t, MastervoltDeviceKind*>(inverterDeviceKind->deviceKind, inverterDeviceKind));
	inverterDeviceKind->addAttribute(INVERTER_AC_AMPS_OUT, "ACAmpsOut", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);
	inverterDeviceKind->addAttribute(INVERTER_DC_AMPS_IN, "DCAmpsIn", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);
	inverterDeviceKind->addAttribute(INVERTER_DC_VOLTAGE_IN, "DCVoltsIn", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);
	inverterDeviceKind->addAttribute(INVERTER_STATE, "InverterState", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);
	inverterDeviceKind->addAttribute(INVERTER_POWER_STATE, "InverterPowerState", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);
}

std::string MastervoltDictionary::toString() const {
	std::stringstream ss;
	ss<< "DeviceKind known="<<deviceKind.size()<<"\n";
	std::map<uint32_t, MastervoltDeviceKind*>::const_iterator it=deviceKind.begin();
	for(;it!=deviceKind.end(); it++){
		ss<<"	Device "<<it->first<<" ->"<<it->second->attributes.size()<<"\n";
		auto attIt=it->second->attributes.begin();
		for(; attIt!=it->second->attributes.end(); attIt++){
			ss<<"		"<<attIt->second->toString()<<"\n";
		}
	}
	return ss.str();
}

const MastervoltDeviceKind* MastervoltDictionary::resolveDevice(uint32_t deviceKindId){
	std::map<uint32_t, MastervoltDeviceKind*>::const_iterator foundDeviceKind=deviceKind.find(deviceKindId);
	if(foundDeviceKind==deviceKind.end()){
		return &UNKNOWN_DEVICE_KIND;
	}
	return foundDeviceKind->second;
}

const MastervoltAttributeKind* MastervoltDictionary::resolveAttribute(uint32_t deviceKindId, uint16_t attributeId){
	std::map<uint32_t, MastervoltDeviceKind*>::const_iterator foundDeviceKind=deviceKind.find(deviceKindId);
	if(foundDeviceKind==deviceKind.end()){
		return &UNKNOWN_ATTRIBUTE_KIND;
	}
	std::map<uint16_t, MastervoltAttributeKind*>::iterator foundAttributeKind=foundDeviceKind->second->attributes.find(attributeId);
	if(foundAttributeKind==foundDeviceKind->second->attributes.end()){
		return &UNKNOWN_ATTRIBUTE_KIND;
	}
	return foundAttributeKind->second;
}

