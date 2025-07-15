#define DEBUG_MODE

#ifdef JSON5
#include "json5.h"
#endif

#ifdef no_almagamation
   #include <open62541/types_generated.h>
//   #include <plugins/ua_network_pubsub_mqtt.h>    // contain UA_PubSubTransportLayerMQTT() header; implementation in plugins/ua_network_pubsub_mqtt.c
//   #include <open62541/plugin/pubsub_udp.h>
//   #include <open62541/plugin/pubsub_ethernet.h>
//   #include <open62541/plugin/securitypolicy_default.h>
//   #include <open62541/plugin/pubsub.h>
   #include <open62541/plugin/log_stdout.h>
   #include <open62541/server.h>
   #include <open62541/server_config_default.h>
   #include <open62541/server_pubsub.h>
//   #include <pubsub/ua_pubsub.h> // in ~/open62541/src/pubsub/ua_pubsub.h :  contain the following struct
//#include "open62541.h"
//#include "ua_pubsub_networkmessage.h"
//#include "ua_pubsub.h"
#else
   #include "open62541.h"
   #define UA_ENABLE_PUBSUB
   #define UA_ENABLE_PUBSUB_SKS
   #define UA_ENABLE_PUBSUB_FILE_CONFIG
   #define UA_ENABLE_PUBSUB_ENCRYPTION
   #define UA_ENABLE_PUBSUB_INFORMATIONMODEL
   #define UA_ENABLE_PUBSUB_MONITORING
   //#define UA_ENABLE_PUBSUB_MQTT
   #define UA_ENABLE_MQTT
   #define UA_ENABLE_WEBSOCKET_SERVER
#endif
#include <pthread.h>
#include "SV_PubSub.h"			// contains all the #define for PUBSUB modules
#include "SV_Misc.h"

// not required for brokerless, but first declared in SV_PubSubInitialiseWithBroker.c
extern UA_Boolean MQTT_Enable;
extern UA_Boolean MQTT_TLS_Enable;
extern UA_Boolean AMQP_Enable;
extern int MQTT_Port;   // default set to 1883

// only 1 PubSubConnection for both reader and writer
extern UA_NodeId PubSubconnectionIdentifier;
static UA_NodeId publishedDataSetIdentifier;

// Subscriber section variables & functions
extern UA_DataSetReaderConfig dataSetReaderConfig;
extern UA_NodeId readerGroupIdentifier;
extern UA_NodeId readerIdentifier;
extern UA_NodeId reader2Id;

//int createWebSocketsServer(UA_NodeId *publishedDataSetidentifier, UA_NetworkAddressUrlDataType *networkAddressUrl, int, char**);
//void createWebSocketsOption(void *);
//void createWebSocketsServerWss(void *);
//void createWebSocketsServerHttps(void *);

void pubSubInitialiseBrokerless(UA_Server *uaServer, char* mode)
{

//--------------------------------------------------------------------------------------------------------------------------------------------------------------
//                                                   step 0 : get the network interface name
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "==========================================================");
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubInitialiseBrokerless.c : Entering pubSubInitialiseBrokerless");
        UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING(NETWORKADDRESSURL_PROTOCOL)}; // "opc.udp://224.0.0.22:4840/"
        char *env_networkInterface = getenv("SVR_NETWORK_INTERFACE");
        if (env_networkInterface != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_PubSubInitialiseBrokerless.c : retrieved environment variable <SVR_NETWORK_INTERFACE : %s>", env_networkInterface);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_PubSubInitialiseBrokerless.c : cannot retrieve environment variable <SVR_NETWORK_INTERFACE>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_PubSubInitialiseBrokerless.c : default to eth0");
                env_networkInterface = (char *)calloc(255, sizeof(char));
                if (env_networkInterface != NULL)
                        strcpy(env_networkInterface, "eth0");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_PubSubInitialiseBrokerless.c : cannot retrieve environment variable <SVR_NETWORK_INTERFACE> : out of memory");
                        exit(UA_FALSE);
                }
        }
        networkAddressUrl.networkInterface = UA_STRING(env_networkInterface); // name of the interface defined in /etc/dhcpcd.conf

