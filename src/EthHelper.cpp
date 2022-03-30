/*
 * EthHelper.cpp
 *
 *  Created on: 3-Mar-2022
 *      Author: jondurrant
 */

#include "EthHelper.h"
#include "MQTTConfig.h"

#include "FreeRTOS.h"
#include "task.h"

#include "sntp.h"
#include "hardware/rtc.h"

#define SOCKET_SNTP 2
#define SOCKET_DNS  1
#define SOCKET_DHCP 0

EthHelper::EthHelper() {
	EthHelper::obj = this;
}

void EthHelper::enableMutex(){

	xSemaphore = xSemaphoreCreateMutex();

	if( xSemaphore == NULL )
	{
		LogError(("Can't create mutex"));
	} else {
		xSemaphoreGive( xSemaphore );
	}
}

void EthHelper::init(uint8_t * pBuf){
	pEthernetBuf = pBuf;

	wizchip_spi_initialize();
	wizchip_cris_initialize();

	wizchip_reset();
	wizchip_initialize();
	wizchip_check();

	wizchip_1ms_timer_initialize(EthHelper::cbRepeatingTimer);

}



EthHelper::~EthHelper() {
	if (xSemaphore != NULL){
		vSemaphoreDelete( xSemaphore );
	}
}



/***
 * Get IP address of unit
 * @param ip - output uint8_t[4]
 */
bool EthHelper::getIPAddress(uint8_t *ip){
	if (xNetInfo.ip[0] == 0){
		return false;
	}
	memcpy(ip, xNetInfo.ip, 4);
	return true;
}

/***
 * Get IP address of unit
 * @param ips - output char * up to 16 chars
 * @return - true if IP addres assigned
 */
bool EthHelper::getIPAddressStr(char *ips){
	if (xNetInfo.ip[0] == 0){
		return false;
	}

	sprintf(ips, "%d.%d.%d.%d",
			xNetInfo.ip[0],
			xNetInfo.ip[1],
			xNetInfo.ip[2],
			xNetInfo.ip[3]
			);
	return true;

}


bool EthHelper::getMACAddressStr(char *macStr){

	for (uint8_t i=0; i < 6; i++ ){
		if (xNetInfo.mac[i]<16){
			sprintf(&macStr[i*2], "0%d", xNetInfo.mac[i]);
		} else {
			sprintf(&macStr[i*2], "%d", xNetInfo.mac[i]);
		}
	}
	macStr[13]=0;
	return true;
}

bool EthHelper::syncRTCwithSNTP(const char **sntpSvrHosts, uint8_t count){
	uint8_t ip[4];
	char s[256];

	for (uint8_t j=0; j < 3; j++){
		for (uint8_t i=0; i < count; i++){
			strcpy(s, sntpSvrHosts[i]);
			if (dnsClient(ip, s)){
				if (syncRTCwithSNTP(ip)){
					printf("SNTP Success host %s\n", sntpSvrHosts[i]);
					return true;
				}
				vTaskDelay(10);
			}
		}
	}
	return false;
}


bool EthHelper::syncRTCwithSNTP(char *sntpSvrHost){
	uint8_t ip[4];
	if (dnsClient(ip, sntpSvrHost)){
		return syncRTCwithSNTP(ip);
	}
	return false;
}

bool EthHelper::syncRTCwithSNTP(uint8_t *sntpSvrIp){
	uint8_t tz = 22;
	uint8_t res, i;
	datetime d;

	if (!rtc_running()){
		rtc_init();
	}

	if( xSemaphore != NULL ){
		if( xSemaphoreTake( xSemaphore, ( TickType_t ) ETHMUTEXTICKS ) == pdTRUE ){

			SNTP_init(SOCKET_SNTP, sntpSvrIp, tz, pEthernetBuf );

			for (uint8_t i=0; i < 5; i++){
				res = SNTP_run(&d);
				if (d.yy >= 2022){
					break;
				}
				vTaskDelay(10);
			}
			xSemaphoreGive( xSemaphore );
		} else {
			LogError(("Did not get Mutex to initialise"));
			return false;
		}
	} else {
		SNTP_init(SOCKET_SNTP, sntpSvrIp, tz, pEthernetBuf );

		for (uint8_t i=0; i < 5; i++){
			res = SNTP_run(&d);
			if (d.yy >= 2022){
				break;
			}
			vTaskDelay(10);
		}
	}

	if ((d.yy < 2022) || (d.yy > 3000)){
		printf("SNTP Failed %d\n", res);
		printf("Failed: %d-%d-%d %d:%d:%d\n", d.yy, d.mo, d.dd, d.hh, d.mm, d.ss);
		return false;
	}

	//rtc_init();
	datetime_t t = {
			 .year  = (int16_t)d.yy,
			 .month = (int8_t) d.mo,
			 .day   = (int8_t) d.dd,
			 .hour  = (int8_t) d.hh,
			 .min   = (int8_t) d.mm,
			 .sec   = (int8_t) d.ss
	 };
	rtc_set_datetime(&t);

	printf("Good DateTime: %d-%d-%d %d:%d:%d\n", d.yy, d.mo, d.dd, d.hh, d.mm, d.ss);


	return true;
}



