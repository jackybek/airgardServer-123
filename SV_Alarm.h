
// function prototypes declaration
UA_StatusCode addConditionSourceObject(UA_Server *);
UA_StatusCode addCondition_1(UA_Server *);
UA_StatusCode addCondition_2(UA_Server *);
void addVariable_1_triggerAlarmOfCondition_1(UA_Server *, UA_NodeId *);
void addVariable_2_changeSeverityOfCondition_2(UA_Server *, UA_NodeId *);
void addVariable_3_returnCondition_1_toNormalState(UA_Server *, UA_NodeId*);
void afterWriteCallbackVariable_1(UA_Server *, const UA_NodeId *,
                             void *, const UA_NodeId *,
                             void *, const UA_NumericRange *,
                             const UA_DataValue *);
void afterWriteCallbackVariable_2(UA_Server *, const UA_NodeId *,
                             void *, const UA_NodeId *,
                             void *, const UA_NumericRange *,
                             const UA_DataValue *);
void afterWriteCallbackVariable_3(UA_Server *, const UA_NodeId *,
                             void *, const UA_NodeId *,
                             void *, const UA_NumericRange *,
                             const UA_DataValue *);
UA_StatusCode enteringEnabledStateCallback(UA_Server *, const UA_NodeId *);
UA_StatusCode enteringAckedStateCallback(UA_Server *, const UA_NodeId *);
UA_StatusCode enteringConfirmedStateCallback(UA_Server *, const UA_NodeId *);
UA_StatusCode setUpEvironment(UA_Server *);
void CreateServerAlarmsAndConditions(UA_Server *);
