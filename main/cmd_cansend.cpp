#include <masterbusConsole.h>
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_console.h"
#include <memory.h>

extern int hex2int(char input);

static struct {
	struct arg_int *canId;
	struct arg_int *mvbus;
	struct arg_str *data;
	struct arg_lit *waitForResponse;
	struct arg_end *end;
} canSendArgs;

static int cansend(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **) &canSendArgs);
    if (nerrors != 0) {
        arg_print_errors(stderr, canSendArgs.end, argv[0]);
        return 1;
    }

    long int canId=canSendArgs.canId->ival[0];
	printf("canSendArgs.canId=0x%lx\n", canId);
	printf("canSendArgs.data=%s\n", canSendArgs.data->sval[0]);
	printf("canSendArgs.waitResponse=%d\n", canSendArgs.waitForResponse->count);
	uint8_t nbHexChars=strlen(canSendArgs.data->sval[0]);
	if(nbHexChars%2!=0){
        fprintf(stderr, "Odd number of hex chars in '%s'", canSendArgs.data->sval[0]);
        return 1;
	}
	uint32_t nbCanBytes=nbHexChars/2;
	uint8_t* canBytes=(uint8_t*) malloc(nbCanBytes);
	const char* hexPtr=canSendArgs.data->sval[0];
	for(int i=0; i<nbCanBytes; i++){
		canBytes[i]=hex2int(hexPtr[0])*16 + hex2int(hexPtr[1]);
		hexPtr+=2;
	}
	uint8_t whichMasterBus=0;
	if(canSendArgs.mvbus->count){
		whichMasterBus=canSendArgs.mvbus->ival[0];
	}
	g_mbctl[whichMasterBus]->send(canId, canBytes, nbCanBytes);

	free(canBytes);

//	for(int i=0; i<canSendArgs.waitForResponse->count;){
//		printf("Waiting for response %d\n", i);
//		if(g_mbctl[whichMasterBus]->print_packet()){
//			i++;
//		}
//	}
	return 0;
}

void registerCanSend(){
	canSendArgs.waitForResponse = arg_lit0("w", "waitresponse", "Wait for response by doing cansend just after");
	canSendArgs.canId = arg_int1("i", "id", "<canId>", "Attribute Id");
	canSendArgs.mvbus = arg_int0("m", "mvbus", "<0|1>", "Mastervolt bus #");
	canSendArgs.data = arg_str1("d", "data", "<d>", "Data to send");
	canSendArgs.end = arg_end(3);

	const esp_console_cmd_t cmd = {
        .command = "cansend",
        .help = "Sends data over the canbus",
        .hint = NULL,
        .func = &cansend,
		.argtable = &canSendArgs,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}
