#ifdef no_almagamation
#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/nodestore_default.h>
#else
   #include "open62541.h"
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <iomanip>
#include <cstring>
#include <algorithm>
#include <AdsDef.h>
#include <AdsDevice.h>
#include <AdsVariable.h>
#include <AdsLib.h>
#include <SymbolAccess.h>
#include <AdsNotificationOOI.h>
//#include <AdsAPI.h>

#define AMSPORT_PROXIMITYSENSOR 851
#define AMSPORT_SOLENOIDVALVE 852
#define REMOTE_NET_ID { 5, 40, 64, 12, 1, 1 }

/* refer to https://github.com/Beckhoff/ADS/blob/master/example/example.cpp */

extern "C" {
#include "SV_Misc.h"
#include "SV_Monitor.h"
#include "SV_TwinCATAds.h"
#include "open62541.h"

int check_port_active(char *, int);

typedef struct _twincatAdsVariables
{
	// ADS information
	int nPort;
        AdsVersion version;     // consists of build version and revision as members
        AmsAddr Addr;
        char DevName[50];

	// open62541 information
	UA_Server *uaServer;
	UA_NodeId nodeId;
        std::string _name, _typeName, _comment, _entryLength, _iGroup, _iOffs, _size, _dataType, _nameLength, _typeLength, _commentLength;
        int _variableType;
	int _sequenceNo;	//
}twincatAdsVariables;

twincatAdsVariables *adsVariables;
int adsVarCounter;	// this is to keep track of the number of items in the array

UA_NodeId outName_Id;
UA_NodeId outTypeName_Id;
UA_NodeId outComment_Id;
UA_NodeId outEntryLength_Id;
UA_NodeId outiGroup_Id;
UA_NodeId outiOffs_Id;
UA_NodeId outSize_Id;
UA_NodeId outDataType_Id;
UA_NodeId outNameLength_Id;
UA_NodeId outTypeLength_Id;
UA_NodeId outCommentLength_Id;
UA_NodeId outValue_Id;

#ifndef _WIN32
   #define _stdcall
#endif

using boolDataType = bool;
using uint8DataType = uint8_t;
using uint16DataType = uint16_t;
using uint32DataType = uint32_t;
using uint64DataType = uint64_t;
using doubleDataType = double;
using floatDataType = float;
using int8DataType = int8_t;
using int16DataType = int16_t;
using int32DataType = int32_t;
using int64DataType = int64_t;

//-------------------------------------------------------
#ifndef NOT_USED
// function prototype declaration
void _stdcall Callback(const AmsAddr*, const AdsNotificationHeader*, unsigned int);


void _stdcall Callback(const AmsAddr* pAddr, const AdsNotificationHeader* pNotification, unsigned int hUser)
{
	int index;
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_TwinCATAds.cpp : Callback invoked");
	//UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"Value 	: %d ", *(unsigned long int *)pNotification->data);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Notification : %d ", pNotification->hNotification);

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SampleSize : %d ", pNotification->cbSampleSize);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "hUser : %d", hUser);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "ServerNetId : ");
		for (index = 0; index < 6; index++)
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "%d.%d", (int)pAddr->netId.b[index], pAddr->port);
}
#endif


void NotifyCallback(const AmsAddr *pAddr, const AdsNotificationHeader *pNotification, uint32_t hUser)
{
    bool boolData{};
    uint8_t uint8Data{};
    uint16_t uint16Data{};
    uint32_t uint32Data{};
    uint64_t uint64Data{};
    double doubleData{};
    float floatData{};
    int8_t int8Data{};
    int16_t int16Data{};
    int32_t int32Data{};
    int64_t int64Data{};

    int i = 0;
    UA_NodeId nodeId;
    UA_Server *uaServer;
    UA_Variant variantType;


/*
step 1: 
*/
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : Entering NotifyCallback() function");

    // initialise placeholder for a variant
    UA_Variant_init(&variantType);
    // run through the array to find a matching nodeId based on the notification event->attrib (_name)
    while (i < adsVarCounter)
    {
	if (adsVariables[i]._variableType != UA_TYPES_BOOLEAN)
	{
		i++;
		continue;
	}
	std::memcpy(&boolData, pNotification + 1, std::min<size_t>(sizeof(boolData), pNotification->cbSampleSize));
	std::cout << "value: "  << +boolData << "\n";
	#ifdef KIV
	if (adsVariables[i]._name = pNotification + 1)	// found a matching record
	{
	    	// update OPCUA server based on the variable and value received in the callback
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : Found a matching record in the adsVariables array.  Updating OPCUA node...");

		// prepare the data : change ADS datatype to VARIANT
		if (adsVariables[i]._variableType == UA_TYPES_BOOLEAN)
		{
			std::memcpy(&boolData, pNotification + 1, std::min<size_t>(sizeof(boolData), pNotification->cbSampleSize));
			UA_Variant_setScalar(&variantType, &boolData, &UA_TYPES[adsVariables[i]._variableType]);
		}
		else if (adsVariables[i]._variableType == UA_TYPES_BYTE)
		{
			std::memcpy(&uint8Data, pNotification + 1, std::min<size_t>(sizeof(uint8Data), pNotification->cbSampleSize));
			UA_Variant_setScalar(&variantType, &uint8Data, &UA_TYPES[adsVariables[i]._variableType]);
		}

		// finally update the new data received from TwinCAT ADS into OPC Server
		UA_Server_writeValue(adsVariables[i].uaServer, adsVariables[i].nodeId, variantType);
		break;
	}
	#endif
   	i++;
	/*
	// sample code
	if (variableType == uint8_t)
    	{
		uint8DataType data{};
		std::memcpy(&uint8Data, pNotification + 1, std::min<size_t>(sizeof(uint8Data), pNotification->cbSampleSize));

		std::cout << std::setfill('0') <<
			"NetId: " << pAddr->netId <<
			" hUser 0x" << std::hex << hUser <<
			" time: " << std::dec << pNotification->nTimeStamp <<
			" size: " << std::dec << pNotification->cbSampleSize <<
			" value:" << " 0x" << std::hex << +data <<
			'\n';
    	}
    	else if (variableType == uint16_t)
    	{
		uint16DataType data{};
		std::memcpy(&uint16Data, pNotification + 1, std::min<size_t>(sizeof(uint16Data), pNotification->cbSampleSize));

        	std::cout << std::setfill('0') <<
                	"NetId: " << pAddr->netId <<
	                " hUser 0x" << std::hex << hUser <<
        	        " time: " << std::dec << pNotification->nTimeStamp <<
                	" size: " << std::dec << pNotification->cbSampleSize <<
	                " value:" << " 0x" << std::hex << +data <<
        	        '\n';
    	}
	*/
    } // while
}

void CB_TwinCATAdsUpdate()
{
	// this is a brute force inefficient way of update OPCUA nodes with all the twinCAT variables via ADS.
	int i;
	UA_Variant variantType;
    	// initialise placeholder for a variant
    	UA_Variant_init(&variantType);

	//UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : CB_TwinCATAdsUpdate() Begin updating all the OPC Nodes using adsVariables array");
}


void notificationByName(std::ostream& out, const AdsDevice& route, std::string variableName, int variableType, std::string iGroup)
{

	#ifdef DEBUG
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : Entering notificationByName() function :");
	#endif
	std::string _iGroup1 = "0xf020", _iGroup2 = "0xf030";
	if ( (iGroup == _iGroup1) || (iGroup == _iGroup2) )
	{
		#ifdef DEBUG
		out << "variableName passed in is " << variableName << " \n";
        	out << "variableType passed in is " << variableType << " \n";
		#endif
	}

	AdsNotificationAttrib attrib = {};
	switch (variableType)
	{
		case UA_TYPES_BOOLEAN :
                        attrib = {
                                sizeof(bool),
                                ADSTRANS_SERVERCYCLE,
                                0,
                                {4000000}
                                };
                        break;

		case UA_TYPES_BYTE : // variableType = uint8_t;
			attrib = {
        			sizeof(uint8_t),
        			ADSTRANS_SERVERCYCLE,
        			0,
        			{4000000}
    				};
			break;
		case UA_TYPES_UINT16 : // dataType = uint16_t;
			attrib = {
                                sizeof(uint16_t),
                                ADSTRANS_SERVERCYCLE,
                                0,
                                {4000000}
                                };
			break;
		case UA_TYPES_UINT32 : // dataType = uint32_t;
			attrib = {
                                sizeof(uint32_t),
                                ADSTRANS_SERVERCYCLE,
                                0,
                                {4000000}
                                };
			break;
		case UA_TYPES_UINT64 : // dataType = uint64_t;
			attrib = {
                                sizeof(uint64_t),
                                ADSTRANS_SERVERCYCLE,
                                0,
                                {4000000}
                                };
			break;

		case UA_TYPES_INT16 :
                        attrib = {
                                sizeof(int16_t),
                                ADSTRANS_SERVERCYCLE,
                                0,
                                {4000000}
                                };
                        break;

		case UA_TYPES_INT32 :
                        attrib = {
                                sizeof(int32_t),
                                ADSTRANS_SERVERCYCLE,
                                0,
                                {4000000}
                                };
                        break;

		case UA_TYPES_INT64 :
                        attrib = {
                                sizeof(int64_t),
                                ADSTRANS_SERVERCYCLE,
                                0,
                                {4000000}
                                };
                        break;

		case UA_TYPES_FLOAT :
                        attrib = {
                                sizeof(float),
                                ADSTRANS_SERVERCYCLE,
                                0,
                                {4000000}
                                };
                        break;

		case UA_TYPES_DOUBLE :
                        attrib = {
                                sizeof(double),
                                ADSTRANS_SERVERCYCLE,
                                0,
                                {4000000}
                                };
                        break;

	}

    //out << __FUNCTION__ << "():\n";
    AdsNotification notification { route, variableName, attrib, &NotifyCallback, 0xBEEFDEAD };

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : %s() notification added for %s", __FUNCTION__, variableName.c_str());

}

std::string removeTrailingSpaces(std::string str) {
  str.erase(std::find_if(str.rbegin(), str.rend(), [](int ch) {
    return !std::isspace(ch);
  }).base(), str.end());
  return str;
}

std::string removeLeadingSpaces(std::string str) {
  str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
    return !std::isspace(ch);
  }));
  return str;
}

