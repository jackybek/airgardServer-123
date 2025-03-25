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


/*
 * Get the endpoint from the server, where we can call RegisterServer2 (or RegisterServer).
 * This is normally the endpoint with highest supported encryption mode.
 *
 * @param discoveryServerUrl The discovery url from the remote server
 * @return The endpoint description (which needs to be freed) or NULL
 */
#ifdef KIV
static UA_EndpointDescription *
getRegisterEndpointFromServer(const char *discoveryServerUrl) {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    UA_EndpointDescription *endpointArray = NULL;
    size_t endpointArraySize = 0;
    UA_StatusCode retval = UA_Client_getEndpoints(client, discoveryServerUrl,
                                                  &endpointArraySize, &endpointArray);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_Array_delete(endpointArray, endpointArraySize,
                        &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "GetEndpoints failed with %s", UA_StatusCode_name(retval));
        UA_Client_delete(client);
        return NULL;
    }

    UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Server has %lu endpoints", (unsigned long)endpointArraySize);
    UA_EndpointDescription *foundEndpoint = NULL;
    for(size_t i = 0; i < endpointArraySize; i++) {
        UA_LOG_DEBUG(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "\tURL = %.*s, SecurityMode = %s",
                     (int) endpointArray[i].endpointUrl.length,
                     endpointArray[i].endpointUrl.data,
                     endpointArray[i].securityMode == UA_MESSAGESECURITYMODE_NONE ? "None" :
                     endpointArray[i].securityMode == UA_MESSAGESECURITYMODE_SIGN ? "Sign" :
                     endpointArray[i].securityMode == UA_MESSAGESECURITYMODE_SIGNANDENCRYPT ? "SignAndEncrypt" :
                     "Invalid"
        );
        // find the endpoint with highest supported security mode
	if ( (UA_String_equal(&endpointArray[i].securityPolicyUri, &UA_SECURITY_POLICY_NONE_URI)) && ( foundEndpoint == NULL || foundEndpoint->securityMode < endpointArray[i].securityMode) )

   //     if((UA_String_equal(&endpointArray[i].securityPolicyUri, &UA_SECURITY_POLICY_NONE_URI) ||
   //         	UA_String_equal(&endpointArray[i].securityPolicyUri, &UA_SECURITY_POLICY_BASIC128_URI)) &&
   //	( foundEndpoint == NULL || foundEndpoint->securityMode < endpointArray[i].securityMode))
            foundEndpoint = &endpointArray[i];
    }
    UA_EndpointDescription *returnEndpoint = NULL;
    if(foundEndpoint != NULL) {
        returnEndpoint = UA_EndpointDescription_new();
        UA_EndpointDescription_copy(foundEndpoint, returnEndpoint);
    }
    UA_Array_delete(endpointArray, endpointArraySize,
                    &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
    UA_Client_delete(client);
    return returnEndpoint;
}
#endif

