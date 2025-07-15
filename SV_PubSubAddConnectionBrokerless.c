#define DEBUG_MODE

#ifdef JSON5
#include "json5.h"
#endif

#ifdef no_almagamation
#include <open62541/types_generated.h>
//#include <plugins/ua_network_pubsub_mqtt.h>    // contain UA_PubSubTransportLayerMQTT() header; implementation in plugins/ua_network_pubsub_mqtt.c
//#include <open62541/plugin/pubsub_udp.h>
//#include <open62541/plugin/pubsub_ethernet.h>
//#include <open62541/plugin/securitypolicy_default.h>
//#include <open62541/plugin/pubsub.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/server_pubsub.h>
//#include <pubsub/ua_pubsub.h> // in ~/open62541/src/pubsub/ua_pubsub.h :  contain the following struct
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
#endif
#include "SV_PubSub.h"
#include <stdio.h>

#ifdef NOT_REQUIRED_FOR_BROKERLESS
#ifdef UA_ENABLE_JSON_ENCODING
 static UA_Boolean useJson = UA_TRUE;
#else
 static UA_Boolean useJson = UA_FALSE;
#endif
#endif

extern UA_Boolean MQTT_Enable;
extern UA_Boolean MQTT_TLS_Enable;
extern UA_Boolean AMQP_Enable;
extern int MQTT_Port;   // default set to 1883

// only 1 PubSubConnection for both reader and writer
extern UA_NodeId PubSubconnectionIdentifier;


void
pubSubAddConnectionBrokerless(UA_Server *uaServer, UA_String *transportProfile,
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
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubAddConnectionBrokerless.c : Entering pubSubAddConnectionBrokerless()");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubAddConnectionBrokerless.c :  NetworkAddressUrl.networkInterface passed in : <%s>", networkAddressUrl->networkInterface.data);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubAddConnectionBrokerless.c :  NetworkAddressUrl.url passed in : <%s>", networkAddressUrl->url.data);
    #endif

//-------------------------------------------------------------------- Step 1 : Set up PubSub Connection Configuration
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubAddConnectionBrokerless.c : Step 1 : Set up PubSub Connection Configuration based on transportProfile : %s", transportProfile->data);

    UA_PubSubConnectionConfig connectionConfig;
    memset(&connectionConfig, 0, sizeof(connectionConfig));

    /*1*/connectionConfig.name = UA_STRING(CONNECTION_CONFIGNAME);      // UA_STRING("Publisher Connection")
    /*2*/connectionConfig.enabled = UA_TRUE;
    /*3*/connectionConfig.publisherIdType = UA_PUBLISHERIDTYPE_UINT16;
    /*4*/connectionConfig.publisherId.uint32 = PUBLISHERID;     // Changed to static publisherId from random generation to identify the publisher on subscriber side
    /*5*/connectionConfig.transportProfileUri = *transportProfile;
    /*6*/UA_Variant_setScalar(&connectionConfig.address, networkAddressUrl, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);

//    if(useJson)
//        connectionConfig.transportProfileUri = UA_STRING(TRANSPORT_PROFILE_URI_JSON);
//    else
        connectionConfig.transportProfileUri = UA_STRING(TRANSPORT_PROFILE_URI_UDP);

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

