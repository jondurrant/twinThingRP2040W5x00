/*
 * MQTTAgent.cpp
 *
 * MQTT Agent to manage MQTT connection and messaging
 *
 *  Created on: 15 Nov 2021
 *      Author: jondurrant
 */

#include "MQTTAgent.h"
#include <stdlib.h>

/* MQTT Agent ports. */
#include "freertos_agent_message.h"
#include "freertos_command_pool.h"

const char * MQTTAgent::WILLTOPICFORMAT = "TNG/%s/LC";
const char * MQTTAgent::WILLPAYLOAD = "{'online':0}";
const char * MQTTAgent::ONLINEPAYLOAD = "{'online':1}";



/***
 * Constructor
 * @param sockNum - Socket Number to use
 * @param eth - Ethernet helper for communicating to hardware
 */
MQTTAgent::MQTTAgent(uint8_t sockNum, EthHelper *eth) {
	pEth = eth;
	xTcpTrans.init(sockNum,eth);

}

/***
 * Destructor
 */
MQTTAgent::~MQTTAgent() {
	if (pWillTopic != NULL){
		vPortFree(pWillTopic);
		pWillTopic = NULL;
	}

	if (pOnlineTopic != NULL){
		vPortFree(pOnlineTopic);
		pOnlineTopic = NULL;
	}

	if (pKeepAliveTopic != NULL){
		vPortFree(pKeepAliveTopic);
		pKeepAliveTopic = NULL;
	}
}

/***
 * Initialisation code
 * @return
 */
MQTTStatus_t MQTTAgent::init(){
	TransportInterface_t xTransport;
	MQTTStatus_t xReturn;
	MQTTFixedBuffer_t xFixedBuffer = { .pBuffer = xNetworkBuffer, .size = MQTT_AGENT_NETWORK_BUFFER_SIZE };


	MQTTAgentMessageInterface_t messageInterface =
	{
		.pMsgCtx        = NULL,
		.send           = Agent_MessageSend,
		.recv           = Agent_MessageReceive,
		.getCommand     = Agent_GetCommand,
		.releaseCommand = Agent_ReleaseCommand
	};

	LogDebug( ( "Creating command queue." ) );
	xCommandQueue.queue = xQueueCreateStatic( MQTT_AGENT_COMMAND_QUEUE_LENGTH,
											  sizeof( MQTTAgentCommand_t * ),
											  xStaticQueueStorageArea,
											  &xStaticQueueStructure );
	if (xCommandQueue.queue == NULL) {
		LogDebug(("MQTTAgent::mqttInit ERROR Queue not initialised"));
		return MQTTIllegalState;
	}
	messageInterface.pMsgCtx = &xCommandQueue;

	/* Initialize the task pool. */
	Agent_InitializePool();

	// Fill in Transforp interface
	xNetworkContext.mqttTask = NULL;
	xNetworkContext.tcpTransport = &xTcpTrans;
	xTransport.pNetworkContext = &xNetworkContext;
	xTransport.send = TCPTransport::staticSend;
	xTransport.recv = TCPTransport::staticRead;


	/* Initialize MQTT library. */
	xReturn = MQTTAgent_Init( &xGlobalMqttAgentContext,
							  &messageInterface,
							  &xFixedBuffer,
							  &xTransport,
							  TCPTransport::getCurrentTime,
							  MQTTAgent::incomingPublishCallback,
							  /* Context to pass into the callback. Passing the pointer to subscription array. */
							  this );

	return xReturn;

}

/***
 * Callback on when new data is received
 * @param pMqttAgentContext
 * @param packetId
 * @param pxPublishInfo
 */
void MQTTAgent::incomingPublishCallback( MQTTAgentContext_t * pMqttAgentContext,
                                        uint16_t packetId,
                                        MQTTPublishInfo_t * pxPublishInfo ){

	LogDebug(("MSG(%d,%d) %.*s:%.*s\n",
			pxPublishInfo->topicNameLength,
			pxPublishInfo->payloadLength,
			pxPublishInfo->topicNameLength,
			pxPublishInfo->pTopicName,
			pxPublishInfo->payloadLength,
			(char *)pxPublishInfo->pPayload
			));

	MQTTAgent *a = (MQTTAgent *)pMqttAgentContext->pIncomingCallbackContext;
	a->route(pxPublishInfo->pTopicName,
			pxPublishInfo->topicNameLength,
			pxPublishInfo->pPayload,
			pxPublishInfo->payloadLength);

}



