#ifdef no_almagamation
  #include <open62541/client_subscriptions.h>
  #include <open62541/plugin/log_stdout.h>
  #include <open62541/server.h>
  #include <open62541/server_config_default.h>
#else
  #include "open62541.h"
#endif
#include "SV_Monitor.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_STRING_SIZE 64
//#include <mariadb/mysql.h>
//#include "myNewMonitor.h"
extern char SoftwareVersion[MAX_STRING_SIZE];
extern char DataBlockVersion[MAX_STRING_SIZE];
extern char InstrumentTime[MAX_STRING_SIZE];
extern char MeasurementTime[MAX_STRING_SIZE];

extern char BootStatus[MAX_STRING_SIZE];
extern char SnapshotStatus[MAX_STRING_SIZE];
extern char SCPStatus[MAX_STRING_SIZE];
extern char SFTPStatus[MAX_STRING_SIZE];
extern char RunScriptStatus[MAX_STRING_SIZE];
extern char ArchiveStatus[MAX_STRING_SIZE];
extern char AncillarySensorStatus[MAX_STRING_SIZE];

extern char Sensor[MAX_STRING_SIZE];
extern UA_Int16 OperatingTime;
extern char WarningMessage[MAX_STRING_SIZE];

extern UA_Float IgramPP;
extern UA_Float IgramDC;
extern UA_Float LaserPP;
extern UA_Float LaserDC;
extern UA_Float SingleBeamAt900;
extern UA_Float SingleBeamAt2500;
extern UA_Int16 SignalToNoiseAt2500;            //----integer
extern UA_Float CenterBurstLocation;
extern UA_Float DetectorTemp;
extern UA_Float LaserFrequency;
extern UA_Int16 HardDriveSpace;                 //----integer
extern UA_Int16 Flow;                           //----integer
extern UA_Int16 Temperature;                    //----integer
extern UA_Float Pressure;
extern UA_Int16 TempOptics;                     //----integer
extern UA_Int16 BadScanCounter;                 //----integer
extern UA_Int16 FreeMemorySpace;                //----integer

extern char LABFilename[MAX_STRING_SIZE];
extern char LOGFilename[MAX_STRING_SIZE];
extern char LgFilename[MAX_STRING_SIZE];
extern char SecondLgFilename[MAX_STRING_SIZE];

extern UA_Float SystemCounter;
extern UA_Float DetectorCounter;
extern UA_Float LaserCounter;
extern UA_Float FlowPumpCounter;
extern UA_Float DesiccantCounter;

extern UA_Int16 NoOfAlarms;
extern UA_Int16 NoOfNonAlarms;

extern UA_NodeId outSoftwareVersion_Id;
extern UA_NodeId outDataBlockVersion_Id;
extern UA_NodeId outInstrumentTime_Id;
extern UA_NodeId outMeasurementTime_Id;
extern UA_NodeId outSensor_Id;
extern UA_NodeId outOperatingTime_Id;
extern UA_NodeId outWarningMessage_Id;
extern UA_NodeId outBootStatus_Id;
extern UA_NodeId outSnapshotStatus_Id;
extern UA_NodeId outSCPStatus_Id;
extern UA_NodeId outSFTPStatus_Id;
extern UA_NodeId outRunScriptStatus_Id;
extern UA_NodeId outArchiveStatus_Id;
extern UA_NodeId outAncilarySensor_Id;

extern UA_NodeId outIgramPP_Id;
extern UA_NodeId outIgramDC_Id;
extern UA_NodeId outLaserPP_Id;
extern UA_NodeId outLaserDC_Id;
extern UA_NodeId outSingleBeamAt900_Id;
extern UA_NodeId outSingleBeamAt2500_Id;
extern UA_NodeId outSignalToNoiseAt2500_Id;
extern UA_NodeId outCenterBurstLocation_Id;
extern UA_NodeId outDetectorTemp_Id;
extern UA_NodeId outLaserFrequency_Id;
extern UA_NodeId outHardDriveSpace_Id;
extern UA_NodeId outFlow_Id;
extern UA_NodeId outTemperature_Id;
extern UA_NodeId outPressure_Id;
extern UA_NodeId outTempOptics_Id;
extern UA_NodeId outBadScanCounter_Id;
extern UA_NodeId outFreeMemorySpace_Id;