UA_ClientConfig *registerToLDS(UA_Server *uaServer1, char* lds_endpoint_A)
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
                        return NULL;
                }
		else
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : successfully loaded LDS certificate");

                UA_ByteString LDSprivateKey = loadFile(DISCOVERY_PRIVATEKEYLOC);  //loadFile("/usr/local/ssl/private/62541LDSprivate-key.pem");
                //UA_ByteString LDSprivateKey = loadFile("/usr/local/ssl/private/ldsprivate-key.pem");
                if (LDSprivateKey.length == 0)
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Unable to load file : %s", DISCOVERY_PRIVATEKEYLOC);
                        //goto cleanup;
                        return NULL;
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
		const char *env_LDSport = getenv("LDS_PORT");
		char lds_endpoint[100];
		//printf("1 : %s, %d \n", lds_endpoint_A, strlen(lds_endpoint_A));
		strcpy(lds_endpoint, "opc.tcp://");
		int index = strlen(lds_endpoint_A);
		strncpy(&lds_endpoint[10], lds_endpoint_A, strlen(lds_endpoint_A));
		lds_endpoint[index+10]= ':';
		strcpy(&lds_endpoint[index+11], env_LDSport);
		//printf("2 : %s, %d \n", lds_endpoint, strlen(lds_endpoint));

		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : successfully assembled lds endpoint url : %s", lds_endpoint);

                UA_ClientConfig *LDSClient_config1 = UA_Client_getConfig(LDSclient);
                //UA_ClientConfig_setDefault(UA_Client_getConfig(LDSclient));
                //UA_ClientConfig_setDefault(LDSClient_config1);


                LDSClient_config1->clientContext = NULL;
		LDSClient_config1->logging = NULL;
		LDSClient_config1->timeout = 0;
                UA_ApplicationDescription_clear(&LDSClient_config1->clientDescription);

		const char *env_ldscertificateloc = getenv("SVR_LDS_SSLCERTIFICATELOC");
		//const char *env_ldsapplicationuri = getenv("SVR_LDS_APPLICATION_URI");// if use SVR_LDS_APPLICATION_URI, will get a warning on mismtach ApplicationURI
		const char *env_ldsapplicationuri = getenv("SVR_APPLICATION_URI_SERVER"); // try to swap with the server application uri

		LDSClient_config1->securityPoliciesSize = 1; // load the LDS cert, since SVR is connecting to LDS
		LDSClient_config1->securityPolicies[0].localCertificate = loadFile(env_ldscertificateloc);
                LDSClient_config1->clientDescription.applicationUri = UA_STRING_ALLOC(env_ldsapplicationuri);

		// endpointUrl format : opc.tcp://192.168.1.44:4841
		LDSClient_config1->endpointUrl = UA_STRING_ALLOC(lds_endpoint);

                // Secure client connect
                LDSClient_config1->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT; // require encryption
                LDSClient_config1->securityPolicyUri = UA_STRING_ALLOC("");
		LDSClient_config1->noSession = UA_FALSE;
		LDSClient_config1->noReconnect = UA_FALSE;
		LDSClient_config1->noNewSession = UA_FALSE;

		//LDSClient_config1->endpoint = ???;
		//LDSClient_config1->userTokenPolicy = ???;
		//LDSClient_config1->applicationUri = ???;

		// custom Data types
                LDSClient_config1->customDataTypes = NULL;

		// Advance Client Configuration
                LDSClient_config1->secureChannelLifeTime = 10 * 60 * 1000;        // 10 minutes
                LDSClient_config1->requestedSessionTimeout = 1200000; /* requestedSessionTimeout */
                LDSClient_config1->localConnectionConfig = UA_ConnectionConfig_default;
                LDSClient_config1->connectivityCheckInterval = 3000; // in milliseconds

		// Event loop
		//LDSClient_config1->eventLoop = ???
		//LDSClient_config1->externalEventLoop = ???

		// Available security policies
		LDSClient_config1->securityPolicies->localCertificate = LDScertificate; 

#ifdef KIV
		UA_SecurityPolicy *securityPolicy;
		LDSClient_config1->securityPoliciesSize = 4;
		for (size_t i=0; i< LDSClient_config1->securityPoliciesSize; i++)
			securityPolicy[i] = (UA_SecurityPolicy)UA_malloc(sizeof(UA_SecurityPolicy));

		/*
		securityPolicy[0].policyUri = UA_String(UA_SECURITY_POLICY_NONE_URI);
		securityPolicy[1].policyUri = UA_String(UA_SECURITY_POLICY_BASIC256SHA256_URI);
		securityPolicy[2].policyUri = UA_String(UA_SECURITY_POLICY_AES256SHA256RSAPSS_URI);
		securityPolicy[3].policyUri = UA_String(UA_SECURITY_POLICY_AES128SHA256RSAOAEP_URI);
		LDSClient_config1->securityPolicies = securityPolicy;
		*/
		//LDSClient_config1->securityPolicies[i] = (UA_SecurityPolicy) UA_malloc(sizeof(UA_SecurityPolicy));
                if (!LDSClient_config1->securityPolicies)
                {
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "--------SV_Register.c.c  Error initialising securityPolicies : %s", UA_StatusCode_name(UA_STATUSCODE_BADOUTOFMEMORY));
                        //goto cleanup;
                        return LDSClient_config1;
                }
		LDSClient_config1->securityPolicies[0].securityPolicies = UA_SECURITY_POLICY_NONE_URI;
		LDSClient_config1->securityPolicies[1].securityPolicies = UA_SECURITY_POLICY_BASIC256SHA256_URI;
		LDSClient_config1->securityPolicies[2].securityPolicies = UA_SECURITY_POLICY_AES256SHA256RSAPSS_URI;
		LDSClient_config1->securityPolicies[3].securityPolicies = UA_SECURITY_POLICY_AES128SHA256RSAOAEP_URI;