/***
 * Set credentials
 * @param user - string pointer. Not copied so pointer must remain valid
 * @param passwd - string pointer. Not copied so pointer must remain valid
 * @param id - string pointer. Not copied so pointer must remain valid.
 * If not provide ID will be user
 *
 */
void MQTTAgent::credentials(const char * user, const char * passwd, const char * id){
	if (strcmp(user, "MAC") == 0){
		pEth->getMACAddressStr(xMacStr);
		this->pUser = xMacStr;
	} else {
		this->pUser = user;
	}
	if (strcmp(passwd, "MAC") == 0){
		pEth->getMACAddressStr(xMacStr);
		this->pPasswd = xMacStr;
	} else {
		this->pPasswd = passwd;
	}

	if (id != NULL){
		this->pId = id;
	} else {
		this->pId = this->pUser;
	}
	LogInfo(("MQTT Credentials Id=%s, usr=%s\n", this->pId, this->pUser));

	if (pWillTopic == NULL){
		pWillTopic = (char *)pvPortMalloc( MQTTTopicHelper::lenLifeCycleTopic(this->pId, MQTT_TOPIC_LIFECYCLE_OFFLINE));
		if (pWillTopic != NULL){
			MQTTTopicHelper::genLifeCycleTopic(pWillTopic, this->pId, MQTT_TOPIC_LIFECYCLE_OFFLINE);
		} else {
			LogError( ("Unable to allocate LC topic") );
		}
	}

	if (pOnlineTopic == NULL){
		pOnlineTopic = (char *)pvPortMalloc( MQTTTopicHelper::lenLifeCycleTopic(this->pId, MQTT_TOPIC_LIFECYCLE_ONLINE));
		if (pOnlineTopic != NULL){
			MQTTTopicHelper::genLifeCycleTopic(pOnlineTopic, this->pId, MQTT_TOPIC_LIFECYCLE_ONLINE);
		} else {
			LogError( ("Unable to allocate LC topic") );
		}
	}

	if (pKeepAliveTopic == NULL){
		pKeepAliveTopic = (char *)pvPortMalloc( MQTTTopicHelper::lenLifeCycleTopic(this->pId, MQTT_TOPIC_LIFECYCLE_KEEP_ALIVE));
		if (pKeepAliveTopic != NULL){
			MQTTTopicHelper::genLifeCycleTopic(pKeepAliveTopic, this->pId, MQTT_TOPIC_LIFECYCLE_KEEP_ALIVE);
		} else {
			LogError( ("Unable to allocate LC topic") );
		}
	}
}

/***
 * Connect to mqtt server
 * @param target - hostname or ip address, Not copied so pointer must remain valid
 * @param port - port number
 * @param ssl - unused
 * @return
 */
bool MQTTAgent::connect(const char * target, uint16_t  port, bool recon, bool ssl){
	this->pTarget = target;
	this->xPort = port;
	this->xSsl = ssl;
	this->xRecon = recon;
	setConnState(TCPReq);
	LogDebug(("TCP Requested\n"));
	return true;
}



/***
*  create the vtask, will get picked up by scheduler
*
*  */
void MQTTAgent::start(UBaseType_t priority){
	if (init() == MQTTSuccess){
		xTaskCreate(
			MQTTAgent::vTask,
			"MQTTAgent",
			2512,
			( void * ) this,
			priority,
			&xHandle
		);
	}
}

/***
 * Internal function used by FreeRTOS to run the task
 * @param pvParameters
 */
 void MQTTAgent::vTask( void * pvParameters ){
	 MQTTAgent *task = (MQTTAgent *) pvParameters;
	task->run();
 }

