int stringEndsWithEncrypt(
        char const * const name,
        char const * const extension_to_find);

//int encryptServer(UA_Server *uaSvrServer, UA_ServerConfig *config, bool ForceGenerateSelfSignedCert);

int encryptServer(UA_Server *, UA_ServerConfig *, UA_Boolean);

int generateSSCert(UA_Server *, UA_ServerConfig *,
                   UA_ByteString *, size_t,
                   UA_ByteString *, size_t,
                   UA_ByteString *, size_t,
                   UA_ByteString *, UA_ByteString *);