#endif
		//LDSClient_config1->certificateVerification = NULL;
		LDSClient_config1->authSecurityPoliciesSize = 0;
		//LDSClient_config1->authSecurityPolicies = NULL;
		//LDSClient_config1->authSecurityPolicyUri = ;

        //      LDSClient_config1->initConnectionFunc = UA_ClientConnectionTCP_init; /* for async client */
        //      LDSClient_config1->pollConnectionFunc = UA_ClientConnectionTCP_poll; /* for async connection */

                LDSClient_config1->inactivityCallback = NULL;
		LDSClient_config1->subscriptionInactivityCallback = NULL;
		// Session config
		LDSClient_config1->sessionName = UA_STRING("AirgardServer33 connection to LDS");
		LDSClient_config1->sessionLocaleIds = NULL;
		LDSClient_config1->sessionLocaleIdsSize = 0;

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Encrypting Svr with LDS certificates");
                retval = UA_ClientConfig_setDefaultEncryption(LDSClient_config1, LDScertificate, LDSprivateKey,
                                                                LDStrustList, LDStrustListSize,
                                                                LDSrevocationList, LDSrevocationListSize);

		if (retval != UA_STATUSCODE_GOOD)
		{
			UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Cannot encrypt itself as Client to LDS server : %s",  UA_StatusCode_name(retval));
			return LDSClient_config1;
		}
 		else
		{
                	//UA_ByteString_clear(&LDScertificate);
                	//UA_ByteString_clear(&LDSprivateKey);
                	//for (size_t deleteCounter=0; deleteCounter < LDStrustListSize; deleteCounter++)
                	//        UA_ByteString_clear(&LDStrustList[deleteCounter]);
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Successfully encrypted Svr with LDS certificates as LDS Client");
		}

		//const char* env_lds_username = getenv("SVR_LDS_USERNAME");
		//const char* env_lds_password = getenv("SVR_LDS_PASSWORD");
		char userid[100];
		char passwd[100];

		printf("Please enter the username to login to LDS : "); scanf("%s", userid);
		printf("Please enter the password to login to LDS : "); scanf("%s", passwd);

                //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Trying to connect to LDS server <%s> using <%s> <%s> ...", lds_endpoint, userid, passwd); //env_lds_username, env_lds_password);
		// lds_endpoint format : opc.tcp://192.168.1.44:4841
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Trying to connect to LDS server <%s> using <%s> <%s> ...", lds_endpoint, userid, passwd);
		#define UA_LOG_LEVEL = 300 // is used in UA_Client_connectUsername and functions
                retval = UA_Client_connectUsername(LDSclient, lds_endpoint, userid, passwd); //DISCOVERY_USERNAME, DISCOVERY_PASSWORD);

                if (retval != UA_STATUSCODE_GOOD)
                {
			UA_ClientConfig *myConfig = LDSClient_config1;
			// myConfig->userIdentityToken is of type UA_ExtensionObject
			UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : endpoint=<%s> userid=<%s> password=<%s>", 
							myConfig->endpointUrl.data, (char*)myConfig->userIdentityToken.content.decoded.data, (char*)myConfig->userIdentityToken.content.decoded.data);
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Cannot login securely to LDS Server : <%s>", lds_endpoint);
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : error is %s", UA_StatusCode_name(retval));
			UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c  : UA_LDS_connectUsername() : failure");
                        //goto cleanup;
                        //return LDSClient_config1;
                }
		else
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c  : Successfully connected to LDS %s", lds_endpoint);

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
                                "--------SV_Register.c : registering to remote LDS server  : Could not create periodic job for server register. StatusCode %s",
                                UA_StatusCode_name(LDS_retval));
                        UA_Client_disconnect(LDSclient);
                        UA_Client_delete(LDSclient);
                        //goto cleanup;
                        return LDSClient_config1;
                }
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c  : End LDS registration process");
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c  : registered to LDS Server <%s>", DISCOVERY_SERVER_ENDPOINT);
                #endif  // UA_ENABLE_DISCOVERY
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "=============================================================");

		return LDSClient_config1;
}
