#define UA_ENABLE_WEBSOCKET_SERVER
#ifdef UA_ENABLE_WEBSOCKET_SERVER
 #define TRANSPORT_PROFILE_URI_WSSBIN   "http://opcfoundation.org/UA_Profile/Transport/wss-uasc-uabinary"
 #define TRANSPORT_PROFILE_URI_WSSJSON  "http://opcfoundation.org/UA-Profile/Transport/wss-uajson"
 #define BROKER_ADDRESS_URL_WSS         "opc.ws://192.168.1.165:7681/"
#endif

#if defined(WIN32)
   #define HAVE_STRUCT_TIMESPEC
   #if defined(pid_t)
      #undef pid_t
   #endif
#endif

#ifdef almagamation
  #include <open62541/websockets.h>
#else
  #include "open62541.h"
#endif

#include <dirent.h>
#include "SV_Misc.h"

#define PRODUCT_URI "http://virtualskies.com.sg"
#define APPLICATION_NAME "Websocket OPCUA client Connector based on open62541"
#define APPLICATION_URI_WSS "urn:opc123.virtualskies.com.sg" //"urn:wss-cln.virtualskies.com.sg"
#define APPLICATION_URI_HTTPS "urn:opc123.virtualskies.com.sg" //"urn:https-cln.virtualskies.com.sg"
#define WS_WSS 1
#define WS_HTTPS 2

