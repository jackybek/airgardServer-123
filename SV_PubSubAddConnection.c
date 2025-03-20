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
#include "SV_PubSub.h"


#ifdef UA_ENABLE_JSON_ENCODING
 static UA_Boolean useJson = UA_TRUE;
#else
 static UA_Boolean useJson = UA_FALSE;
#endif

extern UA_Boolean MQTT_Enable;
extern UA_Boolean MQTT_TLS_Enable;
extern UA_Boolean AMQP_Enable;
extern int MQTT_Port;   // default set to 1883

// only 1 PubSubConnection for both reader and writer
extern UA_NodeId PubSubconnectionIdentifier;


void
pubSubAddConnection(UA_Server *uaServer, UA_String *transportProfile,
                        UA_NetworkAddressUrlDataType *networkAddressUrl)
{
    /* Details about the connection configuration and handling are located
     * in the pubsub connection tutorial */

/*
struct UA_PubSubConnectionConfig {
    1 UA_String name;                           // set
    2 UA_Boolean enabled;                       // set
    3 UA_PublisherIdType publisherIdType;       // set
    union { // std: valid types UInt or String
       UA_UInt32 numeric;
        UA_String string;
    4 } publisherId;                            // set numeric
    5 UA_String transportProfileUri;            // set
    6 UA_Variant address;                       // set
    7 UA_Variant connectionTransportSettings;   // set
    8 size_t connectionPropertiesSize;          // set
    9 UA_KeyValuePair *connectionProperties;    // set
};
*/
    #ifdef DEBUG_MODE
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "==========================================================");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubAddConnection.c : Entering addPubSubConnection()");
    #endif

//-------------------------------------------------------------------- Step 1 : Set up PubSub Connection Configuration
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubAddConnection.c : Step 1 : Set up PubSub Connection Configuration");
 
    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));

    /*1*/connectionConfig.name = UA_STRING(CONNECTION_CONFIGNAME);      // UA_STRING("Publisher Connection")
    /*2*/connectionConfig.enabled = UA_TRUE;
    /*3*/connectionConfig.publisherIdType = UA_PUBLISHERIDTYPE_UINT16;
    /*4*/connectionConfig.publisherId.uint16 = PUBLISHERID;     // Changed to static publisherId from random generation to identify the publisher on subscriber side
    /*5*/connectionConfig.transportProfileUri = *transportProfile;
    /*6*/UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl->url.data, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);

    if(useJson)
        connectionConfig.transportProfileUri = UA_STRING(TRANSPORT_PROFILE_URI_JSON);
    else
        connectionConfig.transportProfileUri = UA_STRING(TRANSPORT_PROFILE_URI_UADP);

    //UA_ServerConfig *config = UA_Server_getConfig(uaServer);
    /*7*/ //UA_Variant_setScalar(&connectionConfig.connectionTransportSettings,
        //      config->pubsubTransportLayers, &UA_TYPES[UA_TYPES_BROKERCONNECTIONTRANSPORTDATATYPE]); // UA_TYPES_DATAGRAMCONNECTIONTRANSPORTDATATYPE
    // UA_TYPES_BROKERCONNECTIONTRANSPORTDATATYPE == 31
    // UA_TYPES_DATAGRAMCONNECTIONTRANSPORTDATATYPE == 85
    // UA_TYPES[31]
    // UA_TYPES[85]

    /*8*///connectionConfig.connectionPropertiesSize = 3;       // to match connectionOptions[N]
    /*9*///connectionConfig.connectionProperties = (UA_KeyValuePair *)calloc(3, sizeof(UA_KeyValuePair));