extern UA_NodeId outLABFilename_Id;
extern UA_NodeId outLOGFilename_Id;
extern UA_NodeId outLgFilename_Id;
extern UA_NodeId outSecondLgFilename_Id;

extern UA_NodeId outSystemCounter_Id;
extern UA_NodeId outDetectorCounter_Id;
extern UA_NodeId outLaserCounter_Id;
extern UA_NodeId outFlowPumpCounter_Id;
extern UA_NodeId outDesiccantCounter_Id;

//extern MYSQL *conn;

//FILE *fpWs;	// this handle writes UA nodes to a html file
int fd;

// function prototypes
void writeHistoryData(const UA_NodeId *nodeId, const UA_DataValue *value);


void dataChangeNotificationCallback(UA_Server *server, UA_UInt32 monitoredItemId,
                               void *monitoredItemContext, const UA_NodeId *nodeId,
                               void *nodeContext, UA_UInt32 attributeId,
                               const UA_DataValue *value)
{
//    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_Monitor.c : dataChangeNotificationCallback : Receiving Notification .. data (MonitoredItem) has changed");

// ------------- only write to DB where the monitoreditem has a monitoredItemContext = UA_True
	UA_Boolean writeToDB = (UA_Boolean)monitoredItemContext;
	#ifdef DEBUG
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        	"--------SV_Monitor.c : in dataChangeNotificationCallback() function : writeToDB Flag is <%d>", writeToDB);
	#endif
	if (writeToDB == UA_TRUE)
	{
		writeHistoryData(nodeId, value);
	}

#ifdef NOT_WROKING
	if (fpWs == NULL) {
		fpWs = fopen("/home/pi/OPCProject/mount-origin/index.html", "w");
	}
	//else {
	// updates the changes to index.html - this is so that the index.html file can be updated with the latest values
	fprintf(fpWs, "<SoftwareVersion>%s</SoftwareVersion>\n", SoftwareVersion);
	fprintf(fpWs, "<DataBlockVersion>%s</DataBlockVersion>\n", DataBlockVersion);
	fprintf(fpWs, "<InstrumentTime>%s</InstrumentTime>\n", InstrumentTime);
	fprintf(fpWs, "<MeasurementTime>%s</MeasurementTime>\n\n", MeasurementTime);

	fprintf(fpWs, "<BootStatus>%s</BootStatus>\n", BootStatus);
	fprintf(fpWs, "<SnapshotStatus>%s</SnapshotStatus>\n", SnapshotStatus);
	fprintf(fpWs, "<SCPStatus>%s</SCPStatus>\n", SCPStatus);
	fprintf(fpWs, "<SFTPStatus>%s</SFTPStatus>\n", SFTPStatus);
	fprintf(fpWs, "<RunScriptStatus>%s</RunScriptStatus>\n", RunScriptStatus);
	fprintf(fpWs, "<ArchiveStatus>%s</ArchiveStatus>\n", ArchiveStatus);
	fprintf(fpWs, "<AncillarySensorStatus>%s</AncillarySensorStatus>\n\n", AncillarySensorStatus);

	fprintf(fpWs, "<Sensor>%s</Sensor>\n", Sensor);
	fprintf(fpWs, "<OperatingTime>%d</OperatingTime>\n", OperatingTime);
	fprintf(fpWs, "<WarningMessage>%s</WarningMessage>\n\n", WarningMessage);

	fprintf(fpWs, "<IgramPP>%f</IgramPP>\n", IgramPP);
	fprintf(fpWs, "<IgramDC>%f</IgramDC>\n", IgramDC);
	fprintf(fpWs, "<LaserPP>%f</LaserPP>\n", LaserPP);
	fprintf(fpWs, "<LaserDC>%f</LaserDC>\n", LaserDC);
	fprintf(fpWs, "<SingleBeamAt900>%f</SingleBeamAt900>\n", SingleBeamAt900);
	fprintf(fpWs, "<SingleBeamAt2500>%f</SingleBeamAt2500>\n", SingleBeamAt2500);
	fprintf(fpWs, "<SignalToNoiseAt2500>%d</SignalToNoiseAt2500>\n", SignalToNoiseAt2500);
	fprintf(fpWs, "<CenterBurstLocation>%f</CenterBurstLocation>\n", CenterBurstLocation);
	fprintf(fpWs, "<DetectorTemp>%f</DetectorTemp>\n", DetectorTemp);
	fprintf(fpWs, "<LaserFrequency>%f</LaserFrequency>\n", LaserFrequency);
	fprintf(fpWs, "<HardDriveSpace>%d</HardDriveSpace>\n", HardDriveSpace);
	fprintf(fpWs, "<Flow>%d</Flow>\n", Flow);
	fprintf(fpWs, "<Temperature>%d</Temperature>\n", Temperature);
	fprintf(fpWs, "<Pressure>%f</Pressure>\n", Pressure);
	fprintf(fpWs, "<TempOptics>%d</TempOptics>\n", TempOptics);
	fprintf(fpWs, "<BadScanCounter>%d</BadScanCounter>\n", BadScanCounter);
	fprintf(fpWs, "<FreeMemorySpace>%d</FreeMemorySpace>\n\n", FreeMemorySpace);

	fprintf(fpWs, "<LABFilename>%s</LABFilename>\n", LABFilename);
	fprintf(fpWs, "<LOGFilename>%s</LOGFilename>\n", LOGFilename);
	fprintf(fpWs, "<LgFilename>%s</LgFilename>\n", LgFilename);
	fprintf(fpWs, "<SecondLgFilename>%s</SecondLgFilename>\n\n", SecondLgFilename);

	fprintf(fpWs, "<SystemCounter>%f</SystemCounter>\n", SystemCounter);
	fprintf(fpWs, "<DetectorCounter>%f</DetectorCounter>\n", DetectorCounter);
	fprintf(fpWs, "<LaserCounter>%f</LaserCounter>\n", LaserCounter);
	fprintf(fpWs, "<FlowPumpCounter>%f</FlowPumpCounter>\n", FlowPumpCounter);
	fprintf(fpWs, "<DesiccantCounter>%f</DesiccantCounter>\n\n", DesiccantCounter);

	fprintf(fpWs, "<NoOfAlarms>%d</NoOfAlarms>\n", NoOfAlarms);
	fprintf(fpWs, "<NoOfNonAlarms>%d</NoOfNonAlarms>\n", NoOfNonAlarms);

	if (fpWs != NULL)
		fclose(fpWs); // need to close so that wget can read the file
	//}
