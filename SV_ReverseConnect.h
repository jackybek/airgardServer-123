void reverseConnect(UA_Server *uaServer, char *ipAddressRaw);
static void reverseConnectStateCallback(UA_Server *server, UA_UInt64 handle,
                                        UA_SecureChannelState state, void *context);
