idf_component_register(
	SRCS "MCP2515.cpp" "masterbusController.cpp" "CANController.cpp" "masterbusConsole.cpp" "cmd_canfwd.cpp" "cmd_canlisten.cpp" "cmd_canmonitor.cpp" "cmd_canrx.cpp" "cmd_cansend.cpp" "cmd_mvparse.cpp" "SPIBus.cpp" "mvParser.cpp" cmd_mvmonitor.cpp
    INCLUDE_DIRS "include"
)