int *extractAMS(char* ams) {
    char ip_address[] = "5.40.64.12.1.1";
    int *numbers;
    int i = 0;
    char *token;
    char *rest = ams; //ip_address;

	numbers = (int *)calloc(6, sizeof(int));
    while ((token = strtok_r(rest, ".", &rest)) && i < 6) {
        if(isdigit(token[0]))
            numbers[i++] = atoi(token);
    }

    return numbers;
}
//twincat_ads *ConnectToTwinCATAds(char* remoteIpV4)
int ConnectToTwinCATAds(UA_Server *uaServer)//char* remoteIpV4, char* ams, int nPlcPort, UA_Server *uaServer)
{
	#ifdef COMMENT
	1. open TwinCAT3->TwinCAT MCU288 project file
	2. choose "Load from Target" (MCU288 - CX9020)
	3. check : Build 4024.53
	4. enable PLC->SolenoidValve
	5. check : POU -> MAIN variables declared at el2024_1 to el2024_4
	#endif


	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : Entering ConnectToTwinCATADS() function");

	char *env_AMSremoteIpV4;
	char *env_AMSremoteNetId;
	char *env_AMSport;
	char *env_AMSTcpPort;

	env_AMSremoteIpV4 = getenv("SVR_AMS_REMOTEIPV4");
	if (env_AMSremoteIpV4 != NULL)
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : retrieved environment variable : SVR_AMS_REMOTEIPV4 : %s", env_AMSremoteIpV4);
	else
	{
		UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : cannot retrieve environment variable <SVR_AMS_REMOTEIPV4>");;
		UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : default to 192.168.1.110");
                env_AMSremoteIpV4 = (char*)calloc(15, sizeof(char));
                if (env_AMSremoteIpV4 != NULL)
                        strcpy(env_AMSremoteIpV4, "192.168.1.110");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : cannot retrieve environment variable <SVR_AMS_REMOTEIPV4> :  out of memory");
                        return UA_FALSE;
                }
        }

        env_AMSremoteNetId = getenv("SVR_AMS_REMOTENETID"); // example "5.40.64.12.1.1"
        if (env_AMSremoteNetId != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : retrieved environment variable : SVR_AMS_REMOTENETID : %s", env_AMSremoteNetId);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATADS.cpp : cannot retrieve environment variable <SVR_AMS_REMOTENETID>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATADS.cpp : default to 5.40.64.12.1.1");
                env_AMSremoteNetId = (char*)calloc(20, sizeof(char));
                if (env_AMSremoteNetId != NULL)
			strcpy(env_AMSremoteNetId, "5.40.64.12.1.1");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATADS.cpp : cannot retrieve environment variable <SVR_AMS_REMOTENETID> :  out of memory");
                        return UA_FALSE;
                }
        }

	//UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : ADS (remoteIpV4)and PLC port passed in is %s %s %d", remoteIpV4, ams, nPlcPort);

	env_AMSport = getenv("SVR_AMS_PORT");	// 852
        if (env_AMSport != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : retrieved environment variable : SVR_AMS_PORT : %s", env_AMSport);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATADS.cpp : cannot retrieve environment variable <SVR_AMS_PORT>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATADS.cpp : default to 852");
                env_AMSport = (char*)calloc(20, sizeof(char));
                if (env_AMSport != NULL)
			strcpy(env_AMSport, "852");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATADS.cpp : cannot retrieve environment variable <SVR_AMS_PORT> :  out of memory");
                        return UA_FALSE;
                }
        }

	env_AMSTcpPort = getenv("SVR_AMS_TCPPORT");	// 48898
	if (env_AMSTcpPort != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : retrieved environment variable : SVR_AMS_TCPPORT : %s", env_AMSTcpPort);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATADS.cpp : cannot retrieve environment variable <SVR_AMS_TCPPORT >");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATADS.cpp : default to 48898");
                env_AMSTcpPort = (char*)calloc(20, sizeof(char));
                if (env_AMSTcpPort != NULL)
                        strcpy(env_AMSTcpPort, "48898");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATADS.cpp : cannot retrieve environment variable <SVR_AMS_TCPPORT> :  out of memory");
                        return UA_FALSE;
                }
        }
	if (check_port_active(env_AMSremoteIpV4, atoi(env_AMSTcpPort)) != 0)   // TwinCAT ADS/AMS tcp port
	{
		UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATADS.cpp : TwinCAT AMS is not running/ active");
		UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATADS.cpp : No data from TwinCAT (%s) will be populated to OPC Server", env_AMSremoteIpV4);

		exit(UA_FALSE);
        }

	// taken from https://github.com/Beckhoff/ADS/blob/master/example/example.cpp

	// create a route from RPI to TwinCAT CX
	//#define REMOTE_NET_ID { 5, 40, 64, 12, 1, 1 }
	int *t_ams = extractAMS(env_AMSremoteNetId);
	char real_ams[30];
	int isFileCreated = 0;

	sprintf(real_ams, "%d, %d, %d, %d, %d, %d", t_ams[0],t_ams[1], t_ams[2], t_ams[3], t_ams[4], t_ams[5]);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : ams : %d.%d.%d.%d.%d.%d", t_ams[0],t_ams[1], t_ams[2], t_ams[3], t_ams[4], t_ams[5]);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : real_ams : %s", real_ams);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : env_AMSremoteIpV4 : <%s>", env_AMSremoteIpV4);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : atoi(env_AMSport) : <%d>", atoi(env_AMSport));

	auto device = bhf::ads::SymbolAccess{ env_AMSremoteIpV4, {real_ams}, (uint16_t)atoi(env_AMSport) };
	auto mymap = device.FetchSymbolEntries();			// Reading symbol info failed with 0x745 - error code = 1861 (timeout elapse) : only if i introduce bFirstTime:BOOL into TwinCAT POU
	auto size = mymap.size();	// returns the number of symbols
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : Number of symbols : %d", size);

	// create an array of structure of type "_twincatAdsVariables"
	// identifier is declared as global "adsVariables"
	// with every variable read from TwinCAT via ADS, a copy is saved to the array.
	// This is to facilitate searching for UA_NodeId based on ADS->attrib (_name);
	adsVariables = new twincatAdsVariables[size];
	if (!adsVariables)
	{
		UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : out of memory when allocating array of adsVariables");
		return UA_FALSE;
	}
	adsVarCounter = 0 ; // initialisation


	// redirect cout to oss
	std::ostringstream oss;
	std::streambuf* originalCoutBuffer = std::cout.rdbuf(); // store the original cout buffer
	std::cout.rdbuf(oss.rdbuf());	// redirect cout to the stringstream
	device.ShowSymbols(std::cout);
	std::string twinCATADSString = oss.str(); // retrieve the string from the stringstream
	std::stringstream twinCATADSStringstream(twinCATADSString); // convert string to stringstream
	std::cout.rdbuf(originalCoutBuffer);	// restores the original cout buffer

	#ifdef DEBUG
	// check the contents of the oss string
	std::cout << "Captured string: " << twinCATADSString << std::endl;
	#endif
	// process the stream
	// Global variables : iGroup = 0xf020
	// Libraries used : iGroup = 0x4040
	// Variables : iGroup = 0xf030
	// System flags : iGroup 0x4040

	AmsNetId remoteNetId { real_ams }; /* { 5, 40, 64, 12, 1, 1 }; */	// equivalent to uint32_t, refers to the CX running TwinCAT
	// bhf::ads::SetLocalAddress({192, 168, 1, 109, 1, 1});			// refers to the ADS client
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : Adding an ADS route to device <%s> ...", env_AMSremoteIpV4);
	AdsDevice route { env_AMSremoteIpV4, REMOTE_NET_ID, (uint16_t)atoi(env_AMSport) }; //REMOTE_NET_ID, AMSPORT_SOLENOIDVALVE };
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : ADS route for device <%s> initialised successfully", env_AMSremoteIpV4);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : trying to get the state of the AMS route ...");

	const auto state = route.GetState();
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : ADS state: %d, devState: %d ", (uint16_t)state.ads, (uint16_t)state.device);
	    //out << "ADS state: " << std::dec << (uint16_t)state.ads << " devState: " << std::dec << (uint16_t)state.device << '\n';
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp::%s() -> trying to read variables ...", __FUNCTION__);
	AdsVariable<bool> readVar1 {route, "MAIN.el2024_1"};
	AdsVariable<bool> readVar2 {route, "MAIN.el2024_2"};
	AdsVariable<bool> readVar3 {route, "MAIN.el2024_3"};
	AdsVariable<bool> readVar4 {route, "MAIN.el2024_4"};
	AdsVariable<int16_t> readVar4a {route, "MAIN.myInteger"};
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : ADS read bool & int : %d %d %d %d %d ", (bool)readVar1, (bool)readVar2, (bool)readVar3, (bool)readVar4, (int32_t)readVar4a);

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : -> trying to write variables ADS write bool test ");
	readVar1 = UA_TRUE;
	readVar2 = UA_TRUE;
	readVar3 = UA_FALSE;
	readVar4 = UA_TRUE;

	#ifdef DEBUG
	//int key;
	//printf("Press any key to continue ...");
	//scanf("%d", &key);
	#endif

	AdsVariable<int16_t> readVar5 {route, "GVL.Temp_probe1"};
	AdsVariable<int16_t> readVar6 {route, "GVL.Temp_probe2"};
	AdsVariable<int16_t> readVar7 {route, "GVL.Temp_probe3"};
	AdsVariable<int16_t> readVar8 {route, "GVL.Temp_probe4"};
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : ADS read int : %d %d %d %d ", (int)readVar5, (int)readVar6, (int)readVar7, (int)readVar8);
	#ifdef DEBUG
	int key;
	printf("Press any key to continue ..."); scanf("%d", &key);
	#endif
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp::%s() ->finished reading variables ...", __FUNCTION__);
	// sample from https://stackoverflow.com/questions/57525108/can-i-query-the-list-of-variables-and-types
	UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : ---------------stackoverflow example-----------------------");

	/*AmsAddr Addr  { {5, 40, 64, 12, 1, 1}, AMSPORT_R0_PLC_TC3 }; // type is AmsAddr*/
	AmsAddr Addr { { real_ams }, (uint16_t) atoi(env_AMSport) };
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : real_ams and plc_port : { %s }, %d", real_ams, atoi(env_AMSport));
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : AMS Addr initialised successfully");

	#ifdef NO_NEED
	FILE *fp;
	fp = fopen("TwinCATAds.txt", "w+");
	if (!fp)
	{
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : Cannot create file : TwinCATAds.txt");
		exit(-1);
	}
	else
	{
		fputs(device.ShowSymbols(std::cout), fp);
		isFileCreated = 1;
	}
	#endif

	#ifdef NO_NEED
	FILE *fp;
	std::ofstream outputFile;
	char output[32767];

	fp = fopen("TwinCATAds.txt", "w+");
	outputFile.open("TwinCATAds.txt");
	if (outputFile.is_open())
	//if (fp != NULL)
	{
		//auto device = bhf::ads::SymbolAccess{ remoteIpV4, {real_ams}, (uint16_t)nPlcPort };
		//device.ShowSymbols(std::cout);

		//!!!!!! something is not correct .. only wrote 2 bytes
		#ifdef DEBUG
		device.ShowSymbols(std::cout);
		#endif

		//char *filebuffer;
		//filebuffer = (char *)malloc(sizeof(device.ShowSymbols(std::cout)));
		//strcpy(filebuffer, "This is a test input");

	#ifdef NO_NEED
		outputFile << device.ShowSymbols(std::cout) << std::endl;
	#endif
		//fwrite(output sizeof(output), 1, fp);
		outputFile.close();
		isFileCreated = 1;
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : file created : TwinCATAds.txt");
	}
	else
	{
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : Cannot create file : TwinCATAds.txt");
		std::cout << "Cannot open TwinCATAds.txt for writing" << "\n" ;
		return UA_FALSE;
	}
	#endif

	if (UA_TRUE) //(isFileCreated == 1)
	{	int x1, counter=0, block=0;
		std::ifstream inputFile;
		inputFile.open("TwinCATAds.txt.new");
		if (UA_TRUE)//(inputFile.is_open())
		{
			int firstTimeAddADSToUAServer = UA_TRUE;
			size_t namespaceIndex;
			int retval;
			char sNameSpace[] = "TwinCATAds";
			char sNameSpaceEx[] = "virtualskies.com.sg/MKS/";
			UA_NodeId r1_twincatADSroot_Id;  /* get the nodeid assigned by the server */

			std::string word;
			std::string line;
			std::string _name, _typeName, _comment, _entryLength, _iGroup, _iOffs, _size, _dataType, _nameLength, _typeLength, _commentLength;
			int _variableType;

			if (firstTimeAddADSToUAServer)
			{
				// add root node
				UA_LOG_INFO(UA_Log_Stdout,UA_LOGCATEGORY_USERLAND,
					("--------SV_TwinCATAds.cpp : Begin the process of adding to OPC UA Tree structure"));
				UA_Server_getNamespaceByName(uaServer, UA_STRING(sNameSpaceEx), &namespaceIndex);
			        // Add new object called TwinCAT in OPCUA tree
				UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
				retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, sNameSpace),
				    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),               // parentNodeId
			       	    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),                // referenceTypeId original UA_NS0ID_ORGANIZES
			            UA_QUALIFIEDNAME(namespaceIndex, sNameSpace),        // variable
			            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),              // typeDefinition this refers to the Object Type identifier
			            oAttr, NULL, &r1_twincatADSroot_Id);                           // attr, nodeContext, outNewNodeId assigned by the server
			        if (retval != UA_STATUSCODE_GOOD)
				{
			           UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			                "--------SV_TwinCATAds.cpp : Error creating object node : Root Node TwinCAT : %s", UA_StatusCode_name(retval));
				}
				else
				{
					firstTimeAddADSToUAServer = UA_FALSE;
					UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
						"--------SV_TwinCATAds.cpp : Root Node TwinCAT added successfully : %s", r1_twincatADSroot_Id);
				}
			}

			// scan through the file and extract base on iGroup
			//while (inputFile >> word)	// read from a fixed file
			while (twinCATADSStringstream >> word) // read from buffer
			{
				#ifdef DEBUG
				 UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                "--------SV_TwinCATAds.cpp : Entering while loop : processing twinCATADSStringStream ");
				#endif
				//std::cout << counter << "\n";

				x1 = word.compare("{");
				if (x1 == 0) continue;

				x1 = word.compare("\"name\":");
				if (x1 == 0)
				{
					std::getline(twinCATADSStringstream, _name);	//std::getline(inputFile, _name);
                                        _name.erase(std::remove(_name.begin(), _name.end(), '"'), _name.end());
                                        _name.erase(std::remove(_name.begin(), _name.end(), ','), _name.end());
					//std::cout << "Retrieved std::getline is " << line << "\n";

					//inputFile >> _name;
					//std::cout << "Found : name is " << _name << "\n";
					continue;
				}

				x1 = word.compare("\"typeName\":");
				if (x1 == 0)
				{
					std::getline(twinCATADSStringstream, _typeName); //std::getline(inputFile, _typeName);
                                        _typeName.erase(std::remove(_typeName.begin(), _typeName.end(), '"'), _typeName.end());
                                        _typeName.erase(std::remove(_typeName.begin(), _typeName.end(), ','), _typeName.end());

					//inputFile >> _typeName;
					//std::cout << "Found : typeName is " << _typeName << "\n";
					continue;
				}

				x1 = word.compare("\"comment\":");
				if (x1 == 0)
				{
					std::getline(twinCATADSStringstream, _comment); //std::getline(inputFile, _comment);
                                        _comment.erase(std::remove(_comment.begin(), _comment.end(), '"'), _comment.end());
                                        _comment.erase(std::remove(_comment.begin(), _comment.end(), ','), _comment.end());

					//std::cout << "Found : comment is " << _comment << "\n";
					continue;
				}

				x1 = word.compare("\"entryLength\":");
				if (x1 == 0)
				{
					std::getline(twinCATADSStringstream, _entryLength); //std::getline(inputFile, _entryLength);
                                        _entryLength.erase(std::remove(_entryLength.begin(), _entryLength.end(), '"'), _entryLength.end());
                                        _entryLength.erase(std::remove(_entryLength.begin(), _entryLength.end(), ','), _entryLength.end());

					//std::cout << "Found : entryLength is " << _entryLength << "\n";
					continue;
				}

				x1 = word.compare("\"iGroup\":");
				if (x1 == 0)
				{
					std::getline(twinCATADSStringstream, _iGroup); //std::getline(inputFile, _iGroup);
                                        _iGroup.erase(std::remove(_iGroup.begin(), _iGroup.end(), '"'), _iGroup.end());
                                        _iGroup.erase(std::remove(_iGroup.begin(), _iGroup.end(), ','), _iGroup.end());

					//std::cout << "Found : iGroup is " << _iGroup << "\n";
					continue;
				}

                                x1 = word.compare("\"iOffs\":");
                                if (x1 == 0)
				{
                                	std::getline(twinCATADSStringstream, _iOffs); //std::getline(inputFile, _iOffs);
                                        _iOffs.erase(std::remove(_iOffs.begin(), _iOffs.end(), '"'), _iOffs.end());
                                        _iOffs.erase(std::remove(_iOffs.begin(), _iOffs.end(), ','), _iOffs.end());
					//std::cout << "Found : iOffs is " << _iOffs << "\n";
					continue;
				}

                                x1 = word.compare("\"size\":");
                                if (x1 == 0)
				{
					std::getline(twinCATADSStringstream, _size); //std::getline(inputFile, _size);
                                        _size.erase(std::remove(_size.begin(), _size.end(), '"'), _size.end());
                                        _size.erase(std::remove(_size.begin(), _size.end(), ','), _size.end());
					//std::cout << "Found : size is " << _size << "\n";
					continue;
				}

                                x1 = word.compare("\"dataType\":");
                                if (x1 == 0)
                                {
                                        std::getline(twinCATADSStringstream, _dataType); //std::getline(inputFile, _dataType);
                                        _dataType.erase(std::remove(_dataType.begin(), _dataType.end(), '"'), _dataType.end());
                                        _dataType.erase(std::remove(_dataType.begin(), _dataType.end(), ','), _dataType.end());
                                        //std::cout << "Found : dataType is " << _dataType << "\n";
                                        continue;
                                }
                                x1 = word.compare("\"nameLength\":");
                                if (x1 == 0)
                                {
                                        std::getline(twinCATADSStringstream, _nameLength); //std::getline(inputFile, _nameLength);
                                        _nameLength.erase(std::remove(_nameLength.begin(), _nameLength.end(), '"'), _nameLength.end());
                                        _nameLength.erase(std::remove(_nameLength.begin(), _nameLength.end(), ','), _nameLength.end());
                                        //std::cout << "Found : nameLength is " << _nameLength << "\n";
                                        continue;
                                }
                                x1 = word.compare("\"typeLength\":");
                                if (x1 == 0)
                                {
                                        std::getline(twinCATADSStringstream, _typeLength); //std::getline(inputFile, _typeLength);
					_typeLength.erase(std::remove(_typeLength.begin(), _typeLength.end(), '"'), _typeLength.end());
					_typeLength.erase(std::remove(_typeLength.begin(), _typeLength.end(), ','), _typeLength.end());
                                        //std::cout << "Found : typeLength is " << _typeLength << "\n";
                                        continue;
                                }
                                x1 = word.compare("\"commentLength\":");
                                if (x1 == 0)
                                {
                                        std::getline(twinCATADSStringstream, _commentLength); //std::getline(inputFile, _commentLength);
                                        _commentLength.erase(std::remove(_commentLength.begin(), _commentLength.end(), '"'), _commentLength.end());
                                        _commentLength.erase(std::remove(_commentLength.begin(), _commentLength.end(), ','), _commentLength.end());
                                        //std::cout << "Found : commentLength is " << _commentLength << "\n";
                                        continue;
                                }

                                x1 = word.compare("}");
                                if (x1 == 0)
				{
					// start processing the data read from the stringstream/ file
					#ifdef DEBUG
					std::cout << "{" << "\n";
                                        std::cout << "Found : name is <" << _name << ">\n";
                                        std::cout << "Found : typeName is <" << _typeName << ">\n";
	                                std::cout << "Found : comment is <" << _comment << ">\n";
                                        std::cout << "Found : entryLength is <" << _entryLength << ">\n";
                                        std::cout << "Found : iGroup is <" << _iGroup << ">\n";
                                        std::cout << "Found : iOffs is <" << _iOffs << ">\n";
                                        std::cout << "Found : size is <" << _size << ">\n";
                                        std::cout << "Found : dataType is <" << _dataType << ">\n";
                                        std::cout << "Found : nameLength is <" << _nameLength << ">\n";
                                        std::cout << "Found : typeLength is ," << _typeLength << ">\n";
					std::cout << "Found : commentLength is <" << _commentLength << ">\n";
					std::cout << "}" << "\n";
					#endif

					// remove leading and trailing spaces
                                        std::string _name_xleadingspaces = removeLeadingSpaces(_name);
                                        _name = removeTrailingSpaces(_name_xleadingspaces);

					// write to OPC Server
					std::string str1 (_iGroup);
					std::string str2 ("0xf020");	// variables tied to terminals
					std::string str3 ("0xf030");	// variables tied to terminals
					std::string str4 ("0x4040");	// system variables

				 	//if ( (str1.compare(str2) == 0) || (str1.compare(str3) == 0) || (str1.compare(str4) == 0) )
					//{
						// "0xf020" : global variables
						// "0xf030" : local variables
						// "0x4040" : // others
					//else
					//{
						// SubTree : TwinCAT-> Variable Name
						char sLocale[] = "en-US";

						UA_NodeId r2_twincat_variableName_Id;
						UA_ObjectAttributes oAttr_r2_twincat_variableName = UA_ObjectAttributes_default;
						char* _nameNew = new char[_name.length() + 1];
						std::strcpy(_nameNew, _name.c_str());
						retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, _nameNew),
						    r1_twincatADSroot_Id,
						    UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
						    UA_QUALIFIEDNAME(namespaceIndex, _nameNew),
						    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
						    oAttr_r2_twincat_variableName, NULL, &r2_twincat_variableName_Id);
						if (retval != UA_STATUSCODE_GOOD)
						     UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
					                "Error creating object node : TwinCAT_VariableName : %s", UA_StatusCode_name(retval));
						#ifdef DEBUG
					        else
					            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
					                "Object Node created : TwinCAT->VariableName Node : %d", r2_twincat_variableName_Id.identifier.numeric);      //, r1_airgardroot_Id);
					        #endif

						// variable
						// Within the block, NodeId is 20,000, 20,001, 20,002, ... 20,011
						// every block created the NodeId increases by 100, 20,000, 20100, 20200, ... up to nBlocks

						// add variable nodes
					        //UA_NodeId SoftwareVersionVariableType = UA_NODEID_NULL;
						char sName[] = "Name";
						char sNameEx[] = "01. Name";
					        UA_VariableAttributes vNameAttr = UA_VariableAttributes_default;
					        vNameAttr.description = UA_LOCALIZEDTEXT(sLocale, sName);
					        vNameAttr.displayName = UA_LOCALIZEDTEXT(sLocale, sNameEx);
					        vNameAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
					        vNameAttr.valueRank = UA_VALUERANK_SCALAR;
					        vNameAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
					        vNameAttr.historizing = UA_FALSE;
					        UA_String Name = UA_STRING(_nameNew);
					        //if (!UA_Variant_isEmpty(&vSoftwareVersionAttr.value))
					                UA_Variant_setScalar(&vNameAttr.value, &Name, &UA_TYPES[UA_TYPES_STRING]);
					        //else UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
					        //        "Error UA_Variant_setScalar() : SoftwareVersion 10001");
					        //https://github.com/open62541/open62541(Example Server Implementation); UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
					        retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 20000+block+counter),          // NodeId
					            r2_twincat_variableName_Id,                          // parent NodeId
					            UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
					            UA_QUALIFIEDNAME(namespaceIndex, sName),       //UA_QUALIFIEDNAME(1, "Software Version"),      // Qualified Name
					            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),        //UA_NODEID_NULL
					            vNameAttr, NULL, &outName_Id);
					        if (retval != UA_STATUSCODE_GOOD)
						{
					            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
					                "Error creating node : Name %d: %s", 20000+counter, UA_StatusCode_name(retval));
						}
					        #ifdef DEBUG
					        else
					            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
					                "Attribute Variable created: TwinCAT->Name %d : %d", 20000+block+counter, r1_twincatroot_Id.identifier.numeric);
					        #endif
						free(_nameNew);

						counter++;
						char sTypeName[] = "TypeName";
						char sTypeNameEx[] = "02. Type Name";
                                                UA_VariableAttributes vTypeNameAttr = UA_VariableAttributes_default;
                                                vTypeNameAttr.description = UA_LOCALIZEDTEXT(sLocale, sTypeName);
                                                vTypeNameAttr.displayName = UA_LOCALIZEDTEXT(sLocale, sTypeNameEx);
                                                vTypeNameAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
                                                vTypeNameAttr.valueRank = UA_VALUERANK_SCALAR;
                                                vTypeNameAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                                vTypeNameAttr.historizing = UA_FALSE;

                                                char* _typeNameNew = new char[_typeName.length() + 1];
                                                std::strcpy(_typeNameNew, _typeName.c_str());
                                                UA_String TypeName = UA_STRING(_typeNameNew);
                                                //if (!UA_Variant_isEmpty(&vSoftwareVersionAttr.value))
                                                        UA_Variant_setScalar(&vTypeNameAttr.value, &TypeName, &UA_TYPES[UA_TYPES_STRING]);
                                                //else UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                //        "Error UA_Variant_setScalar() : SoftwareVersion 10001");
                                                //https://github.com/open62541/open62541(Example Server Implementation); UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 20000+block+counter),          // NodeId
                                                    r2_twincat_variableName_Id,                          // parent NodeId
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                    UA_QUALIFIEDNAME(namespaceIndex, sTypeName),       //UA_QUALIFIEDNAME(1, "Software Version"),      // Qualified Name
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),        //UA_NODEID_NULL
                                                    vTypeNameAttr, NULL, &outTypeName_Id);
                                                if (retval != UA_STATUSCODE_GOOD)
                                                    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "Error creating node : TypeName %d: %s", 20000+counter, UA_StatusCode_name(retval));
                                                #ifdef DEBUG
                                                else
                                                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "Attribute Variable created: TwinCAT->Name %d : %d", 20000+block+counter, r1_twincatroot_Id.identifier.numeric);
                                                #endif
						free(_typeNameNew);

						counter++;
						char sComment[] = "Comment";
						char sCommentEx[] = "03. Comment";
                                                UA_VariableAttributes vCommentAttr = UA_VariableAttributes_default;
                                                vCommentAttr.description = UA_LOCALIZEDTEXT(sLocale, sComment);
                                                vCommentAttr.displayName = UA_LOCALIZEDTEXT(sLocale, sCommentEx);
                                                vCommentAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
                                                vCommentAttr.valueRank = UA_VALUERANK_SCALAR;
                                                vCommentAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                                vCommentAttr.historizing = UA_FALSE;

                                                char* _commentNew = new char[_comment.length() + 1];
                                                std::strcpy(_commentNew, _comment.c_str());
                                                UA_String Comment = UA_STRING(_commentNew);
                                                //if (!UA_Variant_isEmpty(&vSoftwareVersionAttr.value))
                                                        UA_Variant_setScalar(&vCommentAttr.value, &Comment, &UA_TYPES[UA_TYPES_STRING]);
                                                //else UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                //        "Error UA_Variant_setScalar() : SoftwareVersion 10001");
                                                //https://github.com/open62541/open62541(Example Server Implementation); UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 20000+block+counter),          // NodeId
                                                    r2_twincat_variableName_Id,                          // parent NodeId
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                    UA_QUALIFIEDNAME(namespaceIndex, sComment),       //UA_QUALIFIEDNAME(1, "Software Version"),      // Qualified Name
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),        //UA_NODEID_NULL
                                                    vCommentAttr, NULL, &outComment_Id);
                                                if (retval != UA_STATUSCODE_GOOD)
                                                    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "Error creating node : Comment %d: %s", 20000+counter, UA_StatusCode_name(retval));
                                                #ifdef DEBUG
                                                else
                                                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "Attribute Variable created: TwinCAT->Comment %d : %d", 20000+block+counter, r1_twincatroot_Id.identifier.numeric);
                                                #endif
						free(_commentNew);

                                                counter++;
						char sEntryLength[] = "EntryLength";
						char sEntryLengthEx[] = "04. Entry Length";
                                                UA_VariableAttributes vEntryLengthAttr = UA_VariableAttributes_default;
                                                vEntryLengthAttr.description = UA_LOCALIZEDTEXT(sLocale, sEntryLength);
                                                vEntryLengthAttr.displayName = UA_LOCALIZEDTEXT(sLocale, sEntryLengthEx);
                                                vEntryLengthAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
                                                vEntryLengthAttr.valueRank = UA_VALUERANK_SCALAR;
                                                vEntryLengthAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                                vEntryLengthAttr.historizing = UA_FALSE;

                                                char* _entryLengthNew = new char[_entryLength.length() + 1];
                                                std::strcpy(_entryLengthNew, _entryLength.c_str());
                                                UA_String EntryLength = UA_STRING(_entryLengthNew);
                                                //if (!UA_Variant_isEmpty(&vSoftwareVersionAttr.value))
                                                        UA_Variant_setScalar(&vEntryLengthAttr.value, &EntryLength, &UA_TYPES[UA_TYPES_STRING]);
                                                //else UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                //        "Error UA_Variant_setScalar() : SoftwareVersion 10001");
                                                //https://github.com/open62541/open62541(Example Server Implementation); UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 20000+block+counter),          // NodeId
                                                    r2_twincat_variableName_Id,                          // parent NodeId
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                    UA_QUALIFIEDNAME(namespaceIndex, sEntryLength),       //UA_QUALIFIEDNAME(1, "Software Version"),      // Qualified Name
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),        //UA_NODEID_NULL
                                                    vEntryLengthAttr, NULL, &outEntryLength_Id);
                                                if (retval != UA_STATUSCODE_GOOD)
                                                    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "Error creating node : EntryLength %d: %s", 20000+counter, UA_StatusCode_name(retval));
                                                #ifdef DEBUG
                                                else
                                                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "Attribute Variable created: TwinCAT->EntryLength %d : %d", 20000+block+counter, r1_twincatroot_Id.identifier.numeric);
                                                #endif
						free(_entryLengthNew);

                                                counter++;
						char siGroup[] = "iGroup";
						char siGroupEx[] ="05. iGroup";
                                                UA_VariableAttributes viGroupAttr = UA_VariableAttributes_default;
                                                viGroupAttr.description = UA_LOCALIZEDTEXT(sLocale, siGroup);
                                                viGroupAttr.displayName = UA_LOCALIZEDTEXT(sLocale, siGroupEx);
                                                viGroupAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
                                                viGroupAttr.valueRank = UA_VALUERANK_SCALAR;
                                                viGroupAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                                viGroupAttr.historizing = UA_FALSE;

                                                char* _igroupNew = new char[_iGroup.length() + 1];
                                                std::strcpy(_igroupNew, _iGroup.c_str());
                                                UA_String iGroup = UA_STRING(_igroupNew);
                                                //if (!UA_Variant_isEmpty(&vSoftwareVersionAttr.value))
                                                        UA_Variant_setScalar(&viGroupAttr.value, &iGroup, &UA_TYPES[UA_TYPES_STRING]);
                                                //else UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                //        "Error UA_Variant_setScalar() : SoftwareVersion 10001");
                                                //https://github.com/open62541/open62541(Example Server Implementation); UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 20000+block+counter),          // NodeId
                                                    r2_twincat_variableName_Id,                          // parent NodeId
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                    UA_QUALIFIEDNAME(namespaceIndex, siGroup),       //UA_QUALIFIEDNAME(1, "Software Version"),      // Qualified Name
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),        //UA_NODEID_NULL
                                                    viGroupAttr, NULL, &outiGroup_Id);
                                                if (retval != UA_STATUSCODE_GOOD)
                                                    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "Error creating node : iGroup %d: %s", 20000+counter, UA_StatusCode_name(retval));
                                                #ifdef DEBUG
                                                else
                                                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "Attribute Variable created: TwinCAT->iGroup %d : %d", 20000+block+counter, r1_twincatroot_Id.identifier.numeric);
                                                #endif
						free(_igroupNew);

						counter++;
						std::string _iGroup_xleadingspaces = removeLeadingSpaces(_iGroup);
						std::string _iGroup_xtrailingspaces = removeTrailingSpaces(_iGroup_xleadingspaces);

						#ifdef DEBUG
						std::cout << "_iGroup is (" << _iGroup << ")\n";
						#endif
						char s20[] = "0xf020";	// iGroup = 0xf020 => variable is tied to physical terminal
						char s30[] = "0xf030";	// iGroup = 0xf030 => variable is tied to physical terminal
						char s40[] = "0x4040";	// iGroup = 0x4040 => variable is NOT tied to any physical terminal
						//if ( (_iGroup.compare(s20)==0) || (_iGroup.compare(s30)==0) ) // GVL.variables || MAIN.variables
						int x_s20 = _iGroup_xtrailingspaces.compare(s20);
						int y_s30 = _iGroup_xtrailingspaces.compare(s30);
						int z_s40 = _iGroup_xtrailingspaces.compare(s40);
						#ifdef DEBUG
						std::cout << "x_s20 is " << x_s20 << "\n";
						std::cout << "y_s30 is " << y_s30 << "\n";
						std::cout << "z_s40 is " << z_s40 << "\n";
						#endif
						if (x_s20 == 0)
						//if ( (x_s20 == 0) || //( (x_s20 == 0) && (y_s30 == -1) && (z_s40 == 1) )
						//     (y_s30 == 0) ||
						//     (z_s40 == 0) )
						{
							#ifdef NOTE
		 					---------------------------check for GVL.variables (iGroup = 0xf020)
							#endif
							// create a new node (Value) to capture the 'value' of the TwinCAT variable
							UA_String Value;

							std::string _dataType_xleadingspaces = removeLeadingSpaces(_dataType);
							std::string _dataType_xtrailingspaces = removeTrailingSpaces(_dataType_xleadingspaces);

							char sValue[] = "Value";
							char sValueEx[] = "12. Value";
							UA_VariableAttributes vValueAttr = UA_VariableAttributes_default;
							vValueAttr.description = UA_LOCALIZEDTEXT(sLocale, sValue);
							vValueAttr.displayName = UA_LOCALIZEDTEXT(sLocale, sValueEx);
							vValueAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
							vValueAttr.historizing = UA_TRUE;

							// check the data type
							char s21[] = "0x21";	// BOOL
							char s11[] = "0x11";	// BYTE (same as BOOL)
							char s1e[] = "0x1e";	// STRING
							char s2[] = "0x2";	// INT => int16_t
							char s3[] = "0x3";	// DINT => int32_t
							char s4[] = "0x4";	// REAL => float
							char s5[] = "0x5";	// LREAL => double
							char s12[] = "0x12";	// WORD, UINT	=> uint16_t
							char s13[] = "0x13";	// DWORD, UDINT => uint32_t
							char s15[] = "0x15";	// LWORD, ULINT =? uint64_t
							char s41[] = "0x41";	// others

							std::cout << "<" << _dataType_xtrailingspaces << ">" << "\n";

							int a_s21 = _dataType_xtrailingspaces.compare(s21);
							int f_s11 = _dataType_xtrailingspaces.compare(s11);
							int b_s1e = _dataType_xtrailingspaces.compare(s1e);
							int c_s2 = _dataType_xtrailingspaces.compare(s2);
							int d_s3 = _dataType_xtrailingspaces.compare(s3);
							int g_s4 = _dataType_xtrailingspaces.compare(s4);
							int h_s5 = _dataType_xtrailingspaces.compare(s5);
							int i_s12 = _dataType_xtrailingspaces.compare(s12);
							int j_s13 = _dataType_xtrailingspaces.compare(s13);
							int k_s15 = _dataType_xtrailingspaces.compare(s15);
							int e_s41 = _dataType_xtrailingspaces.compare(s41);

							std::cout << "(***) entering IF iGroup = 0xf020 section (GVL.variables)" << "\n";
							#ifdef DEBUG
							std::cout << "a : <" << a_s21 << ">\n";
                                                        std::cout << "f : <" << f_s11 << ">\n";
							std::cout << "b : <" << b_s1e << ">\n";
							std::cout << "c : <" << c_s2 << ">\n";
							std::cout << "d : <" << d_s3 << ">\n";
                                                        std::cout << "g : <" << g_s4 << ">\n";
                                                        std::cout << "h : <" << h_s5 << ">\n";
                                                        std::cout << "i : <" << i_s12 << ">\n";
                                                        std::cout << "j : <" << j_s13 << ">\n";
                                                        std::cout << "k : <" << k_s15 << ">\n";
							std::cout << "e : <" << e_s41 << ">\n";
							#endif

                                                        if ( a_s21==0 ) //(_dataType == "0x021") => bool
                                                        {
                                                                bool bVar;
                                                                AdsVariable<bool> readVar {route, _name} ;
                                                        	std::cout << "in (if x_s20==0) and (if a_s21==0) : _name is : " << _name << ":" << readVar << "\t";
                                                                if (readVar)
                                                                        Value = UA_String_fromChars("0");
                                                                else
                                                                        Value = UA_String_fromChars("1");
								std::cout << "UA_String : Value is " << Value.data << " \n";
                                                                UA_Variant_setScalar(&vValueAttr.value, &Value, &UA_TYPES[UA_TYPES_BOOLEAN]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
								_variableType = UA_TYPES_BOOLEAN;
                                                        }
                                                        else if (b_s1e == 0) //(_dataType == "0x1e") => STRING
                                                        {
								std::cout << "in (if x_s20==0) and (b_s1e==0) : _name is : " << _name << "\n";
							#ifdef ERROR
                                                                std::string sVar;
                                                                AdsVariable<std::string> readVar {route, _name} ;
                                                                sVar = readVar;
                                                                std::cout << "in (if x_s20==0) and (b_s1e==0) : _name is : " << _name << ":" << sVar.c_str() << "\t";
							sleep(5);
                                                                UA_Variant_setScalar(&vValueAttr.value, &sVar, &UA_TYPES[UA_TYPES_STRING]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
							#endif
                                                        }
                                                        else if (c_s2 == 0) //(_dataType == "0x2") => INT, (int16_t)
                                                        {
                                                                short shbuf;
                                                                std::cout << "in (if x_s20==0) and (c_s2==0) : Symbol is <" << _name << ">\n";
                                                                AdsVariable<int16_t> readVar {route, _name} ;       // error code = 0
                                                                std::cout << "INT value is " << readVar << "\n";
                                                                shbuf = readVar;
                                                                UA_Variant_setScalar(&vValueAttr.value, &shbuf, &UA_TYPES[UA_TYPES_INT16]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
                                                        }
                                                        else if (d_s3 == 0) //(_dataType == "0x3") => DINT, (int32_t)
                                                        {
                                                                int dbuf;
                                                                std::cout << "in (if x_s20==0) and (d_s3==0) : Symbol is <" << _name << ">\n";
                                                                AdsVariable<int32_t> readVar {route, _name} ;
                                                                std::cout << "DINT value is " << readVar << "\n";
                                                                dbuf = readVar;
                                                                UA_Variant_setScalar(&vValueAttr.value, &dbuf, &UA_TYPES[UA_TYPES_INT32]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
                                                        }
                                                        else if (e_s41 == 0) //(_dataType == "0x41") => REAL, float
                                                        {
                                                                float rbuf;
								std::cout << "in (if x_s20==0) and (e_s41==0) : Sybmol is <" << _name << ">\n";
                                                                AdsVariable<float> readVar {route, _name} ;
								std::cout << "FLOAT value is " << readVar << "\n";
                                                                rbuf = readVar;
                                                                UA_Variant_setScalar(&vValueAttr.value, &rbuf, &UA_TYPES[UA_TYPES_FLOAT]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
                                                                // to investigate
                                                                //UA_UInt32 arrayDims[1] = {};
                                                                //UA_Variant_setScalar(&vValueAttr.value, &Value, &UA_TYPES[UA_TYPES_STRING]);
                                                                //vValueAttr.valueRank = UA_VALUERANK_ONE_DIMENSION;
                                                                //vValueAttr.arrayDimensions = arrayDims;
                                                                //vValueAttr.arrayDimensionsSize = 1;
                                                        }
                                                        else if (f_s11 == 0) // (_dataType == "0x11") => BYTE (same as BOOL)
                                                        {
                                                                bool bVar;
                                                                AdsVariable<bool> readVar {route, _name} ;
                                                                if (readVar == 0)
                                                                        Value = UA_String_fromChars("0");
                                                                else
                                                                        Value = UA_String_fromChars("1");

                                                                UA_Variant_setScalar(&vValueAttr.value, &Value, &UA_TYPES[UA_TYPES_BOOLEAN]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
								_variableType = UA_TYPES_BYTE;
                                                        }
                                                        else if (g_s4 == 0) // REAL, float
                                                        {
                                                                AdsVariable<float> readVar {route, _name} ;
                                                                UA_Variant_setScalar(&vValueAttr.value, &Value, &UA_TYPES[UA_TYPES_FLOAT]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
								_variableType = UA_TYPES_FLOAT;
                                                        }
                                                        else if (h_s5 == 0) // LREAL, double
                                                        {
                                                                AdsVariable<double> readVar {route, _name} ;
                                                                UA_Variant_setScalar(&vValueAttr.value, &Value, &UA_TYPES[UA_TYPES_DOUBLE]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
								_variableType = UA_TYPES_DOUBLE;
                                                        }
                                                        else if (i_s12 == 0) // WORD, UINT, uint16_t
                                                        {
                                                                AdsVariable<unsigned int> readVar {route, _name} ;
                                                                UA_Variant_setScalar(&vValueAttr.value, &Value, &UA_TYPES[UA_TYPES_UINT16]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
								_variableType = UA_TYPES_UINT16;
                                                        }
                                                        else if (j_s13 == 0) // DWORD, UDINT, uint32_t
                                                        {
                                                                AdsVariable<double> readVar {route, _name} ;
                                                                UA_Variant_setScalar(&vValueAttr.value, &Value, &UA_TYPES[UA_TYPES_UINT32]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
								_variableType = UA_TYPES_UINT32;
                                                        }
                                                        else if (k_s15 == 0) // LWORD, ULINT, uint64_t
                                                        {
                                                                AdsVariable<unsigned long> readVar {route, _name} ;
                                                                UA_Variant_setScalar(&vValueAttr.value, &Value, &UA_TYPES[UA_TYPES_UINT64]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
								_variableType = UA_TYPES_UINT64;
                                                        }
							else
							{
                                                                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                               "Unknown datatype: ", UA_StatusCode_name(retval));
								// default the rest to strings
								UA_Variant_setScalar(&vValueAttr.value, &Value, &UA_TYPES[UA_TYPES_STRING]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
								_variableType = UA_TYPES_STRING;
							}

							retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 20000+block+counter),
							   r2_twincat_variableName_Id, 		// parent NodeId
							   UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                           UA_QUALIFIEDNAME(namespaceIndex, sValue),       //UA_QUALIFIEDNAME(1, "Software Version"),      // Qualified Name
                                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),        //UA_NODEID_NULL
                                                           vValueAttr, NULL, &outValue_Id);
                                                        if (retval != UA_STATUSCODE_GOOD)
                                                            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                               "Error creating node : Value %d: %s", 20000+counter, UA_StatusCode_name(retval));
                                                        #ifdef DEBUG
                                                        else
                                                            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                            "Attribute Variable created: TwinCAT->iGroup %d : %d", 20000+block+counter, r1_twincatroot_Id.identifier.numeric);
                                                        #endif

							// create a ADS notification callback
							notificationByName(std::cout, route, _name, _variableType, _iGroup);

							// next create a monitored item
							UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outValue_Id);
							monRequest.requestedParameters.samplingInterval = 10.0; /* 10 ms interval */
							UA_Server_createDataChangeMonitoredItem(uaServer, UA_TIMESTAMPSTORETURN_SOURCE,
									monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);	// writeToDB flag = UA_TRUE

							// save a copy in global array for search/match purposes
							UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_TwinCATAds.cpp : Saving a copy to array for search/ match purposes : %s", _name.c_str());
						        // ADS information
        						int nPort;
        						AdsVersion version;     // consists of build version and revision as members
        						AmsAddr Addr;
        						char DevName[50];

						        // open62541 information
							adsVariables[adsVarCounter].uaServer = uaServer;
        						adsVariables[adsVarCounter].nodeId = outValue_Id;

							std::string _xleadingspaces = removeLeadingSpaces(_name);
                                                	std::string _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
							adsVariables[adsVarCounter]._name = _xtrailingspaces;

							_xleadingspaces = removeLeadingSpaces(_typeName);
							_xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
							adsVariables[adsVarCounter]._typeName = _xtrailingspaces;

							_xleadingspaces = removeLeadingSpaces(_comment);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
							adsVariables[adsVarCounter]._comment = _xtrailingspaces;

							_xleadingspaces = removeLeadingSpaces(_entryLength);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
							adsVariables[adsVarCounter]._entryLength = _xtrailingspaces;

							_xleadingspaces = removeLeadingSpaces(_iGroup);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
							adsVariables[adsVarCounter]._iGroup = _xtrailingspaces;

							_xleadingspaces = removeLeadingSpaces(_iOffs);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
							adsVariables[adsVarCounter]._iOffs = _xtrailingspaces;

							_xleadingspaces = removeLeadingSpaces(_size);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
							adsVariables[adsVarCounter]._size = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_dataType);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
							adsVariables[adsVarCounter]._dataType = _xtrailingspaces;

							_xleadingspaces = removeLeadingSpaces(_nameLength);
							_xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
							adsVariables[adsVarCounter]._nameLength = _xtrailingspaces;

							_xleadingspaces = removeLeadingSpaces(_typeLength);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
							adsVariables[adsVarCounter]._typeLength = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_commentLength);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
							adsVariables[adsVarCounter]._commentLength = _xtrailingspaces;

        						adsVariables[adsVarCounter]._variableType = _variableType;
        						adsVariables[adsVarCounter]._sequenceNo = adsVarCounter;

						}
					
						if (y_s30 == 0) //( (x_s20 == 1) && (y_s30 == 0) && (z_s40 == 1) )
						{
							#ifdef NOTE
							----------------------------check for MAIN.variables (iGroup = 0xf030)
							#endif
                                                        // create a new node (Value) to capture the 'value' of the TwinCAT variable
                                                        UA_String Value;

							std::string _dataType_xleadingspaces = removeLeadingSpaces(_dataType);
                                                        std::string _dataType_xtrailingspaces = removeTrailingSpaces(_dataType_xleadingspaces);

                                                        char sValue[] = "Value";
                                                        char sValueEx[] = "12. Value";
                                                        UA_VariableAttributes vValueAttr = UA_VariableAttributes_default;
                                                        vValueAttr.description = UA_LOCALIZEDTEXT(sLocale, sValue);
                                                        vValueAttr.displayName = UA_LOCALIZEDTEXT(sLocale, sValueEx);
                                                        vValueAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                                        vValueAttr.historizing = UA_TRUE;

                                                        // check the data type
                                                        char s21[] = "0x21";    // BOOL
                                                        char s11[] = "0x11";    // BYTE (same as BOOL)
                                                        char s1e[] = "0x1e";    // STRING
                                                        char s2[] = "0x2";      // INT => int16_t
                                                        char s3[] = "0x3";      // DINT => int32_t
                                                        char s4[] = "0x4";      // REAL => float
                                                        char s5[] = "0x5";      // LREAL => double
                                                        char s12[] = "0x12";    // WORD, UINT   => uint16_t
                                                        char s13[] = "0x13";    // DWORD, UDINT => uint32_t
                                                        char s15[] = "0x15";    // LWORD, ULINT =? uint64_t
                                                        char s41[] = "0x41";    // others

                                                        std::cout << "<" << _dataType_xtrailingspaces << ">" << "\n";

                                                        int a_s21 = _dataType_xtrailingspaces.compare(s21);
                                                        int f_s11 = _dataType_xtrailingspaces.compare(s11);
                                                        int b_s1e = _dataType_xtrailingspaces.compare(s1e);
                                                        int c_s2 = _dataType_xtrailingspaces.compare(s2);
                                                        int d_s3 = _dataType_xtrailingspaces.compare(s3);
                                                        int g_s4 = _dataType_xtrailingspaces.compare(s4);
                                                        int h_s5 = _dataType_xtrailingspaces.compare(s5);
                                                        int i_s12 = _dataType_xtrailingspaces.compare(s12);
                                                        int j_s13 = _dataType_xtrailingspaces.compare(s13);
                                                        int k_s15 = _dataType_xtrailingspaces.compare(s15);
                                                        int e_s41 = _dataType_xtrailingspaces.compare(s41);

                                                        std::cout << "(***) entering IF iGroup = 0xf030 section (MAIN.variables)" << "\n";
							#ifdef DEBUG
                                                        std::cout << "a : <" << a_s21 << ">\n";
                                                        std::cout << "f : <" << f_s11 << ">\n";
                                                        std::cout << "b : <" << b_s1e << ">\n";
                                                        std::cout << "c : <" << c_s2 << ">\n";
                                                        std::cout << "d : <" << d_s3 << ">\n";
                                                        std::cout << "g : <" << g_s4 << ">\n";
                                                        std::cout << "h : <" << h_s5 << ">\n";
                                                        std::cout << "i : <" << i_s12 << ">\n";
                                                        std::cout << "j : <" << j_s13 << ">\n";
                                                        std::cout << "k : <" << k_s15 << ">\n";
                                                        std::cout << "e : <" << e_s41 << ">\n";
							#endif

                                                        if (a_s21 == 0) //(_dataType == "0x021") => bool
                                                        {
                                                                UA_Boolean bVar;
                                                                AdsVariable<bool> readVar {route, _name} ; //"MAIN.el2024_1"};
                                                        	std::cout << "in (if y_s30==0) and (if a_s21==0) : _name is : " << _name << ":" << readVar << "\t";
                                                                if (readVar)
                                                                        bVar = UA_TRUE;//Value = UA_String_fromChars("0");
                                                                else
                                                                        bVar = UA_FALSE; //Value = UA_String_fromChars("1");

                                                        	std::cout << "bVar is " << bVar << " \n";

                                                                //UA_Variant_setScalar(&vValueAttr.value, &Value, &UA_TYPES[UA_TYPES_BOOLEAN]);
								UA_Variant_setScalar(&vValueAttr.value, &bVar, &UA_TYPES[UA_TYPES_BOOLEAN]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
                                                        }
                                                        else if (b_s1e == 0) //(_dataType == "0x1e") => STRING
                                                        {
								std::cout << "in (if y_s30==0) and (b_s1e==0) : _name is : " << _name << "\n";
							#ifdef ERROR
								std::string sVar;
                                                                AdsVariable<std::string> readVar {route, _name} ;
								sVar = readVar;
								std::cout << "in (if y_s30==0) and (b_s1e==0) : _name is : " << _name << ":" << sVar << "\t";
							sleep(5);
                                                                UA_Variant_setScalar(&vValueAttr.value, &sVar, &UA_TYPES[UA_TYPES_STRING]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
							#endif
                                                        }
                                                        else if (c_s2 == 0) //(_dataType == "0x2") => INT, (int16_t)
                                                        {
                                                                short shbuf;
                                                                std::cout << "in (if y_s30==0) and (c_s2==0) : Symbol is <" << _name << ">\n";
                                                                AdsVariable<int16_t> readVar {route, _name} ;       // error code = 0
                                                                std::cout << "INT value is " << readVar << "\n";
                                                                shbuf = readVar;
                                                                UA_Variant_setScalar(&vValueAttr.value, &shbuf, &UA_TYPES[UA_TYPES_INT16]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
                                                        }
                                                        else if (d_s3 == 0) //(_dataType == "0x3") => DINT, (int32_t)
                                                        {
								int dbuf;
								std::cout << "in (if y_s30==0) and (d_s3==0) : Symbol is <" << _name << ">\n";
                                                                AdsVariable<int32_t> readVar {route, _name} ;
								std::cout << "DINT value is " << readVar << "\n";
								dbuf = readVar;
                                                                UA_Variant_setScalar(&vValueAttr.value, &dbuf, &UA_TYPES[UA_TYPES_INT32]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
                                                        }
                                                        else if (e_s41 == 0) //(_dataType == "0x41") => REAL, float
                                                        {
								float rbuf;
								std::cout << "in (if y_s30==0) and (e_s41==0) : Symbol is <" << _name << "\n";
								AdsVariable<float> readVar {route, _name} ;
								std::cout << "FLOAT value is " << readVar << "\n";
								// convert float to char*, then use UA_Sting()
								rbuf = readVar;
								UA_Variant_setScalar(&vValueAttr.value, &rbuf, &UA_TYPES[UA_TYPES_FLOAT]);
								vValueAttr.valueRank = UA_VALUERANK_SCALAR;
								// to investigate
                                                                //UA_UInt32 arrayDims[1] = {};
                                                                //UA_Variant_setScalar(&vValueAttr.value, &Value, &UA_TYPES[UA_TYPES_STRING]);
                                                                //vValueAttr.valueRank = UA_VALUERANK_ONE_DIMENSION;
                                                                //vValueAttr.arrayDimensions = arrayDims;
                                                                //vValueAttr.arrayDimensionsSize = 1;
                                                        }
                                                        else if (f_s11 == 0) //(_dataType = "0x11") => BYTE (same as BOOL)
                                                        {
                                                                bool bVar;
                                                                AdsVariable<bool> readVar {route, _name} ; //"MAIN.el2024_1"};
                                                                if (readVar)
                                                                        bVar = UA_TRUE;
                                                                else
                                                                        bVar = UA_FALSE;

                                                                UA_Variant_setScalar(&vValueAttr.value, &bVar, &UA_TYPES[UA_TYPES_BOOLEAN]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;

                                                        }
                                                        else if (g_s4 == 0) // (_dataType = "0x4") => REAL, float
                                                        {
								float rVar;
                                                                AdsVariable<float> readVar {route, _name} ; //"MAIN.el2024_1"};
								rVar = readVar;
                                                                UA_Variant_setScalar(&vValueAttr.value, &rVar, &UA_TYPES[UA_TYPES_FLOAT]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;

                                                        }
                                                        else if (h_s5 == 0) // (_dataType = "0x5") => LREAL, double
                                                        {
								double doVar;
                                                                AdsVariable<double> readVar {route, _name} ; //"MAIN.el2024_1"};
								doVar = readVar;
                                                                UA_Variant_setScalar(&vValueAttr.value, &doVar, &UA_TYPES[UA_TYPES_DOUBLE]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
                                                        }
                                                        else if (i_s12 == 0) // (_dataType = "0x12") => WORD, UINT, uint16_t
                                                        {
								uint16_t uint16Var;
                                                                AdsVariable<unsigned int> readVar {route, _name} ; //"MAIN.el2024_1"};
								uint16Var = readVar;
                                                                UA_Variant_setScalar(&vValueAttr.value, &uint16Var, &UA_TYPES[UA_TYPES_UINT16]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
                                                        }
                                                        else if (j_s13 == 0) // (_dataType = 0x13) => WORD, UDINT, uint32_t
                                                        {
								//uint32_t uint32Var;
                                                                AdsVariable<double> readVar {route, _name} ; //"MAIN.el2024_1"};
								//uint32Var = readVar;
                                                                UA_Variant_setScalar(&vValueAttr.value, &readVar, &UA_TYPES[UA_TYPES_UINT32]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
                                                        }
                                                        else if (k_s15 == 0) // (_dataType = 0x15) => LWORD, ULINT, uint64_t
                                                        {
								//uint64_t uint64Var;
                                                                AdsVariable<unsigned long> readVar {route, _name} ; //"MAIN.el2024_1"};
								//uint64Var = readVar;
                                                                UA_Variant_setScalar(&vValueAttr.value, &readVar, &UA_TYPES[UA_TYPES_UINT64]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
                                                        }
                                                        else
                                                        {
								UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                               "Unknown datatype: ", UA_StatusCode_name(retval));
                                                                // default the rest to strings
                                                                UA_Variant_setScalar(&vValueAttr.value, &Value, &UA_TYPES[UA_TYPES_STRING]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
                                                        }

                                                        retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 20000+block+counter),
                                                           r2_twincat_variableName_Id,          // parent NodeId
                                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                           UA_QUALIFIEDNAME(namespaceIndex, sValue),       //UA_QUALIFIEDNAME(1, "Software Version"),      // Qualified Name
                                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),        //UA_NODEID_NULL
                                                           vValueAttr, NULL, &outValue_Id);
                                                        if (retval != UA_STATUSCODE_GOOD)
                                                            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                               "Error creating node : Value %d: %s", 20000+counter, UA_StatusCode_name(retval));
                                                        #ifdef DEBUG
                                                        else
                                                            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                            "Attribute Variable created: TwinCAT->iGroup %d : %d", 20000+block+counter, r1_twincatroot_Id.identifier.numeric);
                                                        #endif

                                                        // create a ADS notification callback
                                                        notificationByName(std::cout, route, _name, _variableType, _iGroup);

                                                        // next create a monitored item
                                                        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outValue_Id);
                                                        monRequest.requestedParameters.samplingInterval = 10.0; /* 10 ms interval */
                                                        UA_Server_createDataChangeMonitoredItem(uaServer, UA_TIMESTAMPSTORETURN_SOURCE,
                                                                        monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);	// writeToDB flag = UA_TRUE

                                                        // save a copy in global array for search/match purposes
							UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_TwinCATAds.cpp : Saving a copy to array for search/ match purposes : %s", _name.c_str());
                                                        // ADS information
                                                        int nPort;
                                                        AdsVersion version;     // consists of build version and revision as members
                                                        AmsAddr Addr;
                                                        char DevName[50];

                                                        // open62541 information
                                                        adsVariables[adsVarCounter].uaServer = uaServer;
                                                        adsVariables[adsVarCounter].nodeId = outValue_Id;

                                                        std::string _xleadingspaces = removeLeadingSpaces(_name);
                                                        std::string _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._name = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_typeName);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._typeName = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_comment);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._comment = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_entryLength);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._entryLength = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_iGroup);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._iGroup = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_iOffs);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._iOffs = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_size);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._size = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_dataType);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._dataType = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_nameLength);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._nameLength = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_typeLength);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._typeLength = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_commentLength);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._commentLength = _xtrailingspaces;

                                                        adsVariables[adsVarCounter]._variableType = _variableType;
                                                        adsVariables[adsVarCounter]._sequenceNo = adsVarCounter;
						} // if (y==0)
					
					
                                                if (z_s40 == 0) //( (x_s20 == -1) && (y_s30 == -1) && (z_s40 == 0) )
                                                {
                                                        #ifdef NOTE
                                                        ----------------------------check for MAIN.variables (iGroup = 0x4040)
                                                        #endif
                                                        // create a new node (Value) to capture the 'value' of the TwinCAT variable
                                                        UA_String Value;

                                                        std::string _dataType_xleadingspaces = removeLeadingSpaces(_dataType);
                                                        std::string _dataType_xtrailingspaces = removeTrailingSpaces(_dataType_xleadingspaces);

                                                        char sValue[] = "Value";
                                                        char sValueEx[] = "12. Value";
                                                        UA_VariableAttributes vValueAttr = UA_VariableAttributes_default;
                                                        vValueAttr.description = UA_LOCALIZEDTEXT(sLocale, sValue);
                                                        vValueAttr.displayName = UA_LOCALIZEDTEXT(sLocale, sValueEx);
                                                        vValueAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                                        vValueAttr.historizing = UA_TRUE;

                                                        // check the data type
                                                        char s21[] = "0x21";    // BOOL
                                                        char s11[] = "0x11";    // BYTE (same as BOOL)
                                                        char s1e[] = "0x1e";    // STRING
                                                        char s2[] = "0x2";      // INT => int16_t
                                                        char s3[] = "0x3";      // DINT => int32_t
                                                        char s4[] = "0x4";      // REAL => float
                                                        char s5[] = "0x5";      // LREAL => double
                                                        char s12[] = "0x12";    // WORD, UINT   => uint16_t
                                                        char s13[] = "0x13";    // DWORD, UDINT => uint32_t
                                                        char s15[] = "0x15";    // LWORD, ULINT =? uint64_t
                                                        char s41[] = "0x41";    // others

                                                        std::cout << "<" << _dataType_xtrailingspaces << ">" << "\n";

                                                        int a_s21 = _dataType_xtrailingspaces.compare(s21);
                                                        int f_s11 = _dataType_xtrailingspaces.compare(s11);
                                                        int b_s1e = _dataType_xtrailingspaces.compare(s1e);
                                                        int c_s2 = _dataType_xtrailingspaces.compare(s2);
                                                        int d_s3 = _dataType_xtrailingspaces.compare(s3);
                                                        int g_s4 = _dataType_xtrailingspaces.compare(s4);
                                                        int h_s5 = _dataType_xtrailingspaces.compare(s5);
                                                        int i_s12 = _dataType_xtrailingspaces.compare(s12);
                                                        int j_s13 = _dataType_xtrailingspaces.compare(s13);
                                                        int k_s15 = _dataType_xtrailingspaces.compare(s15);
                                                        int e_s41 = _dataType_xtrailingspaces.compare(s41);

                                                        std::cout << "(***) entering IF iGroup = 0x4040 section (MAIN.variables not tied to physical terminal)" << "\n";
							#ifdef DEBUG
                                                        std::cout << "a : <" << a_s21 << ">\n";
                                                        std::cout << "f : <" << f_s11 << ">\n";
                                                        std::cout << "b : <" << b_s1e << ">\n";
                                                        std::cout << "c : <" << c_s2 << ">\n";
                                                        std::cout << "d : <" << d_s3 << ">\n";
                                                        std::cout << "g : <" << g_s4 << ">\n";
                                                        std::cout << "h : <" << h_s5 << ">\n";
                                                        std::cout << "i : <" << i_s12 << ">\n";
                                                        std::cout << "j : <" << j_s13 << ">\n";
                                                        std::cout << "k : <" << k_s15 << ">\n";
                                                        std::cout << "e : <" << e_s41 << ">\n";
							#endif

                                                        if (a_s21 == 0) //(_dataType == "0x021") => bool
                                                        {
                                                                bool bVar;
                                                                AdsVariable<bool> readVar {route, _name} ;
								std::cout << "in (if z_z40==0) and (a_s21==0) : _name is : " << _name << ":" << readVar << "\n";
                                                                if (readVar == 1)
                                                                        Value = UA_String_fromChars("TRUE");
                                                                else
                                                                        Value = UA_String_fromChars("FALSE");
        	                                                std::cout << "UA_String : Value is " << Value.data << " \n";

                                                                UA_Variant_setScalar(&vValueAttr.value, &Value, &UA_TYPES[UA_TYPES_BOOLEAN]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
                                                        }
                                                        else if (b_s1e == 0) //(_dataType == "0x1e") => STRING
                                                        {
                                                                std::cout << "in (if z_s40==0) and (b_s1e==0) : _name is : " << _name << "\n";
							#ifdef ERROR

							std::cout << "step 1" << "\n";
                                                                AdsVariable<char*> readVar {route, _name} ;
							//printf("readVar is %s \n", readVar);

							std::cout << "step 2" << "\n";
                                                                char *mystring = (char*)calloc(100, sizeof(char)); strcpy(mystring, readVar);

							//std::cout << "step 2a : sVar.c_str() is : " << sVar.c_str() << "\n";	// segmentation fault here !!!
							std::cout << "step 3" << "\n";
							//std::cout << "sVar.length() is " << sVar.length() << "\n";
							std::cout << "step 3a" << "\n";
							//	char *mystring = new char[sVar.length()+1];
							//	std::strcpy(mystring, sVar.c_str());
							std::cout << "step 3b" << "\n";
								UA_String myUAString = UA_STRING(mystring);
							std::cout << "step 4" << "\n";
                                                                UA_Variant_setScalar(&vValueAttr.value, &myUAString, &UA_TYPES[UA_TYPES_STRING]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
								free(mystring);
							#endif
                                                        }
                                                        else if (c_s2 == 0) //(_dataType == "0x2") => INT, (int16_t)
                                                        {
                                                                short shbuf;
                                                                std::cout << "in (if z_s40==0) and (c_s2==0) : Symbol is <" << _name << ">\n";
                                                                AdsVariable<int16_t> readVar {route, _name} ;       // error code = 0
                                                                std::cout << "INT value is " << readVar << "\n";
                                                                shbuf = readVar;
                                                                UA_Variant_setScalar(&vValueAttr.value, &shbuf, &UA_TYPES[UA_TYPES_INT16]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
                                                        }
                                                        else if (d_s3 == 0) //(_dataType == "0x3") => DINT, (int32_t)
                                                        {
                                                                int dbuf;
                                                                std::cout << "in (if z_s40==0) and (d_s3==0) : Symbol is <" << _name << ">\n";
                                                                AdsVariable<int32_t> readVar {route, _name} ;
                                                                std::cout << "DINT value is " << readVar << "\n";
                                                                dbuf = readVar;
                                                                UA_Variant_setScalar(&vValueAttr.value, &dbuf, &UA_TYPES[UA_TYPES_INT32]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
                                                        }
                                                        else if (e_s41 == 0) //(_dataType == "0x41") => REAL, float
                                                        {
                                                                float rbuf;
                                                                std::cout << "in (if z_s40==0) and (e_s41==0) : Symbol is <" << _name << ">\n";
                                                                AdsVariable<float> readVar {route, _name} ;
                                                                std::cout << "FLOAT value is " << readVar << "\n";
                                                                rbuf = readVar;
                                                                UA_Variant_setScalar(&vValueAttr.value, &rbuf, &UA_TYPES[UA_TYPES_FLOAT]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
                                                                // to investigate
                                                                //UA_UInt32 arrayDims[1] = {};
                                                                //UA_Variant_setScalar(&vValueAttr.value, &Value, &UA_TYPES[UA_TYPES_STRING]);
                                                                //vValueAttr.valueRank = UA_VALUERANK_ONE_DIMENSION;
                                                                //vValueAttr.arrayDimensions = arrayDims;
                                                                //vValueAttr.arrayDimensionsSize = 1;
                                                        }
                                                        else if (f_s11 == 0) //(_dataType = "0x11") => BYTE (same as BOOL)
                                                        {
                                                                bool bVar;
                                                                AdsVariable<bool> readVar {route, _name} ; //"MAIN.el2024_1"};
                                                                if (readVar == 0)
                                                                        Value = UA_String_fromChars("0");
                                                                else
                                                                        Value = UA_String_fromChars("1");

                                                                UA_Variant_setScalar(&vValueAttr.value, &Value, &UA_TYPES[UA_TYPES_BOOLEAN]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;

                                                        }
                                                        else if (g_s4 == 0) // (_dataType = "0x4") => REAL, float
                                                        {
								float rbuf;
								std::cout << "in (if z_s40==0) and (g_s4==0) : Symbol is <" << _name << "\n";
                                                                AdsVariable<float> readVar {route, _name} ; //"MAIN.el2024_1"};
								std::cout << "FLOAT value is " << readVar << "\n";
								rbuf = readVar;
                                                                UA_Variant_setScalar(&vValueAttr.value, &rbuf, &UA_TYPES[UA_TYPES_FLOAT]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;

                                                        }
                                                        else if (h_s5 == 0) // (_dataType = "0x5") => LREAL, double
                                                        {
								double dbuf;
								std::cout << "in (if z_s40==0) and (h_s5==0) : Symbol is <" << _name << "\n";
                                                                AdsVariable<double> readVar {route, _name} ;
								std::cout << "Double value is " << readVar << "\n";
                                                                dbuf = readVar;
                                                                UA_Variant_setScalar(&vValueAttr.value, &dbuf, &UA_TYPES[UA_TYPES_DOUBLE]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
                                                        }
                                                        else if (i_s12 == 0) // (_dataType = "0x12") => WORD, UINT, uint16_t
                                                        {
                                                                AdsVariable<unsigned int> readVar {route, _name} ; //"MAIN.el2024_1"};
                                                                UA_Variant_setScalar(&vValueAttr.value, &Value, &UA_TYPES[UA_TYPES_UINT16]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
                                                        }
                                                        else if (j_s13 == 0) // (_dataType = 0x13) => WORD, UDINT, uint32_t
                                                        {
                                                                AdsVariable<double> readVar {route, _name} ; //"MAIN.el2024_1"};
                                                                UA_Variant_setScalar(&vValueAttr.value, &Value, &UA_TYPES[UA_TYPES_UINT32]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
                                                        }
                                                        else if (k_s15 == 0) // (_dataType = 0x15) => LWORD, ULINT, uint64_t
                                                        {
                                                                AdsVariable<unsigned long> readVar {route, _name} ; //"MAIN.el2024_1"};
                                                                UA_Variant_setScalar(&vValueAttr.value, &Value, &UA_TYPES[UA_TYPES_UINT64]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
                                                        }
                                                        else
                                                        {
                                                                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                               "Unknown datatype: ", UA_StatusCode_name(retval));
                                                                // default the rest to strings
                                                                UA_Variant_setScalar(&vValueAttr.value, &Value, &UA_TYPES[UA_TYPES_STRING]);
                                                                vValueAttr.valueRank = UA_VALUERANK_SCALAR;
                                                        }

                                                        retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 20000+block+counter),
                                                           r2_twincat_variableName_Id,          // parent NodeId
                                                           UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                           UA_QUALIFIEDNAME(namespaceIndex, sValue),       //UA_QUALIFIEDNAME(1, "Software Version"),      // Qualified Name
                                                           UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),        //UA_NODEID_NULL
                                                           vValueAttr, NULL, &outValue_Id);
                                                        if (retval != UA_STATUSCODE_GOOD)
                                                            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                               "Error creating node : Value %d: %s", 20000+counter, UA_StatusCode_name(retval));
                                                        #ifdef DEBUG
                                                        else
                                                            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                            "Attribute Variable created: TwinCAT->iGroup %d : %d", 20000+block+counter, r1_twincatroot_Id.identifier.numeric);
                                                        #endif

                                                        // create a ADS notification callback
                                                        notificationByName(std::cout, route, _name, _variableType, _iGroup);

                                                        // next create a monitored item
                                                        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outValue_Id);
                                                        monRequest.requestedParameters.samplingInterval = 10.0; /* 10 ms interval */
                                                        UA_Server_createDataChangeMonitoredItem(uaServer, UA_TIMESTAMPSTORETURN_SOURCE,
                                                                        monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);	// writeToDB flag = UA_TRUE

                                                        // save a copy in global array for search/match purposes
							UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_TwinCATAds.cpp : Saving a copy to array for search/ match purposes : %s", _name.c_str());
                                                        // ADS information
                                                        int nPort;
                                                        AdsVersion version;     // consists of build version and revision as members
                                                        AmsAddr Addr;
                                                        char DevName[50];

                                                        // open62541 information
                                                        adsVariables[adsVarCounter].uaServer = uaServer;
                                                        adsVariables[adsVarCounter].nodeId = outValue_Id;

                                                        std::string _xleadingspaces = removeLeadingSpaces(_name);
                                                        std::string _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._name = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_typeName);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._typeName = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_comment);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._comment = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_entryLength);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._entryLength = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_iGroup);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._iGroup = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_iOffs);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._iOffs = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_size);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._size = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_dataType);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._dataType = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_nameLength);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._nameLength = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_typeLength);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._typeLength = _xtrailingspaces;

                                                        _xleadingspaces = removeLeadingSpaces(_commentLength);
                                                        _xtrailingspaces = removeTrailingSpaces(_xleadingspaces);
                                                        adsVariables[adsVarCounter]._commentLength = _xtrailingspaces;

                                                        adsVariables[adsVarCounter]._variableType = _variableType;
                                                        adsVariables[adsVarCounter]._sequenceNo = adsVarCounter;

						} // if (z==0)
					
                                                counter++;		// advance the nodeId value
						adsVarCounter++;	// advance to the next array slot

						char siOffs[] = "iOffs";
						char siOffsEx[] = "06. Offsets";
                                                UA_VariableAttributes viOffsAttr = UA_VariableAttributes_default;
                                                viOffsAttr.description = UA_LOCALIZEDTEXT(sLocale, siOffs);
                                                viOffsAttr.displayName = UA_LOCALIZEDTEXT(sLocale, siOffsEx);
                                                viOffsAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
                                                viOffsAttr.valueRank = UA_VALUERANK_SCALAR;
                                                viOffsAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                                viOffsAttr.historizing = UA_FALSE;

                                                char* _iOffsNew = new char[_iOffs.length() + 1];
                                                std::strcpy(_iOffsNew, _iOffs.c_str());
                                                UA_String iOffs = UA_STRING(_iOffsNew);
                                                //if (!UA_Variant_isEmpty(&vSoftwareVersionAttr.value))
                                                        UA_Variant_setScalar(&viOffsAttr.value, &iOffs, &UA_TYPES[UA_TYPES_STRING]);
                                                //else UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                //        "Error UA_Variant_setScalar() : SoftwareVersion 10001");
                                                //https://github.com/open62541/open62541(Example Server Implementation); UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 20000+block+counter),          // NodeId
                                                    r2_twincat_variableName_Id,                          // parent NodeId
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                    UA_QUALIFIEDNAME(namespaceIndex, siOffs),       //UA_QUALIFIEDNAME(1, "Software Version"),      // Qualified Name
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),        //UA_NODEID_NULL
                                                    viOffsAttr, NULL, &outiOffs_Id);
                                                if (retval != UA_STATUSCODE_GOOD)
                                                    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "Error creating node : iOffs %d: %s", 20000+counter, UA_StatusCode_name(retval));
                                                #ifdef DEBUG
                                                else
                                                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "Attribute Variable created: TwinCAT->iOffs %d : %d", 20000+block+counter, r1_twincatroot_Id.identifier.numeric);
                                                #endif
						free(_iOffsNew);

                                                counter++;
						char sSize[] = "size";
						char sSizeEx[] = "07. size";
                                                UA_VariableAttributes vsizeAttr = UA_VariableAttributes_default;
                                                viOffsAttr.description = UA_LOCALIZEDTEXT(sLocale, sSize);
                                                vsizeAttr.displayName = UA_LOCALIZEDTEXT(sLocale, sSizeEx);
                                                vsizeAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
                                                vsizeAttr.valueRank = UA_VALUERANK_SCALAR;
                                                vsizeAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                                vsizeAttr.historizing = UA_FALSE;

                                                char* _sizeNew = new char[_size.length() + 1];
                                                std::strcpy(_sizeNew, _size.c_str());
                                                UA_String size = UA_STRING(_sizeNew);
                                                //if (!UA_Variant_isEmpty(&vSoftwareVersionAttr.value))
                                                        UA_Variant_setScalar(&vsizeAttr.value, &size, &UA_TYPES[UA_TYPES_STRING]);
                                                //else UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                //        "Error UA_Variant_setScalar() : SoftwareVersion 10001");
                                                //https://github.com/open62541/open62541(Example Server Implementation); UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 20000+block+counter),          // NodeId
                                                    r2_twincat_variableName_Id,                          // parent NodeId
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                    UA_QUALIFIEDNAME(namespaceIndex, sSize),       //UA_QUALIFIEDNAME(1, "Software Version"),      // Qualified Name
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),        //UA_NODEID_NULL
                                                    vsizeAttr, NULL, &outSize_Id);
                                                if (retval != UA_STATUSCODE_GOOD)
                                                    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "Error creating node : size %d: %s", 20000+counter, UA_StatusCode_name(retval));
                                                #ifdef DEBUG
                                                else
                                                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "Attribute Variable created: TwinCAT->size %d : %d", 20000+block+counter, r1_twincatroot_Id.identifier.numeric);
                                                #endif
						free(_sizeNew);

                                                counter++;
						char sDataType[] = "dataType";
						char sDataTypeEx[] = "08. Data Type";
                                                UA_VariableAttributes vdataTypeAttr = UA_VariableAttributes_default;
                                                vdataTypeAttr.description = UA_LOCALIZEDTEXT(sLocale, sDataType);
                                                vdataTypeAttr.displayName = UA_LOCALIZEDTEXT(sLocale, sDataTypeEx);
                                                vdataTypeAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
                                                vdataTypeAttr.valueRank = UA_VALUERANK_SCALAR;
                                                vdataTypeAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                                vdataTypeAttr.historizing = UA_FALSE;

                                                char* _dataTypeNew = new char[_dataType.length() + 1];
                                                std::strcpy(_dataTypeNew, _dataType.c_str());
                                                UA_String dataType = UA_STRING(_dataTypeNew);
                                                //if (!UA_Variant_isEmpty(&vSoftwareVersionAttr.value))
                                                        UA_Variant_setScalar(&vdataTypeAttr.value, &dataType, &UA_TYPES[UA_TYPES_STRING]);
                                                //else UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                //        "Error UA_Variant_setScalar() : SoftwareVersion 10001");
                                                //https://github.com/open62541/open62541(Example Server Implementation); UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 20000+block+counter),          // NodeId
                                                    r2_twincat_variableName_Id,                          // parent NodeId
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                    UA_QUALIFIEDNAME(namespaceIndex, sDataType),       //UA_QUALIFIEDNAME(1, "Software Version"),      // Qualified Name
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),        //UA_NODEID_NULL
                                                    vdataTypeAttr, NULL, &outDataType_Id);
                                                if (retval != UA_STATUSCODE_GOOD)
                                                    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "Error creating node : dataType %d: %s", 20000+counter, UA_StatusCode_name(retval));
                                                #ifdef DEBUG
                                                else
                                                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "Attribute Variable created: TwinCAT->dataType %d : %d", 20000+block+counter, r1_twincatroot_Id.identifier.numeric);
                                                #endif
						free(_dataTypeNew);

                                                counter++;
						char sNameLength[] = "NameLength";
						char sNameLengthEx[] = "09. Name Length";
                                                UA_VariableAttributes vNameLengthAttr = UA_VariableAttributes_default;
                                                vNameLengthAttr.description = UA_LOCALIZEDTEXT(sLocale, sNameLength);
                                                vNameLengthAttr.displayName = UA_LOCALIZEDTEXT(sLocale, sNameLengthEx);
                                                vNameLengthAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
                                                vNameLengthAttr.valueRank = UA_VALUERANK_SCALAR;
                                                vNameLengthAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                                vNameLengthAttr.historizing = UA_FALSE;

                                                char* _nameLengthNew = new char[_nameLength.length() + 1];
                                                std::strcpy(_nameLengthNew, _nameLength.c_str());
                                                UA_String NameLength = UA_STRING(_nameLengthNew);
                                                //if (!UA_Variant_isEmpty(&vSoftwareVersionAttr.value))
                                                        UA_Variant_setScalar(&vNameLengthAttr.value, &NameLength, &UA_TYPES[UA_TYPES_STRING]);
                                                //else UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                //        "Error UA_Variant_setScalar() : SoftwareVersion 10001");
                                                //https://github.com/open62541/open62541(Example Server Implementation); UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 20000+block+counter),          // NodeId
                                                    r2_twincat_variableName_Id,                          // parent NodeId
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                    UA_QUALIFIEDNAME(namespaceIndex, sNameLength),       //UA_QUALIFIEDNAME(1, "Software Version"),      // Qualified Name
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),        //UA_NODEID_NULL
                                                    vNameLengthAttr, NULL, &outNameLength_Id);
                                                if (retval != UA_STATUSCODE_GOOD)
                                                    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "Error creating node : NameLength %d: %s", 20000+counter, UA_StatusCode_name(retval));
                                                #ifdef DEBUG
                                                else
                                                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "Attribute Variable created: TwinCAT->NameLength %d : %d", 20000+block+counter, r1_twincatroot_Id.identifier.numeric);
                                                #endif
						free(_nameLengthNew);

                                                counter++;
						char sTypeLength[] = "TypeLength";
						char sTypeLengthEx[] = "10. Type Length";
                                                UA_VariableAttributes vTypeLengthAttr = UA_VariableAttributes_default;
                                                vTypeLengthAttr.description = UA_LOCALIZEDTEXT(sLocale, sTypeLength);
                                                vTypeLengthAttr.displayName = UA_LOCALIZEDTEXT(sLocale, sTypeLengthEx);
                                                vTypeLengthAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
                                                vTypeLengthAttr.valueRank = UA_VALUERANK_SCALAR;
                                                vTypeLengthAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                                vTypeLengthAttr.historizing = UA_FALSE;

                                                char* _typeLengthNew = new char[_typeLength.length() + 1];
                                                std::strcpy(_typeLengthNew, _typeLength.c_str());
                                                UA_String TypeLength = UA_STRING(_typeLengthNew);
                                                //if (!UA_Variant_isEmpty(&vSoftwareVersionAttr.value))
                                                        UA_Variant_setScalar(&vTypeLengthAttr.value, &TypeLength, &UA_TYPES[UA_TYPES_STRING]);
                                                //else UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                //        "Error UA_Variant_setScalar() : SoftwareVersion 10001");
                                                //https://github.com/open62541/open62541(Example Server Implementation); UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 20000+block+counter),          // NodeId
                                                    r2_twincat_variableName_Id,                          // parent NodeId
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                    UA_QUALIFIEDNAME(namespaceIndex, sTypeLength),       //UA_QUALIFIEDNAME(1, "Software Version"),      // Qualified Name
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),        //UA_NODEID_NULL
                                                    vTypeLengthAttr, NULL, &outTypeLength_Id);
                                                if (retval != UA_STATUSCODE_GOOD)
                                                    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "Error creating node : TypeLength %d: %s", 20000+counter, UA_StatusCode_name(retval));
                                                #ifdef DEBUG
                                                else
                                                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "Attribute Variable created: TwinCAT->TypeLength %d : %d", 20000+block+counter, r1_twincatroot_Id.identifier.numeric);
                                                #endif
						free(_typeLengthNew);

                                                counter++;
						char sCommentLength[] = "CommentLength";
						char sCommentLengthEx[] = "11. Comment Length";
                                                UA_VariableAttributes vCommentLengthAttr = UA_VariableAttributes_default;
                                                vCommentLengthAttr.description = UA_LOCALIZEDTEXT(sLocale, sCommentLength);
                                                vCommentLengthAttr.displayName = UA_LOCALIZEDTEXT(sLocale, sCommentLengthEx);
                                                vCommentLengthAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
                                                vCommentLengthAttr.valueRank = UA_VALUERANK_SCALAR;
                                                vCommentLengthAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                                vCommentLengthAttr.historizing = UA_FALSE;

                                                char* _commentLengthNew = new char[_commentLength.length() + 1];
                                                std::strcpy(_commentLengthNew, _commentLength.c_str());
                                                UA_String CommentLength = UA_STRING(_commentLengthNew);
                                                //if (!UA_Variant_isEmpty(&vSoftwareVersionAttr.value))
                                                        UA_Variant_setScalar(&vCommentLengthAttr.value, &CommentLength, &UA_TYPES[UA_TYPES_STRING]);
                                                //else UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                //        "Error UA_Variant_setScalar() : SoftwareVersion 10001");
                                                //https://github.com/open62541/open62541(Example Server Implementation); UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),

                                                retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 20000+block+counter),          // NodeId
                                                    r2_twincat_variableName_Id,                          // parent NodeId
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
						    UA_QUALIFIEDNAME(namespaceIndex, sCommentLength),
                                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),        //UA_NODEID_NULL
                                                    vCommentLengthAttr, NULL, &outCommentLength_Id);
                                                if (retval != UA_STATUSCODE_GOOD)
                                                    UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "Error creating node : CommentLength %d: %s", 20000+counter, UA_StatusCode_name(retval));
                                                #ifdef DEBUG
                                                else
                                                    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "Attribute Variable created: TwinCAT->CommentLength %d : %d", 20000+block+counter, r1_twincatroot_Id.identifier.numeric);
                                                #endif
						free(_commentLengthNew);

						// reset counter to 0 for the next block; block counter is increased by 100
						block = block + 100;
						counter = 0;

					//} //if (firstTimeAddADSToUAServer)
					continue; // go to the next block
				} // if (x == 0)
				break;
			} // while (1)

			#ifdef DEBUG
			// print the contents of the array : adsvariables
			for (int index=0; index < adsVarCounter; index++)
			{
				printf("<%s> <%d> <%s> <%s> <%s> <%s> <%s> <%s> <%s> <%s> <%s> <%s> <%d> <%d> \n",
							adsVariables[index]._name.c_str(),
                                                        //adsVariables[adsVarCounter].uaServer = uaServer;
                                                        adsVariables[index].nodeId.identifier.numeric,
                                                        adsVariables[index]._typeName.c_str(),
                                                        adsVariables[index]._comment.c_str(),
                                                        adsVariables[index]._entryLength.c_str(),
                                                        adsVariables[index]._iGroup.c_str(),
                                                        adsVariables[index]._iOffs.c_str(),
                                                        adsVariables[index]._size.c_str(),
                                                        adsVariables[index]._dataType.c_str(),
                                                        adsVariables[index]._nameLength.c_str(),
                                                        adsVariables[index]._typeLength.c_str(),
                                                        adsVariables[index]._commentLength.c_str(),
                                                        adsVariables[index]._variableType,
                                                        adsVariables[index]._sequenceNo);
			}
			#endif

			return UA_TRUE;
		}
		else
		{
			std::cout << "Fail to open TwinCATAds.txt for reading" << "\n";
			return UA_FALSE;
		}
	}
	else
	{
		std::cout << "Fail to open TwinCATAds.txt for reading" << "\n" ;
		return UA_FALSE;
	}
}










