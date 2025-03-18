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
   #define UA_ENABLE_DISCOVERY_MULTICAST
   #include "open62541.h"
#endif
#include "SV_Register.h"
#include "SV_Misc.h"

#include "SV_Monitor.h"
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

#define UA_ENABLE_HISTORIZING
//#define UA_ENABLE_DISCOVERY
#define UA_ENABLE_DISCOVERY_MULTICAST

#define MANUFACTURER_NAME "Virtual Skies"
#define PRODUCT_URI "http://virtualskies.com.sg/Airgard/"
#define APPLICATION_NAME "OPCUA Server (Airgard) based on open62541"
#define APPLICATION_URI "svr.virtualskies.com.sg"               // maps to CName : virtualskies.com.sg during SSL cert generation
                                                                // maps to openssl.cnf : subjectAltName = URI:virtualskies.com.sg, IP:192.168.1.33
#define APPLICATION_URI_SERVER "svr.virtualskies.com.sg"
#define PRODUCT_NAME "Virtual Skies OPC UA Server"
#define STRINGIFY(arg) #arg
#define VERSION(MAJOR, MINOR, PATCH, LABEL) \
    STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH) LABEL

#define PRIVATEKEYLOC "/usr/local/ssl/private/private-key.pem"
#define SSLCERTIFICATELOC "/etc/ssl/certs/cert33.pem"   //"/etc/ssl/certs/62541ServerCert.pem"
//#define SSLCERTIFICATELOC "/etc/ssl/certs/62541ServerCert.pem"
#define TRUSTLISTLOC "/usr/local/ssl/trustlist/trustlist.crl"

// 192.168.1.33 acting as client to 192.168.1.44 
// need to copy 192.168.1.44::62541LDSServerCert.pem to /usr/local/ssl/certs/.
// need to copy 192.168.1.44::62541LDSprivate-key.pem to /usr/local/ssl/private/.
#define DISCOVERY_SERVER_APPLICATION_URI "lds.virtualskies.com.sg"
#define DISCOVERY_SERVER_ENDPOINT "opc.tcp://192.168.1.44:4841"			// non standard port for LDS
#define DISCOVERY_PRIVATEKEYLOC "/usr/local/ssl/private/ldsprivate-key.pem"
#define DISCOVERY_SSLCERTIFICATELOC "/etc/ssl/certs/ldscert44.pem"
#define DISCOVERY_TRUSTLISTLOC "/usr/local/ssl/trustlist/trustlist.crl"
#define DISCOVERY_USERNAME "admin"
#define DISCOVERY_PASSWORD "defaultadminpassword24"


#define GDS_DISCOVERY_SERVER_APPLICATION_URI "urn:gds.virtualskies.com.sg"
#define GDS_DISCOVERY_SERVER_ENDPPOINT "opc.tpc://192.168.1.44:4841"
#define GDS_DISCOVERY_PRIVATEKEYLOC "/usr/local/ssl/private/gdsprivate-key.pem"
#define GDS_DISCOVERY_SSL_CERTIFICATELOC "/etc/ssl/certs/gdscert88.pem"
#define GDS_DISCOVERY_TRUSTLISTLOC "/usr/loca/ssl/trustlist/trustlist.crl"

/* Struct initialization works across ANSI C/C99/C++ if it is done when the
 * variable is first declared. Assigning values to existing structs is
 * heterogeneous across the three. */
static UA_INLINE UA_UInt32Range
UA_UINT32RANGE(UA_UInt32 min, UA_UInt32 max) {
    UA_UInt32Range range = {min, max};
    return range;
}

static UA_INLINE UA_DurationRange
UA_DURATIONRANGE(UA_Duration min, UA_Duration max) {
    UA_DurationRange range = {min, max};
    return range;
}


