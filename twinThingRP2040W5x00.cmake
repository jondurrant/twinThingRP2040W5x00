# Add library cpp files
#set(FREERTOS_PORT_PATH      ${CMAKE_CURRENT_LIST_DIR}/lib/twinThingPicoESP/port/FreeRTOS)


add_library(twinThingRP2040W5x00 INTERFACE)
target_sources(twinThingRP2040W5x00 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/EthHelper.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/MQTTAgent.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/TCPTransport.cpp
    
    ${CMAKE_CURRENT_LIST_DIR}/lib/twinThingPicoESP/src/MQTTInterface.cpp
    ${CMAKE_CURRENT_LIST_DIR}/lib/twinThingPicoESP/src/MQTTRouter.cpp
    ${CMAKE_CURRENT_LIST_DIR}/lib/twinThingPicoESP/src/MQTTRouterPing.cpp
    ${CMAKE_CURRENT_LIST_DIR}/lib/twinThingPicoESP/src/MQTTPingTask.cpp
    ${CMAKE_CURRENT_LIST_DIR}/lib/twinThingPicoESP/src/MQTTTopicHelper.cpp
    ${CMAKE_CURRENT_LIST_DIR}/lib/twinThingPicoESP/src/State.cpp
    ${CMAKE_CURRENT_LIST_DIR}/lib/twinThingPicoESP/src/StateObserver.cpp
    ${CMAKE_CURRENT_LIST_DIR}/lib/twinThingPicoESP/src/StateTemp.cpp
    ${CMAKE_CURRENT_LIST_DIR}/lib/twinThingPicoESP/src/TwinTask.cpp
    ${CMAKE_CURRENT_LIST_DIR}/lib/twinThingPicoESP/src/MQTTRouterTwin.cpp
	${CMAKE_CURRENT_LIST_DIR}/lib/twinThingPicoESP/src/MQTTAgentObserver.cpp
)

# Add include directory
target_include_directories(twinThingRP2040W5x00 INTERFACE 
	${CMAKE_CURRENT_LIST_DIR}/src
	${CMAKE_CURRENT_LIST_DIR}/lib/twinThingPicoESP/src/
	${TWIN_THING_PICO_CONFIG_PATH}
)

# Add the standard library to the build
target_link_libraries(twinThingRP2040W5x00 INTERFACE 
	pico_stdlib 
	pico_unique_id
	hardware_adc 
	json_maker 
	tiny_json
	IOLIBRARY_FILES
	W5100S_FILES
	ETHERNET_FILES
	DHCP_FILES
	DNS_FILES
	FreeRTOS
	coreMQTT 
	coreMQTTAgent
	)

