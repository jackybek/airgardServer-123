char* itoa(int num, char* str, int base);
int CreateServerWebSockets(UA_NodeId *publishedDataSetidentifier, UA_NetworkAddressUrlDataType *networkAddressUrl);

// only 1 PubSubConnection for both reader and writer
UA_NodeId PubSubconnectionIdentifier;

// publisher section : variables & functions
static UA_NodeId publishedDataSetIdentifier;
static UA_NodeId writerGroupIdentifier;

//void CreateServerPubSub(UA_Server *, char *, UA_Int16);
void CreateServerPubSub(UA_Server *, char *, int, char *);
static void addPubSubConnection(UA_Server *, UA_String *, UA_NetworkAddressUrlDataType *);
static void addPublishedDataSet(UA_Server *);
static void addDataSetField(UA_Server *);
static void addWriterGroup(UA_Server *);
static void addDataSetWriter(UA_Server *);

static UA_StatusCode addReaderGroup(UA_Server *uaServer);
static UA_StatusCode addDataSetReader(UA_Server *uaServer);
static UA_StatusCode addSubscribedVariables(UA_Server *uaServer, UA_NodeId dataSetReaderId);
static void fillDataSetMetaData(UA_DataSetMetaDataType *pMetaData);
static void pubSubStateChangeCallback(UA_NodeId *pubsubComponentId, UA_PubSubState state, UA_StatusCode code);
/*
static UA_StatusCode
pubSubComponent_createMonitoring(UA_Server *uaServer, UA_NodeId Id, UA_PubSubComponentEnumType eComponentType,
                                UA_PubSubMonitoringType eMonitoringType, void *data, UA_ServerCallback callback);
static UA_StatusCode
pubSubComponent_startMonitoring(UA_Server *uaServer, UA_NodeId Id, UA_PubSubComponentEnumType eComponentType,
                                UA_PubSubMonitoringType eMonitoringType, void *data);
static UA_StatusCode
pubSubComponent_stopMonitoring(UA_Server *uaServer, UA_NodeId Id, UA_PubSubComponentEnumType eComponentType,
                                UA_PubSubMonitoringType eMonitoringType, void *data);
static UA_StatusCode
pubSubComponent_updateMonitoringInterval(UA_Server *uaServer, UA_NodeId Id, UA_PubSubComponentEnumType eComponentType,
                                UA_PubSubMonitoringType eMonitoringType, void *data);
static UA_StatusCode
pubSubComponent_deleteMonitoring(UA_Server *uaServer, UA_NodeId Id, UA_PubSubComponentEnumType eComponentType,
                                UA_PubSubMonitoringType eMonitoringType, void *data);
*/
//void addValueCallbackToVariableNode( UA_Server *);

// somehow the compiler is not able to local this header definition in open62541.c
//UA_StatusCode UA_PubSubConnection_regist(UA_Server *server, UA_NodeId *connectionIdentifier);
//void pubsubStateChangeCallback(UA_NodeId *Id, UA_PubSubState state, UA_StatusCode status);
