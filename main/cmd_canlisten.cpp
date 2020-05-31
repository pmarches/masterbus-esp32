#include <masterbusConsole.h>
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include <mvParser.h>

static struct {
	struct arg_int *mvbus;
	struct arg_lit *parse;
	struct arg_end *end;
} canListenArgs;

void print_packet(CANBusPacket packet, bool parsePacket){
	printf("DATA	0x%08lx	%d	", packet.canId, packet.dataLen);
	for(size_t i=0; i<packet.dataLen; i++){
		printf("%02x", packet.data[i]);
	}
	if(parsePacket){
		MvParser p;
		std::string dataStr((char*)packet.data, packet.dataLen);
		MastervoltPacket mvPacket;
		p.parse(packet.canId, dataStr, &mvPacket);
	}
	printf("\n");
}

static int canlisten(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **) &canListenArgs);
    if (nerrors != 0) {
        arg_print_errors(stderr, canListenArgs.end, argv[0]);
        return 1;
    }

    int whichMvBus=0;
    if(canListenArgs.mvbus->count){
    	whichMvBus=canListenArgs.mvbus->ival[0];
    }

    CANBusPacket packet;
    while(true){
    	bool hasPacket=g_mbctl[whichMvBus]->readPacket(packet);
    	if(hasPacket){
    		print_packet(packet, canListenArgs.parse->count>0);
    	}
//		vTaskDelay(5);
	}

	return 0;
}

void registerCanlisten(){
	canListenArgs.mvbus = arg_int0("m", "mvbus", "<0-1>", "Mastervolt bus #");
	canListenArgs.parse = arg_lit0("p", "parse", "Parse packet");
	canListenArgs.end = arg_end(1);

    const esp_console_cmd_t cmd = {
        .command = "canlisten",
        .help = "Listens for data over the canbus",
        .hint = NULL,
        .func = &canlisten,
		.argtable = &canListenArgs,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}
