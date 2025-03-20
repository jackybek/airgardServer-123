#define DEBUG_MODE

#ifdef JSON5
#include "json5.h"
#endif

#ifdef almagamation
#include <open62541/types_generated.h>
#include <plugins/ua_network_pubsub_mqtt.h>    // contain UA_PubSubTransportLayerMQTT() header; implementation in plugins/ua_network_pubsub_mqtt.c
#include <open62541/plugin/pubsub_udp.h>
#include <open62541/plugin/pubsub_ethernet.h>
#include <open62541/plugin/securitypolicy_default.h>
#include <open62541/plugin/pubsub.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
#include <pubsub/ua_pubsub.h> // in ~/open62541/src/pubsub/ua_pubsub.h :  contain the following struct
//#include "open62541.h"
//#include "ua_pubsub_networkmessage.h"
//#include "ua_pubsub.h"
#else
   #include "open62541.h"
   #define UA_ENABLE_PUBSUB
   #define UA_ENABLE_PUBSUB_ENCRYPTION
   #define UA_ENABLE_PUBSUB_INFORMATIONMODEL
   #define UA_ENABLE_PUBSUB_MQTT
#endif
#include "SV_PubSub.h"			// contains all the #define for PUBSUB modules
#include "SV_Misc.h"

UA_Boolean MQTT_Enable = UA_FALSE;
UA_Boolean MQTT_TLS_Enable = UA_FALSE;
UA_Boolean AMQP_Enable = UA_FALSE;
int MQTT_Port = 1883;   // default set to 1883

// only 1 PubSubConnection for both reader and writer
UA_NodeId PubSubconnectionIdentifier;

// Subscriber section variables & functions
UA_DataSetReaderConfig dataSetReaderConfig;
UA_NodeId readerGroupIdentifier;
UA_NodeId readerIdentifier;
UA_NodeId reader2Id;

void pubSubInitialise(UA_Server *uaServer, char* brokeraddress, int port, char* mode)
{
//--------------------------------------------------------------------------------------------------------------------------------------------------------------
//                                                   step 1 : check whether is it a broker-based or broker-less PubSub
//-------------------------------------------------------------------------------------------------------------------------------------------------------------
UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "==========================================================");
UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubInitialise.c : Entering pubSubInitialise");
UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubInitialise.c : broker address : %s", brokeraddress);
UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubInitialise.c : port           : %d", port);
UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubInitialise.c : mode           : %s", mode);

        UA_ServerConfig *config = UA_Server_getConfig(uaServer);

#ifdef UA_ENABLE_PUBSUB_MONITORING
    UA_PubSubMonitoringInterface monitoringInterface;
#endif // UA_ENABLE_PUBSUB_MONITORING


//        UA_StatusCode retval = UA_STATUSCODE_GOOD;
        UA_String transportProfile = UA_STRING("");

        UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL, UA_STRING(NETWORKADDRESSURL_PROTOCOL)}; // "opc.udp://224.0.0.22:4840/"
  //      UA_NetworkAddressUrlDataType networkAddressUrlWss = {UA_STRING_NULL, UA_STRING(NETWORKADDRESSURL_PROTOCOL)};
        // if opc.udp, no need to specify networkinterface
        // if opc.eth then need to specify networkinterface

        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : CreateServerPubSub()");

        if ( brokeraddress != NULL) // broker-based PubSub i.e. MQTT, AMQP
        {
                char portbuf[5];
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : brokeraddress : %s, port %d", brokeraddress, port);
                MQTT_Port = port;       // save to global variable
                if ( (port == 8883) || (port == 8884) || (port == 8885) || (port == 8887) )
                {
                        char URI_mqtt[100];
                        MQTT_Enable = UA_TRUE;
                        MQTT_TLS_Enable = UA_TRUE;
                        AMQP_Enable = UA_FALSE;

                        //similar to sprintf(URI_mqtt, "opc.mqtt://%s:1883", brokeraddress);
                        strcpy(URI_mqtt, "opc.mqtt://");
                        strcat(URI_mqtt, brokeraddress);
                        strcat(URI_mqtt, ":");
                        itoa(port, portbuf, 10);
                        strcat(URI_mqtt, portbuf);
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : URI_mqtt : <%s>", URI_mqtt);

                        transportProfile = UA_STRING(TRANSPORT_PROFILE_URI_MQTT);
                        networkAddressUrl.url = UA_STRING(URI_mqtt);
                }
                else if ( (port == 1883) || (port == 1884) )
                {
                        char URI_mqtt[100];
                        // set the MQTT_Enable global flag
                        MQTT_Enable = UA_TRUE;
                        MQTT_TLS_Enable = UA_FALSE;
                        AMQP_Enable = UA_FALSE;

                        //similar to sprintf(URI_mqtt, "opc.mqtt://%s:1883", brokeraddress);
                        strcpy(URI_mqtt, "opc.mqtt://");
                        strcat(URI_mqtt, brokeraddress);
                        strcat(URI_mqtt, ":");
                        itoa(port, portbuf, 10);
                        strcat(URI_mqtt, portbuf);
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : URI_mqtt : <%s>", URI_mqtt);

                        transportProfile = UA_STRING(TRANSPORT_PROFILE_URI_MQTT);
                        networkAddressUrl.url = UA_STRING(URI_mqtt);
                }
                else if (port == 5672)
                {
                        char URI_amqp[100];
                        // set the AMQP_Enable global flag
                        MQTT_Enable = UA_FALSE;
                        MQTT_TLS_Enable = UA_FALSE;
                        AMQP_Enable = UA_TRUE;

                        //similar to sprintf(URI_mqtt, "opc.amqp://%s:5672", brokeraddress);
                        strcpy(URI_amqp, "opc.amqp://");
                        strcat(URI_amqp, brokeraddress);
                        strcat(URI_amqp, ":5672");
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : URI_amqp : <%s>", URI_amqp);

                        transportProfile = UA_STRING(TRANSPORT_PROFILE_URI_AMQP);
                        networkAddressUrl.url = UA_STRING(URI_amqp);
                }
        }
        else // broker-less PubSub i.e. UDP, ETH
        {
                MQTT_Enable = UA_FALSE;
                MQTT_TLS_Enable = UA_FALSE;
                AMQP_Enable = UA_FALSE;
                if ( strncmp(NETWORKADDRESSURL_PROTOCOL, "opc.udp://", 10) == 0)
                {
                        transportProfile = UA_STRING(TRANSPORT_PROFILE_URI_UDP);
                        networkAddressUrl.url = UA_STRING("opc.udp://224.0.0.22:4840/");
                }
                else if ( strncmp(NETWORKADDRESSURL_PROTOCOL, "opc.eth://", 10) == 0)
                {
                        transportProfile = UA_STRING(TRANSPORT_PROFILE_URI_ETH);
                        networkAddressUrl.url = UA_STRING("opc.eth://224.0.0.22:4840/");
                        //networkAddressUrl.networkInterface = UA_STRING("eth0");
                }

        }

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------
//                                                          Step 2 : configure the PubSub parameters
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------

        networkAddressUrl.networkInterface = UA_STRING("eth0"); // name of the interface defined in /etc/dhcpcd.conf
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

        if (MQTT_Enable == UA_TRUE && AMQP_Enable == UA_FALSE)
        {
                /* defunct:v1.2.1 */ //config->pubsubTransportLayers[2] = UA_PubSubTransportLayerMQTT();
                /* defunct:v1.2.1 */ //config->pubsubTransportLayersSize++;
                /* new:v1.2.2 */ //UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerMQTT());
        }
        else if (MQTT_Enable == UA_FALSE && AMQP_Enable == UA_TRUE)
        {
                // example can be found in github #3850
                // examples/pubsub/tutorial_pubsub_amqp_publish.c
                // defunct: config->pubsubTransportLayers[2] = UA_PubSubTransportLayerAMQP();
                // defunct: config->pubsubTransportLayersSize++;
                // TODO UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerAMQP());
        }

