#include "mvParser.hpp"

#include <esp_log.h>
#include <memory.h>

#include <sstream>
#include <iomanip>
#include <iterator>
#include <mastervoltMessage.h>

void MastervoltPacket::dump(){
	ESP_LOGD(__FUNCTION__, "canId=%x", canId);
	ESP_LOGD(__FUNCTION__, "attributeId=%x", this->attributeId);
	ESP_LOGD(__FUNCTION__, "dataType=%x", this->dataType);
	ESP_LOGD(__FUNCTION__, "floatValue=%f", this->floatValue);
}

MastervoltMessage* MvParser::parse(uint32_t canId, std::string& stringToParse){
	this->stringToParse=stringToParse;
	ESP_LOG_BUFFER_HEXDUMP(__FUNCTION__, stringToParse.c_str(), stringToParse.size(), ESP_LOG_DEBUG);
	uint32_t deviceUniqueId=(canId&0x7FFF0000)>>4;
	uint32_t deviceKindId=(candId&0x0000FFFF);
	bool isRequest=(candId&0x10000000)!=0;
	if(isRequest){
		return new MastervoltMessageRequest(deviceUniqueId, deviceKindId);
	}
	if(stringToParse.size()==0){
		return new MastervoltMessageUnknown(deviceUniqueId, deviceKindId, stringToParse);
	}

	uint8_t attributeId=stringToParse[0];
	uint8_t dataType=stringToParse[1];

	if(dataType==0x00){
		return new MastervoltMessageFloat(deviceUniqueId, deviceKindId, parseValueAsFloat());
	}
	else if(dataType==0x30){
		return new MastervoltMessageLabel(deviceUniqueId, deviceKindId, parseString());
	}

	return new MastervoltMessageUnknown(deviceUniqueId, deviceKindId, stringToParse);
}


float MvParser::parseValueAsFloat(){
	float* valueAsFloat=(float*) (stringToParse.c_str()+2);
	return *valueAsFloat;
}

std::string MvParser::parseString(){
	uint8_t segmentNumber=stringToParse[3];

	uint8_t numberOfCharsInPacket=stringToParse.size()-4;
	return stringToParse.substr(4, numberOfCharsInPacket);
}

MastervoltDeviceKind::MastervoltDeviceKind(uint32_t deviceKind) : deviceKind(deviceKind) {
	const uint32_t DEVICE_KIND_MASK=0x1000FFFF;
	if((deviceKind&DEVICE_KIND_MASK)!=0){
		ESP_LOGE(__FUNCTION__, "Don't pass in a canId here, pass in the device type, which is 0x%X", DEVICE_KIND_MASK);
	}
}

std::string MastervoltDeviceKind::toString() const{
	return "DeviceKindGoesHere";
}

MastervoltAttributeKind::MastervoltAttributeKind(uint16_t attributeKind, std::string textDescription, MastervoltEncoding encoding, MastervoltDataType dataType):
		parent(nullptr), attributeKind(attributeKind), textDescription(textDescription), encoding(encoding), dataType(dataType) {
}

void MastervoltDeviceKind::addAttribute(MastervoltAttributeKind att){
	att.parent=this;
	attributeKind.insert(std::pair<uint16_t, MastervoltAttributeKind>(att.attributeKind, att));
}

std::string MastervoltAttributeKind::packetValueToString(MastervoltPacket& packet) const {
	std::stringstream ss;
	if(encoding==FLOAT_ENCODING){
		if(dataType==TIME_DT){
			//Day of month = float%32, Year=float/32/13, Month=decimal remainder of year*13
			//840528.00==16/06/2020
			//840944.00==16/05/2021
			//840943.00==15/06/2021
			//840527.00==15/06/2020
			//840559.00==15/07/2020
			//840591.00==15/08/2020
			//832207.00==15/06/2000 : The masterview interface does not allow for less than year 2000
			//832034.00==02/01/2000 : The interface fails to show the date if we set the date to 01/01/2000. This is probably the default

			uint32_t secondElapsedSinceMidnight=packet.floatValue;
			uint16_t hour=secondElapsedSinceMidnight/60/60;
			secondElapsedSinceMidnight-=(hour*60*60);
			uint16_t minute=secondElapsedSinceMidnight/60;
			secondElapsedSinceMidnight-=(minute*60);
			uint16_t second=secondElapsedSinceMidnight;
			ss<<hour<<":"<<minute<<":"<<second;
		}
		else if(dataType==DATE_DT){
			uint32_t nbDaysZeroBC=packet.floatValue;
			uint16_t day=nbDaysZeroBC%32;
			nbDaysZeroBC-=day;
			float yearAndFraction=nbDaysZeroBC/416.0; //32 days *13 months=416
			uint16_t year=yearAndFraction;
			yearAndFraction-=year;
			uint16_t month=yearAndFraction*13;
			ss<<day<<"/"<<month<<"/"<<year;
		}
		else{
			ss.setf( std::ios::fixed, std::ios::floatfield );
			ss.precision(2);
			ss << packet.floatValue;
		}
	}
	else if(dataType==REQUEST_DT){
		ss << 'R';
	}
	else if(dataType==UNKNOWN_DT){
		ss<< std::setw(2) << std::setfill('0') << std::hex << std::uppercase;
		std::copy(packet.unparsed.begin(), packet.unparsed.end(), std::ostream_iterator<uint32_t>(ss, "_"));
		return ss.str();
	}
	else if(dataType==LABEL_DT){
		ss << "Label "<<packet.labelContent;
	}

	return ss.str();
}