void registerToLDS(UA_Server *uaServer1, char* lds_endpoint)
{
        UA_StatusCode retval;

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "==========================================================");

                #ifdef UA_ENABLE_DISCOVERY
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Begin LDS registration process");

                //-------------- register itself to the LDS Server <192.168.1.44>
                // code based on githug/open62541/62541/examples/discovery/server_register.c
                // acting as an OPCUA Client to LDS Server
                UA_ByteString LDScertificate = loadFile(DISCOVERY_SSLCERTIFICATELOC);  //loadFile("/etc/ssl/certs/62541LDSServerCert.pem"); //=> symbolic link
                //UA_ByteString certificate = loadFile("/usr/local/ssl/certs/ldscert44.pem"); // actual location
                if (LDScertificate.length == 0)
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c: Unable to load file : %s", DISCOVERY_SSLCERTIFICATELOC);
                        //goto cleanup;
                        return;
                }
		else
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : successfully loaded LDS certificate");

                UA_ByteString LDSprivateKey = loadFile(DISCOVERY_PRIVATEKEYLOC);  //loadFile("/usr/local/ssl/private/62541LDSprivate-key.pem");
                //UA_ByteString LDSprivateKey = loadFile("/usr/local/ssl/private/ldsprivate-key.pem");
                if (LDSprivateKey.length == 0)
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Unable to load file : %s", DISCOVERY_PRIVATEKEYLOC);
                        //goto cleanup;
                        return;
                }
		else
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : successfully loaded LDS privateKey");

                // load the trustlist.  Load trustlist is not supported now
                size_t LDStrustListSize = 0;
                UA_STACKARRAY(UA_ByteString, LDStrustList, LDStrustListSize);
                for(size_t LDStrustListCount=0; LDStrustListCount < LDStrustListSize; LDStrustListCount++)
                        LDStrustList[LDStrustListCount] = loadFile(DISCOVERY_TRUSTLISTLOC);  //loadFile("/usr/local/ssl/trustlist/trustlist.crl");
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : successfully loaded LDS trustList");

                /* Loading of a revocation list currently unsupported */
                UA_ByteString *LDSrevocationList = NULL;
                size_t LDSrevocationListSize = 0;

                UA_Client *LDSclient = UA_Client_new();
                UA_ClientConfig *LDSClient_config1 = UA_Client_getConfig(LDSclient);
                //UA_ClientConfig_setDefault(UA_Client_getConfig(LDSclient));
                //UA_ClientConfig_setDefault(LDSClient_config1);
                LDSClient_config1->localConnectionConfig = UA_ConnectionConfig_default;

                UA_ApplicationDescription_clear(&LDSClient_config1->clientDescription);
                LDSClient_config1->clientDescription.applicationUri = UA_STRING_ALLOC(DISCOVERY_SERVER_APPLICATION_URI);

                // Secure client connect
                LDSClient_config1->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT; //UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;  // require encryption
                LDSClient_config1->securityPolicyUri = UA_STRING_ALLOC("");
                LDSClient_config1->secureChannelLifeTime = 10 * 60 * 1000;        // 10 minutes
                LDSClient_config1->securityPolicies = (UA_SecurityPolicy*)UA_malloc(sizeof(UA_SecurityPolicy));
                if (!LDSClient_config1->securityPolicies)
                {
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "--------SV_Register.c.c  Error setting securityPolicies : %s", UA_StatusCode_name(UA_STATUSCODE_BADOUTOFMEMORY));
                        //goto cleanup;
                        return;
                }
        //      LDSClient_config1->initConnectionFunc = UA_ClientConnectionTCP_init; /* for async client */
        //      LDSClient_config1->pollConnectionFunc = UA_ClientConnectionTCP_poll; /* for async connection */
                LDSClient_config1->customDataTypes = NULL;
                LDSClient_config1->connectivityCheckInterval = 0;
                LDSClient_config1->requestedSessionTimeout = 1200000; /* requestedSessionTimeout */
                LDSClient_config1->inactivityCallback = NULL;
                LDSClient_config1->clientContext = NULL;
                LDSClient_config1->connectivityCheckInterval = 3000; // in milliseconds

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Encrypting Svr with LDS certificates");
                retval = UA_ClientConfig_setDefaultEncryption(LDSClient_config1, LDScertificate, LDSprivateKey,
                                                                LDStrustList, LDStrustListSize,
                                                                LDSrevocationList, LDSrevocationListSize);

		if (retval != UA_STATUSCODE_GOOD)
		{
			UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Cannot encrypt itself as Client to LDS server : %s",  UA_StatusCode_name(retval));
			return;
		}
 		else
		{
                	//UA_ByteString_clear(&LDScertificate);
                	//UA_ByteString_clear(&LDSprivateKey);
                	//for (size_t deleteCounter=0; deleteCounter < LDStrustListSize; deleteCounter++)
                	//        UA_ByteString_clear(&LDStrustList[deleteCounter]);
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Successfully encrypted Svr with LDS certificates as LDS Client");
		}

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Just about to call UA_Client_connectUsername() at line 564");
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : credentials are %s %s %s",lds_endpoint, DISCOVERY_USERNAME, DISCOVERY_PASSWORD);

                retval = UA_Client_connectUsername(LDSclient, lds_endpoint, DISCOVERY_USERNAME, DISCOVERY_PASSWORD);


                if (retval != UA_STATUSCODE_GOOD)
                {
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Cannot login securely to LDS Server : %s", lds_endpoint);
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : error is %s", UA_StatusCode_name(retval));
			UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c  : UA_LDS_connectUsername() : failure");
                        //goto cleanup;
                        return;
                }
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c  : UA_LDS_connectUsername() : success");

		// register the server to LDS
		UA_StatusCode LDS_retval =  UA_Server_registerDiscovery(uaServer1, LDSClient_config1,
                                   UA_STRING( lds_endpoint), UA_STRING_NULL);

                //UA_UInt64 callbackId;
                // server registration starts after 0 (5=5 minutes); initial delay for 500ms
                //UA_StatusCode LDS_retval = UA_Server_addPeriodicServerRegisterCallback(uaServer1,
                //                                                        LDSclient,
                //                                                        DISCOVERY_SERVER_ENDPOINT,              // opc.tcp://192.168.1.44:4841
                //                                                        0 * 60 * 1000,                          // delay after X milliseconds
                //                                                        500,                                    // delay first register for 500 ms
                //                                                        &callbackId);
                // the following check in UA_Server_addPeriodicServerRegisterCallback() failed
                //(client->connection.state != UA_CONNECTIONSTATE_CLOSED)

                if (LDS_retval != UA_STATUSCODE_GOOD)
                {
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                                "--------SV_Register.c : registering to remote LDS server  : Fail at UA_Server_addPeriodicServerRegisterCallback. StatusCode %s",
                                UA_StatusCode_name(LDS_retval));
                        UA_Client_disconnect(LDSclient);
                        UA_Client_delete(LDSclient);
                        //goto cleanup;
                        return;
                }
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c  : End LDS registration process");
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c  : registered to LDS Server <%s>", DISCOVERY_SERVER_ENDPOINT);
                #endif  // UA_ENABLE_DISCOVERY
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "=============================================================");

}
