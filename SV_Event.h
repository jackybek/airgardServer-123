void CreateServerEvents(UA_Server *, UA_NodeId);
static UA_StatusCode addNewEventType(UA_Server *);
static UA_StatusCode setUpEvent(UA_Server *, UA_NodeId *);
static UA_StatusCode generateEventMethodCallback(UA_Server *,
                         const UA_NodeId *, void *,
                         const UA_NodeId *, void *,
                         const UA_NodeId *, void *,
                         size_t, const UA_Variant *,
                         size_t, UA_Variant *);

void addMonitoredItemToEvent(UA_Server *, UA_NodeId *);