bool EthHelper::isJoined(){

	return (isPluggedIn() && (xNetInfo.ip[0] != 0));
}

bool EthHelper::isPluggedIn(){
	uint8_t link;

	if( xSemaphore != NULL ){
		if( xSemaphoreTake( xSemaphore, ( TickType_t ) ETHMUTEXTICKS ) == pdTRUE ){
			link = wizphy_getphylink();
			xSemaphoreGive( xSemaphore );
		} else {
			LogError(("Did not get Mutex to initialise"));
			return false;
		}
	} else {
		link = wizphy_getphylink();
	}

	if (link != PHY_LINK_ON){
		memset(xNetInfo.ip, 0, 4);
	}
	return (link == PHY_LINK_ON);
}


/* Timer */
void EthHelper::cbRepeatingTimer(void)
{
	EthHelper::gMseCnt++;

    if (EthHelper::gMseCnt >= 1000 - 1)
    {
    	EthHelper::gMseCnt = 0;

        DHCP_time_handler();
        DNS_time_handler();
    }
}


bool EthHelper::dhcpClient(){
	bool res = false;
	if( xSemaphore != NULL ){
		if( xSemaphoreTake( xSemaphore, ( TickType_t ) ETHMUTEXTICKS ) == pdTRUE ){
			res = dhcpClientLocal();
			xSemaphoreGive( xSemaphore );
		} else {
			LogError(("Did not get Mutex to initialise"));
		}
	} else {
		res = dhcpClientLocal();
	}

	return res;
}

bool EthHelper::dhcpClientLocal(){
    int retval = 0;
    uint8_t link;
    uint16_t len = 0;
    uint32_t dhcpRetry = 0;
    uint8_t dhcpGetIpFlag=0;

    if (xNetInfo.dhcp == NETINFO_DHCP) // DHCP
    {
    	dhcpInit();
    	dhcpGetIpFlag=0;
    }
    else // static
    {
        network_initialize(xNetInfo);

        /* Get network information */
        print_network_information(xNetInfo);

        return true;
    }

    while (1)
    {
        link = wizphy_getphylink();

        if (link == PHY_LINK_OFF)
        {
            printf("PHY_LINK_OFF\r\n");

            DHCP_stop();

            while (1)
            {
                link = wizphy_getphylink();

                if (link == PHY_LINK_ON)
                {
                	dhcpInit();
					dhcpGetIpFlag=0;

                    dhcpRetry = 0;

                    break;
                }

                vTaskDelay(1000);
            }
        }

        retval = DHCP_run();

        if (retval == DHCP_IP_LEASED)
        {
            if (dhcpGetIpFlag == 0)
            {
            	dhcpRetry = 0;

                printf(" JD: DHCP success\n");

                return true;
            }
        }
        else if (retval == DHCP_FAILED)
        {
            dhcpGetIpFlag = 0;
            dhcpRetry++;

            if (dhcpRetry <= DHCP_RETRY_COUNT)
            {
                printf(" DHCP timeout occurred and retry %d\n", dhcpRetry);
            }
        }

        if (dhcpRetry > DHCP_RETRY_COUNT)
        {
            printf(" DHCP failed\n");

            DHCP_stop();

            return false;
        }

        vTaskDelay(10);
    }
}



void EthHelper::dhcpInit(void)
{
    printf(" JD: DHCP client running\n");

    DHCP_init(SOCKET_DHCP, pEthernetBuf);

    reg_dhcp_cbfunc(EthHelper::cbDhcpAssign, EthHelper::cbDhcpAssign, EthHelper::cbDhcpConflict);

}

void EthHelper::cbDhcpAssign(void){
	EthHelper::obj->dhcpAssign();
}


void EthHelper::dhcpAssign(void){
    getIPfromDHCP(xNetInfo.ip);
    getGWfromDHCP(xNetInfo.gw);
    getSNfromDHCP(xNetInfo.sn);
    getDNSfromDHCP(xNetInfo.dns);

    xNetInfo.dhcp = NETINFO_DHCP;

    /* Network initialize */
    network_initialize(xNetInfo); // apply from DHCP

    print_network_information(xNetInfo);
    printf(" JD: DHCP leased time : %ld seconds\n", getDHCPLeasetime());
}

void EthHelper::cbDhcpConflict(void)
{
    printf(" JD: Conflict IP from DHCP\n");
}

