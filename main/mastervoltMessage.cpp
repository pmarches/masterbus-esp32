#include <mastervoltMessage.hpp>
#include <sstream>
#include <iomanip>
#include <iterator>

std::string MastervoltMessage::toString() const {
	std::stringstream ss;
	ss<<"deviceUniqueId=0x"<< std::setw(5) << std::setfill('0') << std::hex << std::uppercase<<deviceUniqueId;
	ss<< " deviceKindId=0x"<< std::setw(4) << std::setfill('0') << std::hex << std::uppercase<<deviceKindId;
	ss<< " attributeId=0x"<< std::setw(2) << std::setfill('0') << std::hex << attributeId;
	if(FLOAT==type){
	  ss.setf( std::ios::fixed, std::ios::floatfield );
	  ss.precision(2);
	  ss << " float: " << value.floatValue;
	}
	else if(TIME==type){
	  ss<<(uint16_t)value.hour<<":"<<(uint16_t)value.minute<<":"<<(uint16_t)value.second;
	}
	else if(DATE==type){
	  ss<<value.day<<"/"<<value.month<<"/"<<value.year;
	}
	else {
    ss << "Unhandled toString for type "<<type;
	}
	return ss.str();
}


//std::string MastervoltMessageUnknown::toString() const {
//	std::stringstream ss;
//	ss<< MastervoltMessage::toString() ;
//	ss<< " Unknown:" << std::setw(2) << std::setfill('0') << std::hex << std::uppercase;
//	std::copy(unknownBytes.begin(), unknownBytes.end(), std::ostream_iterator<uint32_t>(ss, "_"));
//	return ss.str();
//}
//
