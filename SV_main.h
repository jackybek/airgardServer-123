#include <lely/co/co.h>
#include <lely/co/dcf.h>
#include <lely/co/dev.h>
#include <lely/co/detail/obj.h>

#include <modbus/modbus.h>
// SV_main.c
int main(int argc, char *argv[]);
int CreateCanOpenDeviceFromEdsDcf(co_dev_t **);
int CreateCanOpenMaster(void);
void ConnectToLdap(char *, char *);
void ConnectToRadius(char *, char *);

// ENUM to control state of the program
enum status{READ,
            PAD0,
            PAD1,
            FINISH
            };

int ConnectToTwinCATAds(UA_Server *); //(char *, char*, int, UA_Server*);
void *initialiseModbusSlaveInstance(void *);
void *initialiseCanopen(void *);

// SV_PasswordHash.c
_Bool endianCheck(void);
void endianCheckPrint(void);
int calcFileSize(FILE *);
void printFileContents(FILE *);
int fillMessageBlock(FILE *, union messageBlock *, enum status *, __uint64_t *);
void calculateHash(FILE *);
char *passwordHash(int, char *);

// SV_Callbacks.c
void createRepeatedCallbacks(UA_Server *);
void Callback_1(UA_Server *, void *);
void Callback_2(UA_Server *, void *);
void Callback_3(UA_Server *, void *);
void Callback_4(UA_Server *, void *);


// SV_Configure.c
int configureServer(UA_Server *uaLDSServer, char* userid, char* password);

// SV_CreateNodes.c
UA_NodeId* createNodes(void* x_void_ptr);

// SV_Encrypt.c
int stringEndsWithEncrypt(
        char const * const name,
        char const * const extension_to_find);

//int encryptServer(UA_Server *uaSvrServer, UA_ServerConfig *config, bool ForceGenerateSelfSignedCert);
int encryptServer(UA_Server *uaSvrServer, UA_ServerConfig *config, UA_Boolean);

int generateSSCert(UA_Server *uaSvrServer, UA_ServerConfig *config,
                   UA_ByteString *trustList, size_t trustListSize,
                   UA_ByteString *issuerList, size_t issuerListSize,
                   UA_ByteString *revocationList, size_t revocationListSize,
                   UA_ByteString *derPrivKey, UA_ByteString *derCert);

// SV_Events.c
void createEvents(UA_Server *, UA_NodeId);
void CreateServerEvents(UA_Server *, UA_NodeId);
UA_StatusCode addNewEventType(UA_Server *);
UA_StatusCode setUpEvent(UA_Server *, UA_NodeId *);
UA_StatusCode generateEventMethodCallback(UA_Server *,
                         const UA_NodeId *, void *,
                         const UA_NodeId *, void *,
                         const UA_NodeId *, void *,
                         size_t, const UA_Variant *,
                         size_t, UA_Variant *);

void addMonitoredItemToEvent(UA_Server *, UA_NodeId *);

// SV_Alarms.c

void createAlarmsAndConditions(UA_Server *);
UA_StatusCode setUpEnvironment(UA_Server *);
UA_StatusCode enteringConfirmedStateCallback(UA_Server *, const UA_NodeId *);
UA_StatusCode enteringAckedStateCallback(UA_Server *, const UA_NodeId *);
UA_StatusCode enteringEnabledStateCallback(UA_Server *, const UA_NodeId *);
void afterWriteCallbackVariable_3(UA_Server *,
                                const UA_NodeId *, void *,
                                const UA_NodeId *, void *,
                                const UA_NumericRange *, const UA_DataValue *);
void afterWriteCallbackVariable_2(UA_Server *,
                                const UA_NodeId *, void *,
                                const UA_NodeId *, void *,
                                const UA_NumericRange *, const UA_DataValue *);
void afterWriteCallbackVariable_1(UA_Server *,
                                const UA_NodeId *, void *,
                                const UA_NodeId *, void *,
                                const UA_NumericRange *, const UA_DataValue *);