EthHelper *EthHelper::obj = NULL;
uint32_t EthHelper::gMseCnt = 0;


bool EthHelper::dnsClient(uint8_t *ip, const char * host){
	 char s[256];
	 strcpy(s, host);
	 bool res = false;

	 if( xSemaphore != NULL ){
		if( xSemaphoreTake( xSemaphore, ( TickType_t ) ETHMUTEXTICKS ) == pdTRUE ){
			DNS_init(SOCKET_DNS, pEthernetBuf);
			 if (DNS_run(xNetInfo.dns, (uint8_t *)s, ip) > 0){
				 res = true;
			 }
			xSemaphoreGive( xSemaphore );
		} else {
			LogError(("Did not get Mutex to initialise"));
		}
	 } else {
		 DNS_init(SOCKET_DNS, pEthernetBuf);
		 if (DNS_run(xNetInfo.dns, (uint8_t *)s, ip) > 0){
			 res = true;
		 }
	 }
	 return res;
}


bool EthHelper::tcpSockConnect(uint8_t sock, uint16_t localPort, uint8_t * hostIP, uint16_t hostPort){
	 bool res = true;
	 int8_t b;

	 if( xSemaphore != NULL ){
		if( xSemaphoreTake( xSemaphore, ( TickType_t ) ETHMUTEXTICKS ) == pdTRUE ){

			b = socket(sock, Sn_MR_TCP, localPort, SF_TCP_NODELAY);
			if (b != sock){
				LogError(("Socket %d", res));
				res = false;
			} else {
				b = connect(sock, hostIP, hostPort);
				if (b != SOCK_OK){
					LogError(("Socket connect error %d", res));
					res = false;
				}
			}

			xSemaphoreGive( xSemaphore );
		} else {
			LogError(("Did not get Mutex to initialise"));
		}
	 } else {
		 b = socket(sock, Sn_MR_TCP, localPort, SF_TCP_NODELAY);
		if (b != sock){
			LogError(("Socket %d", res));
			res = false;
		} else {
			b = connect(sock, hostIP, hostPort);
			if (b != SOCK_OK){
				LogError(("Socket connect error %d", res));
				res = false;
			}
		}
	 }
	 return res;
}

bool EthHelper::tcpSockClose(uint8_t sock){
	bool res = false;
	if( xSemaphore != NULL ){
		if( xSemaphoreTake( xSemaphore, ( TickType_t ) ETHMUTEXTICKS ) == pdTRUE ){
			disconnect(sock);
			res = true;
			xSemaphoreGive( xSemaphore );
		} else {
			LogError(("Did not get Mutex to initialise"));
		}
	 } else {
		 disconnect(sock);
	 }
	 return res;
}


uint32_t EthHelper::tcpSockReadLocal(uint8_t sock, uint8_t *buf, size_t bytesToRecv){
	int32_t dataIn=0;
	uint16_t remaining=0;
	int8_t res;
	uint8_t status;

	getsockopt(sock,SO_STATUS, &status);
	if (status != SOCK_ESTABLISHED){
		printf("SOCKET NOT OPEN\n");
		return -1;
	}

	res = getsockopt(sock,SO_REMAINSIZE, &remaining);
	if (res != SOCK_OK){
		return -1;
	}

	if  (remaining > 0){
		dataIn = recv(sock, buf, bytesToRecv);

		if (dataIn == SOCK_BUSY){
			dataIn = 0;
		}
	}
	return dataIn;
}

uint32_t EthHelper::tcpSockRead(uint8_t sock, uint8_t *buf, size_t bytesToRecv){
	int32_t dataIn=0;

	 if( xSemaphore != NULL ){
		if( xSemaphoreTake( xSemaphore, ( TickType_t ) ETHMUTEXTICKS ) == pdTRUE ){
			dataIn = tcpSockReadLocal(sock, buf, bytesToRecv);

			xSemaphoreGive( xSemaphore );
		} else {
			LogError(("Did not get Mutex to initialise"));
		}
	 } else {
		 dataIn = tcpSockReadLocal(sock, buf, bytesToRecv);
	 }
	 return dataIn;
}

uint32_t EthHelper::tcpSockWrite(uint8_t sock, uint8_t *buf, size_t bytesToSend){
	int32_t dataOut=0;

	 if( xSemaphore != NULL ){
		if( xSemaphoreTake( xSemaphore, ( TickType_t ) ETHMUTEXTICKS ) == pdTRUE ){
			dataOut = send(sock, buf, bytesToSend);
			xSemaphoreGive( xSemaphore );
		} else {
			LogError(("Did not get Mutex to initialise"));
		}
	 } else {
		 dataOut = send(sock, buf, bytesToSend);
	 }
	 return dataOut;
}

