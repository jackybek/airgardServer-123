#ifdef no_almagamation
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#else
   #include "open62541.h"
   // no point #define here.. must set in open62541.h
#endif

#include <stdio.h>
#include <pthread.h>
#include <mariadb/mysql.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <modbus/modbus.h>
//#include <lely/co/co.h>
//#include <lely/co/dcf.h>
//#include <lely/co/dev.h>
//#include <lely/co/detail/obj.h>
#include "SV_main.h"
#include "SV_Register.h"
#include "SV_Encrypt.h"
#include "SV_JsonChecker.h"
//#include "SV_CreateCanopenMaster.h"

#define MAX_STRING_SIZE 64

typedef struct {
char Tag[MAX_STRING_SIZE];
char Name[MAX_STRING_SIZE];
float Probability;
char CASnumber[MAX_STRING_SIZE];
int Concentration;
} AlarmStruct;

typedef struct {
char Tag[MAX_STRING_SIZE];
char Name[MAX_STRING_SIZE];
float Probability;
char CASnumber[MAX_STRING_SIZE];
int Concentration;
} NonAlarmStruct;

static volatile UA_Boolean running = true;
float TensorDataArray[3600][4]; /* 1 record per second over 1 hour = 3600 rows,*/
                                /* 4 columns - attributes (id, speed, temperature, displacement) */
				/* 6 modbus serial vibration sensors */
//co_dev_t **CanopenDcfDevices;
int sockfd;
int command_sockfd;

int g_argc;
char g_argv_ip[255];
int g_argv_port;
UA_Boolean UA_Nodes_Setup;

char SoftwareVersion[MAX_STRING_SIZE];
char DataBlockVersion[MAX_STRING_SIZE];
char InstrumentTime[MAX_STRING_SIZE];
char MeasurementTime[MAX_STRING_SIZE];

char BootStatus[MAX_STRING_SIZE];
char SnapshotStatus[MAX_STRING_SIZE];
char SCPStatus[MAX_STRING_SIZE];
char SFTPStatus[MAX_STRING_SIZE];
char RunScriptStatus[MAX_STRING_SIZE];
char ArchiveStatus[MAX_STRING_SIZE];
char AncillarySensorStatus[MAX_STRING_SIZE];

char Sensor[MAX_STRING_SIZE];
UA_Int16 OperatingTime;
char WarningMessage[MAX_STRING_SIZE];

UA_Float IgramPP;
UA_Float IgramDC;
UA_Float LaserPP;
UA_Float LaserDC;
UA_Float SingleBeamAt900;
UA_Float SingleBeamAt2500;
UA_Int16 SignalToNoiseAt2500;
UA_Float CenterBurstLocation;
UA_Float DetectorTemp;
UA_Float LaserFrequency;
UA_Int16 HardDriveSpace;
UA_Int16 Flow;
UA_Int16 Temperature;
UA_Float Pressure;
UA_Int16 TempOptics;
UA_Int16 BadScanCounter;
UA_Int16 FreeMemorySpace;

char LABFilename[MAX_STRING_SIZE];
char LOGFilename[MAX_STRING_SIZE];
char LgFilename[MAX_STRING_SIZE];
char SecondLgFilename[MAX_STRING_SIZE];

UA_Float SystemCounter;
UA_Float DetectorCounter;
UA_Float LaserCounter;
UA_Float FlowPumpCounter;
UA_Float DesiccantCounter;

UA_Int16 NoOfAlarms;
UA_Int16 NoOfNonAlarms;

int NoOfAlarmsNode;
int NoOfNonAlarmsNode;
AlarmStruct arrayOfAlarm[255];  //101
AlarmStruct arrayOfNonAlarm[255];
char AlarmTag[MAX_STRING_SIZE];
char AlarmName[MAX_STRING_SIZE];
UA_Float AlarmProbability;
char AlarmCASnumber[MAX_STRING_SIZE];
UA_Int16 AlarmConcentration;

//UA_Boolean UA_Nodes_Setup = UA_FALSE;

// FULL Namespace requires all outNodeId; also used for Events, Historizing & PubSub
UA_NodeId outSoftwareVersion_Id;
UA_NodeId outDataBlockVersion_Id;
UA_NodeId outInstrumentTime_Id;
UA_NodeId outMeasurementTime_Id;
UA_NodeId outSensor_Id;
UA_NodeId outOperatingTime_Id;
UA_NodeId outWarningMessage_Id;
UA_NodeId outBootStatus_Id;
UA_NodeId outSnapshotStatus_Id;
UA_NodeId outSCPStatus_Id;
UA_NodeId outSFTPStatus_Id;
UA_NodeId outRunScriptStatus_Id;
UA_NodeId outArchiveStatus_Id;
UA_NodeId outAncilarySensor_Id;