void addVariable_3_returnCondition_1_toNormalState(UA_Server *, UA_NodeId *);
void addVariable_2_changeSeverityOfCondition_2(UA_Server *, UA_NodeId *);
void addVariable_1_triggerAlarmOfCondition_1(UA_Server *, UA_NodeId *);
UA_StatusCode addCondition_2(UA_Server *);
UA_StatusCode addCondition_1(UA_Server *);
UA_StatusCode addConditionSourceObject(UA_Server *);


// SV_WebSocketsInitialise.c
void initialiseWebSocketsWssHttps(UA_Server *);
void createWebSocketsServerWss(void *);
void createWebSocketsServerHttps(void *);

// SV_Historizing.c
void createHistorizingItems(UA_Server *);
void
setValue(UA_Server *server, void *hdgContext, const UA_NodeId *sessionId,
        void *sessionContext, const UA_NodeId *nodeId, UA_Boolean historizing,
        const UA_DataValue *value);
void GetHistoryDBConnection(UA_Server *);
void CloseHistoryDBConnection(void);
void writeHistoryData(const UA_NodeId *nodeId, const UA_DataValue *value);
void CreateServerHistorizingItems(UA_Server *);
void readHistoryData(MYSQL *conn, const UA_NodeId *nodeId);

// SV_Method.c
void createMethods(UA_Server *, UA_NodeId);
void CreateServerMethodItems(UA_Server *server, UA_NodeId r2_airgard_method_Id);

void addGetSoftwareVersionMethod(UA_Server *server, UA_NodeId parent);
void addGetDataBlockVersionMethod(UA_Server *server, UA_NodeId parent);
void addGetInstrumentTimeMethod(UA_Server *server, UA_NodeId parent);
void addGetMeasurementTimeMethod(UA_Server *server, UA_NodeId parent);

void addGetBootStatusMethod(UA_Server *server, UA_NodeId parent);
void addGetSnapshotStatusMethod(UA_Server *server, UA_NodeId parent);
void addGetSCPStatusMethod(UA_Server *server, UA_NodeId parent);
void addGetSFTPStatusMethod(UA_Server *server, UA_NodeId parent);
void addGetRunScriptStatusMethod(UA_Server *server, UA_NodeId parent);
void addGetArchiveStatusMethod(UA_Server *server, UA_NodeId parent);
void addGetAncillarySensorStatusMethod(UA_Server *server, UA_NodeId parent);

void addGetSensorMethod(UA_Server *server, UA_NodeId parent);
void addGetOperatingTimeMethod(UA_Server *server, UA_NodeId parent);
void addGetWarningMessageMethod(UA_Server *server, UA_NodeId parent);

void addGetIgramPPMethod(UA_Server *server, UA_NodeId parent);
void addGetIgramDCMethod(UA_Server *server, UA_NodeId parent);
void addGetLaserPPMethod(UA_Server *server, UA_NodeId parent);
void addGetLaserDCMethod(UA_Server *server, UA_NodeId parent);
void addGetSingleBeamAt900Method(UA_Server *server, UA_NodeId parent);
void addGetSingleBeamAt2500Method(UA_Server *server, UA_NodeId parent);
void addGetSignalToNoiseAt2500Method(UA_Server *server, UA_NodeId parent);
void addGetCenterBurstLocationMethod(UA_Server *server, UA_NodeId parent);
void addGetDetectorTempMethod(UA_Server *server, UA_NodeId parent);
void addGetLaserFrequencyMethod(UA_Server *server, UA_NodeId parent);
void addGetHardDriveSpaceMethod(UA_Server *server, UA_NodeId parent);
void addGetFlowMethod(UA_Server *server, UA_NodeId parent);
void addGetTemperatureMethod(UA_Server *server, UA_NodeId parent);
void addGetPressureMethod(UA_Server *server, UA_NodeId parent);
void addGetTempOpticsMethod(UA_Server *server, UA_NodeId parent);
void addGetBadScanCounterMethod(UA_Server *server, UA_NodeId parent);
void addGetFreeMemorySpaceMethod(UA_Server *server, UA_NodeId parent);
void addGetLABFilenameMethod(UA_Server *server, UA_NodeId parent);
void addGetLOGFilenameMethod(UA_Server *server, UA_NodeId parent);
void addGetLgFilenameMethod(UA_Server *server, UA_NodeId parent);
void addGetSecondLgFilenameMethod(UA_Server *server, UA_NodeId parent);
void addGetSystemCounterMethod(UA_Server *server, UA_NodeId parent);
void addGetDetectorCounterMethod(UA_Server *server, UA_NodeId parent);
void addGetLaserCounterMethod(UA_Server *server, UA_NodeId parent);
void addGetFlowPumpCounterMethod(UA_Server *server, UA_NodeId parent);
void addGetDesiccantCounterMethod(UA_Server *server, UA_NodeId parent);

