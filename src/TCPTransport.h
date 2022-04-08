/*
 * TCPTransport.h
 *
 * TCP Socket transport.
 *
 *  Created on: 5 Mar 2022
 *      Author: jondurrant
 */

#ifndef TCPTRANSPORT_H_
#define TCPTRANSPORT_H_

#include "core_mqtt.h"
#include "core_mqtt_agent.h"
#include "EthHelper.h"

extern "C" {
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "port_common.h"

#include "wizchip_conf.h"
#include "w5x00_spi.h"

#include "timer.h"
#include "socket.h"
}

class TCPTransport {
public:
	/***
	 * Constructors
	 * @param sockNum - socket id
	 * @param eth - Ethernet Helper object
	 */
	TCPTransport(uint8_t sockNum, EthHelper* eth);

	/***
	 * Constructor - requires init to be called
	 */
	TCPTransport();

	/***
	 * Distructor
	 */
	virtual ~TCPTransport();

	/***
	 * Provide Socket number and ethernet helper
	 * @param sockNum - socker id
	 * @param eth - Ethernet Helper object
	 */
	void init(uint8_t sockNum, EthHelper* eth);

	/***
	 * Connect TCP Socket
	 * @param host - hostname
	 * @param port - port number
	 * @return true if successful
	 */
	bool transConnect(const char * host, uint16_t port);

	/***
	 * Connect TCP Socket
	 * @param ip - ip address
	 * @param port - port
	 * @return
	 */
	bool transConnect(uint8_t * ip, uint16_t port);

	/***
	 * Get status of socket
	 * @return
	 */
	uint8_t status();

	/***
	 * Close Socket
	 * @return
	 */
	bool transClose();

	/***
	 * Send data to socket in format used by FreeRTOS MQTT Lib
	 * @param pNetworkContext - pointer to this object
	 * @param pBuffer - buffer to send from
	 * @param bytesToSend - number of bytes to send
	 * @return number of bytes sent
	 */
	int32_t transSend(NetworkContext_t * pNetworkContext, const void * pBuffer, size_t bytesToSend);

	/***
	 * Read data from socket
	 * @param pNetworkContext - pointer to this object
	 * @param pBuffer - buffer to read into
	 * @param bytesToRecv - bytes to read
	 * @return returns number of bytes read. 0 if none waiting. negative if error
	 */
	int32_t transRead(NetworkContext_t * pNetworkContext, void * pBuffer, size_t bytesToRecv);


	/***
	 * Static time function used by FreeRtos MQTT
	 * @return
	 */
	static uint32_t getCurrentTime();

	/***
	 * Send data to socket in format used by FreeRTOS MQTT Lib
	 * @param pNetworkContext - pointer to this object
	 * @param pBuffer - buffer to send from
	 * @param bytesToSend - number of bytes to send
	 * @return number of bytes sent
	 */
	static int32_t staticSend(NetworkContext_t * pNetworkContext, const void * pBuffer, size_t bytesToSend);

	/***
	 * Read data from socket
	 * @param pNetworkContext - pointer to this object
	 * @param pBuffer - buffer to read into
	 * @param bytesToRecv - bytes to read
	 * @return returns number of bytes read. 0 if none waiting. negative if error
	 */
	static int32_t staticRead(NetworkContext_t * pNetworkContext, void * pBuffer, size_t bytesToRecv);


private:


	uint8_t xSock = 0;

	uint8_t xHost[4];
	uint16_t xPort=80;
	EthHelper *pEth;

};

#endif /* TCPTRANSPORT_H_ */
