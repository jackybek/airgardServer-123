#ifdef no_almagamation
//#include <open62541/network_tcp.h>
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
#include <dirent.h>

#define UA_ENABLE_ENCRYPTION
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



UA_ClientConfig * registerToLDS(UA_Server *uaServer1, char* lds_endpoint_A)
{
        UA_StatusCode retval;

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "==========================================================");

                #ifdef UA_ENABLE_DISCOVERY
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Begin LDS registration process");

                //-------------- register itself to the LDS Server <192.168.1.44>
                // code based on githug/open62541/62541/examples/discovery/server_register.c
                // acting as an OPCUA Client to LDS Server
		char* env_ldscertificate = getenv("SVR_LDS_SSLCERTIFICATELOC");
		if (env_ldscertificate != NULL)
			 UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : retrieved environment variable <SVR_LDS_SSLCERTIFICATELOC> : %s", env_ldscertificate);
		else
		{
			UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : cannot retrieve environment variable <SVR_LDS_SSLCERTIFICATELOC>");
			UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : default to /usr/local/ssl/certs/ldscert44.pem");
			env_ldscertificate = (char*)calloc(255, sizeof(char));
			if (env_ldscertificate != NULL)
				strcpy(env_ldscertificate, "/usr/local/ssl/certs/ldscert44.pem");
			else
			{
				UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : cannot retrieve environment variable <SVR_LDS_SSLCERTIFICATELOC> : out of memory");
				exit(UA_FALSE);
			}
		}

		char* env_ldsprivatekeyloc = getenv("SVR_LDS_PRIVATEKEYLOC");
		if (env_ldsprivatekeyloc != NULL)
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : retrieved environment variable <SVR_LDS_PRIVATEKEYLOC> : %s", env_ldsprivatekeyloc);
		else
		{
			UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : cannot retrieve environment variable <SVR_LDS_PRIVATEKEYLOC>");
			UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : default to /usr/local/ssl/private/ldsprivate-key44.pem");
			env_ldsprivatekeyloc = (char*)calloc(255, sizeof(char));
			if (env_ldsprivatekeyloc != NULL)
				strcpy(env_ldsprivatekeyloc, "/usr/local/ssl/private/ldsprivate-key44.pem");
			else
			{
				UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : cannot retrieve environment variable <SVR_LDS_PRIVATEKEYLOC> : out of memory");
                                exit(UA_FALSE);
			}
		}

                UA_ByteString LDScertificate = loadFile(env_ldscertificate); //loadFile(DISCOVERY_SSLCERTIFICATELOC);  //loadFile("/etc/ssl/certs/62541LDSServerCert.pem"); //=> symbolic link
                //UA_ByteString certificate = loadFile("/usr/local/ssl/certs/ldscert44.pem"); // actual location
                if (LDScertificate.length == 0)
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c: Unable to load file : %s", env_ldscertificate);
                        //goto cleanup;
                        exit(UA_FALSE);
                }
		else
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : successfully loaded LDS certificate");

                UA_ByteString LDSprivateKey = loadFile(env_ldsprivatekeyloc);  //loadFile("/usr/local/ssl/private/62541LDSprivate-key.pem");
                //UA_ByteString LDSprivateKey = loadFile("/usr/local/ssl/private/ldsprivate-key.pem");
                if (LDSprivateKey.length == 0)
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Unable to load file : %s", env_ldsprivatekeyloc);
                        //goto cleanup;
                        exit(UA_FALSE);
                }
		else
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : successfully loaded LDS privateKey");