int unused()
{
#ifdef OBSOLETE
	//twincat_ads ads;
	long nErr;
	long nAdsPort;
	AdsVersion Version;
	//AdsVersion *pVersion = &Version;
	char DevName[50];
	//AmsAddr Addr;
	//PAmsAddr pAddr = &Addr;

	char *symbols = NULL;
    	struct AdsSymbolUploadInfo {
        	uint32_t nSymbols;
        	uint32_t nSymSize;
    	} uploadInfo;


	// Open communication port of the ADS router
	nAdsPort = AdsPortOpenEx();
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : ADS port <%d> created and opened successfully", nAdsPort);

	nErr = AdsGetLocalAddressEx(nAdsPort, &Addr);
	if (nErr)
	{
		UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : AdsGetLocalAddress() failed : Error code %d", nErr);
		return UA_FALSE;
	}
	else
	{
		uint32_t bytesRead;
		AdsSymbolUploadInfo tAdsSymbolUploadInfo;
		AdsSymbolEntry *adsSymbolEntry;
		Addr.netId = { real_ams };
		Addr.port = nPlcPort; // nPlcPort;
		//Addr.route = route;

		// read the length of the variables declared in the PLC bssed on the port
		nErr = AdsSyncReadReqEx2(nAdsPort, &Addr, ADSIGRP_SYM_UPLOADINFO, 0x0, sizeof(tAdsSymbolUploadInfo), &tAdsSymbolUploadInfo, &bytesRead );
		if (nErr)
		{
			// AdsSyncReadReqEx2() failed : Error code 7 : target machine not found - Missing ADS routes
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : AdsSyncReadReqEx2() failed : Error code %d", nErr);
                        return UA_FALSE;
		}
		else
		{
			symbols = new char[tAdsSymbolUploadInfo.nSymSize];
			assert(symbols);

			// read information about the PLC variables
			nErr = AdsSyncReadReqEx2(nAdsPort, &Addr, ADSIGRP_SYM_UPLOAD, 0, tAdsSymbolUploadInfo.nSymSize, symbols, &bytesRead);
			if (nErr)
			{
				UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : AdsSyncReadReqEx2() read info about PLC variables failed : Error code %d", nErr);
				return UA_FALSE;
			}
			else
			{
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "PLC data retrieved :");
				adsSymbolEntry = (AdsSymbolEntry*)symbols;
				// output information about the PLC variables
				for (int uindex =0; uindex < tAdsSymbolUploadInfo.nSymbols; uindex++)
				{
					// refer to AdsLib/standalone/AdsDef.h:
					//printf("Symbol name 	: %s \n", PADSSYMBOLNAME(&adsSymbolEntry);

					printf("\tentryLength	: %d \n", adsSymbolEntry->entryLength);
					printf("\tindex Group 	: %s \n", adsSymbolEntry->iGroup);
					printf("\tindex Offset 	: %s \n", adsSymbolEntry->iOffs);
					printf("\tsize of symbol: %d \n", adsSymbolEntry->size);
					printf("\tdataType 	: %d \n", adsSymbolEntry->dataType);
					printf("\tflags		: %d \n", adsSymbolEntry->flags);
					printf("\tnameLength	: %d \n", adsSymbolEntry->nameLength);
					printf("\ttypeLength	: %d \n", adsSymbolEntry->typeLength);
					printf("\tcommentLength	: %d \n", adsSymbolEntry->commentLength);

					//printf("Symbol type 	: %s \n", PADSSYMBOLTYPE(&adsSymbolEntry);
					//printf("Symbol comments : %s \n", PADSSYMBOLCOMMENT(&adsSymbolEntry)

					// advance to next entry
					//adsSymbolEntry = adsSymbolEntry + sizeof(AdsSymbolEntry);//PADSNEXTSYBMBOLENTRY(&adsSymbolEntry);
				}
			}
		}
	#ifdef KIV
		nErr = AdsSyncReadDeviceInfoReqEx(nAdsPort, &Addr, DevName, &Version);
		if (nErr)
		{
			// failed here : 0x7 7 0x98110007 ERR_TARGETMACHINENOTFOUND Target computer not found – AMS route was not found.
			UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : AdsSyncReadDeviceInfoReqEx() failed : Error code %d", nErr);
			return UA_FALSE;
		}
		else
		{
			UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : AdsSyncReadDeviceInfoReqEx() success");
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : Device Name : %s", DevName);
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : ADS Version : %d", (uint8_t*)&Version.version);
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : ADS Revision : %d", (uint8_t*)&Version.revision);
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : ADS Build : %s", &Version.build);


			#ifdef KIV_UNTIL_ABOVE_IS_TESTED
			// next, read the length of the variable declaration
			nErr = AdsSyncReadReq(&ads->Addr, ADSIGRP_SYM_UPLOADINFO, 0x0, sizeof(tAdsSymbolUploadInfo), &tAdsSymbolUploadInfo);
			if (nErr)
			{
				UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : AdsSyncReadReq() length failed : Error code %d", nErr);
				return UA_FALSE;
			}
			pchSymbols = new char [tAdsSymbolUploadInfo.nSymSize];

			// next - read information about the PLC variables
			nErr = AdsSyncReadReq(pAddr, ADSIGRP_SYM_UPLOAD, 0, tAdsSymbolUploadInfo.nSymSize, pchSymbols);
			if (nErr)
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : AdsSyncReadReq() info failed : Error code %d", nErr);
                                return UA_FALSE;
                        }

			// output the extracted information
			pAdsSymbolEntry = (PAdsSymbolEntry)pchSymbols;
			for (int i=0; i< tAdsSymbolUploadInfo.nSymbols; i++)
			{
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : symbol name %s", PADSSYMBOLNAME(pAdsSymbolEntry));
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : iGroup %s", pAdsSymbolEntry->iGroup));
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : iOffs %s", pAdsSymbolEntry->iOffs));
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : size %d", pAdsSymbolEntry->size));
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : symbol type %s", PADSSYMBOLTYPE(pAdsSymbolEntry));
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : symbol comment %s", PADSSYMBOLCOMMENT(pAdsSymbolEntry));

				pAdsSymbolEntry = PADSNEXTSYMBOLENTRY(pAdsSymbolEntry);
			}
			#endif

			return UA_TRUE;
		}
	#endif
	}
