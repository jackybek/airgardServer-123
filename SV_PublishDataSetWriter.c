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

// publisher section : variables & functions
static UA_NodeId publishedDataSetIdentifier;
static UA_NodeId writerGroupIdentifier;


void
pubDataSetWriter(UA_Server *uaServer) {

    // We need now a DataSetWriter within the WriterGroup. This means we must
    // create a new DataSetWriterConfig and add call the addWriterGroup function.
    UA_NodeId dataSetWriterIdentifier;
    UA_DataSetWriterConfig dataSetWriterConfig;
    memset(&dataSetWriterConfig, 0, sizeof(UA_DataSetWriterConfig));
    dataSetWriterConfig.name = UA_STRING("Airgard DataSetWriter");
    dataSetWriterConfig.dataSetWriterId = DATASETWRITERID;
    dataSetWriterConfig.dataSetName = UA_STRING("Airgard Dataset");
    dataSetWriterConfig.keyFrameCount = 10;

        // refer to https://github.com/open62541/open62541/blob/master/tests/pubsub/check_pubsub_encoding_json.c
        // enable metaDataVersion in UA_DataSetMessageHeader
        /*
        m.payload.dataSetPayload.dataSetMessages[0].header.configVersionMajorVersionEnabled = UA_TRUE;
        m.payload.dataSetPayload.dataSetMessages[0].header.configVersionMinorVersionEnabled = UA_TRUE;
        m.payload.dataSetPayload.dataSetMessages[0].header.configVersionMajorVersion = 42;
        m.payload.dataSetPayload.dataSetMessages[0].header.configVersionMinorVersion = 7;
        */
        /* no effect
    UA_KeyValuePair dataSetWriterProperties[4];
    size_t dataSetWriterPropertiesIndex = 0;
    UA_Boolean IsTrue = UA_TRUE;

    dataSetWriterProperties[dataSetWriterPropertiesIndex].key = UA_QUALIFIEDNAME(namespaceIndex,"configVersionMajorVersionEnabled");
    UA_Variant_setScalar(&dataSetWriterProperties[dataSetWriterPropertiesIndex].value, &IsTrue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    dataSetWriterPropertiesIndex++;

    dataSetWriterProperties[dataSetWriterPropertiesIndex].key = UA_QUALIFIEDNAME(namespaceIndex, "configVersionMinorVersionEnabled");
    UA_Variant_setScalar(&dataSetWriterProperties[dataSetWriterPropertiesIndex].value, &IsTrue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    dataSetWriterPropertiesIndex++;

    UA_Int16 majorVersion = MAJOR_SOFTWARE_VERSION;
    dataSetWriterProperties[dataSetWriterPropertiesIndex].key = UA_QUALIFIEDNAME(namespaceIndex,"configVersionMajorVersion");
    UA_Variant_setScalar(&dataSetWriterProperties[dataSetWriterPropertiesIndex].value, &majorVersion, &UA_TYPES[UA_TYPES_INT16]);
    dataSetWriterPropertiesIndex++;

    UA_Int16 minorVersion = MINOR_SOFTWARE_VERSION;
    dataSetWriterProperties[dataSetWriterPropertiesIndex].key = UA_QUALIFIEDNAME(namespaceIndex,"configVersionMinorVersion");
    UA_Variant_setScalar(&dataSetWriterProperties[dataSetWriterPropertiesIndex].value, &minorVersion, &UA_TYPES[UA_TYPES_INT16]);

    dataSetWriterConfig.dataSetWriterProperties = dataSetWriterProperties;
// line 1002
        */
        /* addDataSetReader Timeout must be greater than publishing interval of corresponding WriterGroup */


#ifdef UA_ENABLE_JSON_ENCODING          // currently set as UA_TRUE in ccmake
    UA_JsonDataSetWriterMessageDataType jsonDswMd;
    UA_ExtensionObject messageSettings;
    if(useJson)
    {
        #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c :addDataSetWriter : useJson = UA_TRUE");
        #endif
        // JSON config for the dataSetWriter
        jsonDswMd.dataSetMessageContentMask = (UA_JsonDataSetMessageContentMask)
            (UA_JSONDATASETMESSAGECONTENTMASK_DATASETWRITERID |
             UA_JSONDATASETMESSAGECONTENTMASK_SEQUENCENUMBER |
             UA_JSONDATASETMESSAGECONTENTMASK_STATUS |
             UA_JSONDATASETMESSAGECONTENTMASK_METADATAVERSION |
             UA_JSONDATASETMESSAGECONTENTMASK_TIMESTAMP);

        messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_JSONDATASETWRITERMESSAGEDATATYPE];
        messageSettings.content.decoded.data = &jsonDswMd;

        dataSetWriterConfig.messageSettings = messageSettings;
    }
#endif

   if (MQTT_Enable)
    {
        //TODO: Modify MQTT send to add DataSetWriters broker transport settings
        //TODO: Pass the topic as argument from the writer group
        //TODO: Publish Metadata to metaDataQueueName
        // configure the mqtt publish topic
        UA_BrokerDataSetWriterTransportDataType brokerTransportSettings;
        memset(&brokerTransportSettings, 0, sizeof(UA_BrokerDataSetWriterTransportDataType));

        // Assign the Topic at which MQTT publish should happen
        brokerTransportSettings.queueName = UA_STRING(PUBLISHER_TOPIC_EDGE2CLOUD_MQTT);
        brokerTransportSettings.resourceUri = UA_STRING_NULL;
        brokerTransportSettings.authenticationProfileUri = UA_STRING_NULL;
        brokerTransportSettings.metaDataQueueName = UA_STRING(PUBLISHER_METADATAQUEUENAME_MQTT);
        brokerTransportSettings.metaDataUpdateTime = PUBLISHER_METADATAUPDATETIME_MQTT;

        // Choose the QOS Level for MQTT
        brokerTransportSettings.requestedDeliveryGuarantee = UA_BROKERTRANSPORTQUALITYOFSERVICE_BESTEFFORT;

        // Encapsulate config in transportSettings
        UA_ExtensionObject transportSettings;
        memset(&transportSettings, 0, sizeof(UA_ExtensionObject));
        transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        transportSettings.content.decoded.type = &UA_TYPES[UA_TYPES_BROKERDATASETWRITERTRANSPORTDATATYPE];
        transportSettings.content.decoded.data = &brokerTransportSettings;

        dataSetWriterConfig.transportSettings = transportSettings;
    }

    UA_Server_addDataSetWriter(uaServer, writerGroupIdentifier, publishedDataSetIdentifier,
                               &dataSetWriterConfig, &dataSetWriterIdentifier);

}
