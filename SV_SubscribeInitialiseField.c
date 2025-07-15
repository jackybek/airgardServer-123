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

// Subscriber section variables & functions
extern UA_DataSetReaderConfig dataSetReaderConfig;
extern UA_NodeId readerGroupIdentifier;
extern UA_NodeId readerIdentifier;
extern UA_NodeId reader2Id;

static UA_NodeId *newSubscribedNodeId;	// an array of UA_NodeId

//extern UA_NodeId outSoftwareVersion_Id;
UA_String subscribedSoftwareVersion;
UA_String subscribedDataBlockVersion;
UA_String subscribedInstrumentTime;
UA_String subscribedMeasurementTime;
UA_String subscribedBootStatus;
UA_String subscribedSnapshotStatus;
UA_String subscribedSCPStatus;
UA_String subscribedSFTPStatus;
UA_String subscribedRunScriptStatus;
UA_String subscribedArchiveStatus;
UA_String subscribedAncillarySensorStatus;
UA_String subscribedSensor;
/**/UA_Int16 subscribedOperatingTime;
UA_String subscribedWarningMessage;

UA_Float subscribedIgramPP;
UA_Float subscribedIgramDC;
UA_Float subscribedLaserPP;
UA_Float subscribedLaserDC;
UA_Float subscribedSingleBeamAt900;
UA_Float subscribedSingleBeamAt2500;
/**/UA_Int16 subscribedSignalToNoiseAt2500;
UA_Float subscribedCenterBurstLocation;
UA_Float subscribedDetectorTemp;
UA_Float subscribedLaserFrequency;

/**/UA_Int16 subscribedHardDriveSpace;
/**/UA_Int16 subscribedFlow;
/**/UA_Int16 subscribedTemperature;
UA_Float subscribedPressure;
/**/UA_Int16 subscribedTempOptics;
/**/UA_Int16 subscribedBadScanCounter;
/**/UA_Int16 subscribedFreeMemorySpace;
UA_String subscribedLABFilename;
UA_String subscribedLOGFilename;
UA_String subscribedLgFilename;
UA_String subscribedSecondLgFilename;
UA_Float subscribedSystemCounter;
UA_Float subscribedDetectorCounter;
UA_Float subscribedLaserCounter;
UA_Float subscribedFlowPumpCounter;
UA_Float subscribedDesiccantCounter;

