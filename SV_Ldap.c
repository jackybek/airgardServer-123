#include "open62541.h"
#include <ldap.h>
#include <errno.h>
#include <slapi-plugin.h>

#ifdef ALORSETAR
  #define URL "ldap://192.168.0.174:389"
  #define BIND_DN "cn=admin,dc=bookworm,dc=virtualskies,dc=com"
  #define LDAP_BASE "dc=bookworm,dc=virtualskies,dc=com"
  //#define LDAP_FILTER "(cn=jackybek)"
#else
  #define URL "ldap://192.168.1.112:389"
  #define BIND_DN "cn=admin,dc=virtualskies,dc=com"
  #define LDAP_BASE "dc=virtualskies,dc=com"
  //#define LDAP_FILTER "(cn=jacky bek)"
#endif

int ConnectToLdap(char *, char *);
int ldap_get_errno(LDAP *);

int ldap_get_errno(LDAP *ld)
{
	int err;
	if (ldap_get_option(ld, LDAP_OPT_ERROR_NUMBER, &err) != LDAP_OPT_SUCCESS)
		err = LDAP_OTHER;
	return err;
}

int ConnectToLdap(char* userid, char* password)
{
	LDAP *myLdap;
	int error;
	LDAPMessage *search_result;
	int msgid=0, rc=0, parse_rc=0, finished=0;
	char *matched_msg=NULL, *error_msg=NULL;
	char **referrals;
	LDAPControl **serverctls;
	LDAPControl **clientctls;
	struct timeval timeout;
	struct berval cred;

#ifdef COMMENTS
https:www.openldap.org/software/man.cgi?query=ldap
1. ldap_initialize()
2. ldap_set_option(3) - no need
3a ldap_start_tls_s() i.e. Start TLS operation or
3b ldap_sasl_bind() i.e. Bind operation or
3c ldap_search_ext_s() i.e. search operation
#endif

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Ldap.c : Entering ConnectToLdap() ");
	char* env_ldap_url = getenv("SVR_LDAP_URL");
	char* env_ldap_bind_dn = getenv("SVR_LDAP_BIND_DN");
	char* env_ldap_base = getenv("SVR_LDAP_BASE");
	if (env_ldap_url != NULL)
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"-------SV_Ldap.c : retrieved environment variable <SVR_LDAP_URL> : %s", env_ldap_url);
	else
	{
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Ldap.c : cannot retrieve environment variable <SVR_LDAP_URL>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Ldap.c : default to <%s>", URL);
                env_ldap_url = (char*)calloc(255, sizeof(char));
                if (env_ldap_url != NULL)
                        strcpy(env_ldap_url, URL);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                "--------SV_Ldap.c : cannot retrieve environment variable <SVR_LDAP_URL> : out of memory");
                        return (UA_FALSE);
                }
	}

	if (env_ldap_bind_dn != NULL)
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "-------SV_Ldap.c : retrieved environment variable <SVR_LDAP_BIND_DN> : %s", env_ldap_bind_dn);
	else
	{
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Ldap.c : cannot retrieve environment variable <SVR_LDAP_BIND_DN>");
		#ifdef ALORSETAR
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Ldap.c : default to <cn=admin,dc=bookworm,dc=virtualskies,dc=com>");
		#else
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Ldap.c : default to <cn=admin,dc=virtualskies,dc=com>");
		#endif
                env_ldap_bind_dn = (char*)calloc(255, sizeof(char));
                if (env_ldap_bind_dn != NULL)
			#ifdef ALORSETAR
                        strcpy(env_ldap_bind_dn, "cn=admin,dc=bookworm,dc=virtualskies,dc=com");
			#else
			strcpy(env_ldap_bind_dn, "cn=admin,dc=virtualskies,dc=com");
			#endif
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                "--------SV_Ldap.c : cannot retrieve environment variable <SVR_LDAP_BIND_DN> : out of memory");
                        return (UA_FALSE);
                }
	}

	if (env_ldap_base != NULL)
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "-------SV_Ldap.c : retrieved environment variable <SVR_LDAP_BASE> : %s", env_ldap_base);
	else
	{
        	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_Ldap.c : cannot retrieve environment variable <SVR_LDAP_BASE>");
		#ifdef ALORSETAR
        	UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_Ldap.c : default to <dc=bookworm,dc=virtualskies,dc=com>");
		#else
        	UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_Ldap.c : default to <dc=virtualskies,dc=com>");
		#endif
        	env_ldap_base = (char*)calloc(255, sizeof(char));
        	if (env_ldap_base != NULL)
			#ifdef ALORSETAR
                	strcpy(env_ldap_base, "dc=bookworm,dc=virtualskies,dc=com");
			#else
                	strcpy(env_ldap_base, "dc=virtualskies,dc=com");
			#endif
        	else
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
				"--------SV_Ldap.c : cannot retrieve environment variable <SVR_LDAP_BASE> : out of memory");
                	return (UA_FALSE);
        	}
	}

	/* step 1 : Establish a ldap connection */
	error = ldap_initialize(&myLdap, env_ldap_url);	// url = host:port
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "==============================================");
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Ldap.c : initialising ldap : %d", error);
	if (error < 0)
	{
		// error
		UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			 "--------SV_Ldap.c : Unable to initialize ldap server : %s", ldap_err2string(error));
		return UA_FALSE;
	}
	else
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Ldap.c : ldap server %s is initialised", env_ldap_url);

	// step 2 - set ldap version
	ldap_set_option(myLdap, 3, NULL);

	// step 3a - Start TLS operation

	/* step 3b - bind to the ldap server (authenticate) */
	cred.bv_val = (char *) password;
	cred.bv_len = strlen(password);
	/* send a bind request to the ldap server */
	error = ldap_sasl_bind_s(myLdap, env_ldap_bind_dn, LDAP_SASL_SIMPLE,
					&cred, NULL, NULL, NULL); 	//ldap_simple_bind(myLdap, BIND_DN, BIND_PW);
	if (error < 0)
	{
		// error
		rc = ldap_get_errno(myLdap);
		UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Ldap.c : Unable to bind ldap server : %s", ldap_err2string(rc));
		//ldap_unbind_ext_s(myLdap, serverctls, clientctls);
		return UA_FALSE;
	}
	else
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_Ldap.c : Binding to ldap server is successful : %s", env_ldap_bind_dn);

	/* step 3c - perform a search */
	char ldap_filter[100];
	sprintf(ldap_filter, "(cn=%s)", userid);
	printf("ldap_filter is %s \n", ldap_filter);
	printf("env_ldap_base is %s \n", env_ldap_base);
	error = ldap_search_ext_s( myLdap,
				env_ldap_base,		// "dc=bookworm,dc=virtualskies,dc=com"
				LDAP_SCOPE_SUBTREE,
				ldap_filter,		// LDAP_FILTER "(cn=jackybek)"
				NULL, 			// attributes to retrieve (NULL for all)
				0,			// want both attributes descriptionsand attribute values
				NULL,			// serverctls
				NULL,			// clientctls
				&timeout,		// timeout
				0,			// sizelimit
				&search_result);
			#ifdef dontUSE
				LDAP_OPT_SIZELIMIT_SET,	// set a search limit
				LDAP_OPT_TIMELIMIT_SET, // set a search limit
				0, // size limit
				0, // time limit
				NULL, // control list
				&res);
			#endif

	printf("ldap_search_ext_s result is %d \n", error);
	if (error < 0)
	{
                // error
                rc = ldap_get_errno(myLdap);
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Ldap.c : Unable to search ldap server : %s", ldap_err2string(rc));
                ldap_unbind_ext_s(myLdap, serverctls, clientctls);
                return UA_FALSE;
	}
	else
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Ldap.c : search ldap success");

	/* step 4 - parse results
	4a. ldap_compare_ext_s() or ldap_compare_ext()
	4b - get the results - ldap_parse_results()
		4b1 - ldap_first_attribute() followed by ldap_get_values()
		4b2 - ldap_next_attribute() followed by ldap_get_values()
		4b3 - ldap_sort() : sort the entries and values returned bia the ldap search routines
	*/
	if (search_result!=NULL)
	{
	   LDAPMessage *first_entry, *next_entry;
	   BerElement *ber;
	   BerValue **berattr;
	   BerVarray *bervals;
	   char *dn, *first_attr, *next_attr;

	   int num_entries = ldap_count_entries(myLdap, search_result);
	   printf("Found <%d> entries: \n", num_entries);

	   if (num_entries > 0)
	   {

	   //--------------------- Process 'first' LDAP entry
		first_entry = ldap_first_entry(myLdap, search_result);
		dn = ldap_get_dn(myLdap, first_entry);
		if (dn != NULL)
		{
			struct berval **attrValues;
			printf("    DN: %s \n", dn);
			// print the attributes
			first_attr = ldap_first_attribute(myLdap, first_entry, &ber);
			if (first_attr != NULL)
			{
                        	printf("    First attribute %s\n", first_attr);
				attrValues = ldap_get_values_len(myLdap, first_entry, first_attr);
				if (attrValues)
                                {
                                        printf("           Attribute length %d and value %s \n", attrValues[0]->bv_len, attrValues[0]->bv_val);
                                }
			}
			else
			{
				printf("    cannot find first attribute");
				return UA_FALSE;
			}
			next_entry = first_entry;
                        next_attr = first_attr;
                        while (next_attr != NULL)
                        {
				struct ber;
				struct berval **attrValues;
                        	next_attr = ldap_next_attribute(myLdap, next_entry, ber);
				if (next_attr == NULL) break;
				printf("    NextAttribute %s \n", next_attr);
				attrValues = ldap_get_values_len(myLdap, next_entry, next_attr);
				if (attrValues)
				{
					printf("           Attribute length %d and value %s \n", attrValues[0]->bv_len, attrValues[0]->bv_val);
				}
                        }
                        //ldap_memfree(next_attr);
                        //ldap_memfree(first_attr);
			// ldap_memfree(dn)
		}

		printf("Finished process 1st LDAP entry and its attributes \n");

	#ifdef NOT_USED
	// Process 'Next' LDAP entry, if any that matches the same userid (cn=jackybek)
		num_entries--;
		next_entry = first_entry;
		for (int i=0; i< num_entries; i++)
		{
			next_entry = ldap_next_entry(myLdap, next_entry);
			if (next_entry != NULL)
			{
				printf("Entry %d \n", i+1);
				dn = ldap_get_dn(myLdap, next_entry);
				if (dn)
					printf("   DN: %s\n", dn);
				else
					ldap_memfree(dn);

				// print the attributes
				first_attr = ldap_first_attribute(myLdap, next_entry, &ber);
				printf("     First attribute %s\n", first_attr);
				next_attr = first_attr;
				while (next_attr != NULL)
				{
					next_attr = ldap_next_attribute(myLdap, next_entry, ber);
					printf("   NextAttribute %s\n", next_attr);
					//printf("   NextAttribute %s: %s\n", next_attr, ldap_get_values(next_attr));
				}
				//ldap_memfree(next_attr);
				//ldap_memfree(first_attr);
			}
			else
				printf("No results found\n");
		}
		//ldap_memfree(first_entry);
		//ldap_memfree(next_entry);
		//ldap_msgfree(search_result);
	   #endif

	   } // if (num_entries > 0)
	   else
		return UA_FALSE;
	} // if (search_result != NULL)

	printf("Before clean up \n");
	/* clean up */
	ldap_unbind_ext_s(myLdap, NULL, NULL); //serverctls, clientctls);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
        		"--------SV_Ldap.c : Unbind operation successful");
	return UA_TRUE;
}