UA_NodeId outIgramPP_Id;
UA_NodeId outIgramDC_Id;
UA_NodeId outLaserPP_Id;
UA_NodeId outLaserDC_Id;
UA_NodeId outSingleBeamAt900_Id;
UA_NodeId outSingleBeamAt2500_Id;
UA_NodeId outSignalToNoiseAt2500_Id;
UA_NodeId outCenterBurstLocation_Id;
UA_NodeId outDetectorTemp_Id;
UA_NodeId outLaserFrequency_Id;
UA_NodeId outHardDriveSpace_Id;
UA_NodeId outFlow_Id;
UA_NodeId outTemperature_Id;
UA_NodeId outPressure_Id;
UA_NodeId outTempOptics_Id;
UA_NodeId outBadScanCounter_Id;
UA_NodeId outFreeMemorySpace_Id;
UA_NodeId outLABFilename_Id;
UA_NodeId outLOGFilename_Id;
UA_NodeId outLgFilename_Id;
UA_NodeId outSecondLgFilename_Id;
UA_NodeId outSystemCounter_Id;
UA_NodeId outDetectorCounter_Id;
UA_NodeId outLaserCounter_Id;
UA_NodeId outFlowPumpCounter_Id;
UA_NodeId outDesiccantCounter_Id;

char hashpassword[255];
MYSQL *conn;

// function prototypes
UA_NodeId* createNodes(void* x_void_ptr);
int loadAirgardEnvJson(void);

#ifndef UA_AES128CTR_SIGNING_KEY_LENGTH
#define UA_AES128CTR_SIGNING_KEY_LENGTH 16
#endif

#ifndef UA_AES128CTR_KEY_LENGTH
#define UA_AES128CTR_KEY_LENGTH 16
#endif

#ifndef UA_AES128CTR_KEYNONCE_LENGTH
#define UA_AES128CTR_KEYNONCE_LENGTH 4
#endif

UA_Byte signingKey[UA_AES128CTR_SIGNING_KEY_LENGTH] = {0};
UA_Byte encryptingKey[UA_AES128CTR_KEY_LENGTH] = {0};
UA_Byte keyNonce[UA_AES128CTR_KEYNONCE_LENGTH] = {0};

char *discovery_url = NULL;

UA_NodeId publishedDataSetIdentifier;

static void
serverOnNetworkCallback(const UA_ServerOnNetwork *serverOnNetwork, UA_Boolean isServerAnnounce,
                        UA_Boolean isTxtReceived, void *data) {

    if(discovery_url != NULL || !isServerAnnounce) {
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "serverOnNetworkCallback called, but discovery URL "
                     "already initialized or is not announcing. Ignoring.");
        return; // we already have everything we need or we only want server announces
    }

    if(!isTxtReceived)
        return; // we wait until the corresponding TXT record is announced.
                // Problem: how to handle if a Server does not announce the
                // optional TXT?

    // here you can filter for a specific LDS server, e.g. call FindServers on
    // the serverOnNetwork to make sure you are registering with the correct
    // LDS. We will ignore this for now
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Another server announced itself on %.*s",
                (int)serverOnNetwork->discoveryUrl.length, serverOnNetwork->discoveryUrl.data);

    if(discovery_url != NULL)
        UA_free(discovery_url);
    discovery_url = (char*)UA_malloc(serverOnNetwork->discoveryUrl.length + 1);
    memcpy(discovery_url, serverOnNetwork->discoveryUrl.data, serverOnNetwork->discoveryUrl.length);
    discovery_url[serverOnNetwork->discoveryUrl.length] = 0;
}

