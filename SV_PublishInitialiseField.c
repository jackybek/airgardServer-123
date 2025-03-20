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
#endif
#include "SV_PubSub.h"

// publisher section : variables & functions
static UA_NodeId publishedDataSetIdentifier;

/**
 * **DataSetField handling**
 * The DataSetField (DSF) is part of the PDS and describes exactly one published field.
 */
void
pubInitialiseField(UA_Server *uaServer)
{
    /* Add a field to the previous created PublishedDataSet */
    //UA_NodeId dataSetFieldIdentifier;

//    UA_NodeId f_SoftwareVersion_Id, f_DataBlockVersion_Id, f_InstrumentTime_Id, f_MeasurementTime_Id;
//    UA_NodeId f_BootStatus_Id, f_SnapshotStatus_Id, f_SCPStatus_Id, f_SFTPStatus_Id, f_RunscriptStatus_Id, f_ArchiveStatus_Id, f_AncillarySensorStatus_Id;
//    UA_NodeId f_Sensor_Id, f_OperatingTime_Id, f_WarningMessage_Id;
//    UA_NodeId f_IgramPP_Id, f_IgramDC_Id, f_LaserPP_Id, f_LaserDC_Id, f_SingleBeamAt900_Id, f_SingleBeamAt2500_Id, f_SignalToNoiseAt2500_Id;
//    UA_NodeId f_CenterBurstLocation_Id, f_DetectorTemp_Id, f_LaserFrequency_Id, f_HardDriveSpace_Id, f_Flow_Id, f_Temperature_Id, f_Pressure_Id;
//    UA_NodeId f_TempOptics_Id, f_BadScanCounter_Id, f_FreeMemorySpace_Id, f_LABFilename_Id, f_LOGFilename_Id, f_LgFilename_Id, f_SecondLgFilename_Id;
//    UA_NodeId f_SystemCounter_Id, f_DetectorCounter_Id, f_LaserCounter_Id, f_FlowPumpCounter_Id, f_DesiccantCounter_Id;

    size_t namespaceIndex;
    UA_Server_getNamespaceByName(uaServer, UA_STRING("virtualskies.com.sg/MKS/"), &namespaceIndex);

//0
    UA_DataSetFieldConfig dsCfgSoftwareVersion;
    memset(&dsCfgSoftwareVersion, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgSoftwareVersion.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgSoftwareVersion.field.variable.fieldNameAlias = UA_STRING("SoftwareVersion");
    dsCfgSoftwareVersion.field.variable.promotedField = UA_FALSE;
    dsCfgSoftwareVersion.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10001);
    dsCfgSoftwareVersion.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//1
    UA_DataSetFieldConfig dsCfgDataBlockVersion;
    memset(&dsCfgDataBlockVersion, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgDataBlockVersion.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgDataBlockVersion.field.variable.fieldNameAlias = UA_STRING("DataBlockVersion");
    dsCfgDataBlockVersion.field.variable.promotedField = UA_FALSE;
    dsCfgDataBlockVersion.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10002);
    dsCfgDataBlockVersion.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//2
    UA_DataSetFieldConfig dsCfgInstrumentTime;
    memset(&dsCfgInstrumentTime, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgInstrumentTime.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgInstrumentTime.field.variable.fieldNameAlias = UA_STRING("InstrumentTime");
    dsCfgInstrumentTime.field.variable.promotedField = UA_FALSE;
    dsCfgInstrumentTime.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10101);
    dsCfgInstrumentTime.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//3
    UA_DataSetFieldConfig dsCfgMeasurementTime;
    memset(&dsCfgMeasurementTime, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgMeasurementTime.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgMeasurementTime.field.variable.fieldNameAlias = UA_STRING("MeasurementTime");
    dsCfgMeasurementTime.field.variable.promotedField = UA_FALSE;
    dsCfgMeasurementTime.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10102);
    dsCfgMeasurementTime.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//4
    UA_DataSetFieldConfig dsCfgSensor;
    memset(&dsCfgSensor, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgSensor.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgSensor.field.variable.fieldNameAlias = UA_STRING("Sensor");
    dsCfgSensor.field.variable.promotedField = UA_FALSE;
    dsCfgSensor.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10201);
    dsCfgSensor.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//5
    UA_DataSetFieldConfig dsCfgOperatingTime;
    memset(&dsCfgOperatingTime, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgOperatingTime.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgOperatingTime.field.variable.fieldNameAlias = UA_STRING("OperatingTime");
    dsCfgOperatingTime.field.variable.promotedField = UA_FALSE;
    dsCfgOperatingTime.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10202);
    dsCfgOperatingTime.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//6
    UA_DataSetFieldConfig dsCfgWarningMessage;
    memset(&dsCfgWarningMessage, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgWarningMessage.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgWarningMessage.field.variable.fieldNameAlias = UA_STRING("WarningMessage");
    dsCfgWarningMessage.field.variable.promotedField = UA_FALSE;
    dsCfgWarningMessage.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10203);
    dsCfgWarningMessage.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//7
    UA_DataSetFieldConfig dsCfgBootStatus;
    memset(&dsCfgBootStatus, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgBootStatus.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgBootStatus.field.variable.fieldNameAlias = UA_STRING("BootStatus");
    dsCfgBootStatus.field.variable.promotedField = UA_FALSE;
    dsCfgBootStatus.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10211);
    dsCfgBootStatus.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//8
    UA_DataSetFieldConfig dsCfgSnapshotStatus;
    memset(&dsCfgSnapshotStatus, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgSnapshotStatus.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgSnapshotStatus.field.variable.fieldNameAlias = UA_STRING("SnapshotStatus");
    dsCfgSnapshotStatus.field.variable.promotedField = UA_FALSE;
    dsCfgSnapshotStatus.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10212);
    dsCfgSnapshotStatus.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//9
    UA_DataSetFieldConfig dsCfgSCPStatus;
    memset(&dsCfgSCPStatus, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgSCPStatus.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgSCPStatus.field.variable.fieldNameAlias = UA_STRING("SCPStatus");
    dsCfgSCPStatus.field.variable.promotedField = UA_FALSE;
    dsCfgSCPStatus.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10213);
    dsCfgSCPStatus.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//10
    UA_DataSetFieldConfig dsCfgSFTPStatus;
    memset(&dsCfgSFTPStatus, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgSFTPStatus.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgSFTPStatus.field.variable.fieldNameAlias = UA_STRING("SFTPStatus");
    dsCfgSFTPStatus.field.variable.promotedField = UA_FALSE;
    dsCfgSFTPStatus.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10214);
    dsCfgSFTPStatus.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//11
    UA_DataSetFieldConfig dsCfgRunscriptStatus;
    memset(&dsCfgRunscriptStatus, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgRunscriptStatus.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgRunscriptStatus.field.variable.fieldNameAlias = UA_STRING("RunscriptStatus");
    dsCfgRunscriptStatus.field.variable.promotedField = UA_FALSE;
    dsCfgRunscriptStatus.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10215);
    dsCfgRunscriptStatus.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//12
    UA_DataSetFieldConfig dsCfgArchiveStatus;
    memset(&dsCfgArchiveStatus, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgArchiveStatus.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgArchiveStatus.field.variable.fieldNameAlias = UA_STRING("ArchiveStatus");
    dsCfgArchiveStatus.field.variable.promotedField = UA_FALSE;
    dsCfgArchiveStatus.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10216);
    dsCfgArchiveStatus.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//13
    UA_DataSetFieldConfig dsCfgAncillarySensorStatus;
    memset(&dsCfgAncillarySensorStatus, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgAncillarySensorStatus.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgAncillarySensorStatus.field.variable.fieldNameAlias = UA_STRING("AncillarySensorStatus");
    dsCfgAncillarySensorStatus.field.variable.promotedField = UA_FALSE;
    dsCfgAncillarySensorStatus.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10217);
    //out_AncillarySensorStatus; //UA_NODEID_NUMERIC(1, 10217);
    dsCfgAncillarySensorStatus.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;
//14
    UA_DataSetFieldConfig dsCfgIgramPP;
    memset(&dsCfgIgramPP, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgIgramPP.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgIgramPP.field.variable.fieldNameAlias = UA_STRING("IgramPP");
    dsCfgIgramPP.field.variable.promotedField = UA_FALSE;
    dsCfgIgramPP.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10301);
    //outIgramPP_Id; //UA_NODEID_NUMERIC(1, 10301);
    dsCfgIgramPP.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgIgramDC;
    memset(&dsCfgIgramDC, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgIgramDC.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgIgramDC.field.variable.fieldNameAlias = UA_STRING("IgramDC");
    dsCfgIgramDC.field.variable.promotedField = UA_FALSE;
    dsCfgIgramDC.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10302);
    //outIgramDC_Id; //UA_NODEID_NUMERIC(1, 10302);
    dsCfgIgramDC.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgLaserPP;
    memset(&dsCfgLaserPP, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgLaserPP.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgLaserPP.field.variable.fieldNameAlias = UA_STRING("LaserPP");
    dsCfgLaserPP.field.variable.promotedField = UA_FALSE;
    dsCfgLaserPP.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10303);
    //outLaserPP_Id; //UA_NODEID_NUMERIC(1, 10303);
    dsCfgLaserPP.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgLaserDC;
    memset(&dsCfgLaserDC, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgLaserDC.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgLaserDC.field.variable.fieldNameAlias = UA_STRING("LaserDC");
    dsCfgLaserDC.field.variable.promotedField = UA_FALSE;
    dsCfgLaserDC.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10304);
    //outLaserDC_Id; //UA_NODEID_NUMERIC(1, 10304);
    dsCfgLaserDC.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgSingleBeamAt900;
    memset(&dsCfgSingleBeamAt900, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgSingleBeamAt900.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgSingleBeamAt900.field.variable.fieldNameAlias = UA_STRING("SingleBeamAt900");
    dsCfgSingleBeamAt900.field.variable.promotedField = UA_FALSE;
    dsCfgSingleBeamAt900.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10305);
    //outSingleBeamAt900_Id; //UA_NODEID_NUMERIC(1, 10305);
    dsCfgSingleBeamAt900.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgSingleBeamAt2500;
    memset(&dsCfgSingleBeamAt2500, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgSingleBeamAt2500.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgSingleBeamAt2500.field.variable.fieldNameAlias = UA_STRING("SingleBeamAt2500");
    dsCfgSingleBeamAt2500.field.variable.promotedField = UA_FALSE;
    dsCfgSingleBeamAt2500.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10306);
    //outSingleBeamAt2500_Id; //UA_NODEID_NUMERIC(1, 10306);
    dsCfgSingleBeamAt2500.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgSignalToNoiseAt2500;
    memset(&dsCfgSignalToNoiseAt2500, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgSignalToNoiseAt2500.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgSignalToNoiseAt2500.field.variable.fieldNameAlias = UA_STRING("SignalToNoiseAt2500");
    dsCfgSignalToNoiseAt2500.field.variable.promotedField = UA_FALSE;
    dsCfgSignalToNoiseAt2500.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10307);
    //outSignalToNoiseAt2500_Id; //UA_NODEID_NUMERIC(1, 10307);
    dsCfgSignalToNoiseAt2500.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgCenterBurstLocation;
    memset(&dsCfgCenterBurstLocation, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgCenterBurstLocation.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgCenterBurstLocation.field.variable.fieldNameAlias = UA_STRING("CenterBurstLocation");
    dsCfgCenterBurstLocation.field.variable.promotedField = UA_FALSE;
    dsCfgCenterBurstLocation.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10308);
    //outCenterBurstLocation_Id; //UA_NODEID_NUMERIC(1, 10308);
    dsCfgCenterBurstLocation.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgDetectorTemp;
    memset(&dsCfgDetectorTemp, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgDetectorTemp.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgDetectorTemp.field.variable.fieldNameAlias = UA_STRING("DetectorTemperature");
    dsCfgDetectorTemp.field.variable.promotedField = UA_FALSE;
    dsCfgDetectorTemp.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10309);
    //outDetectorTemp_Id; //UA_NODEID_NUMERIC(1, 10309);
    dsCfgDetectorTemp.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgLaserFrequency;
    memset(&dsCfgLaserFrequency, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgLaserFrequency.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgLaserFrequency.field.variable.fieldNameAlias = UA_STRING("LaserFrequency");
    dsCfgLaserFrequency.field.variable.promotedField = UA_FALSE;
    dsCfgLaserFrequency.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10310);
    //outLaserFrequency_Id; //UA_NODEID_NUMERIC(1, 10310);
    dsCfgLaserFrequency.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgHardDriveSpace;
    memset(&dsCfgHardDriveSpace, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgHardDriveSpace.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgHardDriveSpace.field.variable.fieldNameAlias = UA_STRING("HardDriveSpace");
    dsCfgHardDriveSpace.field.variable.promotedField = UA_FALSE;
    dsCfgHardDriveSpace.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10311);
    //outHardDriveSpace_Id; //UA_NODEID_NUMERIC(1, 10311);
    dsCfgHardDriveSpace.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgFlow;
    memset(&dsCfgFlow, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgFlow.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgFlow.field.variable.fieldNameAlias = UA_STRING("Flow");
    dsCfgFlow.field.variable.promotedField = UA_FALSE;
    dsCfgFlow.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10312);
    //outFlow_Id; //UA_NODEID_NUMERIC(1, 10312);
    dsCfgFlow.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgTemperature;
    memset(&dsCfgTemperature, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgTemperature.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgTemperature.field.variable.fieldNameAlias = UA_STRING("Temperature");
    dsCfgTemperature.field.variable.promotedField = UA_FALSE;
    dsCfgTemperature.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10313);
    //outTemperature_Id; //UA_NODEID_NUMERIC(1, 10313);
    dsCfgTemperature.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgPressure;
    memset(&dsCfgPressure, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgPressure.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgPressure.field.variable.fieldNameAlias = UA_STRING("Pressure");
    dsCfgPressure.field.variable.promotedField = UA_FALSE;
    dsCfgPressure.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10314);
    //outPressure_Id; //UA_NODEID_NUMERIC(1, 10314);
    dsCfgPressure.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgTempOptics;
    memset(&dsCfgTempOptics, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgTempOptics.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgTempOptics.field.variable.fieldNameAlias = UA_STRING("TempOptics");
    dsCfgTempOptics.field.variable.promotedField = UA_FALSE;
    dsCfgTempOptics.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10315);
    //outTempOptics_Id; //UA_NODEID_NUMERIC(1, 10315);
    dsCfgTempOptics.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgBadScanCounter;
    memset(&dsCfgBadScanCounter, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgBadScanCounter.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgBadScanCounter.field.variable.fieldNameAlias = UA_STRING("BadScanCounter");
    dsCfgBadScanCounter.field.variable.promotedField = UA_FALSE;
    dsCfgBadScanCounter.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10316);
    //outBadScanCounter_Id; //UA_NODEID_NUMERIC(1, 10316);
    dsCfgBadScanCounter.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgFreeMemorySpace;
    memset(&dsCfgFreeMemorySpace, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgFreeMemorySpace.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgFreeMemorySpace.field.variable.fieldNameAlias = UA_STRING("FreeMemorySpace");
    dsCfgFreeMemorySpace.field.variable.promotedField = UA_FALSE;
    dsCfgFreeMemorySpace.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10317);
    //outFreeMemorySpace_Id; //UA_NODEID_NUMERIC(1, 10317);
    dsCfgFreeMemorySpace.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgLABFilename;
    memset(&dsCfgLABFilename, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgLABFilename.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgLABFilename.field.variable.fieldNameAlias = UA_STRING("LABFilename");
    dsCfgLABFilename.field.variable.promotedField = UA_FALSE;
    dsCfgLABFilename.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10318);
    //outLABFilename_Id; //UA_NODEID_NUMERIC(1, 10318);
    dsCfgLABFilename.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgLOGFilename;
    memset(&dsCfgLOGFilename, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgLOGFilename.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgLOGFilename.field.variable.fieldNameAlias = UA_STRING("LOGFilename");
    dsCfgLOGFilename.field.variable.promotedField = UA_FALSE;
    dsCfgLOGFilename.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10319);
    //outLOGFilename_Id; //UA_NODEID_NUMERIC(1, 10319);
    dsCfgLOGFilename.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgLgFilename;
    memset(&dsCfgLgFilename, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgLgFilename.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgLgFilename.field.variable.fieldNameAlias = UA_STRING("LgFilename");
    dsCfgLgFilename.field.variable.promotedField = UA_FALSE;
    dsCfgLgFilename.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10320);
    //outLgFilename_Id; //UA_NODEID_NUMERIC(1, 10320);
    dsCfgLgFilename.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgSecondLgFilename;
    memset(&dsCfgSecondLgFilename, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgSecondLgFilename.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgSecondLgFilename.field.variable.fieldNameAlias = UA_STRING("SecondLgFilename");
    dsCfgSecondLgFilename.field.variable.promotedField = UA_FALSE;
    dsCfgSecondLgFilename.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10321);
    //outSecondLgFilename_Id; //UA_NODEID_NUMERIC(1, 10321);
    dsCfgSecondLgFilename.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgSystemCounter;
    memset(&dsCfgSystemCounter, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgSystemCounter.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgSystemCounter.field.variable.fieldNameAlias = UA_STRING("SystemCounter");
    dsCfgSystemCounter.field.variable.promotedField = UA_FALSE;
    dsCfgSystemCounter.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10322);
    //outSystemCounter_Id; //UA_NODEID_NUMERIC(1, 10322);
    dsCfgSystemCounter.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgDetectorCounter;
    memset(&dsCfgDetectorCounter, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgDetectorCounter.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgDetectorCounter.field.variable.fieldNameAlias = UA_STRING("DetectorCounter");
    dsCfgDetectorCounter.field.variable.promotedField = UA_FALSE;
    dsCfgDetectorCounter.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10323);
    //outDetectorCounter_Id; //UA_NODEID_NUMERIC(1, 10323);
    dsCfgDetectorCounter.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgLaserCounter;
    memset(&dsCfgLaserCounter, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgLaserCounter.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgLaserCounter.field.variable.fieldNameAlias = UA_STRING("LaserCounter");
    dsCfgLaserCounter.field.variable.promotedField = UA_FALSE;
    dsCfgLaserCounter.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10324);
    //outLaserCounter_Id; //UA_NODEID_NUMERIC(1, 10324);
    dsCfgLaserCounter.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgFlowPumpCounter;
    memset(&dsCfgFlowPumpCounter, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgFlowPumpCounter.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgFlowPumpCounter.field.variable.fieldNameAlias = UA_STRING("FlowPumpCounter");
    dsCfgFlowPumpCounter.field.variable.promotedField = UA_FALSE;
    dsCfgFlowPumpCounter.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10325);
    //outFlowPumpCounter_Id; //UA_NODEID_NUMERIC(1, 10325);
    dsCfgFlowPumpCounter.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    UA_DataSetFieldConfig dsCfgDesiccantCounter;
    memset(&dsCfgDesiccantCounter, 0, sizeof(UA_DataSetFieldConfig));
    dsCfgDesiccantCounter.dataSetFieldType = UA_PUBSUB_DATASETFIELD_VARIABLE;
    dsCfgDesiccantCounter.field.variable.fieldNameAlias = UA_STRING("DesiccantCounter");
    dsCfgDesiccantCounter.field.variable.promotedField = UA_FALSE;
    dsCfgDesiccantCounter.field.variable.publishParameters.publishedVariable = UA_NODEID_NUMERIC(namespaceIndex, 10326);
    //outDesiccantCounter_Id; //UA_NODEID_NUMERIC(1, 10326);
    dsCfgDesiccantCounter.field.variable.publishParameters.attributeId = UA_ATTRIBUTEID_VALUE;

    // add in order :: limit seemed to be 32
    // count = 4
    #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"----------SV_PubSub.c 733 : addDataSetField()");
    #endif
    // added by Jacky on 4/4/2021 to update MQTT payload (MetaDataVersion)
    // the corresponding change has to take place in open62541.c : UA_Server_addDataSetField() - however this change cause publisher to have error
        //UA_open62541/src/pubsub/ua_pubsub_writer.c
        //At line 691
    //UA_PublishedDataSet *currentDataSet = UA_PublishedDataSet_findPDSbyId(uaServer, publishedDataSetIdentifier);
    //currentDataSet->dataSetMetaData.configurationVersion.majorVersion = MAJOR_SOFTWARE_VERSION;
    //currentDataSet->dataSetMetaData.configurationVersion.minorVersion = MINOR_SOFTWARE_VERSION;

    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgSoftwareVersion, NULL); // &f_SoftwareVersion_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgDataBlockVersion, NULL); //&f_DataBlockVersion_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgInstrumentTime, NULL); // &f_InstrumentTime_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgMeasurementTime, NULL); //&f_MeasurementTime_Id);

    // count = 3
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgSensor, NULL); //&f_RunscriptStatus_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgOperatingTime, NULL); //&f_ArchiveStatus_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgWarningMessage, NULL); //&f_AncillarySensorStatus_Id);

    // count = 7 => PubSub MQTT: publish: Send buffer is full. Possible reasons: send buffer is to small, sending to fast, broker not responding.

    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgBootStatus, NULL); //&f_BootStatus_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgSnapshotStatus, NULL); //&f_SnapshotStatus_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgSCPStatus, NULL); //&f_SCPStatus_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgSFTPStatus, NULL); //&f_SFTPStatus_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgRunscriptStatus, NULL); //&f_RunscriptStatus_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgArchiveStatus, NULL); //&f_ArchiveStatus_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgAncillarySensorStatus, NULL); //&f_AncillarySensorStatus_Id);

    #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"----------SV_PubSub.c :755 : addDataSetField()");
    #endif
