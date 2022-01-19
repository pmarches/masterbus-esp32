#include "mvDictionary.hpp"
#include <sstream>
#include <iomanip>

MastervoltDictionary* MastervoltDictionary::instance=nullptr;

//TODO These values are dependant on the device type. The device type is encoded in the CANId, but some bits are unique per device.
//TODO Refactor these attributes into a per device organization
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


const uint16_t MastervoltDictionary::INVERTER_DC_VOLTAGE_IN=0x06;
const uint16_t MastervoltDictionary::INVERTER_DC_AMPS_IN=0x07;
const uint16_t MastervoltDictionary::INVERTER_AC_AMPS_OUT=0x0b;
const uint16_t MastervoltDictionary::INVERTER_LOAD_PERCENT=0x11;
const uint16_t MastervoltDictionary::INVERTER_AC_VOLTAGE_OUT=0x0a;
const uint16_t MastervoltDictionary::INVERTER_SHORE_FUSE=0x13; //4 Amp offset
const uint16_t MastervoltDictionary::INVERTER_MODE=0x14; //1.0=On 0.0=Off 3==Standby?
const uint16_t MastervoltDictionary::INVERTER_POWER_STATE=0x38; //1.0=On 0.0=Off
const uint16_t MastervoltDictionary::CHARGER_STATE=0x3a; //1.0=On 0.0=Off
const uint16_t MastervoltDictionary::INVERTER_INPUT_GENSET=0x0e; //1.0=On 0.0=Off

const uint32_t MastervoltDictionary::DCSHUNT_KIND=0x021b;
const uint32_t MastervoltDictionary::INVERTER_KIND=0x020e;
const uint32_t MastervoltDictionary::BATTERY_CHARGER_KIND=0x667;
const uint32_t MastervoltDictionary::MASTERVIEW_KIND=0x0194;

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
  dcShuntDeviceKind->addAttribute(0x04, "ChargeTimeRemainig?", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);

  MastervoltDeviceKind* inverterDeviceKind=new MastervoltDeviceKind(INVERTER_KIND, "InverterDevice");
  deviceKind.insert(std::pair<uint32_t, MastervoltDeviceKind*>(inverterDeviceKind->deviceKind, inverterDeviceKind));
  inverterDeviceKind->addAttribute(INVERTER_AC_VOLTAGE_OUT, "ACVoltageOut", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);
  inverterDeviceKind->addAttribute(INVERTER_AC_AMPS_OUT, "ACAmpsOut", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);
  inverterDeviceKind->addAttribute(INVERTER_DC_AMPS_IN, "DCAmpsIn", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);
  inverterDeviceKind->addAttribute(INVERTER_DC_VOLTAGE_IN, "DCVoltsIn", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);
  inverterDeviceKind->addAttribute(INVERTER_MODE, "InverterMode", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);
  inverterDeviceKind->addAttribute(INVERTER_POWER_STATE, "InverterPowerState", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);
  inverterDeviceKind->addAttribute(INVERTER_SHORE_FUSE, "InverterShoreFuse", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);
  inverterDeviceKind->addAttribute(CHARGER_STATE, "ChargerState", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);
  inverterDeviceKind->addAttribute(INVERTER_LOAD_PERCENT, "LoadPercent", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);
  inverterDeviceKind->addAttribute(INVERTER_INPUT_GENSET, "InputGenset", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);
  inverterDeviceKind->addAttribute(0x10, "Mode?", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::SIMPLE_FLOAT_DT);

  MastervoltDeviceKind* mainMasterview=new MastervoltDeviceKind(MASTERVIEW_KIND, "Masterview");
  deviceKind.insert(std::pair<uint32_t, MastervoltDeviceKind*>(mainMasterview->deviceKind, mainMasterview));

  MastervoltDeviceKind* mysteryACDevice=new MastervoltDeviceKind(0x060e, "mysteryACDevice");
  deviceKind.insert(std::pair<uint32_t, MastervoltDeviceKind*>(mysteryACDevice->deviceKind, mysteryACDevice));

  MastervoltDeviceKind* deviceShownOnCustomPanel=new MastervoltDeviceKind(0x061b, "deviceShownOnCustomPanel");
  deviceKind.insert(std::pair<uint32_t, MastervoltDeviceKind*>(deviceShownOnCustomPanel->deviceKind, deviceShownOnCustomPanel));
  deviceShownOnCustomPanel->addAttribute(0x00, "Always Zero?", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::UNKNOWN_DT);
  deviceShownOnCustomPanel->addAttribute(0x01, "Always One?", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::UNKNOWN_DT);
  deviceShownOnCustomPanel->addAttribute(0x02, "UKN0x02", MastervoltAttributeKind::STRING_ENCODING, MastervoltAttributeKind::UNKNOWN_DT);


  MastervoltDeviceKind* deviceRefrencingOthers=new MastervoltDeviceKind(0x010e, "deviceRefrencingOthers");
  deviceKind.insert(std::pair<uint32_t, MastervoltDeviceKind*>(deviceRefrencingOthers->deviceKind, deviceRefrencingOthers));
  deviceRefrencingOthers->addAttribute(0x0e, "Always 0E_12_F4_2_36_1_0_0 ? ", MastervoltAttributeKind::STRING_ENCODING, MastervoltAttributeKind::UNKNOWN_DT);

  MastervoltDeviceKind* deviceRefrencingOthers2=new MastervoltDeviceKind(0x011b, "deviceRefrencingOthers2");
  deviceKind.insert(std::pair<uint32_t, MastervoltDeviceKind*>(deviceRefrencingOthers2->deviceKind, deviceRefrencingOthers2));
  deviceRefrencingOthers2->addAttribute(0x1b, "Always 1B_97_12_3_2_2_0_0_ ? ", MastervoltAttributeKind::STRING_ENCODING, MastervoltAttributeKind::UNKNOWN_DT);

  MastervoltDeviceKind* unknownFloats=new MastervoltDeviceKind(0x023b, "unknownFloats");
  deviceKind.insert(std::pair<uint32_t, MastervoltDeviceKind*>(unknownFloats->deviceKind, unknownFloats));
  unknownFloats->addAttribute(0x08, "Some float ", MastervoltAttributeKind::FLOAT_ENCODING, MastervoltAttributeKind::UNKNOWN_DT);
}

std::string MastervoltDictionary::toString() const {
  std::stringstream ss;
  ss<< "DeviceKind known="<<deviceKind.size()<<"\n";
  std::map<uint32_t, MastervoltDeviceKind*>::const_iterator it=deviceKind.begin();
  for(;it!=deviceKind.end(); it++){
    ss<<" Device 0x"<<std::hex<<it->first<<std::dec<<" ->"<<it->second->attributes.size()<<"\n";
    auto attIt=it->second->attributes.begin();
    for(; attIt!=it->second->attributes.end(); attIt++){
      ss<<"   "<<attIt->second->toString()<<"\n";
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

