# twinThingRP2040W5x00
IOT Libary for RP2040 using W5x00 Hat or board from Wiznet, using RJ45 Ethernet cable ([WizNet W5100S Rp2040](https://www.wiznet.io/product-item/w5100s-evb-pico/))  

Example code using the library can be found in  [IotFanControlerW5000](https://github.com/jondurrant/IoTFanControlerW5000). 

## IOT
Library connects RP2040 to MQTT hub as a client which can publish and subscribe to topics. 

Structures

TNG/<ID>/ - THING / ID, same as user name in my example
TNG/<ID>/LC - LC is lifecycle and annouces connection and disconnection
TNG/<ID>/TPC - TOPIC, for messaging to a specific thing

Example assuming you name the pico as "pico"
+ TNG/pico/TPC/PING - ping re	quest sent to pico
+ TNG/pico/TPC/PONG - pong as response to the ping

Group topics also exist in the structure under.

GRP/<Group>/TPC/ - Topics to talk to group of devices

Examle:
GRP/ALL/TPC/PING - ping topic that all IoT devices listen for.

Tested against:
+ [Mosquitto](https://mosquitto.org/) on a RPI4. Followed the docker install guide at: (https://docs.cedalo.com/streamsheets/2.4/installation/)
+ [EMQX](https://www.emqx.io/) 

# Hardware
+ [WizNet W5100S Rp2040](https://www.wiznet.io/product-item/w5100s-evb-pico/)


# Dependencies
## C++
+ [FreeRTOS-Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel): Task management
+ [FreeRTOS-CoreMQTT-Agent](https://www.freertos.org/mqtt-agent/index.html): MQTT Library
+ [ioLibary_Driver](https://github.com/Wiznet/ioLibrary_Driver): Drivers for W5100s
+ [twinThingPicoESP](https://github.com/jondurrant/twinThingPicoESP): IOT Twin Lib
+ [json-maker](https://github.com/rafagafe/json-maker): JSON writting for comms
+ [tiny-json](https://github.com/rafagafe/tiny-json): JSON paser

# Build
See example:
[IotFanControlerW5000](https://github.com/jondurrant/IoTFanControlerW5000). 