//--------------------------------------------------------------------------------------------------------------------------------------------------------------
//                                                   step 1 :  broker-less PubSub
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubInitialiseBrokerless.c : mode           : %s", mode);

        UA_ServerConfig *config = UA_Server_getConfig(uaServer);

#ifdef UA_ENABLE_PUBSUB_MONITORING
//    UA_PubSubMonitoringInterface monitoringInterface;
#endif // UA_ENABLE_PUBSUB_MONITORING


        UA_StatusCode retval = UA_STATUSCODE_GOOD;
        UA_String transportProfile = UA_STRING("");

        UA_NetworkAddressUrlDataType networkAddressUrlWss = {UA_STRING_NULL, UA_STRING(NETWORKADDRESSURL_PROTOCOL)};
        // if opc.udp, no need to specify networkinterface
        // if opc.eth then need to specify networkinterface


        // broker-less PubSub i.e. UDP, ETH
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubInitialiseBrokerless.c : broker-less segment");
                MQTT_Enable = UA_FALSE;
                MQTT_TLS_Enable = UA_FALSE;
                AMQP_Enable = UA_FALSE;

		char *env_SVRport = getenv("SVR_PORT");

    		if (env_SVRport != NULL)
   			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_PubSubInitialiseBrokerless.c : retrieved environment variable <SVR_PORT> : %s", env_SVRport);
    		else
    		{
			UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_PubSubInitialiseBrokerless.c : cannot retrieve environment variable <SVR_PORT>");
			UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_PubSubInitialiseBrokerless.c : default to 4840");
			env_SVRport = (char*)calloc(255, sizeof(char));
			if (env_SVRport != NULL)
				strcpy(env_SVRport, "4840");
			else
			{
        			UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubInitialiseBrokerless.c : cannot retrieve environment variable <SVR_PORT> : out of memory");
        			exit(UA_FALSE);
			}
    		}

                if ( strncmp(NETWORKADDRESSURL_PROTOCOL, "opc.udp://", 10) == 0)
                {
		        char new_URL[100] = "opc.udp://224.0.0.22:";
	        	strncpy(&new_URL[21], env_SVRport, strlen(env_SVRport));
        		new_URL[strlen(new_URL)] = '/';

                        transportProfile = UA_STRING(TRANSPORT_PROFILE_URI_UDP);
                        networkAddressUrl.url = UA_STRING(new_URL); //UA_STRING("opc.udp://224.0.0.22:4840/");
                }
                else if ( strncmp(NETWORKADDRESSURL_PROTOCOL, "opc.eth://", 10) == 0)
                {
                        char new_URL[100] = "opc.eth://224.0.0.22:";
                        strncpy(&new_URL[21], env_SVRport, strlen(env_SVRport));
                        new_URL[strlen(new_URL)] = '/';

                        transportProfile = UA_STRING(TRANSPORT_PROFILE_URI_ETH);
                        networkAddressUrl.url = UA_STRING(new_URL); //UA_STRING("opc.eth://224.0.0.22:4840/");
                }

		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_PubSubInitialiseBrokerless.c : assembled networkAddressUrl is %s",  networkAddressUrl.url.data);
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------
//                                                          Step 2 : configure the PubSub parameters
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------

        //networkAddressUrl.url = UA_STRING(NETWORKADDRESSURL_UDP); // "opc.udp://224.0.0.22:4840/");

        // defunct: config->pubsubTransportLayers = (UA_PubSubTransportLayer *) UA_calloc(3, sizeof(UA_PubSubTransportLayer)); // UDAP, ETHERNET, MQTT, WSS
        // defunct: if(!config->pubsubTransportLayers)
        // defunct: {
        // defunct:     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"CreateServerPubSub : cannot initialise PubSubTransportLayer");
        // defunct:     return;
        // defunct: }

        // initiate the PubSub SecurityPolicy - kiv until missing fields are fixed
        config->pubSubConfig.securityPolicies = (UA_PubSubSecurityPolicy *) UA_malloc(sizeof(UA_PubSubSecurityPolicy));
        config->pubSubConfig.securityPoliciesSize = 1;
        UA_PubSubSecurityPolicy_Aes128Ctr(config->pubSubConfig.securityPolicies, config->logging);

        /* defunct: v1.2.1 */ //config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();      // options: UA_PubSubTransportLayerEthernet(), UA_PubSubTransportLayerMQTT(),
        /* defunct: v1.2.1 */ //config->pubsubTransportLayersSize++;
        /* new: v1.2.2 */ // UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());

