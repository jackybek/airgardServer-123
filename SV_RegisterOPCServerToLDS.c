#ifdef almagamation
#include <open62541/network_tcp.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/nodestore_default.h>
#include <open62541/plugin/pki_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#else
   #include "open62541.h"
#endif

#include "SV_NewMonitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <libwebsockets.h>


// 192.168.1.33 acting as client to 192.168.1.44 (LDS)
// need to copy 192.168.1.44::62541LDSServerCert.pem to /usr/local/ssl/certs/.
// need to copy 192.168.1.44::62541LDSprivate-key.pem to /usr/local/ssl/private/.
#define DISCOVERY_SERVER_APPLICATION_URI "urn:lds.virtualskies.com.sg"
#define DISCOVERY_SERVER_ENDPOINT "opc.tcp://192.168.1.44:4840"
#define DISCOVERY_PRIVATEKEYLOC "/usr/local/ssl/private/ldsprivate-key.pem"
#define DISCOVERY_SSLCERTIFICATELOC "/etc/ssl/certs/ldscert44.pem"
#define DISCOVERY_TRUSTLISTLOC "/usr/local/ssl/trustlist/trustlist.crl"

void RegisterOPCServerToLDS(UA_Server *);

void RegisterOPCServerToLDS(UA_Server *uaServer1)
{
        UA_StatusCode retval;


                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_RegisterOPCUAServerToLDS : Just before LDS registration process");

                #ifdef UA_ENABLE_DISCOVERY
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_RegisterOPCUAServerToLDS :  Begin LDS registration process");

                //-------------- register itself to the LDS Server <192.168.1.44>
                // code based on githug/open62541/62541/examples/discovery/server_register.c
                // acting as an OPCUA Client to LDS Server
                UA_ByteString LDScertificate = loadFile(DISCOVERY_SSLCERTIFICATELOC);  //loadFile("/etc/ssl/certs/62541LDSServerCert.pem"); //=> symbolic link
                //UA_ByteString certificate = loadFile("/usr/local/ssl/certs/ldscert44.pem"); // actual location
                if (LDScertificate.length == 0)
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_RegisterOPCUAServerToLDS : Unable to load file : %s", DISCOVERY_SSLCERTIFICATELOC);
                        //goto cleanup;
                        return;
                }

                UA_ByteString LDSprivateKey = loadFile(DISCOVERY_PRIVATEKEYLOC);  //loadFile("/usr/local/ssl/private/62541LDSprivate-key.pem");
                //UA_ByteString LDSprivateKey = loadFile("/usr/local/ssl/private/ldsprivate-key.pem");
                if (LDSprivateKey.length == 0)
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_RegisterOPCUAServerToLDS : Unable to load file : %s", DISCOVERY_PRIVATEKEYLOC);
                        //goto cleanup;
                        return;
                }
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_RegisterOPCUAServerToLDS : successfully loaded LDS certificate and privateKey");

                // load the trustlist.  Load trustlist is not supported now
                size_t LDStrustListSize = 0;
                UA_STACKARRAY(UA_ByteString, LDStrustList, LDStrustListSize);
                for(size_t LDStrustListCount=0; LDStrustListCount < LDStrustListSize; LDStrustListCount++)
                        LDStrustList[LDStrustListCount] = loadFile(DISCOVERY_TRUSTLISTLOC);  //loadFile("/usr/local/ssl/trustlist/trustlist.crl");
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_RegisterOPCUAServerToLDS : successfully loaded LDS trustList");

                /* Loading of a revocation list currently unsupported */
                UA_ByteString *LDSrevocationList = NULL;
                size_t LDSrevocationListSize = 0;

                UA_Client *LDSclient = UA_Client_new();
                UA_ClientConfig *LDSClient_config1 = UA_Client_getConfig(LDSclient);
                //UA_ClientConfig_setDefault(UA_Client_getConfig(LDSclient));
                //UA_ClientConfig_setDefault(LDSClient_config1);
                LDSClient_config1->localConnectionConfig = UA_ConnectionConfig_default;

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_RegisterOPCUAServerToLDS : here 1");

                UA_ApplicationDescription_clear(&LDSClient_config1->clientDescription);
                LDSClient_config1->clientDescription.applicationUri = UA_STRING_ALLOC(DISCOVERY_SERVER_APPLICATION_URI);

                // Secure client connect
                LDSClient_config1->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT; //UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;  // require encryption
                LDSClient_config1->securityPolicyUri = UA_STRING_ALLOC("");
                LDSClient_config1->secureChannelLifeTime = 10 * 60 * 1000;        // 10 minutes
                LDSClient_config1->securityPolicies = (UA_SecurityPolicy*)UA_malloc(sizeof(UA_SecurityPolicy));
                if (!LDSClient_config1->securityPolicies)
                {
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "--------SV_RegisterOPCUAServerToLDS : Error setting securityPolicies : %s", UA_StatusCode_name(UA_STATUSCODE_BADOUTOFMEMORY));
                        //goto cleanup;
                        return;
                }
                LDSClient_config1->initConnectionFunc = UA_ClientConnectionTCP_init; /* for async client */
                LDSClient_config1->pollConnectionFunc = UA_ClientConnectionTCP_poll; /* for async connection */
                LDSClient_config1->customDataTypes = NULL;
                LDSClient_config1->connectivityCheckInterval = 0;
                LDSClient_config1->requestedSessionTimeout = 1200000; /* requestedSessionTimeout */
                LDSClient_config1->inactivityCallback = NULL;
                LDSClient_config1->clientContext = NULL;
                LDSClient_config1->connectivityCheckInterval = 3000; // in milliseconds

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_RegisterOPCUAServerToLDS : Calling setDefaultEncryption");
                UA_ClientConfig_setDefaultEncryption(LDSClient_config1, LDScertificate, LDSprivateKey,
                                                                LDStrustList, LDStrustListSize,
                                                                LDSrevocationList, LDSrevocationListSize);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_RegisterOPCUAServerToLDS : Finished calling setDefaultEncryption");

                UA_ByteString_clear(&LDScertificate);
                UA_ByteString_clear(&LDSprivateKey);
                for (size_t deleteCounter=0; deleteCounter < LDStrustListSize; deleteCounter++)
                        UA_ByteString_clear(&LDStrustList[deleteCounter]);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_RegisterOPCUAServerToLDS : successfully setDefaultEncryption as LDS Client");

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_RegisterOPCUAServerToLDS : Just about to call UA_Client_connectUsername() at line 564");
                retval = UA_Client_connectUsername(LDSclient, DISCOVERY_SERVER_ENDPOINT, "admin", "defaultadminpassword24");
                if (retval != UA_STATUSCODE_GOOD)
                {
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_RegisterOPCUAServerToLDS : Cannot login securely to OPCUA LDS Server : %s", DISCOVERY_SERVER_ENDPOINT);
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_RegisterOPCUAServerToLDS : error is %s", UA_StatusCode_name(retval));
                        //goto cleanup;
                        return;
                }
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_RegisterOPCUAServerToLDS : UA_LDS_connectUsername() : success");

                UA_UInt64 callbackId;
                // server registration starts after 0 (5=5 minutes); initial delay for 500ms
                UA_StatusCode LDS_retval = UA_Server_addPeriodicServerRegisterCallback(uaServer1,
                                                                        LDSclient,
                                                                        DISCOVERY_SERVER_ENDPOINT,              // opc.tcp://192.168.1.44:4840
                                                                        0 * 60 * 1000,                          // delay after X milliseconds
                                                                        500,                                    // delay first register for 500 ms
                                                                        &callbackId);
                // the following check in UA_Server_addPeriodicServerRegisterCallback() failed
                //(client->connection.state != UA_CONNECTIONSTATE_CLOSED)

                if (LDS_retval != UA_STATUSCODE_GOOD)
                {
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                                "--------SV_RegisterOPCUAServerToLDS : registering to remote LDS server  : Fail at UA_Server_addPeriodicServerRegisterCallback. StatusCode %s",
                                UA_StatusCode_name(LDS_retval));
                        UA_Client_disconnect(LDSclient);
                        UA_Client_delete(LDSclient);
                        //goto cleanup;
                        return;
                }
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_RegisterOPCUAServerToLDS : End LDS registration process");
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_RegisterOPCUAServerToLDS : successfully registered to LDS Server");
                #endif  // UA_ENABLE_DISCOVERY
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "=============================================================");

}