void addGetNoOfAlarmsMethod(UA_Server *server, UA_NodeId parent);
void addGetArrayOfAlarmMethod(UA_Server *server, UA_NodeId parent);
void addGetNoOfNonAlarmsMethod(UA_Server *server, UA_NodeId parent);
void addGetArrayOfNonAlarmsMethod(UA_Server *server, UA_NodeId parent);

void addClearAlarmLEDMethod(UA_Server *server, UA_NodeId parent);
UA_StatusCode GetSoftwareVersionMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetDataBlockVersionMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetInstrumentTimeMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetMeasurementTimeMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetBootStatusMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetSnapshotStatusMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetSCPStatusMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetSFTPStatusMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetRunscriptStatusMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetArchiveStatusMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetAncillaryStatusMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetSensorMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetOperatingTimeMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetWarningMessageMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetIgramPPMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetIgramDCMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetLaserPPMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetLaserDCMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetSingleBeamAt900MethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetSingleBeamAt2500MethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetSignalToNoiseAt2500MethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetCenterBurstLocationMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetDetectorTempMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetLaserFrequencyMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetHardDriveSpaceMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetFlowMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetTemperatureMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetPressureMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetTempOpticsMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetBadScanCounterMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetFreeMemorySpaceMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetLABFilenameMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetLOGFilenameMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetLgFilenameMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetSecondLgFilenameMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetSystemCounterMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetDetectorCounterMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetLaserCounterMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetFlowPumpCounterMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetDesiccantCounterMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetNoOfAlarmsMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetArrayOfAlarmMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetNoOfNonAlarmsMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode GetArrayOfNonAlarmMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

UA_StatusCode ClearAlarmLEDMethodCallback(UA_Server *server,
                         const UA_NodeId *sessionId, void *sessionHandle,
                         const UA_NodeId *methodId, void *methodContext,
                         const UA_NodeId *objectId, void *objectContext,
                         size_t inputSize, const UA_Variant *input,
                         size_t outputSize, UA_Variant *output);

// SV_Misc.c
//xmlNodePtr srSeekChildNodeNamed(xmlNode* p, char* name, int len);
UA_ByteString loadFile(const char *const);
char* itoa(int, char*, int);

// SV_Monitor.c
void createMonitoredItems(UA_Server *);
void CreateServerMonitoredItems(UA_Server *server);

void dataChangeNotificationCallback(UA_Server *server, UA_UInt32 monitoredItemId,
                               void *monitoredItemContext, const UA_NodeId *nodeId,
                               void *nodeContext, UA_UInt32 attributeId,
                               const UA_DataValue *value);


void addMonitoredItemToSoftwareVersionVariable(UA_Server *server);
void addMonitoredItemToDataBlockVersionVariable(UA_Server *server);
void addMonitoredItemToInstrumentTimeVariable(UA_Server *server);
void addMonitoredItemToMeasurementTimeVariable(UA_Server *server);

