#ifdef almagamation
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
#include "SV_Encrypt.h"
#include "SV_Misc.h"

int encryptServer(UA_Server *uaSvrServer, UA_ServerConfig *config)
{
        UA_StatusCode status;

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : Entering encryptServer() function"); 
        //UA_ServerConfig *config = UA_Server_getConfig(uaSvrServer);
        const char *env_sslcertificateloc = getenv("SVR_SSLCERTIFICATELOC");	// export SVR_SSLCERTIFICATELOC /usr/local/ssl/certs
        UA_ByteString certificate = loadFile(env_sslcertificateloc);
        const char *env_privatekeyloc = getenv("SVR_PRIVATEKEYLOC");		// export "SVR_PRIVATEKEYLOC" /usr/local/ssl/private
        UA_ByteString privateKey = loadFile(env_privatekeyloc);
	const char *env_SVRport = getenv("SVR_PORT");

        UA_ServerConfig_setMinimal(config, atoi(env_SVRport), NULL);

        if (certificate.length == 0)
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : cannot find SSL certificate %s", env_sslcertificateloc);
	else
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : Successfully loaded SSL certificate %s", env_sslcertificateloc);
        if (privateKey.length == 0)
                 UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : cannot find SSL private key %s", env_privatekeyloc);
	else
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : Successfully loaded SSL private key %s", env_privatekeyloc);

       // Load trustlist
        //UA_ByteString *trustList = (UA_ByteString *)UA_Array_new(1, &UA_TYPES[UA_TYPES_BYTESTRING]);
        //UA_ByteString_copy(&certificate, &trustList[0]);
        UA_ByteString *trustList = NULL;
        size_t trustListSize = 0;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : Successfully loaded trustlist");
        // Loading of a issuer list, not used in this application
        UA_ByteString *issuerList = NULL;
        size_t issuerListSize = 0;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : issueList is not supported");
        // Loading of a revocation list currently unsupported
        UA_ByteString *revocationList = NULL;
        size_t revocationListSize = 0;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : revocationList is not supported");


        if ( (certificate.length != 0) && (privateKey.length != 0) )
        {
                status = UA_ServerConfig_setDefaultWithSecurityPolicies(config, 4840,
                                                       &certificate, &privateKey,
                                                       trustList, trustListSize,
                                                       issuerList, issuerListSize,
                                                       revocationList, revocationListSize);

                if (status != UA_STATUSCODE_GOOD){
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c (1): Could not initiaise server with default security policies with error code %s",
                        UA_StatusCode_name(status));
                }
                else{
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c (1): Server initialised with default security policies with error code %s",
                        UA_StatusCode_name(status));
                }

               // add the security policies
                status = UA_ServerConfig_addSecurityPolicyBasic256Sha256(config, &certificate, &privateKey);
                if(status != UA_STATUSCODE_GOOD) {
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c (1): Could not add SecurityPolicy#Basic256Sha256 with error code %s",
                        UA_StatusCode_name(status));
                }
                else{
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c (1): Server initialised with SecurityPolicy#Basic256Sha256");
                }

                status = UA_ServerConfig_addSecurityPolicyAes256Sha256RsaPss(config, &certificate, &privateKey);
                if(status != UA_STATUSCODE_GOOD) {
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c (1): Could not add SecurityPolicy#Aes256Sha256RsaPss with error code %s",
                        UA_StatusCode_name(status));
                }
                else{
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c (1): Server initialised with SecurityPolicy#Aes256Sha256RsaPss");
                }

                status = UA_ServerConfig_addSecurityPolicyAes128Sha256RsaOaep(config, &certificate, &privateKey);
                if(status != UA_STATUSCODE_GOOD) {
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c (1): Could not add SecurityPolicy#Aes128Sha256RsaOaep with error code %s",
                        UA_StatusCode_name(status));
                }
                else{
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c (1): Server initialised with SecurityPolicy#Aes128Sha256RsaOaep");
                }

                /* Accept all certificates */
                config->secureChannelPKI.clear(&config->secureChannelPKI);
                UA_CertificateVerification_AcceptAll(&config->secureChannelPKI);

                config->sessionPKI.clear(&config->sessionPKI);
                UA_CertificateVerification_AcceptAll(&config->sessionPKI);

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Encrypt.c (1): Total number of SecurityPolicies added : %d", config->securityPoliciesSize);

        }
        else
        // This section uses self-signed certificate generated using generateSSCert() function
        {
                UA_ByteString derPrivKey = UA_BYTESTRING_NULL;
                UA_ByteString derCert = UA_BYTESTRING_NULL;

                status = generateSSCert(uaSvrServer, config,
                                trustList, trustListSize,
                                issuerList, issuerListSize,
                                revocationList, revocationListSize,
                                &derPrivKey, &derCert);

                UA_assert(status == UA_STATUSCODE_GOOD);

                // add the security policies
                config->securityPoliciesSize = 0; // have to intialise but no need to increment after every addSecurityPolicyXXX

                status = UA_ServerConfig_addSecurityPolicyBasic256Sha256(config, &derCert, &derPrivKey);
                if(status != UA_STATUSCODE_GOOD) {
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c (2): Could not add SecurityPolicy#Basic256Sha256 with error code %s",
                        UA_StatusCode_name(status));
                }
                else{
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c (2): Server initialised with SecurityPolicy#Basic256Sha256");
                }


                status = UA_ServerConfig_addSecurityPolicyAes256Sha256RsaPss(config, &derCert, &derPrivKey);
                if(status != UA_STATUSCODE_GOOD) {
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c (2): Could not add SecurityPolicy#Aes256Sha256RsaPss with error code %s",
                        UA_StatusCode_name(status));
                }
                else{
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c (2): Server initialised with SecurityPolicy#Aes256Sha256RsaPss");
                }

                status = UA_ServerConfig_addSecurityPolicyAes128Sha256RsaOaep(config, &derCert, &derPrivKey);
                if(status != UA_STATUSCODE_GOOD) {
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c (2): Could not add SecurityPolicy#Aes128Sha256RsaOaep with error code %s",
                        UA_StatusCode_name(status));
                }
                else{
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Encrypt.c (2): Server initialised with SecurityPolicy#Aes128Sha256RsaOaep");
                }

                /* Accept all certificates */
               /* Accept all certificates */
                config->secureChannelPKI.clear(&config->secureChannelPKI);
                UA_CertificateVerification_AcceptAll(&config->secureChannelPKI);

                config->sessionPKI.clear(&config->sessionPKI);
                UA_CertificateVerification_AcceptAll(&config->sessionPKI);

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Encrypt.c (2): Total number of SecurityPolicies added : %d", config->securityPoliciesSize);

                //UA_ByteString_clear(&derCert);
                //UA_ByteString_clear(&derPrivKey);
        }

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
        UA_String subject[7] = {UA_STRING_STATIC("C=SG"),
                                UA_STRING_STATIC("S=Singapore"),
                                UA_STRING_STATIC("LO=Singapore"),
                                UA_STRING_STATIC("O=Virtual Skies"),
                                UA_STRING_STATIC("U=IT"),
                                UA_STRING_STATIC("CN=lds.virtualskies.com.sg"),
                                UA_STRING_STATIC("EM=jacky81100@yahoo.com") };
        */


        UA_UInt32 lenSubjectAltName = 1;
        UA_String subjectAltName[1] = {UA_STRING_STATIC("svr.virtualskies.com.sg") };

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
        UA_ServerConfig_setMinimal(config, 4840, NULL);
        status = UA_ServerConfig_setDefaultWithSecurityPolicies(config, 4840,
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

