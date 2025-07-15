#ifdef no_almagamation
#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/nodestore_default.h>
#else
   #include "open62541.h"
#endif

#include <stdio.h>
#include <dirent.h>
#include "SV_Encrypt.h"
#include "SV_Misc.h"

int stringEndsWithEncrypt(
        char const * const name,
        char const * const extension_to_find)
{
	#ifdef DEBUG
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : in stringEndsWithEncrypt() function : name = %s, extension = %s", name, extension_to_find);
	#endif

    int is_found = 0;
    size_t length = 0;
    char* ldot = NULL;
    if (name == NULL) return -1;
    if (extension_to_find == NULL) return -1;
    length = strlen(extension_to_find);
    if (length == 0) return -1;
    ldot = strrchr(name, extension_to_find[0]);
    if (ldot != NULL)
    {
	#ifdef DEBUG
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : ldot = %s, extension = %s, length = %d", ldot, extension_to_find, length);
	#endif
        is_found = strncmp(ldot, extension_to_find, length);
	if (is_found == 0) // found
	{
		//UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : found file <%s>", name);
		return 0;	// success return 0
	}
	else
		return -1;	// fail to find : return -1
    }
    else
       return -1;	// all other situation return -1
}

int encryptServer(UA_Server *uaSvrServer, UA_ServerConfig *config, bool ForceGenerateSelfSignedCert)
{
        UA_StatusCode status;

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : Entering encryptServer() function");
	char *env_SVRport = getenv("SVR_PORT");
	if (env_SVRport != NULL)
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : retrieved environment variable : SVR_PORT : %s", env_SVRport);
	else
	{
		UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : cannot retrieve environment variable <SVR_PORT : %s>", env_SVRport);
		UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : default to 4840");
		env_SVRport = (char*)calloc(5,sizeof(char));
		if (env_SVRport != NULL)
			strcpy(env_SVRport, "4840");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : cannot retrieve environment variable <SVR_PORT> :  out of memory");
                        exit(UA_FALSE);
                }
	}

        char *env_trustlistloc = getenv("SVR_TRUSTLISTLOC");		// then iterate through the directory and save into uaSvrServer object - refer to Load trustlist
	if (env_trustlistloc != NULL)
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : retrieved environment variable : SVR_TRUSTLISTLOC : %s", env_trustlistloc);
	else
	{
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : cannot retrieve environment variable <SVR_TRUSTLISTLOC : %s>", env_trustlistloc);
		UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : default to /usr/local/ssl/trustlist/");
		env_trustlistloc = (char *)calloc(255, sizeof(char));
		if (env_trustlistloc != NULL)
			strcpy(env_trustlistloc, "/usr/local/ssl/trustlist/");
		else
		{
			UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : cannot retrieve environment variable <SVR_TRUSTLISTLOC> : out of memory");
			exit(UA_FALSE);
		}
	}
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : Finished retrieving all environment variables");

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : setMinimal configuration");
        UA_ServerConfig_setMinimal(config, atoi(env_SVRport), NULL);

        // --------------------------------------------------------------------------------------Load trustlist
	UA_ByteString *trustList=NULL;
	size_t trustListSize = 0;
	int total_pem_files=0, total_der_files=0, counter=0;			// count the number of files with .pem and .der extension
	UA_ByteString trustcertificate;

	struct dirent *de;	// pointer for directory entry
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : trustlistloc.data = %s", env_trustlistloc);

	DIR *dr = opendir((char *)env_trustlistloc);
	if (dr == NULL)
		 UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : cannot find trustlist directory %s", env_trustlistloc);
	else
	{
		int retval;
		counter = 0;
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,("--------SV_Encrypt.c : Start while loop ") );

		while ((de = readdir(dr)) != NULL) // read the list of files in the directory
		{
			#ifdef DEBUG
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c :  iterating through the trustlist directory : pass %d", counter);
			#endif
                        //char* file_name = de->d_name;
			// process only the files with .pem extension
			retval = stringEndsWithEncrypt((char*)de->d_name, "pem");		// first count the total number .pem files
			if (retval == 0) // file name ends with .pem
			{
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : Found PEM filename : <%s>", de->d_name);
				total_pem_files++;
			}
			else
			{
				retval = stringEndsWithEncrypt((char*)de->d_name, "der");		// also count the total number of .der files (e.g. UAExpert certificates
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : Found DER filename : <%s>", de->d_name);
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
					UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : converted der filename is <%s>", derToPemfile);

					// run the command to convert .der to .pem
					// sudo openssl x509 -inform der -in uaexpert.der -out uaexpert.pem

					char sys_command[255];
					strcpy(sys_command, "sudo openssl x509 -inform der -in ");
					strcat(sys_command, env_trustlistloc);
					strcat(sys_command, de->d_name);
					strcat(sys_command, " -out ");
					strcat(sys_command, env_trustlistloc);
					strcat(sys_command, derToPemfile);
					UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : command to execute <%s>", sys_command);
					//sprintf(sys_command, "sudo openssl x509 -inform der -in %s -out %s", de->d_name, derfile);
					system(sys_command);
					// alternative, look for a C function in openssl to do the conversion
				}
			}
		}

		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c :  pass 1 iterating through the trustlist directory for .pem and .der files : %d %d", total_pem_files, total_der_files);

		// next, based on the total number of files with .pem extension, allocate memory to trustlist
	        trustList = (UA_ByteString *)UA_Array_new(total_pem_files+total_der_files, &UA_TYPES[UA_TYPES_BYTESTRING]);

		rewinddir(dr);		// now, rewind the directory pointer to the start
		while ((de =  readdir(dr)) != NULL)	// finally process the .pem and .der files and save into trustlist
		{
			// get the file (.pem)
			//char* file_name = de->d_name;
			retval = stringEndsWithEncrypt((char*)de->d_name, ".pem");
			char fullpathname[100];
			if (retval == 0) // file name ends with .pem
			{
				strcpy(fullpathname, env_trustlistloc);
				strcat(fullpathname, de->d_name);
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : .pem certificate found is <%s>", fullpathname);

				// loadFile needs the full path
				trustcertificate = loadFile(fullpathname);
        			UA_ByteString_copy(&trustcertificate, &trustList[counter]);
				counter++;
				//UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : contents of the .pem certificate is <%s>", trustcertificate.data); // display the contents of the certificate

			}

			#ifdef DER_FILES_ARE_NOT_USED_IN_OPEN62541
			// get the file (.der)
			retval = stringEndsWithEncrypt((char*)de->d_name, ".der");
			if (retval == 0) // file name ends with .der
			{
                                strcpy(fullpathname, env_trustlistloc);
                                strcat(fullpathname, de->d_name);
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Encrypt.c : .der certificate found is <%s>", fullpathname);
				// loadFile needs the full path
				trustcertificate = loadFile((char*)fullpathname);
				UA_ByteString_copy(&trustcertificate,&trustList[counter]);
				counter++;
				//UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Encrypt.c : contents of the certificate is %s", trustcertificate.data);	// NULL????????
			}
			#endif
			//skip to the next file
		}
		closedir(dr);
	}
        trustListSize = counter;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : Successfully loaded trustlist : no of trustlist loaded : %d", counter);

	#ifdef DEBUG
	for (counter=0; counter < trustListSize; counter++)
	{
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c :  Display the trustlist contents %d of %d", counter+1, trustListSize);
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"%s", trustList[counter].data);
	}
	#endif
	// ------------------------------------------------------------------------- finish loading trustlist

        // Loading of a issuer list, not used in this application
        UA_ByteString *issuerList = NULL;
        size_t issuerListSize = 0;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : issueList is not supported");
        // Loading of a revocation list currently unsupported
        UA_ByteString *revocationList = NULL;
        size_t revocationListSize = 0;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : revocationList is not supported");

	// new capability : Cert Rejected Directory
	// #define UA_ENABLE_CERT_REJECTED_DIR
	// TODO


        UA_ByteString derPrivKey = UA_BYTESTRING_NULL;		// generated
        UA_ByteString derCert = UA_BYTESTRING_NULL;		// generated
        UA_ByteString certificate = UA_BYTESTRING_NULL;		// loaded
        UA_ByteString privateKey = UA_BYTESTRING_NULL;		// loaded

        if (ForceGenerateSelfSignedCert == UA_TRUE)
        {
	        // This section will create self-signed certificate generated using generateSSCert() function instead of the certs/ keys present in "SVR_SSLCERTIFICATELOC" and "SVR_PRIVATEKEYLOC"
		// We only need the trustList, revocationList and issuerList from the above section.
		// the certificate and key will be regenerated here

		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c : Preparing to generate self-signed certificate and key ..");

                status = generateSSCert(uaSvrServer, config,
                                trustList, trustListSize,
                                issuerList, issuerListSize,
                                revocationList, revocationListSize,
                                &derPrivKey, &derCert);

                UA_assert(status == UA_STATUSCODE_GOOD);
	}
	else
	{
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c : Preparing to load certificate and key ..");

        	char *env_sslcertificateloc = getenv("SVR_SSLCERTIFICATELOC");	// export SVR_SSLCERTIFICATELOC /usr/local/ssl/certs
		if (env_sslcertificateloc != NULL)
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : retrieved environment variable <SVR_SSLCERTIFICATELOC : %s>", env_sslcertificateloc);
		else
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : cannot retrieve environment variable <SVR_SSLCERTIFICATELOC : %s>", env_sslcertificateloc);
                	UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : default to /usr/local/ssl/certs/Svrcert109.pem");
			env_sslcertificateloc = (char *)calloc(255, sizeof(char));
			if (env_sslcertificateloc != NULL)
	                	strcpy(env_sslcertificateloc, "/usr/local/ssl/certs/Svrcert109.pem");
			else
			{
				UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : cannot retrieve environment variable <SVR_SSLCERTIFICATELOC> :  out of memory");
				exit(UA_FALSE);
			}
        	}

		char *env_privatekeyloc = getenv("SVR_PRIVATEKEYLOC");		// export "SVR_PRIVATEKEYLOC" /usr/local/ssl/private
		if (env_privatekeyloc != NULL)
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : retrieved environment variable <SVR_PRIVATEKEYLOC : %s>", env_privatekeyloc);
		else
		{
			UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : cannot retrieve environment variable <SVR_PRIVATEKEYLOC : %s>", env_privatekeyloc);
			UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : default to /usr/local/ssl/private/Svrprivate-key109.pem");
			env_privatekeyloc = (char *)calloc(255, sizeof(char));
			if (env_privatekeyloc != NULL)
				strcpy(env_privatekeyloc, "/usr/local/ssl/private/Svrprivate-key109.pem");
			else
			{
				UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : cannot retrieve environment variable <SVR_PRIVATEKEYLOC> : out of memory");
				exit(UA_FALSE);
			}
		}

        	certificate = loadFile(env_sslcertificateloc);
        	privateKey = loadFile(env_privatekeyloc);

	        if (certificate.length == 0)
        	        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : cannot find SSL certificate %s", env_sslcertificateloc);
		else
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : Successfully loaded SSL certificate %s", env_sslcertificateloc);
        	if (privateKey.length == 0)
                	 UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : cannot find SSL private key %s", env_privatekeyloc);
		else
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : Successfully loaded SSL private key %s", env_privatekeyloc);

           	if ((certificate.length != 0) && (privateKey.length != 0))
           	{
        		int SVRport = atoi(env_SVRport);
                	status = UA_ServerConfig_setDefaultWithSecurityPolicies(config, SVRport,
                                                       &certificate, &privateKey,
                                                       trustList, trustListSize,
                                                       issuerList, issuerListSize,
                                                       revocationList, revocationListSize);

                if (status != UA_STATUSCODE_GOOD){
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c (1): Could not initiaise server with default security policies with error code %s",
                        UA_StatusCode_name(status));
                }
                else
		{
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c (1): Server initialised with default security policies with error code %s",
                        UA_StatusCode_name(status));
                }
	   }
	   else
	   {
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Encrypt.c : Error loading certificate/ private key");
		exit(0);
	   }
	}

	// this section is common whether it is generated or not.

        // add the security policies
        config->securityPoliciesSize = 0; // have to intialise but no need to increment after every addSecurityPolicyXXX

	if (ForceGenerateSelfSignedCert == UA_TRUE)
	{
        	status = UA_ServerConfig_addSecurityPolicyBasic256Sha256(config, &derCert, &derPrivKey);
	}
	else
	{
s		status = UA_ServerConfig_addSecurityPolicyBasic256Sha256(config, &certificate, &privateKey);
	}

        if(status != UA_STATUSCODE_GOOD)
	{
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c : Could not add SecurityPolicy#Basic256Sha256 with error code %s",
                        UA_StatusCode_name(status));
        }
        else
	{
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c : Server initialised with SecurityPolicy#Basic256Sha256");
        }

	if (ForceGenerateSelfSignedCert == UA_TRUE)
	{
	        status = UA_ServerConfig_addSecurityPolicyAes256Sha256RsaPss(config, &derCert, &derPrivKey);
	}
	else
	{
                status = UA_ServerConfig_addSecurityPolicyAes256Sha256RsaPss(config, &certificate, &privateKey);
	}

        if(status != UA_STATUSCODE_GOOD)
	{
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c : Could not add SecurityPolicy#Aes256Sha256RsaPss with error code %s",
                        UA_StatusCode_name(status));
        }
        else
	{
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c : Server initialised with SecurityPolicy#Aes256Sha256RsaPss");
        }

	if (ForceGenerateSelfSignedCert == UA_TRUE)
	{
		status = UA_ServerConfig_addSecurityPolicyAes128Sha256RsaOaep(config, &derCert, &derPrivKey);
	}
       	else
	{
		status = UA_ServerConfig_addSecurityPolicyAes128Sha256RsaOaep(config, &certificate, &privateKey);
	}

        if(status != UA_STATUSCODE_GOOD)
	{
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c : Could not add SecurityPolicy#Aes128Sha256RsaOaep with error code %s",
                        UA_StatusCode_name(status));
        }
        else
	{
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c : Server initialised with SecurityPolicy#Aes128Sha256RsaOaep");
        }

        /* Accept all certificates */
        config->secureChannelPKI.clear(&config->secureChannelPKI);
        UA_CertificateVerification_AcceptAll(&config->secureChannelPKI);

	config->sessionPKI.clear(&config->sessionPKI);
	UA_CertificateVerification_AcceptAll(&config->sessionPKI);

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Encrypt.c : Total number of SecurityPolicies added : %d", (int)config->securityPoliciesSize);

	//UA_ByteString_clear(&derCert);
	//UA_ByteString_clear(&derPrivKey);

        //UA_ByteString_clear(trustList);
        //UA_ByteString_clear(issuerList);
        //UA_ByteString_clear(revocationList);

        return status;
}