void addMonitoredItemToBootStatusVariable(UA_Server *server);
void addMonitoredItemToSnapshotStatusVariable(UA_Server *server);
void addMonitoredItemToSCPStatusVariable(UA_Server *server);
void addMonitoredItemToSFTPStatusVariable(UA_Server *server);
void addMonitoredItemToRunScriptStatusVariable(UA_Server *server);
void addMonitoredItemToArchiveStatusVariable(UA_Server *server);
void addMonitoredItemToAncillarySensorStatusVariable(UA_Server *server);

void addMonitoredItemToSensorVariable(UA_Server *server);
void addMonitoredItemToOperatingTimeVariable(UA_Server *server);
void addMonitoredItemToWarningMessageVariable(UA_Server *server);

void addMonitoredItemToIgramPPVariable(UA_Server *server);
void addMonitoredItemToIgramDCVariable(UA_Server *server);
void addMonitoredItemToLaserPPVariable(UA_Server *server);
void addMonitoredItemToLaserDCVariable(UA_Server *server);
void addMonitoredItemToSingleBeamAt900Variable(UA_Server *server);
void addMonitoredItemToSingleBeamAt2500Variable(UA_Server *server);
void addMonitoredItemToSignalToNoiseAt2500Variable(UA_Server *server);
void addMonitoredItemToCenterBurstLocationVariable(UA_Server *server);
void addMonitoredItemToDetectorTempVariable(UA_Server *server);
void addMonitoredItemToLaserFrequencyVariable(UA_Server *server);
void addMonitoredItemToHardDriveSpaceVariable(UA_Server *server);
void addMonitoredItemToFlowVariable(UA_Server *server);
void addMonitoredItemToTemperatureVariable(UA_Server *server);
void addMonitoredItemToPressureVariable(UA_Server *server);
void addMonitoredItemToTempOpticsVariable(UA_Server *server);
void addMonitoredItemToBadScanCounterVariable(UA_Server *server);
void addMonitoredItemToFreeMemorySpaceVariable(UA_Server *server);
void addMonitoredItemToLABFilenameVariable(UA_Server *server);
void addMonitoredItemToLOGFilenameVariable(UA_Server *server);
void addMonitoredItemToLgFilenameVariable(UA_Server *server);
void addMonitoredItemToSecondLgFilenameVariable(UA_Server *server);
void addMonitoredItemToSystemCounterVariable(UA_Server *server);
void addMonitoredItemToDetectorCounterVariable(UA_Server *server);
void addMonitoredItemToLaserCounterVariable(UA_Server *server);
void addMonitoredItemToFlowPumpCounterVariable(UA_Server *server);
void addMonitoredItemToDesiccantCounterVariable(UA_Server *server);

// SV_PubSub
void pubSubInitialiseWithBroker(UA_Server *, char *, int, char *);
void pubSubInitialiseBrokerless(UA_Server *, char *);
void pubSubAddConnection(UA_Server *, UA_String *, UA_NetworkAddressUrlDataType *, char *, char *);

void pubSendDataSet(UA_Server *);
void pubSubInitialiseWithBroker(UA_Server *, char *, int, char *);
void pubSubInitialiseBrokerless(UA_Server *, char *);


void pubInitialiseField(UA_Server *);
void pubWriterGroupWithBroker(UA_Server *);
void pubWriterGroupBrokerless(UA_Server *);
void pubDataSetWriterBroker(UA_Server *);
void pubDataSetWriterBrokerless(UA_Server *);
void pubWriterGroupBroker(UA_Server *);
void pubWriterGroupBrokerless(UA_Server *);
void pubDataSetWriterWithBroker(UA_Server *);
void pubDataSetWriterBrokerless(UA_Server *);
void pubSubAddConnectionWithBroker(UA_Server *, UA_String *, UA_NetworkAddressUrlDataType *, char *, char *);
void pubSubAddConnectionBrokerless(UA_Server *, UA_String *, UA_NetworkAddressUrlDataType *);

