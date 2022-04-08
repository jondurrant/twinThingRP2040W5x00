/*
 * TCPTransport.cpp
 *
 *TCP Socket transport.
 *TCP
 *  Created on: 5 Mar 2022
 *      Author: jondurrant
 */

#include "TCPTransport.h"
#include <stdlib.h>
#include "MQTTConfig.h"

/***
 * Constructors
 * @param sockNum - socket id
 * @param eth - Ethernet Helper object
 */
TCPTransport::TCPTransport(uint8_t sockNum, EthHelper *eth) {
	init(xSock, pEth);
}

/***
 * Constructor - requires init to be called
 */
TCPTransport::TCPTransport(){
	//NOP
}

/***
 * Provide Socket number and ethernet helper
 * @param sockNum - socker id
 * @param eth - Ethernet Helper object
 */
void TCPTransport::init(uint8_t sockNum, EthHelper *eth) {
	xSock = sockNum;
	pEth = eth;
}

/***
 * Distructor
 */
TCPTransport::~TCPTransport() {
	// TODO Auto-generated destructor stub
}


/***
 * Required by CoreMQTT returns time in ms
 * @return
 */
uint32_t TCPTransport::getCurrentTime(){
	return to_ms_since_boot(get_absolute_time ());
}

/***
 * Send data to socket in format used by FreeRTOS MQTT Lib
 * @param pNetworkContext - pointer to this object
 * @param pBuffer - buffer to send from
 * @param bytesToSend - number of bytes to send
 * @return number of bytes sent
 */
int32_t TCPTransport::transSend(NetworkContext_t * pNetworkContext, const void * pBuffer, size_t bytesToSend){
	uint32_t dataOut;
	//dataOut = send(xSock, (uint8_t *)pBuffer, bytesToSend);
	dataOut = pEth->tcpSockWrite(xSock, (uint8_t *)pBuffer, bytesToSend);
	if (dataOut != bytesToSend){
		LogError(("Send failed %d\n", dataOut));
	}
	return dataOut;
}

/***
 * Read data from socket
 * @param pNetworkContext - pointer to this object
 * @param pBuffer - buffer to read into
 * @param bytesToRecv - bytes to read
 * @return returns number of bytes read. 0 if none waiting. negative if error
 */
int32_t TCPTransport::transRead(NetworkContext_t * pNetworkContext, void * pBuffer, size_t bytesToRecv){
	int32_t dataIn=0;

	dataIn = pEth->tcpSockRead(xSock, (uint8_t *)pBuffer, bytesToRecv);
	return dataIn;
}

/***
 * Send data to socket in format used by FreeRTOS MQTT Lib
 * @param pNetworkContext - pointer to this object
 * @param pBuffer - buffer to send from
 * @param bytesToSend - number of bytes to send
 * @return number of bytes sent
 */
int32_t TCPTransport::staticSend(NetworkContext_t * pNetworkContext, const void * pBuffer, size_t bytesToSend){
	TCPTransport *t = (TCPTransport *)pNetworkContext->tcpTransport;
	return t->transSend(pNetworkContext, pBuffer, bytesToSend);
}


/***
 * Read data from socket
 * @param pNetworkContext - pointer to this object
 * @param pBuffer - buffer to read into
 * @param bytesToRecv - bytes to read
 * @return returns number of bytes read. 0 if none waiting. negative if error
 */
int32_t TCPTransport::staticRead(NetworkContext_t * pNetworkContext, void * pBuffer, size_t bytesToRecv){
	TCPTransport *t = (TCPTransport *)pNetworkContext->tcpTransport;
	return t->transRead(pNetworkContext, pBuffer, bytesToRecv);
}

/***
 * Connect TCP Socket
 * @param host - hostname
 * @param port - port number
 * @return true if successful
 */
bool TCPTransport::transConnect(const char * host, uint16_t port){
	if (!pEth->dnsClient(xHost, host)){
		return false;
	}
	return transConnect(xHost, port);
}

/***
 * Connect TCP Socket
 * @param ip - ip address
 * @param port - port
 * @return
 */
bool TCPTransport::transConnect(uint8_t * ip, uint16_t port){
	int8_t res;

	if (ip != xHost){
		memcpy(xHost, ip, 4);
	}

	return pEth->tcpSockConnect(xSock, 1080, xHost, port);
}

/***
 * Get status of socket
 * @return
 */
uint8_t TCPTransport::status(){
	uint8_t stat;
	getsockopt(xSock,SO_STATUS, &stat);
	return stat;
}

/***
 * Close Socket
 * @return
 */
bool TCPTransport::transClose(){
	pEth->tcpSockClose(xSock);
	//disconnect(xSock);
	return true;
}