#endif

	mode_t permission = 0755;
	if (mkdir("/home/pi/OPCProject/mount-origin", permission) == 0)
		printf("/home/pi/OPCProject/mount-origin/ created successfully \n");
	else
	{
		if (errno == EEXIST)
			printf("/home/pi/OPCProject/mount-origin/ exist.  Proceed to create index.html \n");
		else
			perror("Error creating directory /home/pi/OPCProject/mount-origin/");
	}

	fd = open("/home/pi/OPCProject/mount-origin/index.html", O_CREAT | O_WRONLY | O_TRUNC, 0644);
	// O_CREAT: Create the file if it doesn't exist.
    	// O_WRONLY: Open for writing only.
    	// O_TRUNC: Truncate the file to zero length if it exists.
    	// 0644: Permissions for the new file (read/write for owner, read for group/others).
    	if (fd == -1) {
        	// Handle error (e.g., print an error message and exit)
		perror("Cannot create file descriptor for index.html");
		exit(-1);
    	}
	else
	{
		char buffer[32767] = "\0";
		char tbuffer[32767] ="\0";
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Monitor.c : Preparing the dump buffer ");
	sprintf(tbuffer, "<!DOCTYPE html>\n"); strcat(buffer, tbuffer);
	sprintf(tbuffer, "<html>\n"); strcat(buffer, tbuffer);
	sprintf(tbuffer, "<head>\n"); strcat(buffer, tbuffer);

	sprintf(tbuffer, "\t<title>Sample HTML generated using Airgard data</title>\n"); strcat(buffer, tbuffer);
	sprintf(tbuffer, "</head>\n"); strcat(buffer, tbuffer);

	sprintf(tbuffer, "<body>\n"); strcat(buffer, tbuffer);
	sprintf(tbuffer, "\t<p>&#60;SoftwareVersion&#62;%s&#60;/SoftwareVersion&#62;</p>\n", SoftwareVersion);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;DataBlockVersion&#62;%s&#60;/DataBlockVersion&#62;</p>\n", DataBlockVersion);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;InstrumentTime&#62;%s&#60;/InstrumentTime&#62;</p>\n", InstrumentTime);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;MeasurementTime&#62;%s&#60;/MeasurementTime&#62;</p>\n\n", MeasurementTime);strcat(buffer, tbuffer);

        sprintf(tbuffer, "\t<p>&#60;BootStatus&#62;%s&#60;/BootStatus&#62;</p>\n", BootStatus);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;SnapshotStatus&#62;%s&#60;/SnapshotStatus\n", SnapshotStatus);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;SCPStatus&#62;%s&#60;/SCPStatus&#62;</p>\n", SCPStatus);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;SFTPStatus&#62;%s&#60;/SFTPStatus&#62;</p>\n", SFTPStatus);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;RunScriptStatus&#62;%s&#60;/RunScriptStatus&#62;</p>\n", RunScriptStatus);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;ArchiveStatus&#62;%s&#60;/ArchiveStatus&#62;</p>\n", ArchiveStatus);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;AncillarySensorStatus&#62;%s&#60;/AncillarySensorStatus&#62;</p>\n\n", AncillarySensorStatus);strcat(buffer, tbuffer);

        sprintf(tbuffer, "\t<p>&#60;Sensor&#62;%s&#60;/Sensor&#62;</p>\n", Sensor);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;OperatingTime&#62;%d&#60;/OperatingTime&#62;</p>\n", OperatingTime);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;WarningMessage&#62;%s&#60;/WarningMessage&#62;</p>\n\n", WarningMessage);strcat(buffer, tbuffer);

        sprintf(tbuffer, "\t<p>&#60;IgramPP&#62;%f&#60;/IgramPP&#62;</p>\n", IgramPP);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;IgramDC&#62;%f&#60;/IgramDC&#62;</p>\n", IgramDC);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;LaserPP&#62;%f&#60;/LaserPP&#62;</p>\n", LaserPP);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;LaserDC&#62;%f&#60;/LaserDC&#62;</p>\n", LaserDC);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;SingleBeamAt900&#62;%f&#60;/SingleBeamAt900&#62;</p>\n", SingleBeamAt900);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;SingleBeamAt2500&#62;%f&#60;/SingleBeamAt2500&#62;</p>\n", SingleBeamAt2500);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;SignalToNoiseAt2500&#62;%d&#60;/SignalToNoiseAt2500&#62;</p>\n", SignalToNoiseAt2500);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;CenterBurstLocation&#62;%f&#60;/CenterBurstLocation&#62;</p>\n", CenterBurstLocation);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;DetectorTemp&#62;%f&#60;/DetectorTemp&#62;</p>\n", DetectorTemp);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;LaserFrequency&#62;%f&#60;/LaserFrequency&#62;</p>\n", LaserFrequency);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;HardDriveSpace&#62;%d&#60;/HardDriveSpace&#62;</p>\n", HardDriveSpace);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;Flow&#62;%d&#60;/Flow&#62;</p>\n", Flow);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;Temperature&#62;%d&#60;/Temperature&#62;</p>\n", Temperature);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;Pressure&#62;%f&#60;/Pressure&#62;</p>\n", Pressure);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;TempOptics&#62;%d&#60;/TempOptics&#62;</p>\n", TempOptics);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;BadScanCounter&#62;%d&#60;/BadScanCounter&#62;</p>\n", BadScanCounter);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;FreeMemorySpace&#62;%d&#60;/FreeMemorySpace&#62;</p>\n\n", FreeMemorySpace);strcat(buffer, tbuffer);

	sprintf(tbuffer, "\t<p>&#60;LABFilename&#62;%s&#60;/LABFilename&#62;</p>\n", LABFilename);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;LOGFilename&#62;%s&#60;/LOGFilename&#62;</p>\n", LOGFilename);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;LgFilename&#62;%s&#60;/LgFilename&#62;</p>\n", LgFilename); strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;SecondLgFilename&#62;%s&#60;/SecondLgFilename&#62;</p>\n\n", SecondLgFilename); strcat(buffer, tbuffer);

        sprintf(tbuffer, "\t<p>&#60;SystemCounter&#62;%f&#60;/SystemCounter&#62;</p>\n", SystemCounter);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;DetectorCounter&#62;%f&#60;/DetectorCounter&#62;</p>\n", DetectorCounter);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;LaserCounter&#62;%f&#60;/LaserCounter&#62;</p>\n", LaserCounter);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;FlowPumpCounter&#62;%f&#60;/FlowPumpCounter&#62;</p>\n", FlowPumpCounter);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;DesiccantCounter&#62;%f&#60;/DesiccantCounter&#62;</p>\n\n", DesiccantCounter);strcat(buffer, tbuffer);

        sprintf(tbuffer, "\t<p>&#60;NoOfAlarms&#62;%d&#60;/NoOfAlarms&#62;</p>\n", NoOfAlarms);strcat(buffer, tbuffer);
        sprintf(tbuffer, "\t<p>&#60;NoOfNonAlarms&#62;%d&#60;/NoOfNonAlarms&#62;</p>\n", NoOfNonAlarms);strcat(buffer, tbuffer);
	sprintf(tbuffer, "</body>\n"); strcat(buffer, tbuffer);

	sprintf(tbuffer, "</html>\n");strcat(buffer, tbuffer);
	printf("%s", buffer);

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Monitor.c : Preparing to write the buffer to the file ");
        	ssize_t bytes_written = write(fd, buffer, sizeof(buffer) - 1); // -1 to exclude null terminator
        	if (bytes_written == -1) {
            		// Handle error
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Monitor.c : Fail to write buffer to index.html.. system will try again in the next cycle");
        	}
		else
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Monitor.c : Successfully written the buffer to index.html");
	}
	if (fd != -1) close(fd);
}