/*
typedef struct {
    UA_QualifiedName key;
    UA_Variant value;
} UA_KeyValuePair;
*/

    if (MQTT_Enable == UA_TRUE && AMQP_Enable == UA_FALSE)
    {
	// ------------------------------------------------------------------ Step 1.1 : Setup PubSub Connection "Options" for MQTT
        #ifdef DEBUG_MODE
    	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubAddConnection.c : Step 1.1 : Setup PubSub Connection 'Options' for MQTT");
        #endif

        /*
        UA_ServerConfig *config = UA_Server_getConfig(uaServer);
        //7
        //UA_Variant_setScalar(&connectionConfig.connectionTransportSettings,
        //      config->pubsubTransportLayers, &UA_TYPES[UA_TYPES_BROKERCONNECTIONTRANSPORTDATATYPE]);
        */
	int connectionOptionsCount=0;
        size_t connectionOptionsIndex;
        size_t namespaceIndex;
	UA_KeyValueMap connectionOptions;	//[connectionOptionsCount];	// allocate all slots, but use only necessary slots depending on MQTT_TLS_Enable flag

        if (MQTT_TLS_Enable == UA_TRUE)
	{
                connectionOptionsCount = 10;	// MQTT-TLS needs to setup 10 connection options
	}
        else // MQTT_TLS_Enable == UA_FALSE
	{
                connectionOptionsCount = 5;		// MQTT-Non_TLS needs to setup 5 connection options
	}

        UA_Server_getNamespaceByName(uaServer, UA_STRING("virtualskies.com.sg/MKS/"), &namespaceIndex);

#ifdef COMMENT
 fixed array position to handle different parameters
 1. connectionOptions[0] = mqttClientId
 2. connectionOptions[1] = sendBufferSize
 3. connectionOptions[2] = recvBufferSize
 4. connectionOptions[3] = mqttUsername         // used for port 1884, 8884, 8885
 5. connectionOptions[4] = mqttPassword         // used for port 1884, 8884, 8885
 6. connectionOptions[5] = useTLS               // used for port 8883, 8884, 8885
 7. connectionOptions[6] = mqttCaPath           // used for port 8883, 8885
 8. connectionOptions[7] = mqttCaFilePath       // used for port 8883, 8885
 9. connectionOptions[8] = mqttClientCertPath   // used for port 8884
 10. connectionOptions[9] = mqttClientKeyPath   // used for port 8884
#endif

#ifdef FOR_REFERENCE
        connectionOptions[0].key = UA_QUALIFIEDNAME(namespaceIndex, CONNECTIONOPTION_NAME_MQTT);   // mqttClientId
        connectionOptions[1].key = UA_QUALIFIEDNAME(namespaceIndex, "sendBufferSize");
        connectionOptions[2].key = UA_QUALIFIEDNAME(namespaceIndex, "recvBufferSize");
        connectionOptions[3].key = UA_QUALIFIEDNAME(namespaceIndex, USERNAME_OPTION_NAME_MQTT); // mqttUsername
        connectionOptions[4].key = UA_QUALIFIEDNAME(namespaceIndex, PASSWORD_OPTION_NAME_MQTT); // mqttPassword
        connectionOptions[5].key = UA_QUALIFIEDNAME(namespaceIndex, "mqttUseTLS");
        connectionOptions[6].key = UA_QUALIFIEDNAME(namespaceIndex, "mqttCaPath");
        connectionOptions[7].key = UA_QUALIFIEDNAME(namespaceIndex, "mqttCaFilePath");
        connectionOptions[8].key = UA_QUALIFIEDNAME(namespaceIndex, "mqttClientCertPath");
        connectionOptions[9].key = UA_QUALIFIEDNAME(namespaceIndex, "mqttClientKeyPath");
#endif


// the following are modified due to change from keyvalue to struct keyvaluemap
	connectionOptions.mapSize = connectionOptionsCount;

	// need to allocate memory as map component in UA_KeyValueMap struct is a pointer
	connectionOptions.map = (UA_KeyValuePair *)malloc(connectionOptionsCount);
	if (connectionOptions.map == NULL)
	{
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubAddConnection.c : Fail to allocate heap memory for connectionOptions.map");
		exit(EXIT_FAILURE);
	}

	connectionOptionsIndex = 0;
	connectionOptions.map[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, CONNECTIONOPTION_NAME_MQTT);   // mqttClientId;
	UA_String mqttClientId = UA_STRING(CLIENT_ID_MQTT);
	UA_Variant_setScalar(&connectionOptions.map[connectionOptionsIndex].value, &mqttClientId, &UA_TYPES[UA_TYPES_STRING]); ;

        //if ( (MQTT_Port == 1884) || (MQTT_Port == 8884) )
        //{
        // login credentials to mqtt broker on 192.168.1.11

 	connectionOptionsIndex++;	// 1
        connectionOptions.map[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, USERNAME_OPTION_NAME_MQTT);    // mqttUsername
        UA_String mqttUsername = UA_STRING(USERNAME_MQTT);
        UA_Variant_setScalar(&connectionOptions.map[connectionOptionsIndex].value, &mqttUsername, &UA_TYPES[UA_TYPES_STRING]);

        connectionOptionsIndex++;       // 2
        connectionOptions.map[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, PASSWORD_OPTION_NAME_MQTT);    // mqttPassword
        UA_String mqttPassword = UA_STRING(PASSWORD_MQTT);
        UA_Variant_setScalar(&connectionOptions.map[connectionOptionsIndex].value, &mqttPassword, &UA_TYPES[UA_TYPES_STRING]);

        // preallocate sendBufferSize
        connectionOptionsIndex++;       // 3
        connectionOptions.map[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, "sendBufferSize");
        UA_UInt32 sendBufferSize = 32767;
        UA_Variant_setScalar(&connectionOptions.map[connectionOptionsIndex].value, &sendBufferSize, &UA_TYPES[UA_TYPES_UINT32]);

        // preallocate recvBufferSize
        connectionOptionsIndex++;       // 4
        connectionOptions.map[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, "recvBufferSize");
        UA_Int32 recvBufferSize = 32767;
        UA_Variant_setScalar(&connectionOptions.map[connectionOptionsIndex].value, &recvBufferSize, &UA_TYPES[UA_TYPES_UINT32]);

        //}
        //else
        /*{
           connectionOptionsIndex=3;    // 3
           //connectionOptions[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, USERNAME_OPTION_NAME_MQTT);       // mqttUsername
           UA_String mqttUsername = UA_STRING("");
           UA_Variant_setScalar(&connectionOptions[connectionOptionsIndex].value, &mqttUsername, &UA_TYPES[UA_TYPES_STRING]);

           connectionOptionsIndex=4;    // 4
           //connectionOptions[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, PASSWORD_OPTION_NAME_MQTT);       // mqttPassword
           UA_String mqttPassword = UA_STRING("");
           UA_Variant_setScalar(&connectionOptions[connectionOptionsIndex].value, &mqttPassword, &UA_TYPES[UA_TYPES_STRING]);

        }*/

        if (MQTT_TLS_Enable == UA_TRUE)
        {
        // ------------------------------------------------------------------ Step 1.2 : Setup PubSub Connection TLS options for MQTT
        #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubAddConnection.c : Step 1.2 : Setup PubSub Connection TLS options for MQTT");
        #endif

// https://github.com/open62541/open62541/blob/master/examples/pubsub/tutorial_pubsub_mqtt_publish.c
/*  * MQTT via TLS
 * ^^^^^^^^^^^^
 * Defining EXAMPLE_USE_MQTT_TLS enables TLS for the MQTT connection. This is
 * experimental and currently only available with OpenSSL.
 *
 * https://test.mosquitto.org/ offers a public MQTT broker with TLS support.
 * The access via port 8883 can be used with this example. Download the file
 * ``mosquitto.org.crt`` and point the CA_FILE_PATH define to its location.
 *
 * If the server requires a client certificate, the two options ``mqttClientCertPath``
 * and ``mqttClientKeyPath`` must be added in ``addPubSubConnection()``.
 *
 * To use a directory containing hashed certificates generated by ``c_rehash``,
 * use the ``mqttCaPath`` option instead of ``mqttCaFilePath``. If none of these two
 * options is provided, the system's default certificate location will be used.
 *
 * #define CA_FILE_PATH_MQTT                      "/etc/ssl/certs/ca-11.pem"
 * #define CLIENT_CERT_PATH_MQTT                  "/etc/ssl/certs/broker-cert11.pem"
 * #define CLIENT_KEY_PATH_MQTT                   "/etc/ssl/certs/brokerprivate-key11.pem"
 */

                // the following is tested successfully
                // sudo mosquitto_pub -d -h Mqtt-11 -p 8883 -t AirgardTopic --capath /etc/ssl/certs/ --cafile mosq-ca.crt -m "Secure Message"
                // ./myNewServer 192.168.1.33 192.168.1.119 192.168.1.11 8883 pub ==> can run

		connectionOptionsIndex++; // 5
                connectionOptions.map[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, "mqttUseTLS");
                UA_Boolean mqttUseTLS = UA_TRUE;
                UA_Variant_setScalar(&connectionOptions.map[connectionOptionsIndex].value, &mqttUseTLS, &UA_TYPES[UA_TYPES_BOOLEAN]);

           //if (MQTT_Port == 8884) // only port 8884 requires client certificate
           //{
                connectionOptionsIndex++; // 6
                connectionOptions.map[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, "mqttCaPath");
                UA_String mqttCaPath = UA_STRING(CA_PATH_MQTT);
                UA_Variant_setScalar(&connectionOptions.map[connectionOptionsIndex].value, &mqttCaPath, &UA_TYPES[UA_TYPES_STRING]);

                connectionOptionsIndex++; // 7
                connectionOptions.map[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, "mqttCaFilePath");
                UA_String mqttCaFile = UA_STRING(CA_FILE_PATH_MQTT);
                UA_Variant_setScalar(&connectionOptions.map[connectionOptionsIndex].value, &mqttCaFile, &UA_TYPES[UA_TYPES_STRING]);

                connectionOptionsIndex++; // 8
                connectionOptions.map[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, "mqttClientCertPath");
                UA_String mqttClientCertPath = UA_STRING(CLIENT_CERT_PATH_MQTT);
                UA_Variant_setScalar(&connectionOptions.map[connectionOptionsIndex].value, &mqttClientCertPath, &UA_TYPES[UA_TYPES_STRING]);

                connectionOptionsIndex++; // 9
                connectionOptions.map[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, "mqttClientKeyPath");
                UA_String mqttClientKeyPath = UA_STRING(CLIENT_KEY_PATH_MQTT);
                UA_Variant_setScalar(&connectionOptions.map[connectionOptionsIndex].value, &mqttClientKeyPath, &UA_TYPES[UA_TYPES_STRING]);
            //}// MQTT_Port = 8884
        }
        connectionConfig.connectionProperties.map = connectionOptions.map;
        connectionConfig.connectionProperties.mapSize = connectionOptionsIndex+1;       // add 1 because the index start at 0;

        #ifdef DEBUG_MODE
		int status = print_debug_MQTT(&connectionConfig, networkAddressUrl);
		sleep(status);
        #endif


        // initiatise the value of PubSubconnectionIdentifier (UA_NodeId) so that we can check later in addSubscription()
        #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
		"--------SV_PubSubAddConnection.c : Before calling UA_Server_addPubSubConnection() : MQTT_Enable == UA_TRUE && AMQP_Enable == UA_FALSE, connectionOptionIndex = %d", connectionOptionsIndex);
        #endif

        UA_StatusCode retval = UA_Server_addPubSubConnection(uaServer, &connectionConfig, &PubSubconnectionIdentifier);
        if (retval == UA_STATUSCODE_GOOD)
        {
                UA_String output;
                UA_String_init(&output);

                #ifdef DEBUG_MODE
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubAddConnection.c : The output of UA_Server_addPubSubConnection is a NodeId : PubSubconnectionIdentifier .. check isNull= %d", UA_NodeId_isNull(&PubSubconnectionIdentifier));
                #endif

                #ifdef DEBUG_MODE
                retval = UA_NodeId_print(&PubSubconnectionIdentifier, &output);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubAddConnection.c : NodeId : <%s>", output.data);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"--------SV_PubSubAddConnection.c : PubSubconnectionIdentifier success : %s", UA_StatusCode_name(retval));
                #endif
        }
        else
        {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"--------SV_PubSubAddConnection.c : PubSubconnectionIdentifier failure : %s", UA_StatusCode_name(retval));
                exit(EXIT_FAILURE);
        }
        //sleep(5);
    } // if (MQTT_Enable == UA_TRUE && AMQP_Enable == UA_FALSE)
    else if (MQTT_Enable == UA_FALSE && AMQP_Enable == UA_TRUE)
    {
        // ------------------------------------------------------------------ Step 2.1 : Setup PubSub Connection "Options" for AMQP
        #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubAddConnection.c : Step 2.1 : Setup PubSub Connection 'Options' for AMQP segment");
        #endif

        //sleep(1000);
    } // if (MQTT_Enable == UA_FALSE && AMQP_Enable == UA_TRUE)
    else // MQTT_Enable = UA_FALSE, AMQP_Enable = UA_FALSE
    {
	// ------------------------------------------------------------------ Step 3.1 : Setup PubSub Connection "Options" for UADP
        #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubAddConnection.c : Step 3.1 : Setup PubSub Connection 'Options' for UADP segment");
        #endif
        // the command line options does not cater for this; so we need to hardcode networkAddressUrl = "opc.udp://224.0.0.22:4840/")

        //UA_ServerConfig *config = UA_Server_getConfig(uaServer);
        /* It is possible to use multiple PubSubTransportLayers on runtime. The correct factory
        * is selected on runtime by the standard defined PubSub TransportProfileUri's. */

        /* defunct: v1.2.1 */ //config->pubsubTransportLayers = (UA_PubSubTransportLayer *) UA_malloc(sizeof(UA_PubSubTransportLayer));
        /* defunct: v1.2.1 */ //config->pubsubTransportLayers[0] = UA_PubSubTransportLayerUDPMP();
        /* defunct v1.2.1 :*/ //config->pubsubTransportLayersSize++;
        /* v1.2.2 */ //UA_ServerConfig_addPubSubTransportLayer(config, UA_PubSubTransportLayerUDPMP());

        /* Create a new ConnectionConfig. The addPubSubConnection function takes the
        * config and create a new connection. The Connection identifier is
        * copied to the NodeId parameter.*/

    connectionConfig.name = UA_STRING("UDP-UADP Connection 1");
    connectionConfig.enabled = UA_TRUE;
    connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    /* The address and interface is part of the standard
     * defined UA_NetworkAddressUrlDataType. */
    UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};
    UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
    connectionConfig.publisherId.uint16 = PUBLISHERID;

        /*7*/
        /*UA_Variant_setScalar(&connectionConfig.connectionTransportSettings,
              config->pubsubTransportLayers, &UA_TYPES[UA_TYPES_DATAGRAMCONNECTIONTRANSPORTDATATYPE]);
        */
        size_t connectionOptionsIndex = 0;
	int connectionOptionsCount = 3;		// UDP only need to setup 3 connection options

    // refer to github/open62541/examples/pubsub/tutorial_pubsub_connection.c
        UA_KeyValuePair connectionProperties[connectionOptionsCount];

        size_t namespaceIndex;
        UA_Server_getNamespaceByName(uaServer, UA_STRING("virtualskies.com.sg/MKS/"), &namespaceIndex);

        connectionProperties[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, "ttl"); //CONNECTION_NAME_UADP); //"ttl");
        UA_UInt16 ttl = 10;
        UA_Variant_setScalar(&connectionProperties[connectionOptionsIndex].value, &ttl, &UA_TYPES[UA_TYPES_UINT16]);
        connectionOptionsIndex++;

        connectionProperties[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, "loopback");
        UA_Boolean loopback = UA_FALSE;
        UA_Variant_setScalar(&connectionProperties[connectionOptionsIndex].value, &loopback, &UA_TYPES[UA_TYPES_BOOLEAN]);
        connectionOptionsIndex++;

        connectionProperties[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, "reuse");
        UA_Boolean reuse = UA_TRUE;
        UA_Variant_setScalar(&connectionProperties[connectionOptionsIndex].value, &reuse, &UA_TYPES[UA_TYPES_BOOLEAN]);
        connectionOptionsIndex++;

        connectionConfig.connectionProperties.map = connectionProperties;
        connectionConfig.connectionProperties.mapSize = connectionOptionsIndex;

        /*
        for (int index=0; index < connectionConfig.connectionPropertiesSize; index++)
        {
                printf("assigning connectionProperties KeyValuePair \n");
                connectionConfig.connectionProperties = UA_KeyValuePair_new();  // UA_KeyValuePair *UA_KeyValuePair_new(void)
                UA_KeyValuePair_copy(&connectionProperties[index], &connectionConfig.connectionProperties[index]);
        }
        */
        //UA_NodeId connectionIdentifier = UA_NODEID_STRING_ALLOC(namespaceIndex, "ConnectionIdentifier");

        #ifdef DEBUG_MODE
		//int status = print_debug_UDP(&connectionConfig, networkAddressUrl);
        	//sleep(status);
        #endif

        // initiatise the value of PubSubconnectionIdentifier (UA_NodeId) so that we can check later in addSubscription()
        #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubAddConnection.c : Before calling UA_Server_addPubSubConnection() :FF: MQTT_Enable == UA_FALSE && AMQP_Enable == UA_FALSE");
        #endif

        UA_StatusCode retval = UA_Server_addPubSubConnection(uaServer, &connectionConfig, &PubSubconnectionIdentifier);
        if (retval == UA_STATUSCODE_GOOD)
        {
                UA_String output;
                UA_String_init(&output);

                #ifdef DEBUG_MODE
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_PubSubAddConnection.c : The output of UA_Server_addPubSubConnection is a NodeId : PubSubconnectionIdentifier .. check isNull= %d", UA_NodeId_isNull(&PubSubconnectionIdentifier));
                #endif

                retval = UA_NodeId_print(&PubSubconnectionIdentifier, &output);

                #ifdef DEBUG_MODE
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubAddConnection.c : NodeId : <%s>", output.data);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_PubSubAddConnection.c : PubSubconnectionIdentifier success %s", UA_StatusCode_name(retval));
                #endif
        }
        else
        {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_PubSubAddConnection.c : PubSubconnectionIdentifier failure %s", UA_StatusCode_name(retval));
                exit(EXIT_FAILURE);
        }

    } // MQTT_Enable = UA_FALSE, AMQP_Enable = UA_FALSE

    //printf("networkAddressUrl.url                                             : %s \n", networkAddressUrl->url.data);

