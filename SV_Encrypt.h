
UA_ByteString loadFile(const char *const);
int encryptServer(UA_Server *, UA_ServerConfig *);
int generateSSCert(UA_Server *, UA_ServerConfig *, UA_ByteString *, size_t, UA_ByteString *, size_t, UA_ByteString *, size_t,  UA_ByteString *,  UA_ByteString *);