void addMonitoredItemToSoftwareVersionVariable(UA_Server *server)
{
        //UA_NodeId SoftwareVersionNodeId = UA_NODEID_NUMERIC(0, 10001);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outSoftwareVersion_Id); //(SoftwareVersionNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_FALSE, dataChangeNotificationCallback);
}

void addMonitoredItemToDataBlockVersionVariable(UA_Server *server)
{
        //UA_NodeId DataBlockVersionNodeId = UA_NODEID_NUMERIC(0, 10002);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outDataBlockVersion_Id); //(DataBlockVersionNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_FALSE, dataChangeNotificationCallback);
}


void addMonitoredItemToInstrumentTimeVariable(UA_Server *server)
{
        //UA_NodeId InstrumentTimeNodeId = UA_NODEID_NUMERIC(0, 10101);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outInstrumentTime_Id);	//(InstrumentTimeNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_FALSE, dataChangeNotificationCallback);
}

void addMonitoredItemToMeasurementTimeVariable(UA_Server *server)
{
        //UA_NodeId MeasurementTimeNodeId = UA_NODEID_NUMERIC(0, 10102);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outMeasurementTime_Id); //(MeasurementTimeNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_FALSE, dataChangeNotificationCallback);
}

void addMonitoredItemToBootStatusVariable(UA_Server *server)
{
        //UA_NodeId BootStatusNodeId = UA_NODEID_NUMERIC(0, 10211);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outBootStatus_Id); //(BootStatusNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_FALSE, dataChangeNotificationCallback);
}