#ifdef UA_ENABLE_PUBSUB_ETH_UADP
        /* defunct: v1.2.1 */ //config->pubsubTransportLayers[1] = UA_PubSubTransportLayerEthernet();
        /* defunct: v1.2.1 */ //config->pubsubTransportLayersSize++;
        /* new: v1.2.2 */ UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerEthernet());
#endif

/*
        if (MQTT_Enable == UA_TRUE && AMQP_Enable == UA_FALSE)
        {
                // defunct:v1.2.1  //config->pubsubTransportLayers[2] = UA_PubSubTransportLayerMQTT();
                // defunct:v1.2.1  //config->pubsubTransportLayersSize++;
                // new:v1.2.2 // UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerMQTT());
        }
        else if (MQTT_Enable == UA_FALSE && AMQP_Enable == UA_TRUE)
        {
                // example can be found in github #3850
                // examples/pubsub/tutorial_pubsub_amqp_publish.c
                // defunct: config->pubsubTransportLayers[2] = UA_PubSubTransportLayerAMQP();
                // defunct: config->pubsubTransportLayersSize++;
                // TODO UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerAMQP());
        }

*/

/*
        config->pubsubTransportLayers[3] = UA_PubSubTransportLayerAMQP();
        config->pubsubTransportLayersSize++;
*/


#ifdef COMMENT
To consider moving the entire step 2a into SV_WebSocketsInitialise.c
   initialiseWebSocketsWssHttps();
