#ifdef almagamation
#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/nodestore_default.h>
#include <stdio.h>
#else
   #define UA_ENABLE_DISCOVERY_MULTICAST
   #include "open62541.h"
#endif
#include "SV_Configure.h"

#define STRINGIFY(arg) #arg
#define VERSION(MAJOR, MINOR, PATCH, LABEL) \
    STRINGIFY(MAJOR) "." STRINGIFY(MINOR) "." STRINGIFY(PATCH) LABEL

// the following is required for Unified Automation UAExpert
static const size_t usernamePasswordsSize = 2;
static UA_UsernamePasswordLogin logins[2] = {
       // {UA_STRING_STATIC("jackybek"), UA_STRING_STATIC("thisisatestpassword24")},
        {UA_STRING_STATIC("jackybek"),UA_STRING_STATIC("8ebe744e41fa3494536e9648093ab4f4ae156071eb54274c1dbf4c320c2023e0")},
       //{UA_STRING_STATIC("admin"), UA_STRING_STATIC("defaultadminpassword")}
	{UA_STRING_STATIC("admin"), UA_STRING_STATIC("e10351222ba08fce9d5867d676e8af9a5bef18807d40dea7f048eaf0a3f8a738")}
};

int configureServer(UA_Server *uaLDSServer, char* userid, char* password)
{
    // check the password hash
    UA_Boolean found = UA_FALSE;
    for (int i=0; i < usernamePasswordsSize; i++)
    {

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, " ==============================================================================");
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Configure.c : calling configureServer ()");
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Configure.c : parameters passed in : <%s> <%s>", userid, password);

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Configure.c : Begin by comparing userid : <%s> with <%s> ...", userid, logins[i].username.data);
    	if ( strcmp(userid, (char *)logins[i].username.data) == 0 ) // found the username
	{
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Configure.c : Next, compare password : <%s> with <%s> ...", password, logins[i].password.data);
		if ( strcmp(password, (char *)logins[i].password.data) == 0 ) // password match
		{
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Configure.c : Password provided is found to match OPCUA registry");
			found = UA_TRUE;
			break;
		}
		else
			UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Configure.c : Password provided do not match OPCUA registry");
	}
	else
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Configure.c : Userid provided do not match OPCUA registry");

   }
   if (found == UA_FALSE) // cannot find the username in the list
   {
   	UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Configure.c : your given username and password cannot be found in OPCUA registry.  Shutting down");
	exit(0);
   }

    UA_StatusCode status;
    UA_ServerConfig *config = UA_Server_getConfig(uaLDSServer);

    config->accessControl.clear(&config->accessControl);
    //UA_CertificateVerification verifyX509;
    status = UA_AccessControl_default(config, UA_FALSE, &config->securityPolicies[config->securityPoliciesSize-1].policyUri, usernamePasswordsSize, logins);
    if (status != UA_STATUSCODE_GOOD)
        return EXIT_FAILURE;
    else
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Configure.c : adding 2 user credentials to OPCUA server (without anonymous) : %s", UA_StatusCode_name(status));

    if (config->nodestore.context == NULL)
        UA_Nodestore_HashMap(&config->nodestore);

    config->shutdownDelay = 0; //5000.0; // millisecond
    config->securityPolicyNoneDiscoveryOnly = UA_FALSE;

    // Server Description
    UA_BuildInfo_clear(&config->buildInfo);
    const char* env_product_uri = getenv("SVR_PRODUCT_URI");
    const char* env_manufacturer_name = getenv("SVR_MANUFACTURER_NAME");
    const char* env_product_name = getenv("SVR_PRODUCT_NAME");
    const char* env_application_uri_server = getenv("SVR_APPLICATION_URI_SERVER");
    const char* env_application_name = getenv("SVR_APPLICATION_NAME");

    if (env_application_uri_server== NULL)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Configure.c : Please set the Application URI Server via <export SVR_APPLICATION_URI_SERVER='svr.virtualskies.com.sg'>");
        exit(0);
    }

    config->buildInfo.productUri = UA_STRING_ALLOC(env_product_uri);  //(PRODUCT_URI);
    config->buildInfo.manufacturerName = UA_STRING_ALLOC(env_manufacturer_name); //(MANUFACTURER_NAME);
    config->buildInfo.productName = UA_STRING_ALLOC(env_product_name); //(PRODUCT_NAME);
    config->buildInfo.softwareVersion = UA_STRING_ALLOC(VERSION(UA_OPEN62541_VER_MAJOR, UA_OPEN62541_VER_MINOR,
                                                UA_OPEN62541_VER_PATCH, UA_OPEN62541_VER_LABEL));

    config->buildInfo.buildDate = UA_DateTime_now();
    config->buildInfo.buildNumber = UA_STRING_ALLOC(__DATE__ " " __TIME__);

    UA_ApplicationDescription_clear(&config->applicationDescription);
    UA_String_clear(&config->applicationDescription.applicationUri);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Configure.c : application uri server is --%s--", env_application_uri_server);

    config->applicationDescription.applicationUri = UA_STRING_ALLOC(env_application_uri_server); //UA_String_fromChars(env_application_uri_server); //(APPLICAT>