// --------------------------------------------------------------------------------------Load trustlist
        size_t trustListSize = 0;
	UA_ByteString *trustList = NULL;
        UA_ByteString trustcertificate;
        int total_pem_files=0, total_der_files=0, counter=0;                    // count the number of files with .pem and .der extension
        char *env_trustlistloc = getenv("SVR_TRUSTLISTLOC");            // then iterate through the directory and save into uaSvrServer object - refer to Load trustlist
        if (env_trustlistloc != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : retrieved environment variable : SVR_TRUSTLISTLOC : %s", env_trustlistloc);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : cannot retrieve environment variable <SVR_TRUSTLISTLOC>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : default to /usr/local/ssl/trustlist/");
                env_trustlistloc = (char *)calloc(255, sizeof(char));
                if (env_trustlistloc != NULL)
                        strcpy(env_trustlistloc, "/usr/local/ssl/trustlist/");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : cannot retrieve environment variable <SVR_TRUSTLISTLOC> : out of memory");
                        exit(UA_FALSE);
                }
        }

        struct dirent *de;      // pointer for directory entry
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : trustlistloc.data = %s", env_trustlistloc);
        DIR *dr = opendir((char *)env_trustlistloc);
        if (dr == NULL)
                 UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : cannot find trustlist directory %s", env_trustlistloc);
        else
        {
                int retval;
                counter = 0;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,("--------SV_Register.c : Start while loop ") );

                while ((de = readdir(dr)) != NULL) // read the list of files in the directory
                {
                        #ifdef DEBUG
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c :  iterating through the trustlist directory : pass %d", counter);
                        #endif
                        //char* file_name = de->d_name;
                        // process only the files with .pem extension
                        retval = stringEndsWith((char*)de->d_name, "pem");              // first count the total number .pem files
                        if (retval == 0) // file name ends with .pem
                        {
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : Found PEM filename : <%s>", de->d_name);
                                total_pem_files++;
                        }
                        else
                        {
                                retval = stringEndsWith((char*)de->d_name, "der");              // also count the total number of .der files (e.g. UAExpert certificates
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : Found DER filename : <%s>", de->d_name);
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
                                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : converted der filename is <%s>", derToPemfile);

                                        // run the command to convert .der to .pem
                                        // sudo openssl x509 -inform der -in uaexpert.der -out uaexpert.pem

                                        char sys_command[255];
                                        strcpy(sys_command, "sudo openssl x509 -inform der -in ");
                                        strcat(sys_command, env_trustlistloc);
                                        strcat(sys_command, de->d_name);
                                        strcat(sys_command, " -out ");
                                        strcat(sys_command, env_trustlistloc);
                                        strcat(sys_command, derToPemfile);
                                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : command to execute <%s>", sys_command);
                                        //sprintf(sys_command, "sudo openssl x509 -inform der -in %s -out %s", de->d_name, derfile);
                                        system(sys_command);
                                        // alternative, look for a C function in openssl to do the conversion
                                }
                        }

                }

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c :  pass 1 iterating through the trustlist directory for .pem and .der files : %d %d", total_pem_files, total_der_files);

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
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : .pem certificate found is <%s>", fullpathname);

                                // loadFile needs the full path
                                trustcertificate = loadFile(fullpathname);
                                UA_ByteString_copy(&trustcertificate, &trustList[counter]);
                                counter++;
                                //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : contents of the .pem certificate is <%s>", trustcertificate.data); // display the contents >

                        }

                        #ifdef DER_FILES_ARE_NOT_USED_IN_OPEN62541
                        // get the file (.der)
                        retval = stringEndsWith((char*)de->d_name, ".der");
                        if (retval == 0) // file name ends with .der
                        {
                                strcpy(fullpathname, env_trustlistloc);
                                strcat(fullpathname, de->d_name);
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : .der certificate found is <%s>", fullpathname);
                                // loadFile needs the full path
                                trustcertificate = loadFile((char*)fullpathname);
                                UA_ByteString_copy(&trustcertificate,&trustList[counter]);
                                counter++;
                                //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Encrypt.c : contents of the certificate is %s", trustcertificate.data);       // NULL????????
                        }
                        #endif
                        //skip to the next file
                }
                closedir(dr);
        }
        trustListSize = counter;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : Successfully loaded trustlist : no of trustlist loaded : %d", counter);

        #ifdef DEBUG
        for (counter=0; counter < trustListSize; counter++)
        {
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c :  Display the trustlist contents %d of %d", counter+1, trustListSize);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"%s", trustList[counter].data);
        }
        #endif

