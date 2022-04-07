/*
 * MQTTAgent.h
 *
 *  Created on: 15 Nov 2021
 *      Author: jondurrant
 */

#ifndef MQTTAGENT_H_
#define MQTTAGENT_H_

#include "MQTTConfig.h"
#include "FreeRTOS.h"
#include "core_mqtt.h"
#include "core_mqtt_agent.h"
#include "MQTTInterface.h"
#include "MQTTRouter.h"
#include "MQTTTopicHelper.h"
#include "TCPTransport.h"
#include "EthHelper.h"
#include "MQTTAgentObserver.h"

extern "C" {
#include "freertos_agent_message.h"
#include "freertos_command_pool.h"
}

#ifndef MQTT_AGENT_NETWORK_BUFFER_SIZE
#define MQTT_AGENT_NETWORK_BUFFER_SIZE 512
#endif

#ifndef MAXSUBS
#define MAXSUBS 12
#endif

#ifndef MQTTKEEPALIVETIME
#define MQTTKEEPALIVETIME 10
#endif

#ifndef MQTT_RECON_DELAY
#define MQTT_RECON_DELAY 10
#endif


enum MQTTState {  Offline, TCPReq, TCPConned, MQTTReq, MQTTConned, MQTTRecon, Online};

class MQTTAgent: public MQTTInterface{
public:
	MQTTAgent(uint8_t sockNum, EthHelper *eth);
	virtual ~MQTTAgent();

	/***
	 * Set credentials
	 * @param user - string pointer. Not copied so pointer must remain valid
	 * @param passwd - string pointer. Not copied so pointer must remain valid
	 * @param id - string pointer. Not copied so pointer must remain valid. I
	 * f not provide ID will be user
	 * @return lwespOK if succeeds
	 */
	void credentials(const char * user, const char * passwd, const char * id = NULL );

	/***
	 * Connect to mqtt server
	 * @param target - hostname or ip address, Not copied so pointer must remain valid
	 * @param port - port number
	 * @param ssl - unused
	 * @return
	 */
	 bool connect(const char * target, uint16_t  port, bool recon=false, bool ssl=false);

	 /***
	 *  create the vtask, will get picked up by scheduler
	 *
	 *  */
	void start(UBaseType_t priority = tskIDLE_PRIORITY);

	/***
	 * Stop task
	 * @return
	 */
	void stop();

	/***
	 * Returns the id of the client
	 * @return
	 */
	virtual const char * getId();

	/***
	 * Publish message to topic
	 * @param topic - zero terminated string. Copied by function
	 * @param payload - payload as pointer to memory block
	 * @param payloadLen - length of memory block
	 */
	virtual bool pubToTopic(const char * topic,  const void * payload,
			size_t payloadLen, const uint8_t QoS=0);

	/***
	 * Subscribe to a topic, mesg will be sent to router object
	 * @param topic
	 * @param QoS
	 * @return
	 */
	virtual bool subToTopic(const char * topic, const uint8_t QoS=0);


	/***
	 * Close connection
	 */
	virtual void close();

	/***
	 * Route a message to the router object
	 * @param topic - non zero terminated string
	 * @param topicLen - topic length
	 * @param payload - raw memory
	 * @param payloadLen - payload length
	 */
	virtual void route(const char * topic, size_t topicLen, const void * payload, size_t payloadLen);


	/***
	 * Get the router object handling all received messages
	 * @return
	 */
	MQTTRouter* getRouter() ;


	/***
	 * Set the rotuer object
	 * @param pRouter
	 */
	void setRouter( MQTTRouter *pRouter = NULL);

	/***
	 * Set a single observer to get call back on state changes
	 * @param obs
	 */
	virtual void setObserver(MQTTAgentObserver *obs);


	/***
	 * Get the FreeRTOS task being used
	 * @return
	 */
	virtual TaskHandle_t getTask();

private:
	/***
	 * Initialisation code
	 * @return
	 */
	MQTTStatus_t init();


	/***
	 * Callback on when new data is received
	 * @param pMqttAgentContext
	 * @param packetId
	 * @param pxPublishInfo
	 */
	static void incomingPublishCallback( MQTTAgentContext_t * pMqttAgentContext,
            uint16_t packetId,
            MQTTPublishInfo_t * pxPublishInfo );


	/***
	 * Task object running to manage MQTT interface
	 * @param pvParameters
	 */
	static void vTask( void * pvParameters );

	/***
	 * Call back function on connect
	 * @param pCmdCallbackContext
	 * @param pReturnInfo
	 */
	static void connectCmdCallback( MQTTAgentCommandContext_t * pCmdCallbackContext,
            MQTTAgentReturnInfo_t * pReturnInfo );

	/***
	 * Call back function nwhen subscribe completes
	 * @param pCmdCallbackContext
	 * @param pReturnInfo
	 */
	static void subscribeCmdCompleteCb( MQTTAgentCommandContext_t * pCmdCallbackContext,
	                             MQTTAgentReturnInfo_t * pReturnInfo );

	/***
	 * Call back function when Publish completes
	 * @param pCmdCallbackContext
	 * @param pReturnInfo
	 */
	static void publishCmdCompleteCb( MQTTAgentCommandContext_t * pCmdCallbackContext,
            MQTTAgentReturnInfo_t * pReturnInfo );


	/***
	 * Run loop for the task
	 */
	void run();

	/***
	 * Connect to MQTT hub
	 * @return
	 */
	MQTTStatus_t MQTTconn();

	/***
	 * Subscribe to routers list
	 * @return true if succeeds
	 */
	bool MQTTsub();

	/***
	 * Perform TCP Connection
	 * @return true if succeeds
	 */
	bool TCPconn();

	/***
	 * Set the connection state variable
	 * @param s
	 */
	void setConnState(MQTTState s);


	EthHelper *pEth = NULL;
	NetworkContext_t xNetworkContext;
	TCPTransport xTcpTrans;
	const char * pUser;
	const char * pPasswd;
	const char * pId;
	const char * pTarget = NULL;
	char xMacStr[14];
	uint16_t xPort = 1883 ;
	bool xSsl = false;
	bool xRecon = false;
	MQTTRouter * pRouter = NULL;

	uint8_t xNetworkBuffer[ MQTT_AGENT_NETWORK_BUFFER_SIZE ];
	uint8_t xStaticQueueStorageArea[ MQTT_AGENT_COMMAND_QUEUE_LENGTH * sizeof( MQTTAgentCommand_t * ) ];
	StaticQueue_t xStaticQueueStructure;
	MQTTAgentMessageContext_t xCommandQueue;
	MQTTAgentContext_t xGlobalMqttAgentContext;
	TaskHandle_t xHandle = NULL;

	MQTTState xConnState = Offline;

	static const char * WILLTOPICFORMAT;
	char *pWillTopic = NULL;
	static const char * WILLPAYLOAD;
	MQTTPublishInfo_t xWillInfo;
	static const char * ONLINEPAYLOAD;
	char *pOnlineTopic = NULL;
	char *pKeepAliveTopic = NULL;

	//Temp used bu publish
	MQTTAgentCommandInfo_t xCommandInfo;
	MQTTPublishInfo_t xPublishInfo;


	MQTTAgentCommandInfo_t xSubCommandInfo;
	MQTTSubscribeInfo_t xSubscribeInfo[MAXSUBS] ;
	MQTTAgentSubscribeArgs_t xSubscribeArgs [MAXSUBS];
	uint8_t xCurrentSub = 0;


	//Single Observer
	MQTTAgentObserver *pObserver = NULL;

};

#endif /* MQTTAGENT_H_ */
