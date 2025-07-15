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

#include "SV_JsonChecker.h"
#include <json-c/json.h>        // https://json-c.github.io/json-c; install in /usr/local/include/
//#include "json.h"             //https://raw.githubusercontent.com/udp/json-parser/master/json.h
//#include "ua_pubsub_amqp.h"   //https://github.com/open62541/open62541/pull/3850
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#define PUBLISHERID                     2234    // used in addDataSetReader() and addPubSubConnection()
#define WRITERGROUPID                   100     // used in addDataSetReader() and addWriterGroup()
#define DATASETWRITERID                 2234    // used in addDataSetReader() and addDataSetWriter()

#ifdef UA_ENABLE_PUBSUB_MQTT
 #define MESSAGE_TIMEOUT_MQTT           12000
 #define SUBSCRIBER_METADATAUPDATETIME_MQTT      0
 #define SUBSCRIBER_METADATAQUEUENAME_MQTT       "MetaDataTopic"
 #define SUBSCRIBER_TOPIC_MQTT                   "AirgardTopic"

#endif
#include "SV_PubSub.h"

// Subscriber section variables & functions
extern UA_DataSetReaderConfig dataSetReaderConfig;
extern UA_NodeId readerGroupIdentifier;
extern UA_NodeId readerIdentifier;
extern UA_NodeId reader2Id;
#ifdef UA_ENABLE_JSON_ENCODING
 static UA_Boolean useJson = UA_TRUE;
#else
 static UA_Boolean useJson = UA_FALSE;
#endif

extern UA_Boolean MQTT_Enable;
extern UA_Boolean MQTT_TLS_Enable;
extern UA_Boolean AMQP_Enable;
extern int MQTT_Port;   // default set to 1883


