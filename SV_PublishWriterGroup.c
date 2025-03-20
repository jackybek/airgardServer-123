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

#define WRITERGROUPID                   100     // used in addDataSetReader() and addWriterGroup()

#ifdef UA_ENABLE_PUBSUB_MQTT
 #define MESSAGE_TIMEOUT_MQTT           12000
 #define PUBLISHER_TOPIC_EDGE2CLOUD_MQTT        "AirgardTopicEdgeToCloud"
 #define PUBLISHER_TOPIC_CLOUD2EDGE_MQTT        "AirgardTopicCloudToEdge"

#endif

#ifdef UA_ENABLE_JSON_ENCODING
 static UA_Boolean useJson = UA_TRUE;
#else
 static UA_Boolean useJson = UA_FALSE;
#endif

#define UA_AES128CTR_SIGNING_KEY_LENGTH 16
#define UA_AES128CTR_KEY_LENGTH 16
#define UA_AES128CTR_KEYNONCE_LENGTH 4

UA_Byte signingKey[UA_AES128CTR_SIGNING_KEY_LENGTH]= {0};
UA_Byte encryptingKey[UA_AES128CTR_KEY_LENGTH] = {0};
UA_Byte keyNonce[UA_AES128CTR_KEYNONCE_LENGTH] = {0};

// only 1 PubSubConnection for both reader and writer
extern UA_NodeId PubSubconnectionIdentifier;

// publisher section : variables & functions
static UA_NodeId writerGroupIdentifier;

extern UA_Boolean MQTT_Enable;
extern UA_Boolean MQTT_TLS_Enable;
extern UA_Boolean AMQP_Enable;
extern int MQTT_Port;   // default set to 1883


/**
 * **WriterGroup handling**
 * The WriterGroup (WG) is part of the connection and contains the primary configuration
 * parameters for the message creation.
 */