void addMonitoredItemToSnapshotStatusVariable(UA_Server *server)
{
        //UA_NodeId SnapshotStatusNodeId = UA_NODEID_NUMERIC(0, 10212);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outSnapshotStatus_Id);	//(SnapshotStatusNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_FALSE, dataChangeNotificationCallback);
}

void addMonitoredItemToSCPStatusVariable(UA_Server *server)
{
        //UA_NodeId SCPStatusNodeId = UA_NODEID_NUMERIC(0, 10213);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outSCPStatus_Id); //(SCPStatusNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_FALSE, dataChangeNotificationCallback);
}

void addMonitoredItemToSFTPStatusVariable(UA_Server *server)
{
        //UA_NodeId SFTPStatusNodeId = UA_NODEID_NUMERIC(0, 10214);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outSFTPStatus_Id); //(SFTPStatusNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_FALSE, dataChangeNotificationCallback);
}

void  addMonitoredItemToRunScriptStatusVariable(UA_Server *server)
{
        //UA_NodeId RunScriptStatusNodeId = UA_NODEID_NUMERIC(0, 10215);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outRunScriptStatus_Id); //(RunScriptStatusNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_FALSE, dataChangeNotificationCallback);
}

void addMonitoredItemToArchiveStatusVariable(UA_Server *server)
{
        //UA_NodeId ArchiveStatusNodeId = UA_NODEID_NUMERIC(0, 10216);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outArchiveStatus_Id); //(ArchiveStatusNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_FALSE, dataChangeNotificationCallback);
}