int len = strlen(env_application_uri_server);
config->applicationDescription.applicationUri.data[len]='\0';
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Configure.c : config->applicationDescription.applicationUri is --%s--",  config->applicationDescription.applicationUri.data);

    config->applicationDescription.applicationUri.length = 1000; //(config->applicationDescription.applicationUri.data); //strlen(env_application_uri_server)
    config->applicationDescription.productUri = UA_STRING_ALLOC(env_product_uri); //PRODUCT_URI);
    config->applicationDescription.applicationName = UA_LOCALIZEDTEXT_ALLOC("en", env_application_name); //, APPLICATION_NAME);

    // LDS ++ refer to https://opcfoundation.org/UA/schemas/1.05/ServerCapabilities.csv
    // NA, DA, HD, AC, HE, GDS, LDS, DI, ADI, FDI, FDIC, PLC, S95, RCP, PUB, AUTOID, MDIS, CNC, PLK, FDT, TMC, CSPP, 61850, PACKML, MTC
    // AUTOML, SERCOS, MIMOSA, WITSML, DEXPI, IOLINK, VROBOT, PNO,PADMIN, ALIAS, SKS, FXAC, FXCM 
    config->applicationDescription.applicationType = UA_APPLICATIONTYPE_DISCOVERYSERVER;   // acts as DISCOVERY SERVER ONLY OR UA_APPLICATIONTYPE_SERVER


    // Multicast DNS related settings - LDS - refer to github/open62541/open62541/examples/discovery/server_lds.c
    config->mdnsEnabled = true;
    config->mdnsConfig.mdnsServerName = UA_String_fromChars("Local Discovery Server");
    config->mdnsInterfaceIP = UA_String_fromChars("0.0.0.0");  // 42.42.42.42
    // set the capabilities
    config->mdnsConfig.serverCapabilitiesSize = 7;
    UA_String *caps = (UA_String *)UA_Array_new(config->mdnsConfig.serverCapabilitiesSize, &UA_TYPES[UA_TYPES_STRING]);
    caps[0] = UA_String_fromChars("DA");  // provide current data
    caps[1] = UA_String_fromChars("HD");  // provide historical data
    caps[2] = UA_String_fromChars("AC");   // alarms & conditions that may reqiure operator interaction
    caps[3] = UA_String_fromChars("HE");   // historical alarms & events
    caps[4] = UA_String_fromChars("GDS"); // global discovery Server information model
    caps[5] = UA_String_fromChars("RCP"); // supports the reverse connect capabilities defined in Part 6
    caps[6] = UA_String_fromChars("PUB"); // supports the Publisher capabilities defined in Part 14
    config->mdnsConfig.serverCapabilities = caps;
    config->discoveryCleanupTimeout = 60*60;
    config->verifyRequestTimestamp = UA_RULEHANDLING_ACCEPT; //UA_RULEHANDLING_WARN;

    // config->applicationDescription.gatewayServerUri = UA_STRING_NULL;
    // config->applicationDescription.discoveryProfileUri = UA_STRING_NULL;
    // config->applicationDescription.discoveryUrlsSize = 0;
    // config->applicationDescription.discoveryUrls = NULL;

    // Certificate Verification that accepts every certificate. Can be overwritten when the policy is specialized.
    // required for LDS
    //UA_CertificateVerification_AcceptAll(&config->certificateVerification);
    config->secureChannelPKI.clear(&config->secureChannelPKI);
    //UA_ByteString_clear(&certificate);
    //UA_ByteString_clear(&privateKey);
    //for (size_t i = 0; i < trustListSize; i++)
    //    UA_ByteString_clear(&trustList[i]);
    // Limits for SecureChannels - required for LDS
    config->maxSecureChannels = 100;
    config->maxSecurityTokenLifetime = 10 * 60 * 1000; // 10 minutes */

    // Limits for Sessions - required for LDS
    config->maxSessions = 100;
    config->maxSessionTimeout = 60 * 60 * 1000;    // 1 hour

    // Limits for Discovery - required for LDS
    config->discoveryCleanupTimeout = 60 * 60;

        return UA_STATUSCODE_GOOD;
}
