/*
 * EthHelper.h
 *
 * Ethernet Helper to manage the WizNet Ethernet
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
#define ETHMUTEXTICKS 10
#endif

/* Buffer */
#define ETHERNET_BUF_MAX_SIZE (1024 * 2)

class EthHelper {
public:
	/***
	 * Constructor, requires init to be called afterwoods
	 */
	EthHelper();

	/***
	 * Destructor
	 */
	virtual ~EthHelper();

	/***
	 * Initialise and provide a buffer of length ETHERNET_BUF_MAX_SIZE
	 * @param pBuf
	 */
	void init(uint8_t * pBuf);

	/***
	 * Enable Mutex check for any ethernet operation.
	 */
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


	/***
	 * Get the mac address in string form
	 * @param macStr
	 * @return
	 */
	bool getMACAddressStr(char *macStr);

	/***
	 * Get the mac address as uint8_t array
	 * @param mac - array of uint8_t[6] to write to
	 * @return
	 */
	bool getMACAddress(uint8_t *mac);

	/***
	 * Set the mac address as uint8_t array
	 * @param mac - array of uint8_t[6]
	 * If Null then a default Mac address is used based in Pico Serial
	 * @return
	 */
	bool setMACAddress(uint8_t *mac);

	/***
	 * Initialise the RTC ready for SNTP
	 */
	void rtcInit();

	/***
	 * Set RTC from the SNTP server
	 * @param sntpSvrIp - ip address as uint8_t[4]
	 * @return true if sync with good date and time
	 */
	bool syncRTCwithSNTP(uint8_t *sntpSvrIp);

	/***
	 * Set RTC from SNTP Server
	 * @param sntpSvrHost - Host name as string
	 * @return true if sync was successful
	 */
	bool syncRTCwithSNTP(char *sntpSvrHost);

	/***
	 * Set RTC from SNTP Servers
	 * @param sntpSvrHosts - array of strings
	 * @param count - number of strings in array
	 * @return true if sync was successful
	 */
	bool syncRTCwithSNTP(const char **sntpSvrHosts, uint8_t count);

	/***
	 * Set RTC fro SNTP Server, using previously provided list of servers
	 * @return true if sync was successful
	 */
	bool syncRTCwithSNTP();

	/***
	 * Set list of servers for SNTP operations
	 * @param sntpSvrHosts - array of strings
	 * @param count - number of items in array
	 */
	void setSNTPServers(const char **sntpSvrHosts, uint8_t count);

	/***
	 * Is Ethernet plug in and do we have an IP address
	 * @return
	 */
	bool isJoined();

	/***
	 * Is ethernet plugged in
	 * @return
	 */
	bool isPluggedIn();

	/***
	 * Run DHCP to update the ip address
	 * @return
	 */
	bool dhcpClient();

	/***
	 * Perform a DNS lookup
	 * @param ip - uint8_t[4] ip address of host
	 * @param host - string host name to lookup
	 * @return true if successful
	 */
	bool dnsClient(uint8_t *ip,  const char * host);

	/***
	 * Connect a TCP Socket
	 * @param sock - Socket id
	 * @param localPort - local port number
	 * @param hostIP - host ip address
	 * @param hostPort - host port number
	 * @return true if successful
	 */
	bool tcpSockConnect(uint8_t sock, uint16_t localPort, uint8_t * hostIP, uint16_t hostPort);

	/***
	 * Close a TCP socket
	 * @param sock = socket id
	 * @return true if successful
	 */
	bool tcpSockClose(uint8_t sock);

	/***
	 * Read data from TCP Socket. Returns 0 if no data available.
	 * @param sock - socket id
	 * @param buf - bugger to read into
	 * @param bytesToRecv - length to read (must fit in buf
	 * @return bytes read. 0 if none. Negative if error
	 */
	uint32_t tcpSockRead(uint8_t sock, uint8_t *buf, size_t bytesToRecv);

	/***
	 * Write data to TCP Socket
	 * @param sock - socket id
	 * @param buf - buffer to write from
	 * @param bytesToSend - number of bytes to write
	 * @return number of bytes writtem
	 */
	uint32_t tcpSockWrite(uint8_t sock, uint8_t *buf, size_t bytesToSend);


protected:
	/***
	 * Handle dhcp assignment
	 */
	void dhcpAssign(void);
	/***
	 * Initailise DHCP
	 */
	void dhcpInit(void);



private:

	/***
	 * Read from socket without mutex
	 * @param sock
	 * @param buf
	 * @param bytesToRecv
	 * @return
	 */
	uint32_t tcpSockReadLocal(uint8_t sock, uint8_t *buf, size_t bytesToRecv);

	/***
	 * DHCP renewal without mutex
	 * @return
	 */
	bool dhcpClientLocal();

	/***
	 * Callback if mac address conflict
	 */
	static void cbDhcpConflict(void);

	/***
	 * Call back on dhcp assign
	 */
	static void cbDhcpAssign(void);

	/***
	 * call back on repeat timer
	 */
	static void cbRepeatingTimer(void);


	/***
	 * Setup network info, including a hardcoded MAC address
	 */
	wiz_NetInfo xNetInfo ={
	        .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56}, // MAC address
	        .ip = {0, 0, 0, 0},                     // IP address
	        .sn = {255, 255, 255, 0},                    // Subnet Mask
	        .gw = {192, 168, 11, 1},                     // Gateway
	        .dns = {8, 8, 8, 8},                         // DNS server
	        .dhcp = NETINFO_DHCP                         // DHCP enable/disable
	};

	/***
	 * Ethernet buffer
	 */
	uint8_t * pEthernetBuf;

	/***
	 * Mutex
	 */
	SemaphoreHandle_t xSemaphore;

	/***
	 * SNTP Servers
	 */
	const char **pSntpSvrHosts = NULL;

	/***
	 * SNTP Server count
	 */
	uint8_t xSntpCount = 0;

	/***
	 * Counter
	 */
	static uint32_t gMseCnt;

	/***
	 * Self static pointer for callback to find the object
	 */
	static EthHelper *obj;


};

#endif /* SRC_ETHHELPER_H_ */