void StartCanopen(UA_Server *server)
{
	#ifdef MOVE_TO_SV_CreateCanopen_c
	/* Canopen related functions
	1. read the list of slave Eds/ Dcf files
	2. Create a canopen Master instance
	3. Setup a polling cycle using open62541 callbacks (SV_Callbacks.c)
	4. Every polling cycle, the canopen Master will read the slaves (defined in step 1)
	*/
	co_dev_t **CanopenDcfDevices=NULL;
	int NumberOfDevices = 0;
	int NumberOfFilesRead = CreateCanOpenDeviceFromEdsDcf(CanopenDcfDevices, &NumberOfDevices);
	printf("-----SV_main.c : No of canopen Eds/ Dcf files read : %d \n", NumberOfFilesRead);
	printf("-----SV_main.c : pointer to the list of canopenDcfDevices is *CanopenDcfDevices \n");
	printf("-----SV_main.c : No of devices in the Eds / Dcf files : %d \n", NumberOfDevices);

	int retval = CreateCanOpenMaster(NumberOfDevices);
	printf("-----SV_main.c : retval from CreateCanOpenMaster : %d \n", retval);
	/* end Canopen related functions */
	#endif

	/* ---------- start creating a thread */
	#ifdef COMMENT
	Spawn a separate thread that calls initialiseCanopen().
	1. create a OPCUA Client object and connect to the OPCUA server instance
	2. start the Canopen event loop to poll for data from Canopen slaves
	3. updates the data to OPCUA server
	#endif

	pthread_t canopenthread;

	int result = pthread_create(&canopenthread, NULL, initialiseCanopen, server);	// pass in the pointer to the UA_Server, it will be used to create a UA_Client connection in initialiseCanopen()
	if (result != 0)
	{
		printf("---------------------------------------------------------------------------------\n");
		printf("------SV_main.c : Fail to create canopen thread as a separate process            \n");
		printf("------SV_main.c : Canopen functions will not be available until the next restart \n");
		printf("---------------------------------------------------------------------------------\n");
	}
	else
	{
		int result = pthread_detach(canopenthread);
		if (result != 0)
		{
			printf("-----SV_main.c : cannot detach canopen thread from main thread \n");
			exit(0);
		}
		else
		{
			printf("--------------------------------------------------------------------------\n");
			printf("------SV_main.c : canopen thread is created successfully                  \n");
			printf("------SV_main.c : starting canopen thread and waiting for devices to poll \n");
			printf("--------------------------------------------------------------------------\n");
		}
	}
	/* the sleep(3) function is mandatory */
	/* without it, the application will freeze and cannot move to the prompt (userid and password for opcServer) */
	printf("...."); sleep(3);
	/* ---------- end creating a thread */

}