/***
* Run loop for the task
*/
 void MQTTAgent::run(){
	 LogDebug(("MQTTAgent run\n"));

	 MQTTStatus_t status;


	 for(;;){

		 switch(xConnState){
		 case Offline: {
			 break;
		 }
		 case TCPReq: {
			 if (pEth->isJoined()){
				 TCPconn();
			 } else {
				 LogInfo(("Network offline, awaiting reconnect"));
				 vTaskDelay(MQTT_RECON_DELAY);
			 }
			 break;
		 }
		 case TCPConned: {
			 LogDebug(("Attempting MQTT conn\n"));
			 status = MQTTconn();
			 if (status == MQTTSuccess){
				 setConnState(MQTTReq);
				 LogDebug(("MQTTconn ok\n"));
			 } else {
				 setConnState(Offline);
				 LogDebug(("MQTTConn failed\n"));
			 }
			 break;
		 }
		 case MQTTReq: {
			 MQTTsub();
			 setConnState(MQTTConned);
			 break;
		 }
		 case MQTTConned: {
			 setConnState(Online);
			 pubToTopic(pOnlineTopic, ONLINEPAYLOAD, strlen(ONLINEPAYLOAD), 1);
			 break;
		 }
		 case Online:{
			 LogDebug(("Starting CMD loop\n"));

			 status = MQTTAgent_CommandLoop( &xGlobalMqttAgentContext );

			 // The function returns on either receiving a terminate command,
			 // undergoing network disconnection OR encountering an error.
			 if( ( status == MQTTSuccess ) && ( xGlobalMqttAgentContext.mqttContext.connectStatus == MQTTNotConnected ) )
			 {
				// A terminate command was processed and MQTT connection was closed.
				// Need to close socket connection.
				//Platform_DisconnectNetwork( mqttAgentContext.mqttContext.transportInterface.pNetworkContext );
				 LogDebug(("MQTT Closed\n"));
				 xTcpTrans.transClose();
				 setConnState(Offline);
			 }
			 else if( status == MQTTSuccess )
			 {
				// Terminate command was processed but MQTT connection was not
				// closed. Thus, need to close both MQTT and socket connections.
				 LogDebug(("Terminated processed\n"));
				status = MQTT_Disconnect( &( xGlobalMqttAgentContext.mqttContext ) );
				//assert( status == MQTTSuccess );
				//Platform_DisconnectNetwork( mqttAgentContext.mqttContext.transportInterface.pNetworkContext );
				xTcpTrans.transClose();
				setConnState(Offline);
			 }
			 else
			 {
				 // Handle error.
				 LogDebug(("Command Loop error %d\n", status));
				 setConnState(Offline);

			 }
			 break;
		 }
		 case MQTTRecon:{
			 if (pEth->isJoined()){
				 xTcpTrans.transClose();
			 }
			 vTaskDelay(MQTT_RECON_DELAY);
			 setConnState(TCPReq);
			 break;
		 }
		 default:{

		 }

		 };

		 taskYIELD();
		 //vTaskDelay(1);
	 }



	 LogError(("RUN STOPPED\n"));


 }



 /***
* Returns the id of the client
* @return
*/
const char * MQTTAgent::getId(){
	return pId;
}

/***
* Publish message to topic
* @param topic - zero terminated string. Copied by function
* @param payload - payload as pointer to memory block
* @param payloadLen - length of memory block
*/
bool MQTTAgent::pubToTopic(const char * topic, const void * payload,
	size_t payloadLen, const uint8_t QoS){

	MQTTStatus_t status;

	xCommandInfo.cmdCompleteCallback = MQTTAgent::publishCmdCompleteCb;
	xCommandInfo.blockTimeMs = 500;

	// Fill the information for publish operation.
	xPublishInfo.qos = MQTTQoS1;
	xPublishInfo.pTopicName = topic;
	xPublishInfo.topicNameLength = strlen( xPublishInfo.pTopicName );
	xPublishInfo.pPayload = payload;
	xPublishInfo.payloadLength = payloadLen;

	LogDebug(("Publishing(%d, %d) %.*s:%.*s\n",
			xPublishInfo.topicNameLength,
			xPublishInfo.payloadLength,
			xPublishInfo.topicNameLength,
			xPublishInfo.pTopicName,
			xPublishInfo.payloadLength,
			xPublishInfo.pPayload
			));

	status = MQTTAgent_Publish( &xGlobalMqttAgentContext, &xPublishInfo, &xCommandInfo );
	if (status != MQTTSuccess ){
		LogError(("publish error %d", status));
		return false;
	}

	if (pObserver != NULL){
		pObserver->MQTTSend();
	}

	return true;
}