// ??????????????????????????????????????? 20/3/2025 - why is WEBSOCKETS here ????????????????????????????????????????
        #ifdef NOT_READY_WAIT_FOR_OPEN62541 //THIS WILL CAUSE networkAddressUrl.url = (empty) UA_ENABLE_WEBSOCKET_SERVER
        //1. add UA_ServerConfig_addNetworkLayerWS(UA_Server_getConfig(uaServer1), 7681, 0, 0, NULL, NULL); in SV_StartOPCUAServer.c
        //2. gcc -lwebsockets

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


                networkAddressUrlWss.url = UA_STRING("opc.wss://192.168.1.11:7681/");

                retval = CreateServerWebSockets(&publishedDataSetIdentifier, &networkAddressUrlWss);    // inititalise the port and send the data to this port
                if (retval != UA_STATUSCODE_GOOD)
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"----------SV_PubSubInitialise.c : cannot initialise websockets port 7681");
                else
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"----------SV_PubSubInitialise.c : Successfully initialise websockets port 7681");

        #endif
// ?????????????????????????????????????????????????????????????????????????????????????????????????????????????

/*
        config->pubsubTransportLayers[3] = UA_PubSubTransportLayerAMQP();
        config->pubsubTransportLayersSize++;
*/

        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : networkAddressUrl.networkInterface           : %s", networkAddressUrl.networkInterface.data);
        //printf("config->pubsubTransportLayers[0]              : %s \n", config->pubsubTransportLayers[0].data);
        // defunct: printf("config->pubsubTransportLayersSize           : %d \n", config->pubsubTransportLayersSize);

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//                                                                         Step 3 : Call <addPubSubConnection()>
// --------------------------------------------------------------------------------------------------------------------------------------------------------

        pubSubAddConnection(uaServer, &transportProfile, &networkAddressUrl);
        // at this point, the global variable 'PubSubconnectionIdentifier' should be initialised

        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : after addPubSubConnection() completed");
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : brokeraddress %s", brokeraddress);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : mode %s", mode);

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//                                     Step 4 : Depending on the connection type (i.e. broker/ broker-less) AND mode (i.e pub, sub or pub&sub), call the relevant functions (combination)
//				       (mode = nomode => UADP); (mode = pubsub => publish and subscribe); (mode = pub => publish only); (mode = sub => subscribe only) 
// --------------------------------------------------------------------------------------------------------------------------------------------------------

        if (strlen(mode)!=0)
        {
                if ( (strncmp(mode, "nomode", 6)==0) && (strlen(mode)==6) && (brokeraddress == NULL) )
                {
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : mode = UADP");
                        pubSendDataSet(uaServer);
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : pubSendDataSet() completed");
                        pubInitialiseField(uaServer);
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : pubInitialiseField() completed");
                        pubWriterGroup(uaServer);
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : pubWriterGroup() completed");
                        pubDataSetWriter(uaServer);
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : pubDataSetWriter() completed");

                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"----------SV_PubSubInitialise.c : successfully initialise Publisher routine");
                        return;
                }

                else if ( (strncmp(mode, "pubsub", 6)==0) && (strlen(mode)==6) && (brokeraddress != NULL) )
                {
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PubSubInitialise.c : mode = 'pubsub'");

                        // publish
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

                else if ( (strncmp(mode, "pub", 3)==0) && (strlen(mode)==3) && (brokeraddress != NULL) )
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
	/*
                else if ( (strncmp(mode, "sub", 3)==0) && (strlen(mode)==3) && (brokeraddress != NULL) )
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