int generateSSCert(UA_Server *uaSvrServer, UA_ServerConfig *config,
                   UA_ByteString *trustList, size_t trustListSize,
                   UA_ByteString *issuerList, size_t issuerListSize,
                   UA_ByteString *revocationList, size_t revocationListSize,
                   UA_ByteString *derPrivKey, UA_ByteString *derCert)
{

        //UA_ByteString derPrivKey = UA_BYTESTRING_NULL;
        //UA_ByteString derCert = UA_BYTESTRING_NULL;


        UA_UInt32 lenSubject = 3;
        UA_String subject[3] = {UA_STRING_STATIC("C=SG"),
                                UA_STRING_STATIC("O=Virtual Skies"),
                                UA_STRING_STATIC("CN=svr.virtualskies.com.sg") };


	/*
	UA_UInt32 lenSubject = 7;
        UA_String subject[7] = {UA_STRING_STATIC("C=SG"),
                                UA_STRING_STATIC("S=Singapore"),
                                UA_STRING_STATIC("LO=Singapore"),
                                UA_STRING_STATIC("O=Virtual Skies"),
                                UA_STRING_STATIC("U=IT"),
                                UA_STRING_STATIC("CN=svr.virtualskies.com.sg"),
                                UA_STRING_STATIC("EM=jacky81100@yahoo.com") };
	*/

        UA_UInt32 lenSubjectAltName = 3;
        UA_String subjectAltName[3] = {UA_STRING_STATIC("URI=urn:svr.virtualskies.com.sg"),
				       UA_STRING_STATIC("IP=192.168.1.109"),
				       UA_STRING_STATIC("DNS=OPCSvr-109") };

        UA_KeyValueMap *kvm = UA_KeyValueMap_new();
        UA_UInt16 expiresIn = 3650;
        UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "expires-in-days"),
                                (void *)&expiresIn , &UA_TYPES[UA_TYPES_UINT16]);
        UA_UInt16 keyLength = 2048;
        UA_KeyValueMap_setScalar(kvm, UA_QUALIFIEDNAME(0, "key-size-bits"),
                                (void *)&keyLength, &UA_TYPES[UA_TYPES_UINT16]);

        // creates the certificate and keys
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : Generating self signed certificate and key");
        UA_StatusCode status = UA_CreateCertificate(
                                                UA_Log_Stdout, subject, lenSubject, subjectAltName, lenSubjectAltName,
                                                UA_CERTIFICATEFORMAT_DER, kvm, derPrivKey, derCert);
        UA_KeyValueMap_delete(kvm);
        UA_assert(status == UA_STATUSCODE_GOOD);
        UA_assert(derPrivKey->length > 0);
        UA_assert(derCert->length > 0);
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : Self signed certificate and key generated successfully");

        //UA_ServerConfig *config = UA_Server_getConfig(uaSvrServer);
        //if (!config)
        //     UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"SV_Encrypt.c : Error getting pointer to config");

        const char *env_SVRport = getenv("SVR_PORT");
        int SVRport = atoi(env_SVRport);

        UA_ServerConfig_setMinimal(config, SVRport, NULL);
        status = UA_ServerConfig_setDefaultWithSecurityPolicies(config, SVRport,
                                                        derCert, derPrivKey,
                                                        trustList, trustListSize,
                                                        issuerList, issuerListSize,
                                                        revocationList, revocationListSize);

        if ( status == UA_STATUSCODE_GOOD )
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : Default security policies are set successfully");
        else
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : Error setting security policies : %s", UA_StatusCode_name(status));

        // config->tcpReuseAddr = true;
        //UA_ByteString_clear(&derCert);
        //UA_ByteString_clear(&derPrivKey);

        return status;
}

