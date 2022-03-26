/*
 * TCPTransport.cpp
 *
 *  Created on: 5 Mar 2022
 *      Author: jondurrant
 */

#include "TCPTransport.h"
#include <stdlib.h>
#include "MQTTConfig.h"

TCPTransport::TCPTransport(uint8_t sockNum, EthHelper *eth) {
	init(xSock, pEth);
}

TCPTransport::TCPTransport(){
	//NOP
}


void TCPTransport::init(uint8_t sockNum, EthHelper *eth) {
	xSock = sockNum;
	pEth = eth;
}

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

int32_t TCPTransport::transSend(NetworkContext_t * pNetworkContext, const void * pBuffer, size_t bytesToSend){
	uint32_t dataOut;
	//dataOut = send(xSock, (uint8_t *)pBuffer, bytesToSend);
	dataOut = pEth->tcpSockWrite(xSock, (uint8_t *)pBuffer, bytesToSend);
	if (dataOut != bytesToSend){
		LogError(("Send failed %d\n", dataOut));
	}
	return dataOut;
}


int32_t TCPTransport::transRead(NetworkContext_t * pNetworkContext, void * pBuffer, size_t bytesToRecv){
	int32_t dataIn=0;

	dataIn = pEth->tcpSockRead(xSock, (uint8_t *)pBuffer, bytesToRecv);
	return dataIn;


/*
	uint16_t remaining=0;
	int8_t res;
	uint8_t status;

	getsockopt(xSock,SO_STATUS, &status);
	if (status != SOCK_ESTABLISHED){
		printf("SOCKET NOT OPEN\n");
		return -1;
	}

	res = getsockopt(xSock,SO_REMAINSIZE, &remaining);
	if (res != SOCK_OK){
		//printf("FAIL\b");
		return res;
	}

	if  (remaining > 0){
		dataIn = recv(xSock, (uint8_t *)pBuffer, bytesToRecv);

		//printf("Read(%d)=%d\n", bytesToRecv, dataIn);

		if (dataIn == SOCK_BUSY){
			dataIn = 0;
		}
	}

	//printf("transRead(%d)=%d\n", bytesToRecv, dataIn);
	return dataIn;
*/
}


int32_t TCPTransport::staticSend(NetworkContext_t * pNetworkContext, const void * pBuffer, size_t bytesToSend){
	TCPTransport *t = (TCPTransport *)pNetworkContext->tcpTransport;
	return t->transSend(pNetworkContext, pBuffer, bytesToSend);
}



int32_t TCPTransport::staticRead(NetworkContext_t * pNetworkContext, void * pBuffer, size_t bytesToRecv){
	TCPTransport *t = (TCPTransport *)pNetworkContext->tcpTransport;
	return t->transRead(pNetworkContext, pBuffer, bytesToRecv);
}


bool TCPTransport::transConnect(const char * host, uint16_t port){
	if (!pEth->dnsClient(xHost, host)){
		return false;
	}
	return transConnect(xHost, port);
}

bool TCPTransport::transConnect(uint8_t * ip, uint16_t port){
	int8_t res;

	if (ip != xHost){
		memcpy(xHost, ip, 4);
	}

	return pEth->tcpSockConnect(xSock, 1080, xHost, port);
/*
	res = socket(xSock, Sn_MR_TCP, 1080, SF_TCP_NODELAY);
	if (res != xSock){
		LogError(("Socket %d", res));
		return false;
	}
	res = connect(xSock, xHost, port);
	if (res != SOCK_OK){
		LogError(("Socket connect error %d", res));
		return false;
	}
	return true;
*/
}

uint8_t TCPTransport::status(){
	uint8_t stat;
	getsockopt(xSock,SO_STATUS, &stat);
	return stat;
}

bool TCPTransport::transClose(){
	pEth->tcpSockClose(xSock);
	//disconnect(xSock);
	return true;
}