// count = 25
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgIgramPP, NULL); //&f_IgramPP_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgIgramDC, NULL); //&f_IgramDC_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgLaserPP, NULL); //&f_LaserPP_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgLaserDC, NULL); //&f_LaserDC_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgSingleBeamAt900, NULL); //&f_SingleBeamAt900_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgSingleBeamAt2500, NULL); //&f_SingleBeamAt2500_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgSignalToNoiseAt2500, NULL); //&f_SignalToNoiseAt2500_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgCenterBurstLocation, NULL); //&f_CenterBurstLocation_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgDetectorTemp, NULL); //&f_DetectorTemp_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgLaserFrequency, NULL); //&f_LaserFrequency_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgHardDriveSpace, NULL);//&f_HardDriveSpace_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgFlow, NULL);//&f_Flow_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgTemperature, NULL); //&f_Temperature_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgPressure, NULL); //&f_Pressure_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgTempOptics, NULL); //&f_TempOptics_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgBadScanCounter, NULL); //&f_BadScanCounter_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgFreeMemorySpace, NULL); //&f_FreeMemorySpace_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgLABFilename, NULL); //&f_LABFilename_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgLOGFilename, NULL); //&f_LOGFilename_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgLgFilename, NULL); //&f_LgFilename_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgSecondLgFilename, NULL); //&f_SecondLgFilename_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgDetectorCounter, NULL); //&f_DetectorCounter_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgLaserCounter, NULL); //&f_LaserCounter_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgFlowPumpCounter, NULL); //&f_FlowPumpCounter_Id);
    UA_Server_addDataSetField(uaServer, publishedDataSetIdentifier, &dsCfgDesiccantCounter, NULL); //&f_DesiccantCounter_Id);

    #ifdef DEBUG_MODE
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"----------SV_PublishedInitialiseField.c : addDataSetField()");
    #endif
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"----------SV_PublishedInitialiseField.c : addDataSetField : success");

}