#endif

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//                                                            Step 2a : add WEBSOCKETS Server capabilities, because we need 2 parameters
// --------------------------------------------------------------------------------------------------------------------------------------------------------
/*
        #ifdef NOT_READY_WAIT_FOR_OPEN62541 //THIS WILL CAUSE networkAddressUrl.url = (empty) UA_ENABLE_WEBSOCKET_SERVER
        //1. add UA_ServerConfig_addNetworkLayerWS(UA_Server_getConfig(uaServer1), 7681, 0, 0, NULL, NULL); in SV_StartOPCUAServer.c
        //2. gcc -lwebsockets
*/
            #ifdef UA_ENABLE_JSON_ENCODING
            {
                // no such thing as UA_PubSubTransportLayerWSS()
                // defunct: config->pubsubTransportLayers[3] = UA_PubSubTransportLayerWS();
                // defunct: config->pubSubTransportLayersSize++;

                // send the JSON payload to port 7681
                // sample in libwebsockets -> https://libwebsockets.org/git/libwebsockets/tree/minimal-examples/ws-client/minimal-ws-client-tx/minimal-ws-client.c
                transportProfile = UA_STRING(TRANSPORT_PROFILE_URI_WSSJSON);
            }
            #else
                transportProfile = UA_STRING(TRANSPORT_PROFILE_URI_WSSBIN);
            #endif


                networkAddressUrlWss.url = UA_STRING("opc.ws://192.168.1.123:7681/");
	//#ifdef WAIT
                //retval = createWebSocketsServer(&publishedDataSetIdentifier, &networkAddressUrlWss,);    // inititalise the port and send the data to this port

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------


        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "------SV_PubSubInitialiseBrokerless.c : networkAddressUrl.networkInterface is <%s>", networkAddressUrl.networkInterface.data);
        //printf("config->pubsubTransportLayers[0]              : %s \n", config->pubsubTransportLayers[0].data);
        // defunct: printf("config->pubsubTransportLayersSize           : %d \n", config->pubsubTransportLayersSize);

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//                                                                         Step 3 : Call <PubSubAddConnection()>
// --------------------------------------------------------------------------------------------------------------------------------------------------------

        pubSubAddConnectionBrokerless(uaServer, &transportProfile, &networkAddressUrl);		// transportProfile will determine how to create the connection in  SV_PubSubAddConnection.c
        // at this point, the global variable 'PubSubconnectionIdentifier' should be initialised

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "==========================================================");
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialiseBrokerless.c : after addPubSubConnection() completed");
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialiseBrokerless.c : mode <%s>", mode);

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//                                     Step 4 : Depending on the connection type (i.e. broker/ broker-less) AND mode (i.e pub, sub or pub&sub), call the relevant functions (combination)
//				       (mode = nomode => UADP); (mode = pubsub => publish and subscribe); (mode = pub => publish only); (mode = sub => subscribe only) 
// --------------------------------------------------------------------------------------------------------------------------------------------------------

        if (strlen(mode)!=0)
        {
                if ( (strncmp(mode, "--all", 5)==0) && (strlen(mode)==5) )
                {
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "-------------------------------------------------------");
        	                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialiseBrokerless.c : mode = UADP");
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "-------------------------------------------------------");

                        pubSendDataSet(uaServer);
                	        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialiseBrokerless.c : pubSendDataSet() completed");
                        pubInitialiseField(uaServer);
                	        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialiseBrokerless.c : pubInitialiseField() completed");
                        pubWriterGroupBrokerless(uaServer);
                        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialiseBrokerless.c : pubWriterGroup() completed");
                        pubDataSetWriterBrokerless(uaServer);
                        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialiseBrokerless.c : pubDataSetWriter() completed");
 	                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialiseBrokerless.c : successfully initialise Publisher routine");

			// test after datasetwriter is completed
			subReaderGroup(uaServer);
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialiseBrokerless.c : subReaderGroup() completed");
                        subDataSetReader(uaServer);
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialiseBrokerless.c : subDataSetReader() completed");
                        subInitialiseField(uaServer, readerIdentifier);
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialiseBrokerless.c : subInitialiseField() completed");
                        //subReceiveDataSet() ::: this is called within subDataSetReader function in SV_SubscribeDataSetReader.c
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialiseBrokerless.c : subReceiveDataSet() completed");
                        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialiseBrokerless.c : successfully initialise Subscriber routine");

                        return;
                }
		/*
                else if ( (strncmp(mode, "--all", 6)==0) && (strlen(mode)==6) && (brokeraddress != NULL) )
                {

                        // publish
	                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : mode = 'pubsub'");
                        pubSendDataSet(uaServer);
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : pubSendDataSet() completed");
                        pubInitialiseField(uaServer);
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : pubInitialiseField() completed");
                        pubWriterGroup(uaServer);
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : pubWriterGroup() completed");
                        pubDataSetWriter(uaServer);
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : pubSetWriter() completed");

			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"----------SV_PubSubInitialise.c : successfully initialise Publisher routine");

                        // subscribe
                        subReaderGroup(uaServer);
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : subReaderGroup() completed");
                        subDataSetReader(uaServer);
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : subDataSetReader() completed");
                        subInitialiseField(uaServer, readerIdentifier);
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : subInitialiseField() completed");
			//subReceiveDataSet() ::: this is called within subDataSetReader function in SV_SubscribeDataSetReader.c
			//	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : subReceiveDataSet() completed");

                        //UA_PubSubConnection *connection = UA_PubSubConnection_findConnectionbyId(uaServer, PubSubconnectionIdentifier);
                        //addSubscription(uaServer, connection);
                }
		*/
		/*
                else if ( (strncmp(mode, "--pub", 5)==0) && (strlen(mode)==5) && (brokeraddress != NULL) )
                {
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : mode = 'pub'");
			// publish
                        pubSendDataSet(uaServer);
                        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : addPublishedDataSet() completed");
                        pubInitialiseField(uaServer);
                        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : pubInitialiseField() completed");
                        pubWriterGroup(uaServer);
                        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : pubWriterGroup() completed");
                        pubDataSetWriter(uaServer);
                        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : pubDataSetWriter() completed");

                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"----------SV_PubSubInitialise.c : successfully initialise Publisher routine");
                        return;
                }
		*/
	/*
                else if ( (strncmp(mode, "--sub", 5)==0) && (strlen(mode)==5) && (brokeraddress != NULL) )
                {
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : mode = 'sub'");
                        addReaderGroup(uaServer);
                        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : addReaderGroup() completed");
                        addDataSetReader(uaServer);
                        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c  : addDataSetReader() completed");

                        // create a new OPCUA tree nodes from the subscribed payload - the following statements must come after addSubscription()
                        addSubscribedVariables(uaServer, readerIdentifier);     // line 2395
			 	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c  : addSubscribedVariables() completed");
			fillDataSetMetaData(uaServer);
			 	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c  : addDataSetReader()completed");

                        // now do Subscriber routine : OPCUAServer 'subscribing' for methodcalls messages submitted by client through MQTT
                        // example taken from https://github.com/open62541/open62541/blob/mqtt_demo/examples/pubsub/tutorial_pubsub_mqtt.c
                        UA_PubSubConnection *connection = UA_PubSubConnection_findConnectionbyId(uaServer, PubSubconnectionIdentifier); // somehow PubSubconnectionIdentifier is not properly initiaised
                        if(connection != NULL)
                        {
                                // initialise a callback for subscription
                                addSubscription(uaServer, connection);

                                //#ifdef DEBUG_MODE
                                // inspect the contents of *connection
                                UA_String output, identifier;
                                UA_String_init(&output);
                                UA_String_init(&identifier);

                                UA_NodeId_print(&PubSubconnectionIdentifier, &output);
                                UA_NodeId_print(&connection->identifier, &identifier);

                                printf("SV_PubSub.c : CreateServerPubSub() : NodeId : <%s> \n", output.data);
                                printf("in CreateServerPubSub()::3590\n");
                                printf("connection->componentType               : %s\n");
                                printf("connection->config->name                : %s\n", connection->config->name.data);
                                printf("connection->config->enabled             : %d\n", connection->config->enabled);
                                printf("connection->config->transportProfileUri : %s\n", connection->config->transportProfileUri.data);
                                printf("connection->config->publisherId->numeric : %d\n", connection->config->publisherId.numeric); //should not be NULL

                                // cast UA_Variant to UA_String; example in SV_Historizing.c
                                //printf("connection->config->address           : %s\n", *(UA_String *)connection->config->address.data);       // cast UA_Variant to UA_String
                                printf("connection->config->connectionPropertiesSize : %d\n", connection->config->connectionPropertiesSize);
                                printf("connection->channel                     : \n");
                                printf("connection->identifier                  : %s\n", identifier.data);
                                //#endif
                                printf("connection->writeGroupsSize             : %d\n", connection->writerGroupsSize);
                                printf("connection->readerGroupsSize            : %d\n", connection->readerGroupsSize);
                                printf("connection->configurationFreezeCounter  : %d\n", connection->configurationFreezeCounter);
                                printf("connection->configurationFrozen         : %d\n", connection->configurationFrozen);

                                // output capture during runtime is
                                //in CreateServerPubSub()
                                //connection->componentType               : (null)
                                //connection->config->name                : Publisher Connection
                                //connection->config->enabled             : 1
                                //connection->config->transportProfileUri : http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt
                                //connection->config->publisherId->numeric : 2234
                                //connection->config->address             : (null)
                                //connection->config->connectionPropertiesSize : 3
                                //connection->channel                     :
                                //connection->identifier                  : i=50566       // equivalent to NodeId <i=50566>

                                // initialise a callback for subscription - move to front
                                //addSubscription(uaServer, connection);
                                return;
                        }
                        else
                                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : fail to find connectionId");



                        // provide a callback to get notifications of specific PubSub state changes or timeout (e.g. subscriber MessageReceiveTimeout)
                        config->pubsubConfiguration->pubsubStateChangeCallback = pubSubStateChangeCallback;
                        //if (*useCustomMonitoring == UA_TRUE)
                        //{
                        //        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Use custom monitoring callback implementation");
                        //        UA_PubSubMonitoringInterface monitoringInterface;
                        //        monitoringInterface.createMonitoring = pubSubComponent_createMonitoring;
                        //        monitoringInterface.startMonitoring = pubSubComponent_startMonitoring;
                        //        monitoringInterface.stopMonitoring = pubSubComponent_stopMonitoring;
                        //        monitoringInterface.updateMonitoringInterval = pubSubComponent_updateMonitoringInterval;
                        //        monitoringInterface.deleteMonitoring = pubSubComponent_deleteMonitoring;

                        //        config->pubsubConfiguration->monitoringInterface = monitoringInterface;
                        //}

                        // shift after addDataSetWriter()
                        //addReaderGroup(uaServer);
                        //printf("CreateServerPubSub : addReaderGroup() completed \n");
                        //addDataSetReader(uaServer);
                        //printf("CreateServerPubSub : addDataSetReader()completed \n");

                        // create a new OPCUA tree nodes from the subscribed payload
                        //addSubscribedVariables(uaServer, readerIdentifier);
                        //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"CreateServerPubSub : addSubscribedVariables() completed");
                }
                else
                {
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : <mode> indicated = %s", mode);
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : Error: Unknown <mode>");
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : Error: please indicate 'pub' or 'sub'");
                        exit(0);
                }
	*/
        }
        /*
        else // mode == NULL
        {
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : mode = NULL");
                addPublishedDataSet(uaServer);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : addPublishedDataSet() completed");
                addDataSetField(uaServer);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : addDataSetField() completed");
                addWriterGroup(uaServer);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : addWriterGroup() completed");
                addDataSetWriter(uaServer);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "CreateServerPubSub : addDataSetWriter() completed");

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"CreateServerPubSub : successfully initialise Publisher routine");
                return;
        }
        */
}