UA_StatusCode subReaderGroup(UA_Server *);
UA_StatusCode subDataSetReader(UA_Server *);
UA_StatusCode subInitialiseField(UA_Server *, UA_NodeId);
void subReceiveDataSet(UA_DataSetMetaDataType *);

int print_debug_UDP(UA_PubSubConnectionConfig *, UA_NetworkAddressUrlDataType *);
void print_debug_MQTT(UA_PubSubConnectionConfig *, UA_NetworkAddressUrlDataType *);

#define UA_ENABLE_PUBSUB_INFORMATIONMODEL

#define MAX_STRING_SIZE 64
#define APPLICATION_URI "urn:virtualskies.com.sg"

#define UA_ENABLE_PUBSUB                // activate open62541.h PUBSUB section
#define UA_EANBLE_PUBSUB_ETH_UDAP       // activate open62541.h PUBSUB section
#define UA_ENABLE_PUBSUB_MQTT           // activate open62541.h PUBSUB section
#define UA_ENABLE_JSON_ENCODING         // activate open62541.h JSON section => use : open62541.c.json & open62541.h.json

#define CONNECTION_CONFIGNAME           "Publisher Connection"
#define NETWORKADDRESSURL_PROTOCOL      "opc.udp://224.0.0.22:4840/"    // multicast address range 224.0.0.0 to 224.0.0.255 ; for verifying using wireshark

#define CONNECTION_NAME_UADP            "ttl"   // ""UADP Publisher Connection"
#define TRANSPORT_PROFILE_URI_UDP       "http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp"
#define TRANSPORT_PROFILE_URI_ETH       "http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp"
//#define TRANSPORT_PROFILE_URI_UADP      "http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-uadp"
//#define TRANSPORT_PROFILE_URI_JSON      "http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-json"
#define PUBLISH_INTERVAL_UDP_ETH            9000

#define PUBLISHERID                     2234    // used in addDataSetReader() and addPubSubConnection()
#define WRITERGROUPID                   100     // used in addDataSetReader() and addWriterGroup()
#define DATASETWRITERID                 2234    // used in addDataSetReader() and addDataSetWriter()

