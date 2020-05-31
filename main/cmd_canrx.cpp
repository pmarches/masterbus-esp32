#include <masterbusConsole.h>
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "esp_console.h"

static int canRx(int argc, char **argv) {
//	g_mbctl[0]->print_packet();
	return 0;
}

void registerCanRx(){
	const esp_console_cmd_t cmd = {
        .command = "canrx",
        .help = "Listens for one packet",
        .hint = NULL,
        .func = &canRx,
		.argtable = NULL,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}