UA_StatusCode
subInitialiseField(UA_Server *uaServer, UA_NodeId dataSetReaderId)
{

	// add the variables subscribed from the broker to the OPCUA Address space
	static int firsttime_addSubscribedVariableFlag = UA_TRUE;
	static UA_NodeId airgardfolderId;
	static UA_NodeId datafolderId;
	static UA_NodeId diagnosticfolderId;
	static UA_NodeId methodsfolderId;
	static UA_NodeId statusfolderId;
	static UA_NodeId statusinfofolderId;
	static UA_NodeId timestampfolderId;
	UA_NodeId newNodeId;
	UA_StatusCode retval = UA_STATUSCODE_GOOD;

	if (uaServer == NULL)
		return UA_STATUSCODE_BADINTERNALERROR;
	// TODO:
	// check if there is any changes to the metadata version
	// if firsttime_addSubscribedVariableFlag == UA_TRUE, no action required, continue to create nodes
	// if firsttime_addSubscribedVariableFlag == UA_FALSE and if there are changes, we need to :
	// 	1. reset the array of newNode
	// 	2. reallocate the array of newNode based on the new metadata version
	//	3. reset & recreate targetVars
	// 	3. recreate the nodes and repopulate the values again

	#ifdef DEBUG_MODE
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : addSubscribeVariables() - firstime = %d", firsttime_addSubscribedVariableFlag);
	#endif


	if (firsttime_addSubscribedVariableFlag == UA_TRUE)	// only need to add nodes 1 time
	{
                UA_String folderName = UA_STRING("Airgard Subscribed"); //dataSetReaderConfig.dataSetMetaData.name;
                UA_ObjectAttributes oAttrObj = UA_ObjectAttributes_default;
		//UA_VariableAttributes oAttVar = UA_VariableAttributes_default;
                UA_QualifiedName airgardfolderBrowseName;
		//UA_QualifiedName diagnosticfolderBrowseName;

	        //Add a new namespace to the server => Returns the index of the new namespace i.e. namespaceIndex
        	//UA_Int16 nsIdx_MKS = UA_Server_addNamespace(uaServer, "virtualskies.com.sg/MKS/");
		size_t namespaceIndex;
		UA_Server_getNamespaceByName(uaServer, UA_STRING("virtualskies.com.sg/MKS/"), &namespaceIndex);

                if (folderName.length > 0)
                {
                        oAttrObj.displayName.locale = UA_STRING("en-US");
                        oAttrObj.displayName.text = folderName;		// actual shown in UAExpert - "en-US", "AirgardSensor MetaData"
                        airgardfolderBrowseName.namespaceIndex = namespaceIndex;	// actual shown in UAExpert - 1, "AirgardSensor MetaData"
                        airgardfolderBrowseName.name = folderName;	// actual shown in UAExpert - 1, "AirgardSensor MetaData"
                }
                else
                {
                        oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "AirgardSensor Subscribed");
                        airgardfolderBrowseName.namespaceIndex = namespaceIndex;
			airgardfolderBrowseName.name = UA_STRING("AirgardSensor Subscribed");
                }

		// create a UA structure to receive the incoming mqtt streams
                retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "AirGardSensor Subscribed"), //UA_NODEID_NULL,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT), //UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                        airgardfolderBrowseName,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                        oAttrObj, NULL, &airgardfolderId);

		#ifdef DEBUG_MODE
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c :Pass 0 : %s", UA_StatusCode_name(retval));
		#endif
        /* Create a rudimentary objectType
         *
         * airgardfolderId
         * |
         * + (V) SoftwareVersion
         * + (V) DataBlockVersion
         * +-(OT) TimestampType
         * |   + (V) InstrumentTime
         * |   + (V) MeasurementTime
         * +-(OT) StatusType
         * |   + (V) Sensor
         * |   + (V) OperatingTime
         * |   + (V) WarningMessage
         * |   +(OT) InfoType
         * |   |--+ (V) BootStatus
         * |   |  + (V) SnapshotStatus
         * |   |  + (V) SFTPStatus
         * |-(OT) DiagnosticsType
         * |   + (V) IgramPP
         * |   + (V) ...
         * |   + (V) DesiccantCounter
         * |-(OT) DataType (10400)
         * |   + (V) Alarms (20000)
         * |   + (OT) Alarm
         * |   |---+ (V) Tag
         * |   |   + (V) Name
         * |   |   + (V) Probability
         * |   |   + (V) CASnumber
         * |   |   + (V) Concentration
         * |   + (OT) Alarm
         * |   | <repeat>
         * |   + (V) NonAlarms (30000)
	 */

                // Subtree : Airgard->Data       == 
                oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "Data");
                retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "Airgard_Data_S"),
                                        airgardfolderId,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                        UA_QUALIFIEDNAME(namespaceIndex, "Data"), //diagnosticfolderBrowseName,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                        oAttrObj, NULL, &datafolderId);
		#ifdef DEBUG_MODE
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c Pass 1 : %s", UA_StatusCode_name(retval));
		#endif

		// Subtree : Airgard->Diagnostics 	== OKAY
		oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "Diagnostics");
		retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "Airgard_Diagnostics_S"),
					airgardfolderId,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
					UA_QUALIFIEDNAME(namespaceIndex, "Diagnostic"),
					UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
					oAttrObj, NULL, &diagnosticfolderId);
		#ifdef DEBUG_MODE
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c Pass 2 : %s", UA_StatusCode_name(retval));
		#endif

                // Subtree : Airgard->Methods       ==
                oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "Methods");
                retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "Airgard_Methods_S"),
                                        airgardfolderId,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                        UA_QUALIFIEDNAME(namespaceIndex, "Methods"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                        oAttrObj, NULL, &methodsfolderId);
		#ifdef DEBUG_MODE
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c Pass 3 : %s", UA_StatusCode_name(retval));
		#endif

		// Subtree : Airgard->Status	== OKAY
		oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "Status");
		retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "AirGard_Status_S"),
					airgardfolderId,
					UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
					UA_QUALIFIEDNAME(namespaceIndex, "Status"),
					UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
					oAttrObj, NULL, &statusfolderId);
		#ifdef DEBUG_MODE
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c Pass 4 : %s", UA_StatusCode_name(retval));
		#endif

		// Subtree : Airgard->Status->Info	== OKAY
		oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "Info");
                retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "AirGard_Status_Info_S"),
                                        statusfolderId,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                        UA_QUALIFIEDNAME(namespaceIndex, "Info"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                        oAttrObj, NULL, &statusinfofolderId);
		#ifdef DEBUG_MODE
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c Pass 5 : %s", UA_StatusCode_name(retval));
		#endif

		// Subtree : Airgard->Timestamp		== OKAY
		oAttrObj.displayName = UA_LOCALIZEDTEXT("en-US", "TimeStamp");
		retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "AirGard_TimeStamp_S"),
					airgardfolderId,
					UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
					UA_QUALIFIEDNAME(namespaceIndex, "Timestamp"),
					UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
					oAttrObj, NULL, &timestampfolderId);
		#ifdef DEBUG_MODE
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c Pass 6 : %s", UA_StatusCode_name(retval));
		#endif

		// Create target variables with respect to DataSetMetaDeta fields
		//UA_DataSetReaderConfig dataSetReaderConfig -- global variable
		/*
                UA_FieldTargetVariable *targetVars = (UA_FieldTargetVariable *) \
						UA_calloc(dataSetReaderConfig.dataSetMetaData.fieldsSize, \
						sizeof(UA_FieldTargetVariable));
		*/

		//UA_Variant variant_float;
		//UA_Variant variant_int;
		UA_Float data_float = -99.99;
                UA_Int32 data_int = -99;
                UA_String data_string = UA_STRING("Default string");

		newSubscribedNodeId = (UA_NodeId *) calloc(dataSetReaderConfig.dataSetMetaData.fieldsSize, sizeof(dataSetReaderConfig.dataSetMetaData));

                for (size_t i=0; i< dataSetReaderConfig.dataSetMetaData.fieldsSize; i++)
                {
			// Variable to subscribed data */
			UA_VariableAttributes vAttr = UA_VariableAttributes_default;
			UA_LocalizedText_copy(&dataSetReaderConfig.dataSetMetaData.fields[i].description,
						&vAttr.description);
			vAttr.displayName.locale = UA_STRING("en-US");
			vAttr.displayName.text = dataSetReaderConfig.dataSetMetaData.fields[i].name;	// UA_String
/* NodeId dataType */	vAttr.dataType = dataSetReaderConfig.dataSetMetaData.fields[i].dataType;
/* UA_Int32 */		vAttr.valueRank = dataSetReaderConfig.dataSetMetaData.fields[i].valueRank;
			if (vAttr.valueRank > 0)
			{
				vAttr.arrayDimensionsSize = dataSetReaderConfig.dataSetMetaData.fields[i].arrayDimensionsSize;
				vAttr.arrayDimensions = dataSetReaderConfig.dataSetMetaData.fields[i].arrayDimensions;
			}
			else
			{
				vAttr.arrayDimensionsSize = 0;
			}
			if ((i>=14) || (i<=39))
			{
/* UA_Byte */			vAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE | UA_ACCESSLEVELMASK_HISTORYREAD;
/* UA_Byte */			vAttr.userAccessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE | UA_ACCESSLEVELMASK_HISTORYREAD;// needed by UAExpert
/* UA_Boolean */		vAttr.historizing = UA_TRUE;
			}
			else
			{
/* UA_Byte */                   vAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
/* UA_Byte */                   vAttr.userAccessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
/* UA_Boolean */                vAttr.historizing = UA_FALSE;
			}

 			//UA_NodeId newNode;
                        // changed on 2 Jan 2022
                        if (dataSetReaderConfig.dataSetMetaData.fields[i].builtInType == UA_NS0ID_FLOAT)
                                UA_Variant_setScalar(&vAttr.value, &data_float, &UA_TYPES[UA_TYPES_FLOAT]);
                        else if (dataSetReaderConfig.dataSetMetaData.fields[i].builtInType == UA_NS0ID_INT16)
                                UA_Variant_setScalar(&vAttr.value, &data_int, &UA_TYPES[UA_TYPES_INT16]);
                        else if (dataSetReaderConfig.dataSetMetaData.fields[i].builtInType == UA_NS0ID_STRING)
                                UA_Variant_setScalar(&vAttr.value, &data_string, &UA_TYPES[UA_TYPES_STRING]);

			if ((i==0) || (i==1))
                                retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, (UA_UInt32)i+1+80300),
                                                        airgardfolderId,
                                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                                        UA_QUALIFIEDNAME(namespaceIndex, (char *)dataSetReaderConfig.dataSetMetaData.fields[i].name.data),
                                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                        vAttr, NULL, &newNodeId);

			else if ((i==2) || (i==3))
                                retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, (UA_UInt32)i+1+80300),
                                                        timestampfolderId,
                                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                                        UA_QUALIFIEDNAME(namespaceIndex, (char *)dataSetReaderConfig.dataSetMetaData.fields[i].name.data),
                                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                        vAttr, NULL, &newNodeId);

                        else if ((i==4) || (i==5) || (i==6))
                                retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, (UA_UInt32)i+1+80300),
                                                        statusfolderId,
                                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                                        UA_QUALIFIEDNAME(namespaceIndex, (char *)dataSetReaderConfig.dataSetMetaData.fields[i].name.data),
                                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                        vAttr, NULL, &newNodeId);

                        else if ((i==7) || (i==8) || (i==9) || (i==10) || (i==11) || (i==12)|| (i==13))
                                retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, (UA_UInt32)i+1+80300),
                                                        statusinfofolderId,
                                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                                        UA_QUALIFIEDNAME(namespaceIndex, (char *)dataSetReaderConfig.dataSetMetaData.fields[i].name.data),
                                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                        vAttr, NULL, &newNodeId);


			else //if ((i>=14) || (i<=39))
				retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, (UA_UInt32)i+1+80300),
							diagnosticfolderId,
							UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                           		UA_QUALIFIEDNAME(namespaceIndex, (char *)dataSetReaderConfig.dataSetMetaData.fields[i].name.data),
                                           		UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                           		vAttr, NULL, &newNodeId);


			if (retval == UA_STATUSCODE_GOOD)
     			{
        			//printf("SV_PubSub.c : addSubscribedVariables(): UA_Server_addVariableNode : success : %d, datatype is %s \n", i, vAttr.dataType);
				#ifdef DEBUG_MODE
        			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c :addSubscribedVariables() : success %s", UA_StatusCode_name(retval));
				#endif
     			}
     			else
     			{
        			//printf("SV_PubSub.c : addSubscribedVariables(): UA_Server_addVariableNode : failure \n");
				#ifdef DEBUG_MODE
        			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c : addSubscribedVariables() : failure %s", UA_StatusCode_name(retval));
				#endif
        			exit(EXIT_FAILURE);
     			}
			newSubscribedNodeId[i] = newNodeId;

		}// finished creating the new nodes
	// set flag to UA_FALSE after 1 round of execution
	firsttime_addSubscribedVariableFlag = UA_FALSE;
	return firsttime_addSubscribedVariableFlag;
	}
	else
	{// firsttime_addSubScribedVariableFlag == UA_FALSE;
		//printf("SV_PubSub.c : addSubscribedVariables(): UA_Server_addVariableNode :In 'else firsttime_adSubscribedVariableFlag' section : %d %d \n",
		//			firsttime_addSubscribedVariableFlag, dataSetReaderConfig.dataSetMetaData.fieldsSize);


		// now update the value into the nodes array
		for (size_t i=0; i< dataSetReaderConfig.dataSetMetaData.fieldsSize; i++)
		{
			//printf("SV_PubSub.c 1671: addSubscribedVariables() switch section : (%d) %f\n", i, subscribedIgramPP);
			//break;
			// should be grabbing from the MQTT message instead of dataSetReaderConfig.dataSetMetaData.fields[i].properties->value;
			// variant_value = dataSetReaderConfig.dataSetMetaData.fields[i].properties->value;
			UA_Variant variant_float, variant_string, variant_int;
			//UA_String outputStr;
			//int test;

			//UA_NodeId_toString(&newSubscribedNodeId[i], &outputStr); // deprecated

		//UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_PubSub.c : BEFORE SWITCH %d ", i);//, NodeId is %d ", i); //, outputStr.data);
			/* Segmentation fault will ocurr if UAExpert is connected to the OPCUA Server*/
			switch (i)
			{
				case 0 : // 80300 Software Version-----------------General
                                        #ifdef DEBUG_MODE
					printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() SoftwareVersion: %s\n", (int)i, subscribedSoftwareVersion.data);
					#endif
					//outputStr = UA_STRING(subscribedSoftwareVersion);
					UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedSoftwareVersion, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 1 : // 80301 Data Block Version
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Block Version : %s\n", (int)i, subscribedDataBlockVersion.data);
					#endif
					//outputStr = UA_STRING(subscribedDataBlockVersion);
					UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedDataBlockVersion, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 2 : // 80302---------------------Timestamp
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Instrument Time : %s\n", (int)i, subscribedInstrumentTime.data);
					#endif
                                        //outputStr = UA_STRING(subscribedInstrumentTime);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedInstrumentTime, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
					break;

                                case 3 : // 80303
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Measurement Time : %s\n", (int)i, subscribedMeasurementTime.data);
					#endif
                                        //outputStr = UA_STRING(subscribedMeasurementTime);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedMeasurementTime, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;


                                case 4 : // 80304--------------------Status
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Sensor : %s\n", (int)i, subscribedSensor.data);
					#endif
                                        //outputStr = UA_STRING(subscribedSensor);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedSensor, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

				case 5 : // 80305
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() ******Operating Time : %d\n", (int)i, subscribedOperatingTime);
					#endif
                                        //outputStr = UA_STRING(subscribedOperatingTime);
                                        UA_Variant_setScalar(&variant_int, &subscribedOperatingTime, &UA_TYPES[UA_TYPES_INT16]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_int);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 6 : // 80306
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Warning Message : %s\n", (int)i, subscribedWarningMessage.data);
					#endif
                                        //outputStr = UA_STRING(subscribedWarningMessage);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedWarningMessage, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 7 : // 80307---------------------Status(info)
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Boot Status : %s\n", (int)i, subscribedBootStatus.data);
					#endif
                                        //outputStr = UA_STRING(subscribedBootStatus);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedBootStatus, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 8 : // 80308
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Snapshot Status : %s\n", (int)i, subscribedSnapshotStatus.data);
					#endif
                                        //outputStr = UA_STRING(subscribedSnapshotStatus);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedSnapshotStatus, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 9 : // 80309
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() SCP Status : %s\n", (int)i, subscribedSCPStatus.data);
					#endif
                                        //outputStr = UA_STRING(subscribedSCPStatus);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedSCPStatus, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 10 : // 80310
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() SFTP Status : %s\n", (int)i, subscribedSFTPStatus.data);
					#endif
                                        //outputStr = UA_STRING(subscribedSFTPStatus);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedSFTPStatus, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 11 : // 80311
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Runscript Status : %s\n", (int)i, subscribedRunScriptStatus.data);
					#endif
                                        //outputStr = UA_STRING(subscribedRunScriptStatus);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedRunScriptStatus, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 12 : // 80312
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Archive Status : %s\n", (int)i, subscribedArchiveStatus.data);
                                        //outputStr = UA_STRING(subscribedArchiveStatus);
					#endif
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedArchiveStatus, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 13 : // 80313
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Ancillary Sensor Status : %s\n", (int)i, subscribedAncillarySensorStatus.data);
					#endif
                                        //outputStr = UA_STRING(subscribedAncillarySensorStatus);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedAncillarySensorStatus, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

				case 14 : // 80314 IgramPP-------------------------Diagnostics
                                        #ifdef DEBUG_MODE
					printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() IgramPP : %f\n", (int)i, subscribedIgramPP);
					#endif

					UA_Variant_setScalar(&variant_float, &subscribedIgramPP, &UA_TYPES[UA_TYPES_FLOAT]);
					UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is VARIANT

                        		//UA_FieldTargetDataType_init(&targetVars[i].targetVariable);
                        		//targetVars[i].targetVariable.attributeId = UA_ATTRIBUTEID_VALUE;
                        		//targetVars[i].targetVariable.targetNodeId = newSubscribedNodeId[i];

					// update OPCUA address space
					// data = subscribedIgramPP;
					//UA_Variant_setScalar(&varData,&data, &UA_TYPES[UA_TYPES_FLOAT]);
					//UA_Server_writeValue(uaServer, targetVars[i].targetVariable.targetNodeId, variant_float);

					break;

				case 15: // 80315 IgramDC
                                        #ifdef DEBUG_MODE
					printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() IgramDC : %f \n", (int)i, subscribedIgramDC);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedIgramDC, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 16: // 80316 LaserPP
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() LaserPP : %f \n", (int)i, subscribedLaserPP);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedLaserPP, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 17: // 80317 LaserDC
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() LaserDC : %f \n", (int)i, subscribedLaserDC);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedLaserDC, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 18: // 80318 SingleBeamAt900
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() SingleBeamAt900 : %f \n", (int)i, subscribedSingleBeamAt900);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedSingleBeamAt900, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 19: // 80319 SingleBeamAt2500
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() SingleBeamAt2500 : %f \n", (int)i, subscribedSingleBeamAt2500);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedSingleBeamAt2500, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 20: // 80320 SignalToNoiseAt2500
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() ******SignalToNoiseAt2500 : %d \n", (int)i, subscribedSignalToNoiseAt2500);
					#endif

                                        UA_Variant_setScalar(&variant_int, &subscribedSignalToNoiseAt2500, &UA_TYPES[UA_TYPES_INT16]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_int);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 21: // 80321 CenterBurstLocation
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() CenterBurstLocation : %f \n", (int)i, subscribedCenterBurstLocation);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedCenterBurstLocation, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 22: // 80322 DetectorTemp
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() DetectorTemp : %f \n", (int)i, subscribedDetectorTemp);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedDetectorTemp, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 23: // 80323 LaserFrequency
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Laser Frequency : %f \n", (int)i, subscribedLaserFrequency);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedLaserFrequency, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 24: // 80324 HardDriveSpace
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() ******Hard Drive Space : %d \n", (int)i, subscribedHardDriveSpace);
					#endif

                                        UA_Variant_setScalar(&variant_int, &subscribedHardDriveSpace, &UA_TYPES[UA_TYPES_INT16]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_int);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 25: // 80325 Flow
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() ******Flow : %d \n", (int)i, subscribedFlow);
					#endif

                                        UA_Variant_setScalar(&variant_int, &subscribedFlow, &UA_TYPES[UA_TYPES_INT16]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_int);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 26: // 80326 Temperature
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() ******Temperature : %d \n", (int)i, subscribedTemperature);
					#endif

                                        UA_Variant_setScalar(&variant_int, &subscribedTemperature, &UA_TYPES[UA_TYPES_INT16]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_int);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 27: // 80327 Pressure
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Pressure : %f \n", (int)i, subscribedPressure);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedPressure, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 28: // 80328 TempOptics
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() ******Temp Optics : %d \n", (int)i, subscribedTempOptics);
					#endif

                                        UA_Variant_setScalar(&variant_int, &subscribedTempOptics, &UA_TYPES[UA_TYPES_INT16]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_int);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 29: // 80329 BadScanCounter
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() ******BadScan Counter : %d \n", (int)i, subscribedBadScanCounter);
					#endif

                                        UA_Variant_setScalar(&variant_int, &subscribedBadScanCounter, &UA_TYPES[UA_TYPES_INT16]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_int);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 30: // 80330 FreeMemorySpace
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() ******Free Memory Space : %d \n", (int)i, subscribedFreeMemorySpace);
					#endif

                                        UA_Variant_setScalar(&variant_int, &subscribedFreeMemorySpace, &UA_TYPES[UA_TYPES_INT16]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_int);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 31: // 80331
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1687: (%d) addSubscribedVariables() LAB Filename : %s\n", (int)i, subscribedLABFilename.data);
					#endif
                                        //outputStr = UA_STRING(subscribedLABFilename);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedLABFilename, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 32: // 80332
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1687: (%d) addSubscribedVariables() LOG Filename : %s\n", (int)i, subscribedLOGFilename.data);
                                        //outputStr = UA_STRING(subscribedLOGFilename);
					#endif
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedLOGFilename, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 33: // 80333
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1687: (%d) addSubscribedVariables() Lg Filename : %s\n", (int)i, subscribedLgFilename.data);
					#endif
                                        //outputStr = UA_STRING(subscribedLgFilename);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedLgFilename, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;

                                case 34: // 80334
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1687: (%d) addSubscribedVariables() Second Lg Filename : %s\n", (int)i, subscribedSecondLgFilename.data);
					#endif
                                        //outputStr = UA_STRING(subscribedSecondLgFilename);
                                        UA_Variant_init(&variant_string);
                                        UA_Variant_setScalar(&variant_string, &subscribedSecondLgFilename, &UA_TYPES[UA_TYPES_STRING]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_string);  // properties is UA_KeyValuePair, value is VARIANT
                                        break;


                                case 35: // 80335 SystemCounter
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() System Counter : %f \n", (int)i, subscribedSystemCounter);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedSystemCounter, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 36: // 80336 DetectorCounter
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Detector Counter : %f \n", (int)i, subscribedDetectorCounter);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedDetectorCounter, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 37: // 80337 LaserCounter
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Laser Counter : %f \n", (int)i, subscribedLaserCounter);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedLaserCounter, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 38: // 80338 FlowPumpCounter
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Flow Pump Counter : %f \n", (int)i, subscribedFlowPumpCounter);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedFlowPumpCounter, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

                                case 39: // 80339 DesiccantCounter
                                        #ifdef DEBUG_MODE
                                        printf("SV_PubSub.c 1838: (%d) addSubscribedVariables() Desiccant Counter : %f \n", (int)i, subscribedDesiccantCounter);
					#endif

                                        UA_Variant_setScalar(&variant_float, &subscribedDesiccantCounter, &UA_TYPES[UA_TYPES_FLOAT]);
                                        UA_Server_writeValue(uaServer, newSubscribedNodeId[i], variant_float);  // properties is UA_KeyValuePair, value is V$
                                        break;

				// -----------Data (KIV)

			}

                }

		#ifdef DEBUG_MODE
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_PubSub.c : after addTargetVariable (subscriber) loop");
		#endif

		/* no need, since the OPCUA Address are updated with new nodes in the loop above.

                retval = UA_Server_DataSetReader_createTargetVariables(uaServer, dataSetReaderId,
								dataSetReaderConfig.dataSetMetaData.fieldsSize,
								targetVars);
		for (size_t i=0; i< dataSetReaderConfig.dataSetMetaData.fieldsSize; i++)
		{
			if (&targetVars[0].targetVariable)
	                	UA_FieldTargetDataType_clear(&targetVars[0].targetVariable);
		}
		*/

		/*
                if (targetVars)
			UA_free(targetVars);
		if (dataSetReaderConfig.dataSetMetaData.fields)
                	UA_free(dataSetReaderConfig.dataSetMetaData.fields);
		*/
		return firsttime_addSubscribedVariableFlag;

	}
}


/* Periodically refreshes the MQTT stack (sending/receiving) */
/*
//https://github.com/open62541/open62541/blob/master/examples/pubsub/server_pubsub_subscriber_rt_level.c
static void
mqttYieldPollingCallback(UA_Server *uaServer, UA_PubSubConnection *connection)
{
    connection->channel->yield(connection->channel, (UA_UInt16) 30);	// timeout : 30 seconds
    addSubscribedVariables(uaServer, readerIdentifier);	// repeated update the UA AddressSpace

}
*/