/***
* Close connection
*/
void MQTTAgent::close(){
	xTcpTrans.transClose();
	xRecon=false;
	setConnState(Offline);
}

/***
* Route a message to the router object
* @param topic - non zero terminated string
* @param topicLen - topic length
* @param payload - raw memory
* @param payloadLen - payload length
*/
void MQTTAgent::route(const char * topic, size_t topicLen, const void * payload, size_t payloadLen){
	if (pRouter != NULL){
		pRouter->route(topic, topicLen, payload, payloadLen, this);
	}
	if (pObserver != NULL){
		pObserver->MQTTRecv();
	}
}


/***
* Call back function on connect
* @param pCmdCallbackContext
* @param pReturnInfo
*/
void MQTTAgent::connectCmdCallback( MQTTAgentCommandContext_t * pCmdCallbackContext,
                         MQTTAgentReturnInfo_t * pReturnInfo ){
	LogDebug(("ConnectCmdCallback called\n"));
}


/***
* Connect to MQTT server
* @return
*/
MQTTStatus_t  MQTTAgent::MQTTconn(){
	MQTTStatus_t xResult;
	MQTTConnectInfo_t xConnectInfo;
	bool xSessionPresent = false;

	/* Many fields not used in this demo so start with everything at 0. */
	( void ) memset( ( void * ) &xConnectInfo, 0x00, sizeof( xConnectInfo ) );

	/* Start with a clean session i.e. direct the MQTT broker to discard any
	 * previous session data. Also, establishing a connection with clean
	 * session will ensure that the broker does not store any data when this
	 * client gets disconnected. */
	xConnectInfo.cleanSession = true;

	/* The client identifier is used to uniquely identify this MQTT client to
	 * the MQTT broker. In a production device the identifier can be something
	 * unique, such as a device serial number. */
	xConnectInfo.pClientIdentifier = pId;
	xConnectInfo.clientIdentifierLength = ( uint16_t ) strlen(pId);
	xConnectInfo.pUserName = pUser;
	xConnectInfo.userNameLength = ( uint16_t ) strlen(pUser);
	xConnectInfo.pPassword = pPasswd;
	xConnectInfo.passwordLength= ( uint16_t ) strlen(pPasswd);

	xWillInfo.qos = MQTTQoS1;
	sprintf(pWillTopic, MQTTAgent::WILLTOPICFORMAT, pId);
	xWillInfo.pTopicName = pWillTopic;
	xWillInfo.topicNameLength = strlen( xWillInfo.pTopicName );
	xWillInfo.pPayload = MQTTAgent::WILLPAYLOAD;
	xWillInfo.payloadLength = strlen( MQTTAgent::WILLPAYLOAD );


	/* Set MQTT keep-alive period. It is the responsibility of the application
	 * to ensure that the interval between Control Packets being sent does not
	 * exceed the Keep Alive value.  In the absence of sending any other
	 * Control Packets, the Client MUST send a PINGREQ Packet. */
	xConnectInfo.keepAliveSeconds = MQTTKEEPALIVETIME;

	/* Send MQTT CONNECT packet to broker. LWT is not used in this demo, so it
	 * is passed as NULL. */
	LogDebug(("MQTT Connect \n"));
	xResult = MQTT_Connect( &(xGlobalMqttAgentContext.mqttContext),
							&xConnectInfo,
							&xWillInfo,
							30000U,
							&xSessionPresent );

	if (xResult != MQTTSuccess){
		LogError(("MQTTConnect error %d", xResult));
	}
	return xResult ;
}



/***
* Get the router object handling all received messages
* @return
*/
MQTTRouter* MQTTAgent::getRouter()  {
	return pRouter;
}

/***
* Set the rotuer object
* @param pRouter
*/
void MQTTAgent::setRouter( MQTTRouter *pRouter) {
	this->pRouter = pRouter;
}

/***
 * Subscribe to routers list
 * @return true if succeeds
 */
bool MQTTAgent::MQTTsub(){

	if (pRouter != NULL){
		xCurrentSub = 0;
		pRouter->subscribe(this);
		return true;
	}
	return false;
}

