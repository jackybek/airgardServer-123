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

// publisher section : variables & functions
static UA_NodeId publishedDataSetIdentifier;


/**
 * **PublishedDataSet handling**
 * The PublishedDataSet (PDS) and PubSubConnection are the toplevel entities and
 * can exist alone. The PDS contains the collection of the published fields. All
 * other PubSub elements are directly or indirectly linked with the PDS or * connection.
 */
void
pubSendDataSet(UA_Server *uaServer) {
    /* The PublishedDataSetConfig contains all necessary public
    * informations for the creation of a new PublishedDataSet */

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "==========================================================");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "----------SV_PublishSendDataSet.c ");

    UA_PublishedDataSetConfig publishedDataSetConfig;
    memset(&publishedDataSetConfig, 0, sizeof(UA_PublishedDataSetConfig));
    publishedDataSetConfig.publishedDataSetType = UA_PUBSUB_DATASET_PUBLISHEDITEMS;
    publishedDataSetConfig.name = UA_STRING("Airgard PDS");

    /* Create new PublishedDataSet based on the PublishedDataSetConfig. */
    UA_Server_addPublishedDataSet(uaServer, &publishedDataSetConfig, &publishedDataSetIdentifier);

   // added by Jacky on 4/4/2021 to update MQTT payload (MetaDataVersion)
   // the corresponding change has to take place in open62541.c : UA_Server_addDataSetField() however this change cause publisher to have error
        //UA_open62541/src/pubsub/ua_pubsub_writer.c
        //At line 691
/*   UA_PublishedDataSet *currentDataSet = UA_PublishedDataSet_findPDSbyId(uaServer, publishedDataSetIdentifier);
   currentDataSet->dataSetMetaData.configurationVersion.majorVersion = MAJOR_SOFTWARE_VERSION;
   currentDataSet->dataSetMetaData.configurationVersion.minorVersion = MINOR_SOFTWARE_VERSION;
*/
}