void
pubWriterGroup(UA_Server *uaServer)
{
    /* Now we create a new WriterGroupConfig and add the group to the existing PubSubConnection. */
    UA_ServerConfig *ua_config = UA_Server_getConfig(uaServer);

    UA_WriterGroupConfig writerGroupConfig;
    memset(&writerGroupConfig, 0, sizeof(UA_WriterGroupConfig));
    writerGroupConfig.name = UA_STRING("Airgard WriterGroup");
        /* addDataSetReader Timeout= must be greater than publishing interval of corresponding WriterGroup */
    writerGroupConfig.publishingInterval = (UA_Double) MESSAGE_TIMEOUT_MQTT;    // set as 12000 millisec
    writerGroupConfig.enabled = UA_FALSE;       // in the example (tutorial_pubsub_mqtt_publish.c)  this is UA_FALSE
    writerGroupConfig.writerGroupId = WRITERGROUPID;
    writerGroupConfig.maxEncapsulatedDataSetMessageCount = 100;
    writerGroupConfig.rtLevel = UA_PUBSUB_RT_NONE;

    UA_StatusCode retval = UA_STATUSCODE_GOOD;

    /* decide whether to use JSON or UADP encoding*/
    if(useJson)
    {
        #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PublishWriterGroup.c :addWriterGroup : useJson = UA_TRUE");
        #endif
        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_JSON;
        writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_JSONWRITERGROUPMESSAGEDATATYPE];
        /* The configuration flags for the messages are encapsulated inside the
         * message- and transport settings extension objects. These extension
         * objects are defined by the standard. e.g.
         * UadpWriterGroupMessageDataType */

        #ifdef KIV
        // Add the encryption key information
        if (MQTT_TLS_Enable == UA_TRUE)
        {
                UA_ServerConfig *config = UA_Server_getConfig(uaServer);
                writerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
                writerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];
        }
        #endif

        UA_JsonWriterGroupMessageDataType *Json_writerGroupMessage = UA_JsonWriterGroupMessageDataType_new();
        /* Change message settings of writerGroup to send PublisherId,
         * DataSetMessageHeader, SingleDataSetMessage and DataSetClassId in PayloadHeader
         * of NetworkMessage */
        Json_writerGroupMessage->networkMessageContentMask =    (UA_JsonNetworkMessageContentMask)(UA_JSONNETWORKMESSAGECONTENTMASK_NETWORKMESSAGEHEADER |
                                                                (UA_JsonNetworkMessageContentMask)UA_JSONNETWORKMESSAGECONTENTMASK_DATASETMESSAGEHEADER |
                                                                (UA_JsonNetworkMessageContentMask)UA_JSONNETWORKMESSAGECONTENTMASK_SINGLEDATASETMESSAGE |
                                                                (UA_JsonNetworkMessageContentMask)UA_JSONNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                                (UA_JsonNetworkMessageContentMask)UA_JSONNETWORKMESSAGECONTENTMASK_DATASETCLASSID);
        writerGroupConfig.messageSettings.content.decoded.data = Json_writerGroupMessage;
        // the following seemed to be defunct : tutorial_pubsub_mqtt_publish.c
        UA_Server_addWriterGroup(uaServer, PubSubconnectionIdentifier, &writerGroupConfig, &writerGroupIdentifier);
        UA_Server_setWriterGroupOperational(uaServer, writerGroupIdentifier);
        UA_JsonWriterGroupMessageDataType_delete(Json_writerGroupMessage);
    }
    else
    {
        #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PublishWriterGroup.c : addWriterGroup : useJson = UA_FALSE");
        #endif

        writerGroupConfig.encodingMimeType = UA_PUBSUB_ENCODING_UADP;
        writerGroupConfig.messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        writerGroupConfig.messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_UADPWRITERGROUPMESSAGEDATATYPE];
        /* The configuration flags for the messages are encapsulated inside the
         * message- and transport settings extension objects. These extension
         * objects are defined by the standard. e.g.
         * UadpWriterGroupMessageDataType */

        #ifdef KIV
        // Add the encryption key information
        if (MQTT_TLS_Enable == UA_TRUE)
        {
                UA_ServerConfig *config = UA_Server_getConfig(uaServer);
                writerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
                writerGroupConfig.securityPolicy = &config->pubSubConfig.securityPolicies[0];
        }
        #endif

        UA_UadpWriterGroupMessageDataType *writerGroupMessage = UA_UadpWriterGroupMessageDataType_new();
        /* Change message settings of writerGroup to send PublisherId,
         * WriterGroupId in GroupHeader and DataSetWriterId in PayloadHeader
         * of NetworkMessage */
        writerGroupMessage->networkMessageContentMask = (UA_UadpNetworkMessageContentMask)(UA_UADPNETWORKMESSAGECONTENTMASK_PUBLISHERID |
                                                        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_GROUPHEADER |
                                                        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_WRITERGROUPID |
                                                        (UA_UadpNetworkMessageContentMask)UA_UADPNETWORKMESSAGECONTENTMASK_PAYLOADHEADER);
        writerGroupConfig.messageSettings.content.decoded.data = writerGroupMessage;
        // the following seemed to be defunct : tutorial_pubsub_mqtt_publish.c
        UA_Server_addWriterGroup(uaServer, PubSubconnectionIdentifier, &writerGroupConfig, &writerGroupIdentifier);
        UA_Server_setWriterGroupOperational(uaServer, writerGroupIdentifier);
        UA_UadpWriterGroupMessageDataType_delete(writerGroupMessage);
     } // if (useJson)

    #ifdef KIV
     // Now Add the encryption key information for UADP - default is ON
     UA_ByteString sk = {UA_AES128CTR_SIGNING_KEY_LENGTH, signingKey};
     UA_ByteString ek = {UA_AES128CTR_KEY_LENGTH, encryptingKey};
     UA_ByteString kn = {UA_AES128CTR_KEYNONCE_LENGTH, keyNonce};

     // Set the group key for the message encryption
     retval = UA_Server_setWriterGroupEncryptionKeys(uaServer, writerGroupIdentifier, 1, sk, ek, kn);        // 1 = securityTokenId
     if (retval!= UA_STATUSCODE_GOOD)
     {
             UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"----------SV_PubWriterGroup.c : addWriterGroup : UA_Server_setWriterGroupEncryptionKeys : failure %s", UA_StatusCode_name(retval));
             sleep(2);
             //exit (EXIT_FAILURE);
     }
     #endif

    // The above is for UDAP; now this is for MQTT
    if (MQTT_Enable == UA_TRUE) // publish to MQTT Broker
    {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PublishWriterGroup.c addWriterGroup : MQTT_Enable = %d \n", MQTT_Enable);
        // configure the mqtt publish topic
        UA_BrokerWriterGroupTransportDataType brokerTransportSettings;
        memset(&brokerTransportSettings, 0, sizeof(UA_BrokerWriterGroupTransportDataType));
        // Assign the Topic at which MQTT publish should happen
        //ToDo: Pass the topic as argument from the writer group
        brokerTransportSettings.queueName = UA_STRING(PUBLISHER_TOPIC_EDGE2CLOUD_MQTT);
        brokerTransportSettings.resourceUri = UA_STRING_NULL;
        brokerTransportSettings.authenticationProfileUri = UA_STRING_NULL;

        // Choose the QOS Level for MQTT
        brokerTransportSettings.requestedDeliveryGuarantee = UA_BROKERTRANSPORTQUALITYOFSERVICE_BESTEFFORT;

        // Encapsulate config in transportSettings
        UA_ExtensionObject transportSettings;
        memset(&transportSettings, 0, sizeof(UA_ExtensionObject));
        transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        transportSettings.content.decoded.type = &UA_TYPES[UA_TYPES_BROKERWRITERGROUPTRANSPORTDATATYPE];
        transportSettings.content.decoded.data = &brokerTransportSettings;

        writerGroupConfig.transportSettings = transportSettings;
        //UA_Server_addWriterGroup(uaServer, PubSubconnectionIdentifier, &writerGroupConfig, &writerGroupIdentifier);
        //UA_Server_setWriterGroupOperational(uaServer, writerGroupIdentifier);

        #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PublishWriterGroup.c :addWriterGroup : MQTT_TLS_Enable = %d", MQTT_TLS_Enable);
        #endif
        if (MQTT_TLS_Enable == UA_TRUE) //port 888x
        {
           if (!ua_config->pubSubConfig.securityPolicies)
           {
                ua_config->pubSubConfig.securityPolicies = (UA_PubSubSecurityPolicy *) UA_malloc(sizeof(UA_PubSubSecurityPolicy));
                ua_config->pubSubConfig.securityPoliciesSize = 1;
                UA_PubSubSecurityPolicy_Aes128Ctr(ua_config->pubSubConfig.securityPolicies, ua_config->logging);
           }
           //assert(ua_config->pubSubConfig.securityPolicies);

           // Encryption settings
           // Message are encrypted if a SecurityPolicy is configured and the
           // securityMode set accordingly. The symmetric key is a runtime information
           // and has to be set via UA_Server_setWriterGroupEncryptionKey. */
           writerGroupConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;
           writerGroupConfig.securityPolicy = &ua_config->pubSubConfig.securityPolicies[0];

           UA_Server_addWriterGroup(uaServer, PubSubconnectionIdentifier, &writerGroupConfig, &writerGroupIdentifier);
           UA_Server_setWriterGroupOperational(uaServer, writerGroupIdentifier);

                // Now Add the encryption key information for UADP - default is ON
                UA_ByteString sk = {UA_AES128CTR_SIGNING_KEY_LENGTH, signingKey};
                UA_ByteString ek = {UA_AES128CTR_KEY_LENGTH, encryptingKey};
                UA_ByteString kn = {UA_AES128CTR_KEYNONCE_LENGTH, keyNonce};

                // Set the group key for the message encryption
                retval = UA_Server_setWriterGroupEncryptionKeys(uaServer, writerGroupIdentifier, 1, sk, ek, kn);        // 1 = securityTokenId
                if (retval!= UA_STATUSCODE_GOOD)
                {
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"----------SV_PublishWriterGroup.c : addWriterGroup (with TLS) : UA_Server_setWriterGroupEncryptionKeys : failure %s", UA_StatusCode_name(retval));
                        sleep(2);
                        //exit (EXIT_FAILURE);
                }
        }
        else
        {
           UA_Server_addWriterGroup(uaServer, PubSubconnectionIdentifier, &writerGroupConfig, &writerGroupIdentifier);
           UA_Server_setWriterGroupOperational(uaServer, writerGroupIdentifier);
        }
     }
     else // MQTT_Enable == UA_FALSE
     {
        #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PublishWriterGroup.c : addWriterGroup : MQTT_Enable = %d", MQTT_Enable);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PublishWriterGroup.c : addWriterGroup : MQTT_TLS_Enable = %d", MQTT_TLS_Enable);
        // do nothing
        #endif
     }
}
