#ifdef almagamation
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#else
   #include "open62541.h"
#endif

#include <stdio.h>
#include <mariadb/mysql.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#include "SV_main.h"
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

MYSQL *conn;

// function prototypes
UA_NodeId* createNodes(void* x_void_ptr);

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
		case 1: argv[0]= "./myNewServer" ;
			argv[1] = "192.168.1.33";
			argv[2] = "192.168.1.157";
			argv[3] = "192.168.1.11";
			argv[4] = "192.168.1.44"; //"OPCLds-44";	//"192.168.1.44";
			argv[5] = "1883";
			argv[6] = "--pub";
			argc=7;
			printf("%s %s %s %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);sleep(1);break;
                case 3: break;
                case 5: break;
                case 6: break;
		case 7: break;
                default:
                        printf("Usage : ./myNewServer <local ip> <sensor ip> <LDS ip> [<*broker ip> <port> <{--all}|--pub|--sub] \n");
                        printf("Note* : port number to differentiate between MQTT (1883) or AMQP (5672) \n");
                        exit (0);
        }

        UA_Server *server = UA_Server_new();	// UA_Server_new(config)

	size_t namespaceIndex;
	UA_Int16 nsIdx_MKS = UA_Server_addNamespace(server, "virtualskies.com.sg/MKS/");
	UA_Server_getNamespaceByName(server, UA_STRING("virtualskies.com.sg/MKS/"), &namespaceIndex);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_main.c : New Namespace added: <virtualskies.com.sg/MKS/>, nsIdx_MKS assigned is %d", nsIdx_MKS);

	UA_ServerConfig *config = UA_Server_getConfig(server);
	UA_ServerConfig_setDefault(config);

        g_argc = argc;
//	strcpy(g_argv_ip, argv[1]);	// 192.168.2.88
//	g_argv_port = atoi(argv[2]);	// 20004

//	printf("In main(): g_argc = %d, argv = %s %s %s \n", g_argc, argv[0], argv[1], argv[2]);
//	printf("In main() after processing argv: g_argc = %d, g_argv = %s %s %d\n", g_argc, argv[0], g_argv_ip, g_argv_port);

    //if (results = pthread_create(&OPCUAServerthread, NULL, StartOPCUAServer, server))
		//StartOPCUAServer(server, argv[1], argv); //(server, 192.168.1.109, 192.168.1.11);
	encryptServer(server, config);
	configureServer(server);

        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_main.c : OPCUA server ready to start ");
        status = UA_Server_run_startup(server);
        if (status != UA_STATUSCODE_GOOD)
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"--------SV_main.c : Could not start and run LDS Server : %s", UA_StatusCode_name(status));
        else
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_main.c : OPCUA server started successfully ...");

	registerToLDS(server, argv[4]);
	UA_NodeId *nodes, r2_airgard_method_Id, r2_airgard_event_Id;

	nodes = createNodes(server);
	r2_airgard_method_Id = nodes[0];
	r2_airgard_event_Id = nodes[1];
		createMethods(server, r2_airgard_method_Id);
		createEvents(server, r2_airgard_event_Id);
		createMonitoredItems(server);
		//createAlarmsAndConditions(server);	github codes are not ready
		GetHistoryDBConnection(server);
		createHistorizingItems(server);
		int port = atoi(argv[5]);
		pubSubInitialise(server, argv[3],port,argv[6]);

		// finally execute other new functions
		//createWebSockets(server);
		//createModbusExtension(server);

	//

//	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_mainOPCUAServer.c : OPCUA server ready to start "); 
//		status = UA_Server_run_startup(server);
//                if (status != UA_STATUSCODE_GOOD)
//			UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,"--------SV_mainOPCUAServer.c : Could not start and run LDS Server : %s", UA_StatusCode_name(status));
//                else
//                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_mainOPCUAServer.c : OPCUA server started successfully ...");

		while (running)
			UA_Server_run_iterate(server, true);
		if (!running)
		{
 			UA_Server_delete(server);
                }
		return EXIT_SUCCESS;
}