void addMonitoredItemToAncillarySensorStatusVariable(UA_Server *server)
{
        //UA_NodeId AncillarySensorStatusNodeId = UA_NODEID_NUMERIC(0, 10217);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outAncilarySensor_Id); //(AncillarySensorStatusNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_FALSE, dataChangeNotificationCallback);
}

void addMonitoredItemToSensorVariable(UA_Server *server)
{
        //UA_NodeId SensorNodeId = UA_NODEID_NUMERIC(0, 10201);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outSensor_Id); //SensorNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_FALSE, dataChangeNotificationCallback);
}

void addMonitoredItemToOperatingTimeVariable(UA_Server *server)
{
        //UA_NodeId OperatingTimeNodeId = UA_NODEID_NUMERIC(0, 10202);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outOperatingTime_Id); //OperatingTimeNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_FALSE, dataChangeNotificationCallback);
}

void addMonitoredItemToWarningMessageVariable(UA_Server *server)
{
        //UA_NodeId WarningMessageNodeId = UA_NODEID_NUMERIC(0, 10203);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outWarningMessage_Id); //WarningMessageNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_FALSE, dataChangeNotificationCallback);
}


void addMonitoredItemToIgramPPVariable(UA_Server *server)
{
#ifdef _DEBUG
if (UA_NodeId_isNull(&outIgramPP_Id))
{
        printf("SV_Monitor.c : ERROR!!!!!!!!!!!!! outIgramPP_Id is NULL");
        exit(0);
}
#endif
  	//UA_NodeId IgramPPNodeId = UA_NODEID_NUMERIC(1, 10301);
    	UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outIgramPP_Id); //(IgramPPNodeId);
    	monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
    	UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToIgramDCVariable(UA_Server *server)
{
        //UA_NodeId IgramDCNodeId = UA_NODEID_NUMERIC(1, 10302);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outIgramDC_Id);  // (IgramDCNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToLaserPPVariable(UA_Server *server)
{
        //UA_NodeId LaserPPNodeId = UA_NODEID_NUMERIC(1, 10303);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outLaserPP_Id); // (LaserPPNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToLaserDCVariable(UA_Server *server)
{
        //UA_NodeId LaserDCNodeId = UA_NODEID_NUMERIC(1, 10304);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outLaserDC_Id); // (LaserDCNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToSingleBeamAt900Variable(UA_Server *server)
{
        //UA_NodeId SingleBeamAt900NodeId = UA_NODEID_NUMERIC(1, 10305);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outSingleBeamAt900_Id);  // (SingleBeamAt900NodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToSingleBeamAt2500Variable(UA_Server *server)
{
        //UA_NodeId SingleBeamAt2500NodeId = UA_NODEID_NUMERIC(1, 10306);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outSingleBeamAt2500_Id); // (SingleBeamAt2500NodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToSignalToNoiseAt2500Variable(UA_Server *server)
{
        //UA_NodeId SignalToNoiseAt2500NodeId = UA_NODEID_NUMERIC(1, 10307);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outSignalToNoiseAt2500_Id);  // SignalToNoiseAt2500NodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToCenterBurstLocationVariable(UA_Server *server)
{
        //UA_NodeId CenterBurstLocationNodeId = UA_NODEID_NUMERIC(1, 10308);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outCenterBurstLocation_Id); // (CenterBurstLocationNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToDetectorTempVariable(UA_Server *server)
{
        //UA_NodeId DetectorTempNodeId = UA_NODEID_NUMERIC(1, 10309);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outDetectorTemp_Id); // DetectorTempNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToLaserFrequencyVariable(UA_Server *server)
{
        //UA_NodeId LaserFrequencyNodeId = UA_NODEID_NUMERIC(1, 10310);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outLaserFrequency_Id); // (LaserFrequencyNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToHardDriveSpaceVariable(UA_Server *server)
{
        //UA_NodeId HardDriveSpaceNodeId = UA_NODEID_NUMERIC(1, 10311);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outHardDriveSpace_Id); // (HardDriveSpaceNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToFlowVariable(UA_Server *server)
{
        //UA_NodeId FlowNodeId = UA_NODEID_NUMERIC(1, 10312);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outFlow_Id); // (FlowNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToTemperatureVariable(UA_Server *server)
{
        //UA_NodeId TemperatureNodeId = UA_NODEID_NUMERIC(1, 10313);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outTemperature_Id); // (TemperatureNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToPressureVariable(UA_Server *server)
{
        //UA_NodeId PressureNodeId = UA_NODEID_NUMERIC(1, 10314);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outPressure_Id);  // (PressureNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToTempOpticsVariable(UA_Server *server)
{
        //UA_NodeId TempOpticsNodeId = UA_NODEID_NUMERIC(1, 10315);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outTempOptics_Id); // (TempOpticsNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToBadScanCounterVariable(UA_Server *server)
{
        //UA_NodeId BadScanCounterNodeId = UA_NODEID_NUMERIC(1, 10316);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outBadScanCounter_Id); // (BadScanCounterNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToFreeMemorySpaceVariable(UA_Server *server)
{
        //UA_NodeId FreeMemorySpaceNodeId = UA_NODEID_NUMERIC(1, 10317);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outFreeMemorySpace_Id);  // FreeMemorySpaceNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToLABFilenameVariable(UA_Server *server)
{
        //UA_NodeId LABFilenameNodeId = UA_NODEID_NUMERIC(1, 10318);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outLABFilename_Id); //LABFilenameNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToLOGFilenameVariable(UA_Server *server)
{
        //UA_NodeId LOGFilenameNodeId = UA_NODEID_NUMERIC(1, 10319);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outLOGFilename_Id); //LOGFilenameNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToLgFilenameVariable(UA_Server *server)
{
        //UA_NodeId LgFilenameNodeId = UA_NODEID_NUMERIC(1, 10320);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outLgFilename_Id); //LgFilenameNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToSecondLgFilenameVariable(UA_Server *server)
{
        //UA_NodeId SecondLgFilenameNodeId = UA_NODEID_NUMERIC(1, 10321);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outSecondLgFilename_Id); //SecondLgFilenameNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToSystemCounterVariable(UA_Server *server)
{
        //UA_NodeId SystemCounterNodeId = UA_NODEID_NUMERIC(1, 10322);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outSystemCounter_Id); // (SystemCounterNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToDetectorCounterVariable(UA_Server *server)
{
        //UA_NodeId DetectorCounterNodeId = UA_NODEID_NUMERIC(1, 10323);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outDetectorCounter_Id); // (DetectorCounterNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToLaserCounterVariable(UA_Server *server)
{
        //UA_NodeId LaserCounterNodeId = UA_NODEID_NUMERIC(1, 10324);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outLaserCounter_Id); // (LaserCounterNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToFlowPumpCounterVariable(UA_Server *server)
{
        //UA_NodeId FlowPumpCounterNodeId = UA_NODEID_NUMERIC(1, 10325);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outFlowPumpCounter_Id); // (FlowPumpCounterNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void addMonitoredItemToDesiccantCounterVariable(UA_Server *server)
{
        //UA_NodeId DesiccantCounterNodeId = UA_NODEID_NUMERIC(1, 10326);
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outDesiccantCounter_Id); // (DesiccantCounterNodeId);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

// the following function is called in SV_Event.c
void addMonitoredItemToEvent(UA_Server *server, UA_NodeId outEvent_Id)
{
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(outEvent_Id);
        monRequest.requestedParameters.samplingInterval = 100.0; /* 100 ms interval */
        UA_Server_createDataChangeMonitoredItem(server, UA_TIMESTAMPSTORETURN_SOURCE,
                                            monRequest, (UA_Boolean *)UA_TRUE, dataChangeNotificationCallback);
}

void createMonitoredItems(UA_Server *server)
{
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        ("=========================================================="));
	        UA_LOG_INFO(UA_Log_Stdout,UA_LOGCATEGORY_USERLAND,
        ("--------SV_Monitor.c : Add monitoring items in progress ..."));
        addMonitoredItemToSoftwareVersionVariable(server);	// 1
        addMonitoredItemToDataBlockVersionVariable(server);	// 2
        addMonitoredItemToInstrumentTimeVariable(server);	// 3
        addMonitoredItemToMeasurementTimeVariable(server);	// 4
        UA_LOG_INFO(UA_Log_Stdout,UA_LOGCATEGORY_USERLAND,
        ("--------SV_Monitor.c : Finished adding batch of 1-4  monitoring items ..."));

        addMonitoredItemToBootStatusVariable(server);		// 5
        addMonitoredItemToSnapshotStatusVariable(server);	// 6
        addMonitoredItemToSCPStatusVariable(server);		// 7
        addMonitoredItemToSFTPStatusVariable(server);		// 8
        addMonitoredItemToRunScriptStatusVariable(server);	// 9
        addMonitoredItemToArchiveStatusVariable(server);	// 10
        addMonitoredItemToAncillarySensorStatusVariable(server);	// 11
        UA_LOG_INFO(UA_Log_Stdout,UA_LOGCATEGORY_USERLAND,
        ("--------SV_Monitor.c : Finished adding batch of 5-11  monitoring items ..."));

        addMonitoredItemToSensorVariable(server);		// 12
        addMonitoredItemToOperatingTimeVariable(server);	// 13
        addMonitoredItemToWarningMessageVariable(server);	// 14
        UA_LOG_INFO(UA_Log_Stdout,UA_LOGCATEGORY_USERLAND,
        ("--------SV_Monitor.c : Finished adding batch of 12-14  monitoring items ..."));

#ifdef COMMENT
printf("Segmentation fault when adding IgramPPVariable monitor item \n");
        addMonitoredItemToIgramPPVariable(server);		// 15
        addMonitoredItemToIgramDCVariable(server);		// 16
        addMonitoredItemToLaserPPVariable(server);		// 17
        addMonitoredItemToLaserDCVariable(server);		// 18
        addMonitoredItemToSingleBeamAt900Variable(server);	// 19
        addMonitoredItemToSingleBeamAt2500Variable(server);	// 20
        UA_LOG_INFO(UA_Log_Stdout,UA_LOGCATEGORY_USERLAND,
        ("--------SV_Monitor.c : Finished adding batch of 15-20  monitoring items ..."));
#endif

#ifdef COMMENT
printf("Segmentation fault when adding SignalToNoiseAt2500Variable monitor item \n");
        addMonitoredItemToSignalToNoiseAt2500Variable(server);	// 21
        addMonitoredItemToCenterBurstLocationVariable(server);	// 22
        addMonitoredItemToDetectorTempVariable(server);		// 23
        addMonitoredItemToLaserFrequencyVariable(server);	// 24
        addMonitoredItemToHardDriveSpaceVariable(server);	// 25
        addMonitoredItemToFlowVariable(server);			// 26
        addMonitoredItemToTemperatureVariable(server);		// 27
        addMonitoredItemToPressureVariable(server);		// 28
        addMonitoredItemToTempOpticsVariable(server);		// 29
        addMonitoredItemToBadScanCounterVariable(server);	// 30
        UA_LOG_INFO(UA_Log_Stdout,UA_LOGCATEGORY_USERLAND,
        ("--------SV_Monitor.c : Finished adding batch of 21-30  monitoring items ..."));

#endif

#ifdef COMMENT
printf("Segmentation fault when adding FreeMemorySpace monitor item \n");
        addMonitoredItemToFreeMemorySpaceVariable(server);	// 31
        addMonitoredItemToLABFilenameVariable(server);		// 32
        addMonitoredItemToLOGFilenameVariable(server);		// 33
	addMonitoredItemToLgFilenameVariable(server);		// 34
        addMonitoredItemToSecondLgFilenameVariable(server);	// 35
        addMonitoredItemToSystemCounterVariable(server);	// 36
        addMonitoredItemToDetectorCounterVariable(server);	// 37
        addMonitoredItemToLaserCounterVariable(server);		// 38
        addMonitoredItemToFlowPumpCounterVariable(server);	// 39
        addMonitoredItemToDesiccantCounterVariable(server);	// 40
        UA_LOG_INFO(UA_Log_Stdout,UA_LOGCATEGORY_USERLAND,
        ("--------SV_Monitor.c : Finished adding batch of 15-40  monitoring items ..."));
#endif

      UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        "--------SV_Monitor.c : Completed creation of monitoring nodes");

}