#endif

	return UA_TRUE;
}


#ifdef ToBeChanged

void CB_readTwinCATAds(twincat_ads *data)
{

	long nErr, nPort = data->nPort;
	char *pchSymbols = NULL;
	unsigned int uIndex;
	AmsAddr Addr;

	AdsSymbolUploadInfo tAdsSymbolUploadInfo;
			/* read the ADS variables, starting from port 851 (default starting port in TwinCAT */
			/* initialise the callback notification */
			AdsNotificationAttrib adsNotificationAttrib;

			adsNotificationAttrib.cbLength = 4;
			adsNotificationAttrib.nTransMode = ADSTRANS_SERVERONCHA;
			adsNotificationAttrib.nMaxDelay = 0;
			adsNotificationAttrib.nCycleTime = 10000000; // 1sec

			/* get the handle to the PLC variable */
			uint32_t hHandle;
			uint32_t bytesRead;
			unsigned long int hUser;
			unsigned int hNotification;
			char  szVar [] = "MAIN.PLCVar";
			nErr = AdsSyncReadWriteReqEx2(data->nPort, &data->Addr, ADSIGRP_SYM_HNDBYNAME, 0x0, sizeof(hHandle), &hHandle, sizeof(szVar), szVar, &bytesRead);
			if (nErr)
			{
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : AdsSyncReadWriteReq failed : %d", nErr);
			}

			/* initiate the transmission of the PLC-variable */
			nErr = AdsSyncAddDeviceNotificationReqEx(data->nPort, &data->Addr, ADSIGRP_SYM_VALBYHND, hUser, &adsNotificationAttrib, Callback, hUser, &hNotification);
			if (nErr)
			{
				UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : AdsSyncAddDeviceNotification failed : %d", nErr);
                        }

			// finish the transmission of the PLC-variable
			nErr = AdsSyncDelDeviceNotificationReqEx(nPort, &Addr, hNotification);
			if (nErr)
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : AdsSyncDelDeviceNotification failed : %d", nErr);
                        }

			// release handle
			nErr = AdsSyncWriteReqEx(nPort, &Addr, ADSIGRP_SYM_RELEASEHND, 0, sizeof(hUser), &hUser);
                        if (nErr)
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : AdsSyncWriteReq failed : %d", nErr);
                        }

			nErr = AdsPortCloseEx(nPort);
			if (nErr)
			{
				UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_TwinCATAds.cpp : AdsPortClose failed : %d", nErr);
			}
		}
	}
}
#endif

} /* extern "C" */