// search for a running instance
int main(int argc, char *argv[])
{
        //pthread_t OPCUAServerthread;
        //pthread_t Airgardthread;
	int status;

	UA_Nodes_Setup = UA_FALSE;
	//UA_NodeId r2_airgard_data_Id;
        switch (argc)
        {
		case 1: 
			argv[0] = (char *)"./myNewServer" ;
			argv[1] = (char *)"192.168.1.123";	// local ip : 192.168.1.123
			argv[2] = (char *)"192.168.1.157";	// sensor ip : 192.168.1.157
			argv[3] = (char *)"192.168.1.44";	// lds ip "OPCLds-44";	// to match the hostname of the LDS server, no need to define in /etc/hosts
			argv[4] = NULL;	// broker ip : 192.168.1.157
			argv[5] = NULL;		// broker port :  1883
			argv[6] = (char *)"nomode";
			printf("%s %s %s %s %s \n", argv[0], argv[1], argv[2], argv[3], argv[6]);

			#ifdef KIV
			if (check_port_active(argv[2], "20000") != 0)	// sensor ip
			{
				printf("Sensor is not running/ active \n");
				exit(0);
			}

			if (check_port_active(argv[3], "4841") != 0)	// lds ip
			{
				printf("LDS (%s:%d) is not running/ active \n", argv[3], 4841);
				exit(0);
			}

			if (check_port_active(argv[4], argv[5] != 0))	// broker ip
			{
				printf("Message broker (%s:%s) is not running/ active \n", argv[4], argv[5]);
				exit(0);
			}
			#endif

			break;
/*
		case 1: printf("Usage : ./myNewServer <local ip> <sensor ip> <LDS ip> [<*broker ip> <port> <{--all}|--pub|--sub] \n");
                        printf("Note* : port number to differentiate between MQTT (1883) or AMQP (5672) \n");
                        exit(0);
*/
		case 2: // to test reverseConnect, need the local ip
			argv[2] = NULL; // sensor ip
			argv[3] = (char *)"192.168.1.44"; // LDS ip
			argv[4] = NULL; // broker ip
			argv[5] = (char *)"0"; // port
			argv[6] = (char *)"nomode"; // mode
			printf("%s %s %s %s %s \n", argv[0], argv[1], argv[3], argv[5], argv[6]);
			break;

                case 4: 
			argv[4] = NULL; //broker ip
			argv[5] = (char *)"0";	// port
			argv[6] = (char *)"nomode"; //mode
			printf("%s %s %s %s %s %s \n", argv[0], argv[1], argv[2], argv[3], argv[5], argv[6]);
			break;

                case 5: printf("Usage : ./myNewServer <local ip> <sensor ip> <LDS ip> [<*broker ip> <port> <{--all}|--pub|--sub] \n");
                        printf("Note* : port number to differentiate between MQTT (1883) or AMQP (5672) \n");
                        exit (0);

                case 6: printf("%s %s %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3], argv[4], argv[5]);
			argv[6] = (char *)"--all"; // mode
                        break;

		case 7: break; // ok

                default:
                        printf("Usage : ./myNewServer <local ip> <sensor ip> <LDS ip> [<*broker ip> <port> <{--all}|--pub|--sub] \n");
                        printf("Note* : port number to differentiate between MQTT (1883) or AMQP (5672) \n");
                        exit (0);
        }

	char reply = 'N';
	UA_Server *server;
	printf("Do you want to load UA Server configuration from file (y/n) : "); scanf("%c", &reply);
	if (toupper(reply) == 'N')	// use system-defined nodes (i.e in source code
	{
	        server = UA_Server_new();	// UA_Server_new(config)
	   	status = UA_STATUSCODE_GOOD;
	}
	else	// load UA nodes from file
	{
	   // function prototype declaration
	   UA_Server *UA_Server_newFromFile(UA_ByteString);

	   UA_ByteString json_config = UA_BYTESTRING_NULL;
	   status = UA_STATUSCODE_GOOD;
	   char filename[100];

	   printf("Please enter the json config file (json5ConfigFile) : ");
	   scanf("%s", filename);

	   json_config = loadFile(filename);
	   server = UA_Server_newFromFile(json_config);
	   if (server == NULL)
	   {
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"--------SV_main.c : Error loading from %s ", filename);
		exit(0);
	   }
	   else
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_main.c :  Json config file loaded successfully.");
	}

//   #ifdef USE_ENV_JSON
//	int res = LoadAirgardEnvJson();
//	if (res != 0)
//	{
//		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"--------SV_main.c : Error loading Environment config file");
		//exit(0);
//	}
//  #endif
	// prompt for userid and password for opcServer => to be replaced by OPCpasswd file

	char userid[255], passwd[255];
	printf("Please provide the userid and password for opcServer : "); scanf("%s %s", userid, passwd);

	// save the password to a temporary file
	FILE *fp = fopen("passwordfile", "w");
        if (fp == NULL) {
         	UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_main.c : Unknown error : cannot create temporary file <passwordfile>");
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_main.c : Check the group and owner settings of <passwordfile>");
           	exit(0);
        }
	fwrite(passwd, sizeof(char), strlen(passwd), fp);
	fclose(fp);

	if (passwordHash(2, (char *)"passwordfile") != 0)
	{
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"--------SV_main.c : Hashing your password failed");
		exit(0);
	}
	// after hashing the password, it is stored in a global variable <char hashpassword[255]>
	// inspect the hashpassword
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_main.c : The generated hashpassword is %s", hashpassword);


	// prompt for userid and password for mySQL
	//TODO



	UA_ServerConfig *config = UA_Server_getConfig(server);
	UA_ServerConfig_setDefault(config);

        size_t namespaceIndex;
        UA_Int16 nsIdx_MKS = UA_Server_addNamespace(server, "virtualskies.com.sg/MKS/");
        UA_Server_getNamespaceByName(server, UA_STRING((char *)"virtualskies.com.sg/MKS/"), &namespaceIndex);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_main.c : New Namespace added: <virtualskies.com.sg/MKS/>, nsIdx_MKS assigned is %d", nsIdx_MKS);



        g_argc = argc;

	char reply2 = 'Y';
	printf("--------\n");
	printf("Do you want the OPCUA Server to generate its own ssl certificate/ key (y/n) : ");
	scanf("%c", &reply2);
	scanf("%c", &reply2);
	printf("reply2 is %c \n", reply2);

	if ( toupper(reply2) == 'Y' )
		encryptServer(server, config, UA_TRUE);
	else
		encryptServer(server, config, UA_FALSE);

	// Note: if ConnectToRadius comes before ConnectToLdap, error occurs
	// -- ConnectToLdap(userid, hashpassword);
	// -- ConnectToRadius(userid, hashpassword);

        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_main.c : OPCUA server ready to start ");
	configureServer(server, userid, hashpassword);
        status = UA_Server_run_startup(server);
        if (status != UA_STATUSCODE_GOOD)
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"--------SV_main.c : Could not start and run OPCUA Server : %s", UA_StatusCode_name(status));
        else
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_main.c : OPCUA server started successfully ...");


	if (status == UA_STATUSCODE_GOOD)
	{
		// Activate multicast-capabilities
		// callback which is called when a new server is detected through mDNS
    		// needs to be set after UA_Server_run_startup or UA_Server_run
		UA_Server_setServerOnNetworkCallback(server, serverOnNetworkCallback, NULL);
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "--------SV_main.c : Server started. Waiting for announce of LDS Server.");

		// start making function call