/***
 * Call back function nwhen subscribe completes
 * @param pCmdCallbackContext
 * @param pReturnInfo
 */
void MQTTAgent::subscribeCmdCompleteCb( MQTTAgentCommandContext_t * pCmdCallbackContext,
	                             MQTTAgentReturnInfo_t * pReturnInfo ){
	LogDebug(("Subscription complete\n"));
}

/***
* Call back function when Publish completes
* @param pCmdCallbackContext
* @param pReturnInfo
*/
void MQTTAgent::publishCmdCompleteCb( MQTTAgentCommandContext_t * pCmdCallbackContext,
            MQTTAgentReturnInfo_t * pReturnInfo ){
	LogDebug(("Publish complete\n"));

}

/***
 * Subscribe to a topic, mesg will be sent to router object
 * @param topic
 * @param QoS
 * @return
 */
bool MQTTAgent::subToTopic(const char * topic,  const uint8_t QoS){
	MQTTStatus_t status = MQTTNoDataAvailable ;

	// Fill the command information.
	xSubCommandInfo.cmdCompleteCallback = MQTTAgent::subscribeCmdCompleteCb;
	xSubCommandInfo.blockTimeMs = 500;
	//xSubCommandInfo.pCmdCompleteCallbackContext = this;

	uint8_t subNum = xCurrentSub++;
	if (xCurrentSub > MAXSUBS){
		LogError(("OVERFLOW"));
	}
	MQTTSubscribeInfo_t      *pSubInfo = &xSubscribeInfo[subNum];
	MQTTAgentSubscribeArgs_t *pSubArgs = &xSubscribeArgs[subNum];


	// Fill the information for topic filters to subscribe to.
	switch(QoS){
	case 0:{
		pSubInfo->qos = MQTTQoS0;
		break;
	}
	case 1:{
		pSubInfo->qos = MQTTQoS1;
		break;
	}
	case 2:{
		pSubInfo->qos = MQTTQoS2;
		break;
	}
	default:{
		pSubInfo->qos = MQTTQoS1;
		break;
	}
	}
	pSubInfo->pTopicFilter = topic;
	pSubInfo->topicFilterLength = strlen(topic);
	pSubArgs->pSubscribeInfo = pSubInfo;
	pSubArgs->numSubscriptions = 1U;

	status = MQTTAgent_Subscribe( &xGlobalMqttAgentContext, pSubArgs, &xSubCommandInfo );
	if (status != MQTTSuccess){
		LogError(("Sub error %d", status));
		return false;
	}

	if (pObserver != NULL){
		pObserver->MQTTSend();
	}

	return true;

}

/***
 * Connect to MQTT hub
 * @return
 */
bool MQTTAgent::TCPconn(){
	LogDebug(("TCP Connect...."));
	if (xTcpTrans.transConnect(pTarget, xPort)){
		setConnState(TCPConned);
		LogDebug(("TCP Connected"));
		return true;
	} else {
		LogDebug(("TCP Connection failed"));
		setConnState(Offline);
	}
	return false;
}

/***
 * Set a single observer to get call back on state changes
 * @param obs
 */
void MQTTAgent::setObserver(MQTTAgentObserver *obs){
	pObserver = obs;
}



/***
 * Stop task
 * @return
 */
void MQTTAgent::stop(){
	if (xConnState != Offline){
		xTcpTrans.transClose();
		setConnState(Offline);
	}
	if (xHandle != NULL){
		vTaskDelete(  xHandle );
		xHandle = NULL;
	}
}

/***
 * Set the connection state variable
 * @param s
 */
void MQTTAgent::setConnState(MQTTState s){
	xConnState = s;

	if (pObserver != NULL){
		switch(xConnState){
		case Offline:{
			pObserver->MQTTOffline();
			if (xRecon){
				setConnState(MQTTRecon);
			}
			break;
		}
		case Online:{
			pObserver->MQTTOnline();
			break;
		}
		default:{
			;
		}
		}
	}
}

/***
* Get the FreeRTOS task being used
* @return
*/
TaskHandle_t MQTTAgent::getTask(){
	return xHandle;
}