std::string MastervoltAttributeKind::toString() const {
	return textDescription;
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

const uint32_t MastervoltDictionary::DCSHUNT_KIND=666;
const uint32_t MastervoltDictionary::INVERTER_KIND=666;
const uint32_t MastervoltDictionary::BATTERY_CHARGER_KIND=666;
const uint32_t MastervoltDictionary::MASTERVIEW_KIND=666;

MastervoltDictionary::MastervoltDictionary() : UNKNOWN_DEVICE_KIND(0), UNKNOWN_ATTRIBUTE_KIND(&UNKNOWN_DEVICE_KIND, MastervoltAttributeKind::UNKNOWN_DT, "Unknown", MastervoltAttributeKind::STRING_ENCODING, MastervoltAttributeKind::BYTES_DT) {
	MastervoltDeviceKind dcShuntDeviceKind(DCSHUNT_KIND);
	deviceKind.insert(std::pair<uint32_t, MastervoltDeviceKind>(DCSHUNT_KIND, dcShuntDeviceKind));
	dcShuntDeviceKind.addAttribute(MastervoltAttributeKind(DCSHUNT_BATTERY_PERCENTAGE, "BatteryPercentage", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT));
	dcShuntDeviceKind.addAttribute(MastervoltAttributeKind(DCSHUNT_BATTERY_VOLTS, "BatteryVolts", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT));
	dcShuntDeviceKind.addAttribute(MastervoltAttributeKind(DCSHUNT_BATTERY_AMPS, "BatteryAmps", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT));
	dcShuntDeviceKind.addAttribute(MastervoltAttributeKind(DCSHUNT_BATTERY_AMPS_CONSUMED, "BatteryAmpsConsumed", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT));
	dcShuntDeviceKind.addAttribute(MastervoltAttributeKind(DCSHUNT_BATTERY_TEMPERATURE, "BatteryTemperature", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT));
	dcShuntDeviceKind.addAttribute(MastervoltAttributeKind(DCSHUNT_DATE, "Date", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::DATE_DT));
	dcShuntDeviceKind.addAttribute(MastervoltAttributeKind(DCSHUNT_TIME, "Time", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::TIME_DT));

	MastervoltDeviceKind inverterDeviceKind(INVERTER_KIND);
	deviceKind.insert(std::pair<uint32_t, MastervoltDeviceKind>(INVERTER_KIND, inverterDeviceKind));
	inverterDeviceKind.addAttribute(MastervoltAttributeKind(INVERTER_AC_AMPS_OUT, "ACAmpsOut", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT));
	inverterDeviceKind.addAttribute(MastervoltAttributeKind(INVERTER_DC_AMPS_IN, "DCAmpsIn", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT));
	inverterDeviceKind.addAttribute(MastervoltAttributeKind(INVERTER_DC_VOLTAGE_IN, "DCVoltsIn", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT));
	inverterDeviceKind.addAttribute(MastervoltAttributeKind(INVERTER_STATE, "InverterState", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT));
}

const MastervoltAttributeKind& MastervoltDictionary::resolveAttribute(uint32_t canId, uint16_t attributeId){
	uint32_t deviceKindId=canId&0x0000FFFF;
	std::map<uint32_t, MastervoltDeviceKind>::iterator foundDeviceKind=deviceKind.find(deviceKindId);
	if(foundDeviceKind==deviceKind.end()){
		return UNKNOWN_ATTRIBUTE_KIND;
	}
	std::map<uint16_t, MastervoltAttributeKind>::iterator foundAttributeKind=foundDeviceKind->second.attributeKind.find(attributeId);
	if(foundAttributeKind==foundDeviceKind->second.attributeKind.end()){
		return UNKNOWN_ATTRIBUTE_KIND;
	}
	return foundAttributeKind->second;
}