//------------------------------------------------------------------------------------------------------------------------------------------------------
// This function will be called by Javascript using Emscripten libraries to connect to the OPCUA Server to get data by subscribing to the monitoreditems
//------------------------------------------------------------------------------------------------------------------------------------------------------
UA_Client *createWebSocketsUAClient(int type, char* OPCUAServerIP)
{

	UA_Client *uaClient = UA_Client_new();
	UA_ClientConfig *config1 = UA_Client_getConfig(uaClient);;
	UA_StatusCode retval;

	UA_ClientConfig_setDefault(config1);
	config1->localConnectionConfig = UA_ConnectionConfig_default;
                //UA_ApplicationDescription_clear(&config1->clientDescription); //causes segmentation fault

		// config1->clienContext = ;
		// config1->logging = ;
		config1->timeout = 60000;
		if (type == WS_WSS)
                	config1->clientDescription.applicationUri = UA_STRING_ALLOC(APPLICATION_URI_WSS);
		else if (type == WS_HTTPS)
			config1->clientDescription.applicationUri = UA_STRING_ALLOC(APPLICATION_URI_HTTPS);

                config1->clientDescription.productUri = UA_STRING_ALLOC(PRODUCT_URI);
                config1->clientDescription.applicationName = UA_LOCALIZEDTEXT_ALLOC("en", APPLICATION_NAME);
                //config1->clientDescription.applicationType = UA_APPLICATIONTYPE_CLIENT; // causes segmentation fault
		//config1->clientDescription.gatewayServerUri = ??
		//config1->clientDescription.discoveryProfileUri = ??
		//config1->clientDescription.discoveryUrlsSize = ??
		//config1->clientDescription.discoveryUrls = ??

		config1->endpointUrl = UA_STRING_ALLOC(OPCUAServerIP);

		//strcpy((char *)config1->endpointUrl.data, OPCUAServerIP);
		//config1->endpointUrl.length = strlen(OPCUAServerIP);

		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "-------SV_CreateWebSocketsUAClient.c : endpointUrl = %s, length of endpointUrl is %d\n", (char *)config1->endpointUrl.data, (int)config1->endpointUrl.length);

		// Secure client connect
    		// config1->securityPoliciesSize = 4;	// Basic128Rsa15, Basic256, Basic256Sha256, Aes128Sha256RsaOaep
		config1->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;	// require encryption
		config1->securityPolicyUri = UA_STRING_ALLOC("");
		config1->secureChannelLifeTime = 10 * 60 * 1000;	// 10 minutes
    		config1->securityPolicies = (UA_SecurityPolicy*)UA_malloc(sizeof(UA_SecurityPolicy));
    		if (!config1->securityPolicies)
		{
			UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "-------SV_CreateWebSocketsUAClient.c : BAD_OUT_OF_MEMOMRY");
        		return NULL;
		}
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "-------SV_CreateWebSocketsUAClient.c : config->securityPolicies : success");

    		//config1->initConnectionFunc = UA_ClientConnectionTCP_init; /* for async client */
    		//config1->pollConnectionFunc = UA_ClientConnectionTCP_poll; /* for async connection */
    		config1->customDataTypes = NULL;

		// Set stateCallback : https://raw.githubusercontent.com/open62541/open62541/master/examples/client_method_async.c
		// static void stateCallback(UA_Client *client, UA_SecureChannelState channelState, UA_SessionState sessionState, UA_StatusCode connectStatus)
		//config1->timeout = 60000;
    		//config1->stateCallback = stateCallback;	// function implemented in AG_NewMethod.c

		config1->connectivityCheckInterval = 0;
    		config1->requestedSessionTimeout = 1200000; /* requestedSessionTimeout */
    		config1->inactivityCallback = NULL;
    		config1->clientContext = NULL;
		config1->connectivityCheckInterval = 3000; // in milliseconds

		#ifdef UA_ENABLE_SUBSCRIPTIONS
			config1->outStandingPublishRequests = 10;
    			config1->subscriptionInactivityCallback = NULL;
		#endif


	// load the client certificate
	char *env_sslcertificateloc = getenv("SVR_SSLCERTIFICATELOC");	// export SVR_SSLCERTIFICATELOC /usr/local/ssl/certs
	if (env_sslcertificateloc != NULL)
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c: retrieved environment variable <CLN_SSLCERTIFICATELOC : %s>", env_sslcertificateloc);
	else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c: cannot retrieve environment variable <CLN_SSLCERTIFICATELOC>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c: default to /usr/local/ssl/certs/Clncert20.pem");
		env_sslcertificateloc = (char *)calloc(255, sizeof(char));
		if (env_sslcertificateloc != NULL)
	                strcpy(env_sslcertificateloc, "/usr/local/ssl/certs/Opccert123.pem");
		else
		{
			UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c: cannot retrieve environment variable <CLN_SSLCERTIFICATELOC> :  out of memory");
			exit(UA_FALSE);
		}
        }

	// load the client key
	char *env_privatekeyloc = getenv("SVR_PRIVATEKEYLOC");		// export "SVR_PRIVATEKEYLOC" /usr/local/ssl/private
	if (env_privatekeyloc != NULL)
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c: retrieved environment variable <CLN_PRIVATEKEYLOC : %s>", env_privatekeyloc);
	else
	{
		UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c: cannot retrieve environment variable <CLN_PRIVATEKEYLOC>");
		UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c : default to /usr/local/ssl/private/Clnprivate-key20.pem");
		env_privatekeyloc = (char *)calloc(255, sizeof(char));
		if (env_privatekeyloc != NULL)
			strcpy(env_privatekeyloc, "/usr/local/ssl/private/Opcprivate-key123.pem");
		else
		{
			UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c: cannot retrieve environment variable <CLN_PRIVATEKEYLOC> : out of memory");
			exit(UA_FALSE);
		}
	}

        UA_ByteString certificate = loadFile(env_sslcertificateloc);
        UA_ByteString privateKey = loadFile(env_privatekeyloc);

