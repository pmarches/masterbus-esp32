#include <mastervoltMessage.hpp>
#include <sstream>
#include <iomanip>
#include <iterator>

std::string MastervoltMessage::toString() const {
	std::stringstream ss;
	ss<<"deviceUniqueId=0x"<< std::setw(2) << std::setfill('0') << std::hex << std::uppercase<<deviceUniqueId;
	ss<< " deviceKindId=0x"<< std::setw(2) << std::setfill('0') << std::hex << std::uppercase<<deviceKindId;
	ss<< " attributeId=0x"<< std::setw(2) << std::setfill('0') << std::hex << attributeId;
	return ss.str();
}


std::string MastervoltMessageUnknown::toString() const {
	std::stringstream ss;
	ss<< MastervoltMessage::toString() ;
	ss<< " Unknown:" << std::setw(2) << std::setfill('0') << std::hex << std::uppercase;
	std::copy(unknownBytes.begin(), unknownBytes.end(), std::ostream_iterator<uint32_t>(ss, "_"));
	return ss.str();
}

std::string MastervoltMessageFloat::toString() const {
	std::stringstream ss;
	ss.setf( std::ios::fixed, std::ios::floatfield );
	ss.precision(2);
	ss << MastervoltMessage::toString() << " float: " << floatValue;
	return ss.str();
}

std::string MastervoltMessageLabel::toString() const{
	return this->label;
}
