#include <masterbusConsole.h>
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_console.h"

static struct {
	struct arg_lit *bidirection;
	struct arg_end *end;
} canFwdArgs;


static int canfwd(int argc, char **argv) {
    int nerrors = arg_parse(argc, argv, (void **) &canFwdArgs);
    if (nerrors != 0) {
        arg_print_errors(stderr, canFwdArgs.end, argv[0]);
        return 1;
    }

    CANBusPacket packet;
	while(true){
		if(g_mbctl[0]->readPacket(packet)){
			g_mbctl[1]->send(packet.canId, packet.data, packet.dataLen);
		}
		if(canFwdArgs.bidirection->count==1){
			if(g_mbctl[1]->readPacket(packet)){
				g_mbctl[0]->send(packet.canId, packet.data, packet.dataLen);
			}
		}
	}
	return 0;
}

void registerCanFwd(){
	canFwdArgs.bidirection = arg_lit0("b", "bidirectional", "Bidirectional mode");
	canFwdArgs.end = arg_end(1);

	const esp_console_cmd_t cmd = {
        .command = "canfwd",
        .help = "Forwards data over the canbus",
        .hint = NULL,
        .func = &canfwd,
		.argtable = &canFwdArgs,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}