// --------------------------------------------------------------------------------------Load trustlist
        char *env_trustlistloc = getenv("SVR_TRUSTLISTLOC");            // then iterate through the directory and save into uaSvrServer object - refer to Load trustlist
        if (env_trustlistloc != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c : retrieved environment variable : CLN_TRUSTLISTLOC : %s", env_trustlistloc);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c : cannot retrieve environment variable <CLN_TRUSTLISTLOC>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c : default to /usr/local/ssl/trustlist/");
                env_trustlistloc = (char *)calloc(255, sizeof(char));
                if (env_trustlistloc != NULL)
                        strcpy(env_trustlistloc, "/usr/local/ssl/trustlist/");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c : cannot retrieve environment variable <CLN_TRUSTLISTLOC> : out of memory");
                        exit(UA_FALSE);
                }
        }

        UA_ByteString *trustList=NULL;
        size_t trustListSize = 0;
        int total_pem_files=0, total_der_files=0, counter=0;                    // count the number of files with .pem and .der extension
        UA_ByteString trustcertificate;

        struct dirent *de;      // pointer for directory entry
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c : trustlistloc.data = %s", env_trustlistloc);

        DIR *dr = opendir((char *)env_trustlistloc);
        if (dr == NULL)
                 UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c : cannot find trustlist directory %s", env_trustlistloc);
        else
        {
                int retval;
                counter = 0;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,("--------AG_mainOPCUAClient.c : Start while loop ") );

                while ((de = readdir(dr)) != NULL) // read the list of files in the directory
                {
                        #ifdef DEBUG
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c :  iterating through the trustlist directory : pass %d", counter);
                        #endif
                        //char* file_name = de->d_name;
                        // process only the files with .pem extension
                        retval = stringEndsWith((char*)de->d_name, "pem");               // first count the total number .pem files
                        if (retval == 0) // file name ends with .pem
                        {
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c : Found PEM filename : <%s>", de->d_name);
                                total_pem_files++;
                        }
                        else
                        {
                                retval = stringEndsWith((char*)de->d_name, "der");               // also count the total number of .der files (e.g. UAExpert certificates
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c : Found DER filename : <%s>", de->d_name);
                                if (retval == 0) // file name ends with .der
                                {
                                        total_der_files++;
                                        // convert to .pem format as loadFile() can only read PEM files
                                        char derToPemfile[255];
                                        strncpy(derToPemfile, de->d_name, strlen(de->d_name));
                                        int len = strlen(de->d_name);
                                        derToPemfile[len-3] = 'p';
                                        derToPemfile[len-2] = 'e';
                                        derToPemfile[len-1] = 'm';
                                        derToPemfile[len] = '\0';
                                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c : converted der filename is <%s>", derToPemfile);

                                        // run the command to convert .der to .pem
                                        // sudo openssl x509 -inform der -in uaexpert.der -out uaexpert.pem

                                        char sys_command[255];
                                        strcpy(sys_command, "sudo openssl x509 -inform der -in ");
                                        strcat(sys_command, env_trustlistloc);
                                        strcat(sys_command, de->d_name);
                                        strcat(sys_command, " -out ");
                                        strcat(sys_command, env_trustlistloc);
                                        strcat(sys_command, derToPemfile);
                                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c : command to execute <%s>", sys_command);

                                        system(sys_command);
                                        // alternative, look for a C function in openssl to do the conversion
                                }
                        }
                }

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c :  pass 1 iterating through the trustlist directory for .pem and .der files : %d %d", total_pem_files, total_der_files);

                // next, based on the total number of files with .pem extension, allocate memory to trustlist
                trustList = (UA_ByteString *)UA_Array_new(total_pem_files+total_der_files, &UA_TYPES[UA_TYPES_BYTESTRING]);

                rewinddir(dr);          // now, rewind the directory pointer to the start
                while ((de =  readdir(dr)) != NULL)     // finally process the .pem and .der files and save into trustlist
                {
                        // get the file (.pem)
                        //char* file_name = de->d_name;
                        retval = stringEndsWith((char*)de->d_name, ".pem");
                        char fullpathname[100];
                        if (retval == 0) // file name ends with .pem
                        {
                                strcpy(fullpathname, env_trustlistloc);
                                strcat(fullpathname, de->d_name);
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c : .pem certificate found is <%s>", fullpathname);

                                // loadFile needs the full path
                                trustcertificate = loadFile(fullpathname);
                                UA_ByteString_copy(&trustcertificate, &trustList[counter]);
                                counter++;
                                //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c : contents of the .pem certificate is <%s>", trustcertificate.data); // display the contents of the certificate

                        }
                        #ifdef DER_FILES_ARE_NOT_USED_IN_OPEN62541
                        // get the file (.der)
                        retval = stringEndsWithEncrypt((char*)de->d_name, ".der");
                        if (retval == 0) // file name ends with .der
                        {
                                strcpy(fullpathname, env_trustlistloc);
                                strcat(fullpathname, de->d_name);
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------AG_mainOPCUAClient.c : .der certificate found is <%s>", fullpathname);
                                // loadFile needs the full path
                                trustcertificate = loadFile((char*)fullpathname);
                                UA_ByteString_copy(&trustcertificate,&trustList[counter]);
                                counter++;
                                //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------AG_mainOPCUAClient.c : contents of the certificate is %s", trustcertificate.data);       // NULL????????
                        }
                        #endif
                        //skip to the next file
                }
                closedir(dr);
        }
        trustListSize = counter;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c : Successfully loaded trustlist : no of trustlist loaded : %d", counter);

        #ifdef DEBUG
        for (counter=0; counter < trustListSize; counter++)
        {
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------AG_mainOPCUAClient.c :  Display the trustlist contents %d of %d", counter+1, trustListSize);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"%s", trustList[counter].data);
        }
        #endif
        // ------------------------------------------------------------------------- finish loading trustlist


                /* Loading of a revocation list currently unsupported */
                UA_ByteString *revocationList = NULL;
                size_t revocationListSize = 0;


	// ------------------------------------------------------------------------ finish loading revocationlist



                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "-------SV_CreateWebSocketsUAClient.c : Calling setDefaultEncryption");
                UA_ClientConfig_setDefaultEncryption(config1, certificate, privateKey,
                                                        trustList, trustListSize,
                                                        revocationList, revocationListSize);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "-------SV_CreateWebSocketsUAClient.c : Finished calling setDefaultEncryption");


                UA_ByteString_clear(&certificate);
                UA_ByteString_clear(&privateKey);
                for(size_t deleteCount = 0; deleteCount < trustListSize; deleteCount++)
                        UA_ByteString_clear(&trustList[deleteCount]);

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "-------SV_CreateWebSocketsUAClient.c : successfully setDefaultEncryption");

		char userid[100], password[100];
		printf("Please enter the userid to connect to the OPCUA Server : "); scanf("%s", userid);
		printf("Please enter the password to connecto the OPCUA Server : "); scanf("%s", password);


		if ( check_port_active(OPCUAServerIP, 4840) != 0)
		{
			UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CreateWebSocketsUAClient.c : remote server is not running");
			exit(EXIT_FAILURE);
		}

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CreateWebSocketsUAClient.c : Trying to connect to OPCUA server <%s> using <%s> <%s> ...", OPCUAServerIP, userid, password);
                #define UA_LOG_LEVEL = 300 // is used in UA_Client_connectUsername and functions

                //retval = UA_Client_connectUsername(uaClient, OPCUAServerIP, userid, password);
		retval = UA_ClientConfig_setAuthenticationUsername(config1, userid, password);
		if (retval != UA_STATUSCODE_GOOD)
                {
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CreateWebSocketsUAClient.c : fail to setAuthenticationUsername");
                        exit(EXIT_FAILURE);
                }
                else
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CreateWebSocketsUAClient.c : SetAuthenticationUsername success");

		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CreateWebSocketsUAClient.c : endpoint passed into UA_Client_connect() is <%s>", OPCUAServerIP);
		// format of OPCUAServerIP = opc.tcp://192.168.1.123:4840
                retval = UA_Client_connect(uaClient, OPCUAServerIP); 

                if (retval != UA_STATUSCODE_GOOD)
                {
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "-------SV_CreateWebSocketsUAClient.c : Cannot login securely to OPCUAServer : %s, %s", OPCUAServerIP, UA_StatusCode_name(retval));
                        exit(EXIT_FAILURE);
                }
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "-------SV_CreateWebSocketsUAClient.c : UA_Client_connectUsername() : success");

		return uaClient;

}


