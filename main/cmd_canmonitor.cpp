#include <masterbusConsole.h>
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_console.h"


#include <map>
#include <vector>
#include <algorithm>

#include <cstring>
#include <freertos/task.h>

class RegisterMonitor {
public:
	long int canId;
	uint32_t updateCounter;
	bool hasChangedSinceLastPrint;
	TickType_t lastTickCount;
	std::vector<std::string> previousValues;

	RegisterMonitor(): canId(0), updateCounter(0), hasChangedSinceLastPrint(false), lastTickCount(0){
	}
};

std::map<long int, RegisterMonitor> registers;

void printRegister(RegisterMonitor& value){
	TickType_t now=xTaskGetTickCount();
	printf("0x%08lx    %06d	%04d	%c	", value.canId, now-value.lastTickCount, value.updateCounter, value.hasChangedSinceLastPrint?'X':' ');
	for(auto hexValIt=value.previousValues.begin(); hexValIt!=value.previousValues.end(); hexValIt++){
		printf("%s ", hexValIt->c_str());
	}
	printf("\n");
}

void printRegisters(){
	printf("\033cBegin------\n");
	std::vector<long int> sortedKeys;

	auto it=registers.begin();
	for(;it!=registers.end(); it++){
		sortedKeys.push_back(it->first);
	}
	std::sort(sortedKeys.begin(), sortedKeys.end());


	auto sortedKeyIt=sortedKeys.begin();
	for(;sortedKeyIt<sortedKeys.end(); sortedKeyIt++){
		RegisterMonitor& reg=registers[*sortedKeyIt];
		printRegister(reg);
		reg.hasChangedSinceLastPrint=false;
	}
}

static int canmonitor(int argc, char **argv) {
	CANBusPacket packetRead;
	for(uint32_t i=0;i<200000; i++){
		if(g_mbctl[0]->readPacket(packetRead)){
			RegisterMonitor& existingRegister=registers[packetRead.canId];
			if(existingRegister.canId==0){ //First time we get this packet type
				existingRegister.canId=packetRead.canId;
			}
			existingRegister.updateCounter++;
			existingRegister.lastTickCount=xTaskGetTickCount();
			std::string valueHexStr=packetRead.valueToHexString();
			if(existingRegister.previousValues.size()==0 || existingRegister.previousValues.front()!=valueHexStr){
				existingRegister.previousValues.insert(existingRegister.previousValues.begin(), valueHexStr);
				existingRegister.previousValues.resize(8);
			}

			existingRegister.hasChangedSinceLastPrint=true;

			if(i%10==0) printRegisters();
		}
	}

	return 0;
}

void registerCanMonitor(){
	const esp_console_cmd_t cmd = {
        .command = "canmonitor",
        .help = "Display live canbus data",
        .hint = NULL,
        .func = &canmonitor,
		.argtable = NULL,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}