// ------------------------------------------------------------------------- finish loading trustlist

        // Loading of a issuer list, not used in this application
        //UA_ByteString *issuerList = NULL;
        //size_t issuerListSize = 0;
        //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------LDS_encryptServer.c : issueList is not supported : %d", (int)issuerListSize);

                /* Loading of a revocation list currently unsupported */
                UA_ByteString *revocationList = NULL;
                size_t revocationListSize = 0;

                UA_Client *LDSclient = UA_Client_new();
		char *env_LDSport = getenv("SVR_LDS_PORT");

		if (env_LDSport != NULL)
   			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : retrieved environment variable <SVR_LDS_PORT> : %s", env_LDSport);
    		else
    		{
			UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : cannot retrieve environment variable <SVR_LDS_PORT>");
			UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : default to 4841");
			env_LDSport = (char*)calloc(255, sizeof(char));
			if (env_LDSport != NULL)
				strcpy(env_LDSport, "4841");
			else
			{
        			UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : cannot retrieve environment variable <SVR_LDS_PORT> : out of memory");
        			exit(UA_FALSE);
			}
    		}


		char lds_endpoint[100];
		//printf("1 : %s, %d \n", lds_endpoint_A, strlen(lds_endpoint_A));
		strcpy(lds_endpoint, "opc.tcp://");
		int index = strlen(lds_endpoint_A);
		strncpy(&lds_endpoint[10], lds_endpoint_A, strlen(lds_endpoint_A));
		lds_endpoint[index+10]= ':';
		strcpy(&lds_endpoint[index+11], env_LDSport);
		int total_length = strlen(lds_endpoint);
		//lds_endpoint[total_length] = '/';
		lds_endpoint[total_length] =  '\0';
		//printf("2 : %s, %d \n", lds_endpoint, strlen(lds_endpoint));

		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : successfully assembled lds endpoint url : %s", lds_endpoint);


                UA_ClientConfig *LDSClient_config1 = UA_Client_getConfig(LDSclient);
                UA_ClientConfig_setDefault(LDSClient_config1);	// initiailise the clientConfig object with basic information


                //LDSClient_config1->clientContext = NULL;
		//LDSClient_config1->logging = NULL;
		LDSClient_config1->timeout = 5000; // in millisecond, 0 = no timeout

		// -------- start setting the ApplicationUri
		// The description must be internally consistent.
     		// - The ApplicationUri set in the ApplicationDescription must match the
     		//   URI set in the certificate

                UA_ApplicationDescription_clear(&LDSClient_config1->clientDescription);

		char *env_ldscertificateloc = getenv("SVR_LDS_SSLCERTIFICATELOC");
		if (env_ldscertificateloc != NULL)
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : retrieved environment variable : SVR_LDS_SSLCERTIFICATELOC : %s", env_ldscertificateloc);
		else
		{
			UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : cannot retrieve environment variable : SVR_LDS_SSLCERTIFICATELOC");
        	        env_ldscertificateloc = (char*)calloc(100,sizeof(char));
                	if (env_ldscertificateloc != NULL)
			{
                        	strcpy(env_ldscertificateloc, "/usr/local/ssl/cert/ldscert44.pem");
				UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : default to <%s>", env_ldscertificateloc);
			}
                	else
                	{
                        	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : cannot retrieve environment variable <SVR_LDS_SSLCERTIFICATELOC> :  out of memory");
                        	exit(UA_FALSE);
                	}
		}
		//const char *env_ldsapplicationuri = getenv("SVR_LDS_APPLICATION_URI");// if use SVR_LDS_APPLICATION_URI, will get a warning on mismtach ApplicationURI

		char *env_ldsapplicationuri = getenv("SVR_LDS_APPLICATION_URI"); // try to swap with the server application uri
		if (env_ldsapplicationuri != NULL)
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : retrieved environment variable : SVR_LDS_APPLICATION_URI : %s", env_ldsapplicationuri);
		else
		{
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : cannot retrieve environment variable : SVR_LDS_APPLICATION_URI");
                        env_ldscertificateloc = (char*)calloc(100,sizeof(char));
                        if (env_ldscertificateloc != NULL)
			{
                                strcpy(env_ldscertificateloc, "urn:lds.virtualskies.com.sg");
				UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : default to <%s>", env_ldscertificateloc);
			}
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : cannot retrieve environment variable <SVR_LDS_APPLICATION_URI> :  out of memory");
                                exit(UA_FALSE);
                        }
		}
                LDSClient_config1->clientDescription.applicationUri = UA_STRING_ALLOC(env_ldsapplicationuri);
		// ---- end setting the ApplicationUri

		// endpointUrl format : opc.tcp://192.168.1.44:4841/
		LDSClient_config1->endpointUrl = UA_STRING_ALLOC(lds_endpoint);

                // Secure client connect
		//LDSClient_config1->userIdentityToken = NULL;
                LDSClient_config1->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT; // require encryption
                LDSClient_config1->securityPolicyUri = UA_STRING_ALLOC(""); // empty string indicates the client to select any matching SecurityPolicy
		LDSClient_config1->noSession = UA_FALSE;
		LDSClient_config1->noReconnect = UA_FALSE;
		LDSClient_config1->noNewSession = UA_FALSE;

		//LDSClient_config1->endpoint = ???;
		//LDSClient_config1->userTokenPolicy = ???;
		//LDSClient_config1->applicationUri = ???;

		// custom Data types
                //LDSClient_config1->customDataTypes = ???;

		// Advance Client Configuration
                LDSClient_config1->secureChannelLifeTime = 10 * 60 * 1000;        // 10 minutes
                LDSClient_config1->requestedSessionTimeout = 1200000; /* milliseconds */
                LDSClient_config1->localConnectionConfig = UA_ConnectionConfig_default;
                LDSClient_config1->connectivityCheckInterval = 3000; // in milliseconds

		// Event loop
		//LDSClient_config1->eventLoop = ???
		//LDSClient_config1->externalEventLoop = ???

		//------- Start Available security policies
		LDSClient_config1->securityPoliciesSize = 0;	// refer to the code below ; what if I want to set all 4 policies in the client object???
			// are the following READONLY ???

			//LDSClient_config1->securityPolicies->policyContext = ???;
			//LDSClient_config1->securityPolicies->policyUri = ???;
			//LDSClient_config1->securityPolicies->localCertificate =???;
			//LDSClient_config1->securityPolicies->asymmetricModule = ???;
			//LDSClient_config1->securityPolicies->symmetricModule = ???;
			//LDSClient_config1->securityPolicies->certificateSigningAlgorithm = ???;
			//LDSClient_config1->securityPolicies->channelModule = ???;
			//LDSClient_config1->securityPolicies->logger = ???;

		UA_SecurityPolicy securityPolicy[LDSClient_config1->securityPoliciesSize];
		LDSClient_config1->securityPolicies = securityPolicy;

	/* -----------------------------------------------NOTE : the following is NOT required, as it will be configured automatically
		//for (size_t i=0; i< LDSClient_config1->securityPoliciesSize; i++)
		//	securityPolicy[i] = (UA_SecurityPolicy)UA_malloc(sizeof(UA_SecurityPolicy));
		// try 1 : configure a secure connection with Basic256Sha256
		LDSClient_config1->securityPolicyUri = UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
		retval = UA_SecurityPolicy_Basic256Sha256(&LDSClient_config1->securityPolicies[1], LDScertificate, LDSprivateKey, LDSClient_config1->logging); //securityPolicy[1].policyUri = UA_String(UA_SECURITY_POLICY_BASIC256SHA256_URI);
		if (retval != UA_STATUSCODE_GOOD)
		{
			UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : fail to set securityPolicyUri (Basic256Sha256) : %s", UA_StatusCode_name(retval) );
			// try 2 : configure a secure connection with Aes256Sha256Rsapss
			LDSClient_config1->securityPolicyUri = UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Aes256Sha256RsaPss");
			retval = UA_SecurityPolicy_Aes256Sha256RsaPss(&LDSClient_config1->securityPolicies[2], LDScertificate, LDSprivateKey, LDSClient_config1->logging); //securityPolicy[2].policyUri = UA_String(UA_SECURITY_POLICY_AES256SHA256RSAPSS_URI);
			if (retval != UA_STATUSCODE_GOOD)
			{
				UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : fail to set securityPolicyUri (Aes256Sha256RsaPss) : %s", UA_StatusCode_name(retval));
				// try 3 : configure a secure connection with Aes128Sha256Rsaoaep
				LDSClient_config1->securityPolicyUri = UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Aes128Sha256RsaOaep");
				retval = UA_SecurityPolicy_Aes128Sha256RsaOaep(&LDSClient_config1->securityPolicies[3], LDScertificate, LDSprivateKey, LDSClient_config1->logging); // securityPolicy[3].policyUri = UA_String(UA_SECURITY_POLICY_AES128SHA256RSAOAEP_URI);
				if (retval != UA_STATUSCODE_GOOD)
				{
					UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : fail to set securityPolicyUri (Aes128Sha256RsaOaep) : %s", UA_StatusCode_name(retval));
                			// Default is None
                			retval = UA_SecurityPolicy_None(&LDSClient_config1->securityPolicies[0], UA_BYTESTRING_NULL, LDSClient_config1->logging); //securityPolicy[0].policyUri = UA_String(UA_SECURITY_POLICY_NONE_URI);
					if (retval != UA_STATUSCODE_GOOD)
					{
						UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : fail to set securityPolicyUri (None) : %s", UA_StatusCode_name(retval));
                                		exit(UA_FALSE);
                        		}
					else
					{
						LDSClient_config1->securityMode = UA_MESSAGESECURITYMODE_NONE;
						UA_ByteString_clear(&LDSClient_config1->securityPolicyUri);
						LDSClient_config1->securityPolicyUri = UA_String_fromChars("http://opcfoundation.org/UA/SecurityPolicy#None");
						UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : Successfully set securityPolicyUri : UA_SecurityPolicy_None");
					}
				}
				else
				{
					LDSClient_config1->securityMode = UA_MESSAGESECURITYMODE_INVALID;	// allows everything
					UA_ByteString_clear(&LDSClient_config1->securityPolicyUri);
					LDSClient_config1->securityPolicyUri = UA_String_fromChars("http://opcfoundation.org/UA/SecurityPolicy#Aes128Sha256RsaOaep");
					UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : Successfully set securityPolicyUri : UA_SecurityPolicy_Aes128Sha256RsaOaep");
				}
			}
			else
			{
				LDSClient_config1->securityMode = UA_MESSAGESECURITYMODE_INVALID;       // allows everything
				UA_ByteString_clear(&LDSClient_config1->securityPolicyUri);
				LDSClient_config1->securityPolicyUri = UA_String_fromChars("http://opcfoundation.org/UA/SecurityPolicy#Aes256Sha256RsaPss");
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : Successfully set securityPolicyUri : UA_SecurityPolicy_Aes256Sha256RsaPss");
			}
		}
		else
		{
			LDSClient_config1->securityMode = UA_MESSAGESECURITYMODE_INVALID;       // allows everything
			UA_ByteString_clear(&LDSClient_config1->securityPolicyUri);
			LDSClient_config1->securityPolicyUri = UA_String_fromChars("http://opcfoundation.org/UA/SecurityPolicy#Basic256Sha256");
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Register.c : Successfully set securityPolicyUri : UA_SecurityPolicy_Basic256Sha256");
		}
		//------- End Available security policies
	*/
		//LDSClient_config1->certificateVerification = NULL;
		//LDSClient_config1->authSecurityPoliciesSize = 0;
		//LDSClient_config1->authSecurityPolicies = NULL;
		//LDSClient_config1->authSecurityPolicyUri = ;

        //      LDSClient_config1->initConnectionFunc = UA_ClientConnectionTCP_init; /* for async client */
        //      LDSClient_config1->pollConnectionFunc = UA_ClientConnectionTCP_poll; /* for async connection */

                LDSClient_config1->inactivityCallback = NULL;
		LDSClient_config1->subscriptionInactivityCallback = NULL;
		// Session config
		LDSClient_config1->sessionName = UA_STRING("AirgardServer-109 connection to LDS");
		LDSClient_config1->sessionLocaleIds = NULL;
		LDSClient_config1->sessionLocaleIdsSize = 0;

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Encrypting Svr's client object with LDS certificates");
		if (LDSClient_config1->securityMode == UA_MESSAGESECURITYMODE_NONE)
                	retval = UA_ClientConfig_setDefaultEncryption(LDSClient_config1, LDScertificate, LDSprivateKey,			// by right LDSprivateKey should be NULL here
                                                                trustList, trustListSize,
                                                                revocationList, revocationListSize);
		else // securityMode == UA_MESSAGESECURITYMODE_INVALID;       // allows everything
			retval = UA_ClientConfig_setDefaultEncryption(LDSClient_config1, LDScertificate, LDSprivateKey,
                                                                trustList, trustListSize,
                                                                revocationList, revocationListSize);

		if (retval != UA_STATUSCODE_GOOD)
		{
			UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Cannot encrypt Svr's client object with LDS certificate : %s",  UA_StatusCode_name(retval));
			return LDSClient_config1;
		}
 		else
		{
                	//UA_ByteString_clear(&LDScertificate);
                	//UA_ByteString_clear(&LDSprivateKey);
                	//for (size_t deleteCounter=0; deleteCounter < LDStrustListSize; deleteCounter++)
                	//        UA_ByteString_clear(&LDStrustList[deleteCounter]);
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Successfully encrypted Svr's client object with LDS certificates (as LDS Client)");
		}

		//const char* env_lds_username = getenv("SVR_LDS_USERNAME");
		//const char* env_lds_password = getenv("SVR_LDS_PASSWORD");
		char userid[100], passwd[100];
		printf("Please enter the username to login to LDS : "); scanf("%s", userid);
		printf("Please enter the password to login to LDS : "); scanf("%s", passwd);

                //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Trying to connect to LDS server <%s> using <%s> <%s> ...", lds_endpoint, userid, passwd); //env_lds_username, env_lds_password);
		// lds_endpoint format : opc.tcp://192.168.1.44:4841
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Trying to connect to LDS server <%s> using <%s> <%s> ...", lds_endpoint, userid, passwd);
		#define UA_LOG_LEVEL = 300 // is used in UA_Client_connectUsername and functions
                //retval = UA_Client_connectUsername(LDSclient, lds_endpoint, userid, passwd); //DISCOVERY_USERNAME, DISCOVERY_PASSWORD);
		retval = UA_ClientConfig_setAuthenticationUsername(LDSClient_config1, userid, passwd);
		if (retval != UA_STATUSCODE_GOOD)
		{
			UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : fail to setAuthenticationUsername");
			return LDSClient_config1;
		}
		else
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : SetAuthenticationUsername success");

		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : endpoint passed into UA_Client_connect() is <%s>",lds_endpoint);
		retval = UA_Client_connect(LDSclient, lds_endpoint); // this function check if the endpointUrl set in LDSclient->config = 
                if (retval != UA_STATUSCODE_GOOD)
                {
			UA_ClientConfig *myConfig = LDSClient_config1;
			// the following debug statements won't work : invalid initializer, invalid use of incomplete typedef ‘UA_Client’
			// myConfig->userIdentityToken is of type UA_ExtensionObject
			// UA_String myString = getEndpointUrl(LDSclient);
			//UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : LDSclient->endpointUrl.data = %s", myString.data);
			//UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : LDSclient->endpointUrl.length = %d", LDSclient->endpointUrl.length);
			//UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : LDSclient->discoveryUrl.data = %s", LDSclient->discoveryUrl.data);
			//UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : LDSclient->discoveryUrl.length = %d", LDSclient->discoveryUrl.length);

			UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : endpoint=<%s> userid=<%s> password=<%s>",
							myConfig->endpointUrl.data, (char*)myConfig->userIdentityToken.content.decoded.data, (char*)myConfig->userIdentityToken.content.decoded.data);
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Cannot login securely to LDS Server : <%s>", lds_endpoint);
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : error is %s", UA_StatusCode_name(retval));
			UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c  : UA_LDS_connectUsername() : failure");


			UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "---------------------------------------------------------");
			UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : Default to Anonymous login to LDS");
			retval = UA_ClientConfig_setAuthenticationUsername(LDSClient_config1, NULL, NULL);
	                if (retval != UA_STATUSCODE_GOOD)
        	        {
                	        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : fail to setAuthenticationUsername to anonymous login");
                        	return LDSClient_config1;
                	}
                	else
                        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c : SetAuthenticationUsername to anonymous login success");
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
                        return  LDSClient_config1;
                }
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c  : End LDS registration process");
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Register.c  : registered to LDS Server <%s>", DISCOVERY_SERVER_ENDPOINT);
                #endif  // UA_ENABLE_DISCOVERY

		return LDSClient_config1;
}
