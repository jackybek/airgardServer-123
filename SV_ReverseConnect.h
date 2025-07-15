UA_UInt64 reverseConnect(UA_Server *uaServer, char *ipAddressRaw);
void reverseConnectStateCallback(UA_Server *server, UA_UInt64 handle,
                                        UA_SecureChannelState state, void *context);