#ifdef Shift_to_SV_WebSocketsInitialise_c
	#ifdef COMMENT
		Here we are creating a thread to launch websockets server using pthread
		It is an alternate implementation to open62541 websockets implementation
	#endif

	pthread_t webSocketServerthread1, webSocketServerthread2;
	//int createWebSocketsServer(UA_NodeId *publishedDataSetidentifier, UA_NetworkAddressUrlDataType *networkAddressUrl, int, char**);
	typedef struct webSocketStruct {
		UA_Server *uaServer;
		UA_String uaServerIP;
		UA_NodeId *pdsId;
		UA_NetworkAddressUrlDataType networkAddrUrl;
		int argC;
		char** argV;
	} wsThreadArgs;

	#ifdef COMMENT
		"Usage: lwsws [-c <config dir>] "
		"[-d <log bitfield>] [--help] "
		"[-n]");
	#endif

	wsThreadArgs webSocketArgs1, webSocketArgs2, webSocketArgs3, webSocketArgs4;
	// websocket HTTPS
	webSocketArgs1.uaServer = uaServer;
	webSocketArgs1.pdsId = &publishedDataSetIdentifier;
	webSocketArgs1.networkAddrUrl = networkAddressUrl;
	webSocketArgs1.uaServerIP = UA_STRING("192.168.1.123");
	// websocket WSS
	webSocketArgs2.uaServer = uaServer;
        webSocketArgs2.pdsId = &publishedDataSetIdentifier;
        webSocketArgs2.networkAddrUrl = networkAddressUrl;
	webSocketArgs2.uaServerIP = UA_STRING("192.168.1.123");
	// websocket HTTPS & WSS
	webSocketArgs3.uaServer = uaServer;
        webSocketArgs3.pdsId = &publishedDataSetIdentifier;
        webSocketArgs3.networkAddrUrl = networkAddressUrl;
	webSocketArgs3.uaServerIP = UA_STRING("192.168.1.123");
	webSocketArgs4.uaServer = uaServer;
        webSocketArgs4.pdsId = &publishedDataSetIdentifier;
        webSocketArgs4.networkAddrUrl = networkAddressUrl;
	webSocketArgs4.uaServerIP = UA_STRING("192.168.1.123");

	bool result, result1, result2;
	char* env_WebSocketsServerType = getenv("SVR_WEBSOCKETS_SERVERTYPE");

    	if (env_WebSocketsServerType != NULL)
        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_PubSubInitialiseBrokerless.c : retrieved environment variable <SVR_WEBSOCKETS_SERVERTYPE> : %s", env_WebSocketsServerType);
    	else
    	{
        	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_PubSubInitialiseBrokerless.c : cannot retrieve environment variable <SVR_WEBSOCKETS_SERVERTYPE>");
        	UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_PubSubInitialiseBrokerless.c : default to HTTPS");
        	env_WebSocketsServerType = (char*)calloc(255, sizeof(char));
        	if (env_WebSocketsServerType != NULL)
                	strcpy(env_WebSocketsServerType, "WSS");
        	else
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubInitialiseBrokerless.c : cannot retrieve environment variable <SVR_WEBSOCKETS_SERVERTYPE> : out of memory");
                	exit(UA_FALSE);
        	}
    	}

        int response;
	while (1)
	{
        printf("Choose the type of server instance to create \n");
        printf("1 : Http Secure instance (https)\n");
        printf("2 : Web Sockets Secure (wss) \n");
	printf("3 : Both https and wss intances \n");
        printf("> "); scanf("%d", &response);

        switch (response)
        {
                case 1 : // Websocket HTTPS
			char *argumentV1[] = {"./lws-minimal-ws-server", "-s"};		// -s means "serve using TLS selfsigned cert
        		int argumentC1 = sizeof(argumentV1) / sizeof(char*);
			webSocketArgs1.argC = argumentC1;
			webSocketArgs1.argV = argumentV1;
			result = pthread_create(&webSocketServerthread1, NULL, (void *)create1WebSocketsServerHttps, &webSocketArgs1);
			break;

                case 2 : // Websocket WSS
		        char *argumentV2[] = {"./lwsws", "/etc/lwsws/conf.d", "-d", "1151"};             // -s "serve using TLS selfsigned cert, -d debug
                        int argumentC2 = sizeof(argumentV2) / sizeof(char*);
                        webSocketArgs2.argC = argumentC2;
                        webSocketArgs2.argV = argumentV2;
			result = pthread_create(&webSocketServerthread2, NULL, (void *)create1WebSocketsServerWss, &webSocketArgs2); break;

		case 3 : // Websocket HTTPS
			char *argumentV3[] = {"./lws-minimal-ws-server", "-s"};                // -s means "serve using TLS selfsigned cert
                        int argumentC3 = sizeof(argumentV3) / sizeof(char*);
                        webSocketArgs3.argC = argumentC3;
                        webSocketArgs3.argV = argumentV3;
			result1 = pthread_create(&webSocketServerthread1, NULL, (void *)create1WebSocketsServerHttps, &webSocketArgs3);
			// Websocket WSS
                        char *argumentV4[] = {"./lwsws", "/etc/lwsws/conf.d", "-d", "1151"};             // -s "serve using TLS selfsigned cert, -d debug
                        int argumentC4 = sizeof(argumentV4) / sizeof(char*);
                        webSocketArgs4.argC = argumentC4;
                        webSocketArgs4.argV = argumentV4;
			result2 = pthread_create(&webSocketServerthread2, NULL, (void *)create1WebSocketsServerWss, &webSocketArgs4);

			result = result1 | result2;
			break;

                default : printf("Unknown response.. please try again \n"); continue;
        }
	} // while (1)

	//result = pthread_create(&webSocketServerthread, NULL, (void *)createWebSocketsOption, &webSocketArgs);	// argC is corrupted
     	if (result != 0)
        {
                printf("-----------------------------------------------------------------------------------------------------------------\n");
                printf("------SV_PubSubInitialiseBrokerless.c : Fail to create websocket server thread as a separate process             \n");
                printf("------SV_PubSubInitialiseBrokerless.c : Websockets server functions will not be available until the next restart \n");
                printf("-----------------------------------------------------------------------------------------------------------------\n");
        }
        else
        {
	   switch (response)
	   {
	     case 1 : //
                result = pthread_detach(webSocketServerthread1);
                if (result != 0)
                {
                        printf("-----SV_PubSubInitialiseBrokerless.c : cannot detach Websockets server thread from main thread \n");
                        exit(0);
                }
                else
                {
                        printf("---------------------------------------------------------------------------------------------------------\n");
                        printf("------SV_PubSubInitialiseBrokerless.c : websocket server HTTPS thread is created successfully                  \n");
	                printf("------SV_PubSubInitialiseBrokerless.c : starting Websockets server HTTPS thread and waiting for request  \n");
			printf("------SV_PubSubInitialiseBrokerless.c : data will be made available in https://<UAServer url>:8443 \n");
                        printf("---------------------------------------------------------------------------------------------------------\n");
		}
		break;
	     case 2 : //
		result = pthread_detach(webSocketServerthread2);
		if (result != 0)
                {
                        printf("-----SV_PubSubInitialiseBrokerless.c : cannot detach Websockets server thread from main thread \n");
                        exit(0);
                }
		else
		{
			printf("---------------------------------------------------------------------------------------------------------\n");
                        printf("------SV_PubSubInitialiseBrokerless.c : websocket server WSS thread is created successfully                  \n");
			printf("------SV_PubSubInitialiseBrokerless.c : starting Websockets server WSS thread and waiting for request    \n");
			printf("------SV_PubSubInitialiseBrokerless.c : data will be made available in https://<UAServer url>:7681 \n");
                        printf("---------------------------------------------------------------------------------------------------------\n");
                }
		break;
	    case 3 : //
		result1 = pthread_detach(webSocketServerthread1);
		result2 = pthread_detach(webSocketServerthread2);
		if (result1 || result2 != 0)
                {
                        printf("-----SV_PubSubInitialiseBrokerless.c : cannot detach Websockets server thread from main thread \n");
                        exit(0);
                }
                else
		{
                        printf("---------------------------------------------------------------------------------------------------------\n");
                        printf("------SV_PubSubInitialiseBrokerless.c : websocket server HTTPS and WSS threads are created successfully                  \n");
                        printf("------SV_PubSubInitialiseBrokerless.c : starting Websockets server HTTPS and WSS threads and waiting for request    \n");
                        printf("------SV_PubSubInitialiseBrokerless.c : data will be made available in https://<UAServer url>:8443 and https://<UAServer url>:7681\n");
                        printf("---------------------------------------------------------------------------------------------------------\n");
                }
		break;
	   }
        }
#endif