#ifdef UA_ENABLE_PUBSUB_MQTT
 #define TRANSPORT_PROFILE_URI_MQTT_UADP     "http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-uadp"
 #define TRANSPORT_PROFILE_URI_MQTT_JSON     "http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-json"
 #define TRANSPORT_PROFILE_URI_AMQP     "http://opcfoundation.org/UA-Profile/Transport/pubsub-amqp"
 #define PUBLISH_INTERVAL_MQTT          9000
 #define PUBLISH_INTERVAL_AMQP          9000
 #define MESSAGE_TIMEOUT_MQTT           12000
 #define MESSAGE_TIMEOUT_AMQP           12000
 #define CONNECTIONOPTION_NAME_MQTT     "mqttClientId"
 #define CONNECTIONOPTION_NAME_AMQP     "amqpClientId"
 #define CLIENT_ID_MQTT                 "OPCServer-109-Mqtt" // during execution, mqtt-11 will show : New client connected from 192.168.1.33 as OPCServer-33-Mqtt (c0, k400, u'jackybek')
 #define CLIENT_ID_AMQP                 "TESTCLIENTPUBUSBAMQP"
 #define PUBLISHER_METADATAUPDATETIME_MQTT      0
 #define PUBLISHER_METADATAUPDATETIME_AMQP      0
 #define PUBLISHER_METADATAQUEUENAME_MQTT       "MetaDataTopic"
 #define PUBLISHER_METADATAQUEUENAME_AMQP       "MetaDataTopic"
 #define PUBLISHER_TOPIC_EDGE2CLOUD_MQTT        "AirgardTopicEdgeToCloud"
 #define PUBLISHER_TOPIC_CLOUD2EDGE_MQTT        "AirgardTopicCloudToEdge"
 #define PUBLISHER_QUEUE_EDGE2CLOUD_AMQP        "AirgardQueueEdgeToCloud"
 #define PUBLISHER_QUEUE_CLOUD2EDGE_AMQP        "AirgardQueueCloudToEdge"
 //#define BROKER_ADDRESS_URL_MQTT              "opc.mqtt://192.168.1.119:1883/"
 //#define BROKER_ADDRESS_URL_AMQP              "opc.amqp://192.168.1.119:5672/"
 #define USERNAME_OPTION_NAME_MQTT              "mqttUsername"
 #define USERNAME_OPTION_NAME_AMQP              "amqpUsername"
 #define PASSWORD_OPTION_NAME_MQTT              "mqttPassword"
 #define PASSWORD_OPTION_NAME_AMQP              "amqpPassword"
 #define USERNAME_MQTT                          "jackybek" // during execution, mqtt-11 will show : New client connected from 192.168.1.33 as OPCServer-33-Mqtt (c0, k400, u'jackybek')
 #define USERNAME_AMQP                          "jackybek"
 #define PASSWORD_MQTT                          "molekhaven24"
 #define PASSWORD_AMQP                          "molekhaven24"

 #define CA_FILE_PATH_MQTT                      "/etc/ssl/certs/mosq-ca.crt"            // symbolic link => /home/pi/Downloads/openest.io/mosq-ca.crt
 #define CA_PATH_MQTT                           "/etc/ssl/certs"
 #define CLIENT_CERT_PATH_MQTT                  "/etc/ssl/certs/mosq-client-33.crt"     // symbolic link => /home/pi/Downloads/openest.io/mosq-client-33.crt
 #define CLIENT_KEY_PATH_MQTT                   "/etc/ssl/certs/mosq-client-33.key"     // symbolic link => /home/pi/Downloads/openest.io/mosq-client-33.key
 // tested successfully on command line
 // mosquitto_sub -u jackybek -P molekhaven24 -h 192.168.1.11 -p 8883 -t AirgardTopicEdgeToCloud
 // --capath /etc/ssl/certs --cafile /etc/ssl/certs/mosq-ca.crt --cert /etc/ssl/certs/mosq-client-33.crt
 // --key /etc/ssl/certs/mosq-client-33.key --tls-version tlsv1.2 -d -i OPCServer-33

 #define SUBSCRIBER_METADATAUPDATETIME_MQTT      0
 #define SUBSCRIBER_METADATAQUEUENAME_MQTT       "MetaDataTopic"
 #define SUBSCRIBER_TOPIC_MQTT                   "AirgardTopic"
 #define MILLI_AS_NANO_SECONDS                  (1000 * 1000)
 #define SECONDS_AS_NANO_SECONDS                (1000 * 1000 * 1000)

 #define MAJOR_SOFTWARE_VERSION                 2
 #define MINOR_SOFTWARE_VERSION                 1
#endif

#ifdef UA_ENABLE_WEBSOCKET_SERVER
 #define TRANSPORT_PROFILE_URI_WSSBIN   "http://opcfoundation.org/UA_Profile/Transport/wss-uasc-uabinary"
 #define TRANSPORT_PROFILE_URI_WSSJSON  "http://opcfoundation.org/UA-Profile/Transport/wss-uajson"
 #define BROKER_ADDRESS_URL_WSS         "opc.wss://192.168.1.165:7681/"
#endif

#define UA_AES128CTR_SIGNING_KEY_LENGTH 16
#define UA_AES128CTR_KEY_LENGTH 16
#define UA_AES128CTR_KEYNONCE_LENGTH 4

// SV_Register.c
UA_ClientConfig* registerToLDS(UA_Server *, char *);
UA_ByteString loadFile(const char *const path);

// SV_ReverseConnect.c
UA_UInt64 reverseConnect(UA_Server *uaServer, char *ipAddressRaw);
void reverseConnectStateCallback(UA_Server *server, UA_UInt64 handle,
                                        UA_SecureChannelState state, void *context);

// SV_Modbus.c
modbus_t *createModbusExtension(UA_Server *);
