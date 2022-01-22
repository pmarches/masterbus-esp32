idf_component_register(
	SRCS "main/MCP2515.cpp" 
	 "main/masterbusController.cpp" 
	 "main/SPIBus.cpp" 
	 "main/mvParser.cpp" 
	 main/mastervoltMessage.cpp
	 main/mvDictionary.cpp
	 
    INCLUDE_DIRS "main/include"
)