//		UA_UInt64 reverseConnecthandle = reverseConnect(server, argv[2]);

		UA_NodeId *nodes;
		UA_NodeId  r2_airgard_method_Id, r2_airgard_event_Id;

	   	nodes = createNodes(server);
		r2_airgard_method_Id = nodes[0];
  	   	r2_airgard_event_Id = nodes[1];
//	   	createMethods(server, r2_airgard_method_Id);
//	   	createEvents(server, r2_airgard_event_Id);
		createMonitoredItems(server); // comment out if want to test ADS

//		createRepeatedCallbacks(server);
//	   	GetHistoryDBConnection(server);
//	  	createHistorizingItems(server);

		if (argv[5] == NULL) // brokerless-based pubsub
		{
			#define UA_ENABLE_PUBSUB
			pubSubInitialiseBrokerless(server, argv[6]);
			//UA_Server_enableAllPubSubComponents(server);
		}
		else
		{
			#define UA_ENABLE_PUBSUB
			int port = atoi(argv[5]);
                	pubSubInitialiseWithBroker(server, argv[4],port,argv[6]);
			//UA_Server_enableAllPubSubComponents(server);
		}

				// finally execute other new functions

				// TODO createAlarmsAndConditions(server);	// KIV since github codes are not ready
				// TODO LDS   // somehow if i activate LDS, then pubsub UADP won't work
	 			UA_ClientConfig *lds=NULL;
				// TODO UA_ClientConfig *lds = (UA_ClientConfig*) registerToLDS(server, argv[3]);

				// TODO GDS
				//
				/*------ Start a new thread and launch Canopen instance -----*/
//				StartCanopen(server);
				/* ----------------------------------------------------------*/

//				ConnectToTwinCATAds(server); //remoteIpV4, remoteAms, remotePlcPort, server);
				// NOT here : createModbusExtension(server); - this function is called in SV_Callbacks.c
				// NOT here : createWebSockets(server, argv[1]); - this function is called in SV_PubSubInitialiseBrokerless.c  and SV_PubSubInitialistWithBroker.c


printf("SV_main.c : just before while(running) \n");

// This is how we can use an external "main loop" to integrate the OPCServer into another event loop used in GUI toolkits.
// step 1 : calls UA_Server_run_startup(server)
// step 2 : in a while loop, calls UA_Server_run_iterate();

// Alternatively, just call UA_Server_run(server, true);

		UA_Boolean firstTime = UA_TRUE;
		while (running)
		{
			UA_Server_run_iterate(server, true);
			if (firstTime)
			{
				// because a uaClient instance needs to connect to a running instance of OPCUA server, this function can only be called after run_iterate()
				firstTime = UA_FALSE;
				initialiseWebSocketsWssHttps(server);
			}
		}

		if (!running)
		{
			if (lds != NULL)
			{
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_main.c : attempting to deregister Discovery object");
				int status = UA_Server_deregisterDiscovery(server, lds, UA_STRING(argv[3]));
				if (status != UA_STATUSCODE_GOOD)
					UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     					"--------SV_main.c : Could not unregister OPCUA server from discovery server. StatusCode %s",
                     					UA_StatusCode_name(status));

//				UA_Server_removeReverseConnect(server, reverseConnecthandle);
				UA_Server_run_shutdown(server);
 				UA_Server_delete(server);

			}
                }
		return EXIT_SUCCESS;
	}
}