// *************************************************************
// only this point onwards is relevant for brokerless connection
// *************************************************************


	// ------------------------------------------------------------------ Step 3.1 : Setup PubSub Connection "Options" for UADP
        #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubAddConnectionBrokerless.c : Step 3.1 : Setup PubSub Connection 'Options' for UADP segment");
        #endif
        // the command line options does not cater for this; so we need to hardcode networkAddressUrl = "opc.udp://224.0.0.22:4840/")

        //UA_ServerConfig *config = UA_Server_getConfig(uaServer);
        /* It is possible to use multiple PubSubTransportLayers on runtime. The correct factory
        * is selected on runtime by the standard defined PubSub TransportProfileUri's. */

        /* Create a new ConnectionConfig. The addPubSubConnection function takes the
        * config and create a new connection. The Connection identifier is
        * copied to the NodeId parameter.*/

    	connectionConfig.name = UA_STRING("UDP-UADP Connection 1");
    	connectionConfig.enabled = UA_TRUE;
    	connectionConfig.transportProfileUri = UA_STRING("http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp");
    /* The address and interface is part of the standard
     * defined UA_NetworkAddressUrlDataType. */
	/*
        const char *env_SVRport = getenv("SVR_PORT");
        char new_URL[100] = "opc.udp://224.0.0.22:";
	strncpy(&new_URL[21], env_SVRport, strlen(env_SVRport));
	new_URL[strlen(new_URL)] = '/';

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubAddConnectionBrokerless.c : Step 3.1 : Assembled URL is <%s>", new_URL);
      //  UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL , UA_STRING("opc.udp://224.0.0.22:4840/")};
        UA_NetworkAddressUrlDataType networkAddressUrl = {UA_STRING_NULL , UA_STRING(new_URL)};
        UA_Variant_setScalar(&connectionConfig.address, &networkAddressUrl, &UA_TYPES[UA_TYPES_NETWORKADDRESSURLDATATYPE]);
	*/
        connectionConfig.publisherId.uint32 = PUBLISHERID;
	//connectionConfig.publisherId.idType = UA_PUBLISHERIDTYPE_UINT32;

        /*7*/
        /*UA_Variant_setScalar(&connectionConfig.connectionTransportSettings,
              config->pubsubTransportLayers, &UA_TYPES[UA_TYPES_DATAGRAMCONNECTIONTRANSPORTDATATYPE]);
        */
        size_t connectionOptionsIndex = 0;
	int connectionOptionsCount = 3;		// UDP only need to setup 3 connection options
        UA_KeyValuePair connectionProperties[connectionOptionsCount];

        size_t namespaceIndex;
        UA_Server_getNamespaceByName(uaServer, UA_STRING("virtualskies.com.sg/MKS/"), &namespaceIndex);

        connectionProperties[connectionOptionsIndex].key = UA_QUALIFIEDNAME(namespaceIndex, "ttl"); //CONNECTION_NAME_UADP); //"ttl");
        UA_UInt16 ttl = 10;
        UA_Variant_setScalar(&connectionProperties[connectionOptionsIndex].value, &ttl, &UA_TYPES[UA_TYPES_UINT32]);
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
        connectionConfig.connectionProperties.mapSize = connectionOptionsCount;

        // initiatise the value of PubSubconnectionIdentifier (UA_NodeId) so that we can check later in addSubscription()
        #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubAddConnectionBrokerless.c : Step 3.2 Before calling UA_Server_addPubSubConnection()");
        #endif

        UA_StatusCode retval = UA_Server_addPubSubConnection(uaServer, &connectionConfig, &PubSubconnectionIdentifier);
        if (retval == UA_STATUSCODE_GOOD)
        {
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubAddConnectionBrokerless.c : The PubSub Connection is created successfully");

                UA_String output;
                UA_String_init(&output);

                #ifdef DEBUG_MODE
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
		"--------SV_PubSubAddConnectionBrokerless.c : The output of UA_Server_addPubSubConnection is a NodeId : PubSubconnectionIdentifier .. check isNull= %d", UA_NodeId_isNull(&PubSubconnectionIdentifier));
                #endif

                retval = UA_NodeId_print(&PubSubconnectionIdentifier, &output);

                #ifdef DEBUG_MODE
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_PubSubAddConnectionBrokerless.c : NodeId : <%s>", output.data);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_PubSubAddConnectionBrokerless.c : UA_Server_addPubSubConnection() success : %s", UA_StatusCode_name(retval));
                #endif
        }
        else
        {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_PubSubAddConnectionBrokerless.c : UA_Server_addPubSubConnection() failure : %s", UA_StatusCode_name(retval));
                exit(EXIT_FAILURE);
        }

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

