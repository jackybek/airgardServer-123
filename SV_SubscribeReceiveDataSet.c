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

void subReceiveDataSet(UA_DataSetMetaDataType *pMetaData)
{

        UA_Variant varData;
        UA_Float data = 500.4;
        UA_Variant_setScalar(&varData,&data, &UA_TYPES[UA_TYPES_FLOAT]);

        #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : fillDataSetMetaData");
        #endif
        if (pMetaData == NULL)
                return;

        UA_DataSetMetaDataType_init(pMetaData);

        //pMetaData->namespacesSize = 1;
        //pMetaData->namespaces[0] = (UA_STRING *)calloc(pMetaData->namespacesSize, sizeof(UA_STRING("MKS")) );

                        //UA_FieldTargetVariable *targetVars = (UA_FieldTargetVariable *) UA_calloc(dataSetReaderConfig.dataSetMetaData.fieldsSize, sizeof(UA_FieldTargetVariable));

        //pMetaData->namespaces[0] = UA_STRING("MKS");
        //pMetaData->structureDataTypesSize = ??;
        //pMetaData->enumDataTypesSize = ??
        //pMetaData->enumDataTypes = "";
        //pMetaData->simpleDataTypesSize == ??;
        //pMetadata->simpleDataTypes = ??

        pMetaData->name = UA_STRING("AirGardSensor MetaData");
        pMetaData->description = UA_LOCALIZEDTEXT("en-US","AirGardSensor MetaData");
        pMetaData->fieldsSize = (size_t)40;     // number of nodes : 40 or 26
        pMetaData->fields = (UA_FieldMetaData*) UA_Array_new (pMetaData->fieldsSize, &UA_TYPES[UA_TYPES_FIELDMETADATA]);

        // non-diagnostics = 14 (n)
        // diagnostics = 26 (m)
        // alarms/ non alarms

        // static definition of number of fields size to 4 to create four different
        //  targetVariables of distinct datatype
        //  Currently the publisher sneds only DateTime data type
        //

        // header section

/*
typedef struct {
    UA_String name;
    UA_LocalizedText description;
    UA_DataSetFieldFlags fieldFlags;
    UA_Byte builtInType;
    UA_NodeId dataType;
    UA_Int32 valueRank;
    size_t arrayDimensionsSize;
    UA_UInt32 *arrayDimensions;
    UA_UInt32 maxStringLength;
    UA_Guid dataSetFieldId;
    size_t propertiesSize;
    UA_KeyValuePair *properties;
} UA_FieldMetaData;

*/
        UA_UInt32 *t_arrayDimensions;
        t_arrayDimensions = (UA_UInt32*)calloc(1, sizeof(UA_UInt32));
        t_arrayDimensions[0] = -1;      // match 10001

        int m=0, n=0;
        // 80300
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("1. Software Version");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "1. Software Version");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR;   //UA_VALUERANK_ONE_DIMENSION;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)-1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;    //1 : 80301
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("2. Data Block Version");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "2. DataBlock Version");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;    //2
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("1. Instrument Time");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "1. Instrument Time");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;    //3
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("2. Measurement Time");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "2. Measurement Time");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;    //4
        // Sensor section
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("1. Sensor");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "1. Sensor");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1; // MAX_STRING_SIZE
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;    //5
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("2. Operating Time");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "2. Operating Time");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_INT16;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT16].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;    //6
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("3. Warning Message");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "3. Warning Message");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;    //7
        // Status->Info
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("1. Boot Status");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "1. Boot Status");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;    //8
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("2. Snapshot Status");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "2. Snapshot Status");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;    //9
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("3. SCP Status");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "3. SCP Status");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;    //10 : 80310
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("4. SFTP Status");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "4. SFTP Status");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_ANY; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;    //11
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("5. Runscript Status");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "5. Runscript Status");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;    //12
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("6. Archive Status");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "6. Archive Status");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;    //13
        UA_FieldMetaData_init (&pMetaData->fields[n]);
        pMetaData->fields[n].name = UA_STRING("7. Ancillary Sensor Status");
        pMetaData->fields[n].description = UA_LOCALIZEDTEXT("en-US", "7. Ancillary Sensor Status");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_STRING;     //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[n].dataType);
        pMetaData->fields[n].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[n].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[n].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[n].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[n].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        n++;    //14
        m = n;
        // Diagnostics section
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("01. Igram PP");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "01. Igram PP");
        pMetaData->fields[n].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[n].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[0].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbing from the MQTT message

        m++; // 1, 15
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("02. Igram DC");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "02. Igram DC");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbling from the MQTT message

        m++; // 2, 16
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("03. Laser PP");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "03. Laser PP");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should be grabbling from the MQTT message

        m++; // 3, 17
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("04. Laser DC");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "04. Laser DC");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; // 4, 18
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("05. Single Beam At 900");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "05. Single Beam At 900");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setSc4lar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; // 5, 19
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("06. Single Beam At 2500");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "06. Single Beam At 2500");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; // 6, 20 : 80320
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("07. SignalToNoiseAt2500");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "07. SignalToNoiseAt2500");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_INT16;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT16].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; // 7, 21
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("08. Center Burst Location");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "08. Center Burst Location");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; // 8, 22
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("09. Detector Temperature");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "09. Detector Temperature");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; // 9, 23
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("10. Laser Frequency");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "10. Laser Frequency");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; //10, 24
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("11. Hard Drive Space");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "11. Hard Drive Space");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_INT16;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT16].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; //11, 25
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("12. Flow");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "12. Flow");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_INT16;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT16].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; //12, 26
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("13. Temperature");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "13. Temperature");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_INT16;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT16].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; //13, 27
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("14. Pressure");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "14. Pressure");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; //14, 28
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("15. Temp Optics");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "15. Temp Optics");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_INT16;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT16].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; //15, 29
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("16. Bad Scan Counter");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "16. Bad Scan Counter");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_INT16;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT16].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; //16, 30 : 80330
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("17. Free Memory Space");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "17. Free Memory Space");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_INT16;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_INT16].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; //17, 31
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("18. LAB Filename");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "18. LAB Filename");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_STRING;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[m].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[m].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[m].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[m].propertiesSize = 0;
       //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; //18, 32
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("19. LOG Filename");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "19. LOG Filename");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_STRING;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[m].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[m].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[m].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; //19, 33
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("20. Lg Filename");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "20. Lg Filename");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_STRING;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[m].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[m].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[m].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; //20, 34
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("21. Second Lg Filename");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "21. Second Lg Filename");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_STRING;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_STRING].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        //pMetaData->fields[m].arrayDimensionsSize = (size_t)1;
        //pMetaData->fields[m].arrayDimensions = t_arrayDimensions;
        pMetaData->fields[m].maxStringLength = 1; //MAX_STRING_SIZE;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; //21, 35
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("22. System Counter");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "22. System Counter");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; // 22, 36
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("23. Detector Counter");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "23. Detector Counter");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; // 23, 37
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("24. Laser Counter");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "24. Laser Counter");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; // 24, 38
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("25. Flow Pump Counter");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "25. Flow Pump Counter");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; // 25, 39
        UA_FieldMetaData_init (&pMetaData->fields[m]);
        pMetaData->fields[m].name = UA_STRING("26. Desiccant Counter");
        pMetaData->fields[m].description = UA_LOCALIZEDTEXT("en-US", "26. Desiccant Counter");
        pMetaData->fields[m].fieldFlags = UA_DATASETFIELDFLAGS_NONE;
        pMetaData->fields[m].builtInType = UA_NS0ID_FLOAT;      //UA_NS0ID_BASEDATATYPE;
        UA_NodeId_copy (&UA_TYPES[UA_TYPES_FLOAT].typeId, &pMetaData->fields[m].dataType);
        pMetaData->fields[m].valueRank = UA_VALUERANK_SCALAR; // scalar;
        pMetaData->fields[m].propertiesSize = 0;
        //UA_Variant_setScalar(&pMetaData->fields[1].properties->value, &varData, &UA_TYPES[UA_TYPES_FLOAT]); // should$

        m++; // 26, 40

        #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c (line 966) : leaving fillDataSetMetaData");
        #endif
}

