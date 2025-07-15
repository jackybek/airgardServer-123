void
setValue(UA_Server *server, void *hdgContext, const UA_NodeId *sessionId,
        void *sessionContext, const UA_NodeId *nodeId, UA_Boolean historizing,
        const UA_DataValue *value);
void GetHistoryDBConnection(void);
void CloseHistoryDBConnection(void);
void writeHistoryData(const UA_NodeId *nodeId, const UA_DataValue *value);
void CreateServerHistorizingItems(UA_Server *);
void readHistoryData(MYSQL *conn, const UA_NodeId *nodeId);
