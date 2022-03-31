/*
 * EthHelper.h
 *
 *  Created on: 3-Mar-2022
 *      Author: jondurrant
 */

#ifndef SRC_ETHHELPER_H_
#define SRC_ETHHELPER_H_

//#include "MQTTConfig.h"
#include <stdlib.h>

extern "C" {
#include "port_common.h"
#include "wizchip_conf.h"
#include "w5x00_spi.h"
#include "dhcp.h"
#include "dns.h"
#include "timer.h"

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include "socket.h"
}
#include <stdint.h>


#ifndef DHCP_RETRY_COUNT
#define DHCP_RETRY_COUNT 5
#endif

#ifndef ETHMUTEXTICKS
#define ETHMUTEXTICKS 100
#endif

class EthHelper {
public:
	EthHelper();

	virtual ~EthHelper();

	void init(uint8_t * pBuf);

	void enableMutex();

	/***
	 * Get IP address of unit
	 * @param ip - output uint8_t[4]
	 * @return - true if IP addres assigned
	 */
	 bool getIPAddress(uint8_t *ip);

	/***
	 * Get IP address of unit
	 * @param ips - output char * up to 16 chars
	 * @return - true if IP addres assigned
	 */
	bool getIPAddressStr(char *ips);


	bool getMACAddressStr(char *macStr);


	void rtcInit();
	bool syncRTCwithSNTP(uint8_t *sntpSvrIp);
	bool syncRTCwithSNTP(char *sntpSvrHost);
	bool syncRTCwithSNTP(const char **sntpSvrHosts, uint8_t count);
	bool syncRTCwithSNTP();

	void setSNTPServers(const char **sntpSvrHosts, uint8_t count);

	bool isJoined();

	bool isPluggedIn();

	bool dhcpClient();

	bool dnsClient(uint8_t *ip,  const char * host);

	bool tcpSockConnect(uint8_t sock, uint16_t localPort, uint8_t * hostIP, uint16_t hostPort);
	bool tcpSockClose(uint8_t sock);
	uint32_t tcpSockRead(uint8_t sock, uint8_t *buf, size_t bytesToRecv);
	uint32_t tcpSockWrite(uint8_t sock, uint8_t *buf, size_t bytesToSend);


protected:
	void dhcpAssign(void);
	void dhcpInit(void);



private:

	uint32_t tcpSockReadLocal(uint8_t sock, uint8_t *buf, size_t bytesToRecv);

	bool dhcpClientLocal();


	static void cbDhcpConflict(void);
	static void cbDhcpAssign(void);
	static void cbRepeatingTimer(void);


	wiz_NetInfo xNetInfo ={
	        .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56}, // MAC address
	        .ip = {0, 0, 0, 0},                     // IP address
	        .sn = {255, 255, 255, 0},                    // Subnet Mask
	        .gw = {192, 168, 11, 1},                     // Gateway
	        .dns = {8, 8, 8, 8},                         // DNS server
	        .dhcp = NETINFO_DHCP                         // DHCP enable/disable
	};

	uint8_t * pEthernetBuf;

	SemaphoreHandle_t xSemaphore;

	const char **pSntpSvrHosts = NULL;
	uint8_t xSntpCount = 0;


	static uint32_t gMseCnt;

	static EthHelper *obj;


};

#endif /* SRC_ETHHELPER_H_ */