UA_StatusCode subDataSetReader(UA_Server *uaServer)
{
        if (uaServer == NULL)
                return UA_STATUSCODE_BADINTERNALERROR;

        #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addDataSetReader()");
        #endif

        UA_StatusCode retval = UA_STATUSCODE_GOOD;
        //UA_Variant varPublisherId;
        //UA_Int32 publisherId = 1000;

        //UA_Variant_setScalar(&varPublisherId, &publisherId, &UA_TYPES[UA_TYPES_INT32]);

        memset(&dataSetReaderConfig, 0, sizeof(UA_DataSetReaderConfig));
        dataSetReaderConfig.name = UA_STRING("Airgard DataSetReader");
        /* Parameters to filter which DataSetMessage has to be processed
         * by the DataSetReader */
        /* The following parameters are used to show that the data published by
         * tutorial_pubsub_publish.c is being subscribed and is being updated in
         * the information model */
//      dataSetReaderConfig.publisherId.type = &UA_TYPES[UA_TYPES_UINT16];
//      dataSetReaderConfig.publisherId.data = &publisherIdentifier;
        UA_UInt16 publisherIdentifier = PUBLISHERID;
        UA_Variant_setScalar(&dataSetReaderConfig.publisherId, &publisherIdentifier, &UA_TYPES[UA_TYPES_INT16]); // filter away messages based on this parameter : UA_Variant
        dataSetReaderConfig.writerGroupId = WRITERGROUPID;              // filter away messages based on this parameter
        dataSetReaderConfig.dataSetWriterId = DATASETWRITERID;          // filter away messages based on this parameter
//      dataSetReaderConfig.dataSetMetaData = ???               // UA_DataSetFieldContentMask
//      dataSetReaderConfig.dataSetFieldContentMask = ???       // UA_DataSetFieldContentMask
        dataSetReaderConfig.messageReceiveTimeout = (UA_Double) MESSAGE_TIMEOUT_MQTT;   // * Timeout must be greater than publishing interval of corresponding WriterGroup */
//      dataSetReaderConfig.messageSettings = ??                // UA_ExtensionObject
//      dataSetReaderConfig.transportSettings = ??              // UA_ExtensionObject
        dataSetReaderConfig.subscribedDataSetType = UA_PUBSUB_SDS_TARGET;       // UA_SubscribedDataSetEnumType
//      dataSetReaderConfig.linkedStandaloneSubscribedDataSetName = ??          //UA_String
//      dataSetReaderConfig.expectedEncoding = ??               // UA_PubSubRtEncoding


// the following is not in the sample (section X)
//      dataSetReaderConfig.dataSetFieldContentMask = UA_DATASETFIELDCONTENTMASK_RAWDATA;       // UA_DataSetFieldContentMask
//      dataSetReaderConfig.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;       // UA_PubSubSecurityParameters
//      dataSetReaderConfig.securityParameters.securityGroupId = UA_STRING("AirgardSecurityGroupId-1");
        //dataSetReaderConfig.securityParameters.keyServersSize = (size_t)1;    // size_t;
//      dataSetReaderConfig.subscribedDataSet.subscribedDataSetTarget.targetVariablesSize = 2;  // UA_TargetVariables
//      dataSetReaderConfig.subscribedDataSet.subscribedDataSetTarget.targetVariables.externalDataValue = *MQTT data*   // UA_FieldTargetVariable .. UA_DataValue**
// the above is not in the sample

// added by Jacky by comparing the code with addDataSetWriter()
#ifdef UA_ENABLE_JSON_ENCODING
        UA_JsonDataSetReaderMessageDataType jsonDsrMd;
        UA_ExtensionObject messageSettings;
        if(useJson)
        {
                #ifdef DEBUG_MODE
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c :addDataSetReader : useJson = UA_TRUE");
                #endif
                // JSON config for the dataSetReader
                jsonDsrMd.dataSetMessageContentMask = (UA_JsonDataSetMessageContentMask)
                       (UA_JSONDATASETMESSAGECONTENTMASK_DATASETWRITERID |
                        UA_JSONDATASETMESSAGECONTENTMASK_SEQUENCENUMBER |
                        UA_JSONDATASETMESSAGECONTENTMASK_STATUS |
                        UA_JSONDATASETMESSAGECONTENTMASK_METADATAVERSION |
                        UA_JSONDATASETMESSAGECONTENTMASK_TIMESTAMP);

                messageSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
                messageSettings.content.decoded.type = &UA_TYPES[UA_TYPES_JSONDATASETWRITERMESSAGEDATATYPE];
                messageSettings.content.decoded.data = &jsonDsrMd;

                dataSetReaderConfig.messageSettings = messageSettings;
        }
#endif

    if (MQTT_Enable)
    {
        //TODO: Modify MQTT send to add DataSetWriters broker transport settings
        //TODO: Pass the topic as argument from the writer group
        //TODO: Publish Metadata to metaDataQueueName
        // configure the mqtt publish topic
        UA_BrokerDataSetReaderTransportDataType brokerTransportSettings;
        memset(&brokerTransportSettings, 0, sizeof(UA_BrokerDataSetReaderTransportDataType));

        // Assign the Topic at which MQTT subscribe should happen
        brokerTransportSettings.queueName = UA_STRING(SUBSCRIBER_TOPIC_MQTT);
        brokerTransportSettings.resourceUri = UA_STRING_NULL;
        brokerTransportSettings.authenticationProfileUri = UA_STRING_NULL;
        brokerTransportSettings.metaDataQueueName = UA_STRING(SUBSCRIBER_METADATAQUEUENAME_MQTT);

        // Choose the QOS Level for MQTT
        brokerTransportSettings.requestedDeliveryGuarantee = UA_BROKERTRANSPORTQUALITYOFSERVICE_BESTEFFORT;

        // Encapsultate config in transportSettings
        UA_ExtensionObject transportSettings;
        memset(&transportSettings, 0, sizeof(UA_ExtensionObject));
        transportSettings.encoding = UA_EXTENSIONOBJECT_DECODED;
        transportSettings.content.decoded.type = &UA_TYPES[UA_TYPES_BROKERDATASETREADERTRANSPORTDATATYPE];
        transportSettings.content.decoded.data = &brokerTransportSettings;

        dataSetReaderConfig.transportSettings = transportSettings;
    }
    // end added by Jacky by comparing the code with addDataSetWriter()

        subReceiveDataSet(&dataSetReaderConfig.dataSetMetaData);
        // the following caused segmentation fault if section X is not configured properly
        retval = UA_Server_addDataSetReader(uaServer, readerGroupIdentifier, &dataSetReaderConfig, &readerIdentifier);


        // Create a second reader for the same writer in the same readergroup - future use.. currently cannot think of use case
        UA_NodeId reader2Id;
        retval = UA_Server_addDataSetReader(uaServer, readerGroupIdentifier, &dataSetReaderConfig, &reader2Id);

        return retval;
}// end addDataSetReader()