// this statement appears in tutorial_pubsub_subscribe.c but not in tutorial_pubsub_mqtt_publish.c or server_pubsub_subscribe_custom_monitoring.c
// !!! addPubSubConnection : PubSubconnectionIdentifier registration failure BadArgumentsMissing !!!
/*    retval |= UA_PubSubConnection_regist(uaServer, &PubSubconnectionIdentifier);
    if (retval != UA_STATUSCODE_GOOD)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"addPubSubConnection : PubSubconnectionIdentifier registration failure %s", UA_StatusCode_name(retval));
        exit(EXIT_FAILURE);
    }
*/
}

int print_debug_UDP(UA_PubSubConnectionConfig *connectionConfig, UA_NetworkAddressUrlDataType *networkAddressUrl)
{
        // to extract value from a VARIANT
        //UA_Int16 raw_data = *(UA_Int16 *)varStrNonAlarms->data;

        #ifdef DEBUG_MODE
	printf("=====================================================================\n");
        printf("in print_debug_UDP segment \n");
        printf("connectionConfig.name                                           : %s \n", connectionConfig->name.data);
        printf("connectionConfig.enabled                                        : %d \n", connectionConfig->enabled);
        printf("connectionConfig.PublisherIdType                                : %d \n", connectionConfig->publisherIdType);
        printf("connectionConfig.PublisherId                                    : %d \n", connectionConfig->publisherId.uint16);
        printf("connectionConfig.transportProfileUri                            : %s \n", connectionConfig->transportProfileUri.data);
        printf("connectionConfig.address                                        : %s \n", (char *)connectionConfig->address.data);
        //printf("connectionConfig.connectionTransportSettings                  : %d \n", *(UA_Int16 *)connectionConfig.connectionTransportSettings.data); // variant
        //printf("connectionConfig.connectionPropertiesSize                     : %d \n", connectionConfig.connectionPropertiesSize);
        printf("---------------------------------------------------------------------\n");
        printf("connectionConfig.connectionProperties.map[0].key   (string)     : %s \n", connectionConfig->connectionProperties.map[0].key.name.data); // [0].key.name is of type UA_STRING
        //printf("connectionConfig.connectionProperties[0].value (int16)                : %d \n", *(UA_Int16 *)connectionConfig.connectionProperties[0].value.data);  // [0].value is a variant;
        printf("connectionConfig.connectionProperties.map[1].key   (string)     : %s \n", connectionConfig->connectionProperties.map[1].key.name.data);
        //printf("connectionConfig.connectionProperties[1].value (boolean)      : %d \n", *(UA_Boolean *)connectionConfig.connectionProperties[1].value.data);
        printf("connectionConfig.connectionProperties.map[2].key   (string)     : %s \n", connectionConfig->connectionProperties.map[2].key.name.data);
        //printf("connectionConfig.connectionProperties[2].value (boolean)      : %d \n", *(UA_Boolean *)connectionConfig.connectionProperties[2].value.data);
        printf("---------------------------------------------------------------------\n");
        printf("networkAddressUrl.networkInterface                              : %s \n", networkAddressUrl->networkInterface.data);
        printf("networkAddressUrl.url                                           : %s \n", networkAddressUrl->url.data);
        printf("=====================================================================\n");
        #endif
        //sleep(5);

	return 0;

}

