/*
 * TCPTransport.h
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
	TCPTransport(uint8_t sockNum, EthHelper* eth);
	TCPTransport();
	virtual ~TCPTransport();

	void init(uint8_t sockNum, EthHelper* eth);

	bool transConnect(const char * host, uint16_t port);
	bool transConnect(uint8_t * ip, uint16_t port);

	uint8_t status();

	bool transClose();

	int32_t transSend(NetworkContext_t * pNetworkContext, const void * pBuffer, size_t bytesToSend);
	int32_t transRead(NetworkContext_t * pNetworkContext, void * pBuffer, size_t bytesToRecv);


	static uint32_t getCurrentTime();
	static int32_t staticSend(NetworkContext_t * pNetworkContext, const void * pBuffer, size_t bytesToSend);
	static int32_t staticRead(NetworkContext_t * pNetworkContext, void * pBuffer, size_t bytesToRecv);


private:


	uint8_t xSock = 0;

	uint8_t xHost[4];
	uint16_t xPort=80;
	EthHelper *pEth;

};

#endif /* TCPTRANSPORT_H_ */
