#ifndef MAIN_INCLUDE_MVPARSER_H_
#define MAIN_INCLUDE_MVPARSER_H_

#include <mastervoltMessage.hpp>
#include <string>
#include <map>

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
		: MastervoltAttributeKind(attributeKind, textDescription, encoding, dataType)
		  {
		this->parent=parent;
	}
	MastervoltAttributeKind(uint16_t attributeKind, std::string textDescription, MastervoltEncoding encoding, MastervoltDataType dataType);
	const MastervoltDeviceKind* parent;
	uint16_t attributeKind;
	std::string textDescription;
	MastervoltEncoding encoding; //How is the underlying data encoded? With floats? Strings?
	MastervoltDataType dataType; //What does the data look like? A date? A float? Text?

	std::string toString() const;
};

class MastervoltDeviceKind {
public:
	MastervoltDeviceKind(uint32_t deviceKind, std::string textDescription);
	void addAttribute(uint16_t attributeKind, std::string textDescription, MastervoltAttributeKind::MastervoltEncoding encoding, MastervoltAttributeKind::MastervoltDataType dataType);
	std::string toString() const;
	uint32_t deviceKind;
	std::string textDescription;
	std::map<uint16_t, MastervoltAttributeKind*> attributes;
};

class MastervoltDictionary {
protected:
	MastervoltDictionary();
	static MastervoltDictionary* instance;
	std::map<uint32_t, MastervoltDeviceKind*> deviceKind;
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
	static const uint16_t INVERTER_AC_VOLTAGE_OUT;
	static const uint16_t INVERTER_SHORE_FUSE;
	static const uint16_t INVERTER_MODE;
	static const uint16_t INVERTER_LOAD_PERCENT;
	static const uint16_t INVERTER_POWER_STATE; //0=Off, 1=On
	static const uint16_t CHARGER_STATE; //0=Off, 1=On
	static const uint16_t INVERTER_INPUT_GENSET;

	static const uint16_t DCSHUNT_DAYOFMONTH;
	static const uint16_t DCSHUNT_MONTHOFYEAR;
	static const uint16_t DCSHUNT_YEAROFCALENDAR;


	static MastervoltDictionary* getInstance() { if(instance==nullptr) instance=new MastervoltDictionary(); return instance;}
	const MastervoltAttributeKind* resolveAttribute(uint32_t deviceKindId, uint16_t attributeId);
	const MastervoltDeviceKind* resolveDevice(uint32_t deviceKindId);
	std::string toString() const;
};

class MvParser {
public:
	MastervoltMessage* parse(uint32_t canId, std::string& stringToParse);
	std::string parseString();
	float parseValueAsFloat();
	std::string stringToParse;
};


#endif /* MAIN_INCLUDE_MVPARSER_H_ */
