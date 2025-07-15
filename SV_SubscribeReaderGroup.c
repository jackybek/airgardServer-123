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

#ifdef UA_ENABLE_JSON_ENCODING
 static UA_Boolean useJson = UA_TRUE;
#else
 static UA_Boolean useJson = UA_FALSE;
#endif

extern UA_Byte signingKey[UA_AES128CTR_SIGNING_KEY_LENGTH];
extern UA_Byte encryptingKey[UA_AES128CTR_KEY_LENGTH];
extern UA_Byte keyNonce[UA_AES128CTR_KEYNONCE_LENGTH];


// only 1 PubSubConnection for both reader and writer
extern UA_NodeId PubSubconnectionIdentifier;

// Subscriber section variables & functions
extern UA_DataSetReaderConfig dataSetReaderConfig;
extern UA_NodeId readerGroupIdentifier;
extern UA_NodeId readerIdentifier;
extern UA_NodeId reader2Id;

UA_StatusCode
subReaderGroup(UA_Server *uaServer)
{
    if (uaServer == NULL) {
        return UA_STATUSCODE_BADINTERNALERROR;
    }

    #ifdef DEBUG_MODE
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addReaderGroup()");
    #endif

    UA_ReaderGroupConfig readerGroupConfig;
    memset(&readerGroupConfig, 0, sizeof(UA_ReaderGroupConfig));
/*
typedef struct {
    UA_String name;
    UA_PubSubSecurityParameters securityParameters;
    // PubSub Manager Callback
    UA_PubSub_CallbackLifecycle pubsubManagerCallback;
    // non std. field
    UA_Duration subscribingInterval; // Callback interval for subscriber: set the least publishingInterval value of all DSRs in this RG
    UA_Boolean enableBlockingSocket; // To enable or disable blocking socket option
    UA_UInt32 timeout; // Timeout for receive to wait for the packets
    UA_PubSubRTLevel rtLevel;
} UA_ReaderGroupConfig;
*/
    readerGroupConfig.name = UA_STRING("Airgard ReaderGroup");
    //readerGroupConfig.securityParameters = ???
    //readerGroupConfig.pubSubManagerCallback = ???

    // removed from v.1.2.1
    //readerGroupConfig.subscribingInterval = 4500;     // milliseconds
    //readerGroupConfig.enableBlockingSocket = UA_FALSE;
    //readerGroupConfig.timeout = 30;   // 30 seconds

    readerGroupConfig.rtLevel = UA_PUBSUB_RT_NONE;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* decide whether to use JSON or UADP encoding */
#ifdef UA_ENABLE_JSON_ENCODING
    if(useJson)
    {
        #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addedReaderGroup : useJson = UA_TRUE");
        #endif
        //readerGroupConfig.name = ??
        //readerGroupConfig.securityParameters = ??
        //readerGroupConfig.pubsubManagerCallback = ??
        //readerGroupConfig.subscribingInterval = ???
        //readerGroupConfig.enableBlockingSocket = UA_FALSE;
        //readerGroupConfig.timeout = 30    // 30 seconds
        //readerGroupConfig.rtLevel = UA_PUBSUB_RT_NONE;
     }
     else
#endif
    {
        #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addedReaderGroup : useJson = UA_FALSE");
        #endif

        retval = UA_Server_addReaderGroup(uaServer, PubSubconnectionIdentifier, &readerGroupConfig, &readerGroupIdentifier);
        //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c : addReaderGroup %s", UA_StatusCode_name(retval));

        /* this following caused segmentation fault
        retval = UA_Server_setReaderGroupOperational(uaServer, readerGroupIdentifier);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c : addReaderGroupOperational %s", UA_StatusCode_name(retval));
        */
    }
/*
    if (MQTT_Enable)
    {
        printf("addReaderGroup : MQTT_Enable = %d \n", MQTT_Enable);
        // configure the mqtt subscribe topic
        UA_BrokerReaderGroupTransportDataType brokerTransportSettings;
        memset(&brokerTransportSettings, 0, sizeof(UA_BrokerReaderGroupTransportDataType));
        // Assign the Topic at which MQTT subscribe should happen
        brokerTransportSettings.queueName = UA_STRING(SUBSCRIBER_TOPIC_MQTT);
        brokerTransportSettings.resourceUri = UA_STRING_NULL;
        brokerTransportSettings.authenticationProfileUri = UA_STRING_NULL;

       // Choose the QOS Level for MQTT
        brokerTransportSettings.requestedDeliveryGuarantee = UA_BROKERTRANSPORTQUALITYOFSERVICE_BESTEFFORT;

        // Encapsulate config in transportSettings
        UA_ExtensionObject transportSettings;
        memset(&transportSettings, 0, sizeof(UA_ExtensionObject));
        transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        transportSettings.content.decoded.type = &UA_TYPES[UA_TYPES_BROKERREADERGROUPTRANSPORTDATATYPE];
        transportSettings.content.decoded.data = &brokerTransportSettings;

        readerGroupConfig.transportSettings = transportSettings;
    }
    retval = UA_Server_addReaderGroup(uaServer, PubSubconnectionIdentifier, &readerGroupConfig, &readerGroupIdentifier);
*/
    if (retval == UA_STATUSCODE_GOOD)
    {
        #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : UA_Server_addReaderGroup : success");
        #endif

        /* Add the encryption key informaton */
        UA_ByteString sk = {UA_AES128CTR_SIGNING_KEY_LENGTH, signingKey};
        UA_ByteString ek = {UA_AES128CTR_KEY_LENGTH, encryptingKey};
        UA_ByteString kn = {UA_AES128CTR_KEYNONCE_LENGTH, keyNonce};

        // TODO security token not necessary for readergroup (extracted from security-header)
        UA_Server_setReaderGroupEncryptionKeys(uaServer, readerGroupIdentifier, 1, sk, ek, kn);

        UA_Server_setReaderGroupOperational(uaServer, readerGroupIdentifier);

        #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c : addReaderGroup : success %s", UA_StatusCode_name(retval));
        #endif
	return retval;
    }
    else
    {
        #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c : addReaderGroup : failure %s", UA_StatusCode_name(retval));
        #endif
        exit(EXIT_FAILURE);
    }
}