int print_debug_MQTT(UA_PubSubConnectionConfig *connectionConfig, UA_NetworkAddressUrlDataType *networkAddressUrl)
{
        // to extract value from a VARIANT
        //UA_Int16 raw_data = *(UA_Int16 *)varStrNonAlarms->data;

        #ifdef DEBUG_MODE
        printf("=====================================================================\n");
        printf("in print_debug_MQTT segment \n");
        printf("connectionConfig.name                                           : %s \n", connectionConfig->name.data);
        printf("connectionConfig.enabled                                        : %d \n", connectionConfig->enabled);
        printf("connectionConfig.PublisherIdType                                : %d \n", connectionConfig->publisherIdType);
        printf("connectionConfig.PublisherId                                    : %d \n", connectionConfig->publisherId.uint16);
        printf("connectionConfig.transportProfileUri                            : %s \n", connectionConfig->transportProfileUri.data);
        printf("connectionConfig.address                                        : %s \n", (char*)connectionConfig->address.data);
        //printf("connectionConfig.connectionTransportSettings                  : %d \n", *(UA_Int16 *)connectionConfig.connectionTransportSettings.data); // variant
        //printf("connectionConfig.connectionPropertiesSize                       : %d \n", connectionConfig.connectionPropertiesSize);
        printf("---------------------------------------------------------------------\n");
        printf("connectionConfig.connectionProperties.map[0].key : mqttClientId(string)   : %s \n", connectionConfig->connectionProperties.map[0].key.name.data); // [0].key.name is of t$
        //printf("connectionConfig.connectionProperties[0].value (int16)                : %d \n", *(UA_Int16 *)connectionConfig.connectionProperties[0].value.data);  // [$
        printf("connectionConfig.connectionProperties.map[1].key : mqttUsername(string)   : %s \n", connectionConfig->connectionProperties.map[1].key.name.data);
        //printf("connectionConfig.connectionProperties[1].value (boolean)      : %d \n", *(UA_Boolean *)connectionConfig.connectionProperties[1].value.data);
        printf("connectionConfig.connectionProperties.map[2].key : mqttPassword(string)   : %s \n", connectionConfig->connectionProperties.map[2].key.name.data);
        //printf("connectionConfig.connectionProperties[2].value (boolean)      : %d \n", *(UA_Boolean *)connectionConfig.connectionProperties[2].value.data);
        printf("connectionConfig.connectionProperties.map[3].key : sendBufferSize(string) : %s \n", connectionConfig->connectionProperties.map[3].key.name.data);
        //printf("connectionConfig.connectionProperties[3].value (UInt32)      : %d \n", *(UA_Boolean *)connectionConfig.connectionProperties[3].value.data);
        printf("connectionConfig.connectionProperties.map[4].key : recvBufferSize(string) : %s \n", connectionConfig->connectionProperties.map[4].key.name.data);
        //printf("connectionConfig.connectionProperties[4].value (UInt32)      : %d \n", *(UA_Boolean *)connectionConfig.connectionProperties[4].value.data);
        if (MQTT_TLS_Enable == UA_TRUE)
	{
                printf("connectionConfig.connectionProperties.map[5].key : mqttUseTLS(string)         : %s \n", connectionConfig->connectionProperties.map[5].key.name.data);
                //printf("connectionConfig.connectionProperties[5].value (UInt32)      : %d \n", *(UA_Boolean *)connectionConfig.connectionProperties[5].value.data);
                printf("connectionConfig.connectionProperties.map[6].key : mqttCaPath(string)         : %s \n", connectionConfig->connectionProperties.map[6].key.name.data);
                //printf("connectionConfig.connectionProperties[6].value (boolean)      : %d \n", *(UA_Boolean *)connectionConfig.connectionProperties[6].value.data);
                printf("connectionConfig.connectionProperties.map[7].key : mqttCaFilePath(string)     : %s \n", connectionConfig->connectionProperties.map[7].key.name.data);
                //printf("connectionConfig.connectionProperties[7].value (string)      : %d \n", *(UA_Boolean *)connectionConfig.connectionProperties[7].value.data);
                printf("connectionConfig.connectionProperties.map[8].key : mqttClientCertPath(string) : %s \n", connectionConfig->connectionProperties.map[8].key.name.data);
                //printf("connectionConfig.connectionProperties[8].value (UInt32)      : %d \n", *(UA_Boolean *)connectionConfig.connectionProperties[8].value.data);
                printf("connectionConfig.connectionProperties.map[9].key : mqttClientKeyPath(string)  : %s \n", connectionConfig->connectionProperties.map[9].key.name.data);
                //printf("connectionConfig.connectionProperties[9].value (UInt32)      : %d \n", *(UA_Boolean *)connectionConfig.connectionProperties[9].value.data);
        }
        printf("---------------------------------------------------------------------\n");
        printf("networkAddressUrl.networkInterface                              : %s \n", networkAddressUrl->networkInterface.data);
        printf("networkAddressUrl.url                                           : %s \n", networkAddressUrl->url.data);
        printf("=====================================================================\n");
        #endif

	return 0;

}
