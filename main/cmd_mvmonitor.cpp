#include <masterbusConsole.h>
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_console.h"


#include <map>
#include <vector>
#include <algorithm>

#include <sstream>
#include <iomanip>
#include <iterator>

#include <cstring>
#include <freertos/task.h>

#include <mvParser.h>

std::map<std::string, std::vector<std::string>> allValueHistory;

void printValueHistory(std::string key, std::vector<std::string> valueHistory){
	printf("%s	", key.c_str());
	for(auto it=valueHistory.begin(); it!=valueHistory.end(); it++){
		printf("%s	", it->c_str());
	}
	printf("\n");
}

void printValueHistory(){
	printf("\033cBegin------\n");
	std::vector<std::string> sortedKeys;

	auto it=allValueHistory.begin();
	for(;it!=allValueHistory.end(); it++){
		sortedKeys.push_back(it->first);
	}
	std::sort(sortedKeys.begin(), sortedKeys.end());


	auto sortedKeyIt=sortedKeys.begin();
	for(;sortedKeyIt<sortedKeys.end(); sortedKeyIt++){
		std::vector<std::string>& history=allValueHistory[*sortedKeyIt];
		printValueHistory(*sortedKeyIt, history);
	}
}

std::string makeKeyFromMVPacket(MastervoltPacket& mastervoltPacket){
	std::stringstream ss;
	ss << MastervoltDictionary::getInstance()->resolveCanIdToLabel(mastervoltPacket.canId)<< '/';
	ss << "0x" << std::setw(2) << std::setfill('0') << std::hex << (int) mastervoltPacket.dataType << std::dec << '/';
	ss << MastervoltDictionary::getInstance()->resolveAttributeIdToLabel(mastervoltPacket.attributeId);
	return ss.str();
}

std::string makeValueFromMVPacket(MastervoltPacket& mastervoltPacket){
	std::stringstream ss;
	if(mastervoltPacket.valueType==MastervoltPacket::FLOAT){
		ss.setf( std::ios::fixed, std::ios::floatfield );
		ss.precision(2);
		ss << mastervoltPacket.floatValue;
	}
	else if(mastervoltPacket.valueType==MastervoltPacket:: UNKNOWN){
		ss<< std::setw(2) << std::setfill('0') << std::hex << std::uppercase;
		std::copy(mastervoltPacket.unparsed.begin(), mastervoltPacket.unparsed.end(), std::ostream_iterator<uint32_t>(ss, "_"));
		return ss.str();
	}
	else if(mastervoltPacket.valueType==MastervoltPacket::REQUEST){
		ss << 'R';
	}
	else if(mastervoltPacket.valueType==MastervoltPacket::LABEL){
		ss << "Label "<<mastervoltPacket.labelContent;
	}
	return ss.str();
}

static int mvmonitor(int argc, char **argv) {
	esp_log_level_set("*", ESP_LOG_ERROR);

	CANBusPacket packetRead;
	MvParser parser;
	for(uint32_t i=0;i<200000; i++){
		if(g_mbctl[0]->readPacket(packetRead)){
			std::string packetData=packetRead.getData();
			if((packetRead.canId & 0x10000000) != 0){ //Ignoring packets with canid like 0x18ef1297. They seems to be related to stats or configuration...
				continue;
			}

			MastervoltPacket mastervoltPacket;
			if(parser.parse(packetRead.canId, packetData, &mastervoltPacket)){
				if(mastervoltPacket.valueType==MastervoltPacket::REQUEST){
					continue;
				}
				std::string key=makeKeyFromMVPacket(mastervoltPacket);
				std::string value=makeValueFromMVPacket(mastervoltPacket);
				std::vector<std::string>& values=allValueHistory[key];

//				if(values.size()>=6){
//					values.erase(values.begin());
//				}
#if 0
				values.push_back(value);
#else
				if(std::find(values.begin(), values.end(), value)==values.end()){
					values.push_back(value);
				}
#endif
			}
			if(i%10==0) printValueHistory();
		}
	}

	return 0;
}

void registerMvMonitor(){
	const esp_console_cmd_t cmd = {
        .command = "mvmonitor",
        .help = "Display live mastervolt data",
        .hint = NULL,
        .func = &mvmonitor,
		.argtable = NULL,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}
