/* sample taken from github.com/FreeRadius/freeradius-client/blob/master/src/radexample.c */

#include "open62541.h"
#include <freeradius-client.h>
#include <includes.h>
#include <pathnames.h>
#define RC_CONFIG_FILE "/usr/local/etc/radiusclient/radiusclient.conf" //"/usr/local/src/freeradius-client/etc/radiusclient.conf"

#ifdef ALOR_SETAR
  #define URL "192.168.0.174:1812"
  #define USERNAME ""
  #define PASSWORD ""
  #define SECRET "mysecret"
#else
  #define URL "192.168.1.33:1812"
  #define NAS_PORT "1812"
  #define USERNAME1 "sybek1708"
  #define USERNAME2 "jackybek"
  #define USERNAME3 "myUser"
  #define PASSWORD_1_2 "ThisIsAnotherLongPassword_88"
  #define PASSWORD_3 "ThisIsALongPassword_24"
  #define SECRET "bookwormsecret"
#endif
#ifdef COMMENT

Using the smart switch, enter userid:password and it can connect via Radius server
sybek1708:ThisIsAnotherLongPassword_88
jackybek:ThisisAnotherLongPassword_88

However, if using radius-client.c, error ocurrs.
#endif

static char rcsid[] = "$Id: radexample.c,v 1.8 2007/07/11 17:29:30 cparker Exp $";
static char *pname = NULL;

int ConnectToRadius(char*, char*);

int ConnectToRadius(char* userid, char* hashpassword)
{
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Radius.c : Entering ConnectToRadius()");
	char* env_radius_url = getenv("SVR_RADIUS_URL");
	char* env_radius_secret = getenv("SVR_RADIUS_SECRET");
	char* env_radius_username = getenv("SVR_RADIUS_USERNAME");
	char* env_radius_password = getenv("SVR_RADIUS_PASSWORD");

	if (env_radius_url != NULL)
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"-------SV_Radius.c : retrieved environment variable <SVR_RADIUS_URL> : %s", env_radius_url);
	else
	{
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Radius.c : cannot retrieve environment variable <SVR_RADIUS_URL>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Radius.c : default to <%s>", URL);
                env_radius_url = (char*)calloc(255, sizeof(char));
                if (env_radius_url != NULL)
                        strcpy(env_radius_url, URL); //"192.168.0.174:389:1812");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                "--------SV_Radius.c : cannot retrieve environment variable <SVR_RADIUS_URL> : out of memory");
                        return (UA_FALSE);
                }
	}


        if (env_radius_secret != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "-------SV_Radius.c : retrieved environment variable <SVR_RADIUS_SECRET> : %s", env_radius_secret);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Radius.c : cannot retrieve environment variable <SVR_RADIUS_SECRET>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Radius.c : default to <%s>", SECRET);
                env_radius_secret = (char*)calloc(255, sizeof(char));
                if (env_radius_secret != NULL)
                        strcpy(env_radius_secret, SECRET);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                "--------SV_Radius.c : cannot retrieve environment variable <SVR_RADIUS_SECRET> : out of memory");
                        return (UA_FALSE);
                }
        }

	if (env_radius_username != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "-------SV_Radius.c : retrieved environment variable <SVR_RADIUS_USERNAME> : %s", env_radius_username);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Radius.c : cannot retrieve environment variable <SVR_RADIUS_USERNAME>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Radius.c : default to %s", USERNAME1);
                env_radius_username = (char*)calloc(255, sizeof(char));
                if (env_radius_username != NULL)
			#ifdef ALOR_SETAR
                        strcpy(env_radius_username, "jackybek");
			#else
			strcpy(env_radius_username, USERNAME1);
			#endif
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                "--------SV_Radius.c : cannot retrieve environment variable <SVR_RADIUS_USERNAME> : out of memory");
                        return (UA_FALSE);
                }
        }

        if (env_radius_password != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "-------SV_Radius.c : retrieved environment variable <SVR_RADIUS_PASSWORD> : %s", env_radius_password);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Radius.c : cannot retrieve environment variable <SVR_RADIUS_PASSWORD>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Radius.c : default to %s", PASSWORD_1_2);
                env_radius_password = (char*)calloc(255, sizeof(char));
                if (env_radius_password != NULL)
			#ifdef ALOR_SETAR
                        strcpy(env_radius_password, "ThisIsALongPassword_24");
			#else
			strcpy(env_radius_password, PASSWORD_1_2);
			#endif
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                "--------SV_Radius.c : cannot retrieve environment variable <SVR_RADIUS_PASSWORD> : out of memory");
                        return (UA_FALSE);
                }
        }

/*
 * https://github.com/FreeRadius/freeradius-client/blob/master/src/radexample.c
 *
 * $Id: radexample.c,v 1.8 2007/07/11 17:29:30 cparker Exp $
 *
 * Copyright (C) 1995,1996,1997 Lars Fenneberg
 *
 * See the file COPYRIGHT for the respective terms and conditions.
 * If the file is missing contact me at lf@elemental.net
 * and I'll send you a copy.
 *
 */

	int             result;
	char		username[128];
	char            passwd[AUTH_PASS_LEN + 1];
	VALUE_PAIR 	*send, *received;
	uint32_t	service;
	char 		msg[PW_MAX_MSG_SIZE], username_realm[256];
	char		*default_realm;
	rc_handle	*rh;
	//char		**argv;

	//strcpy(argv[0], "a.out jackybek molekhaven /");
	//pname = (pname = strrchr(argv[0],'/'))?pname+1:argv[0];	/* search for the last ocurrence of '/' */
	pname = (char*) malloc(100);
	rc_openlog(pname);

	if ((rh = rc_read_config(RC_CONFIG_FILE)) == NULL)		/* all the settings to intialise radius client is in */
		return (UA_FALSE); //ERROR_RC;				/* </usr/local/etc/radiusclient/radiusclient.conf> */
							/* update the parameter :
							authserver 192.168.1.33:1812
							authserver 192.168.1.109:1812
							authserver 192.168.1.155:1812

							acctserver      192.168.1.33:1813
							acctserver      192.168.1.109:1813
							acctserver      192.168.1.155:1813
							*/
							/* secrets are defined in /usr/local/etc/radiusclient/servers : */
							/* update the parameter :
							192.168.1.33 	bookwormsecret
							192.168.1.109	bookwormsecret
							192.168.1.155	bookwormsecret
							*/

	if (rc_read_dictionary(rh, rc_conf_str(rh, "dictionary")) != 0)
		return UA_FALSE; //ERROR_RC;

	default_realm = rc_conf_str(rh, "default_realm");
	printf("default_realm = %s \n", default_realm);
	/* default_realm is empty as defined in /usr/local/etc/radiusclient.conf */
	//strncpy(username, env_radius_username, sizeof(username));
	strncpy(username, rc_getstr (rh, "login (sybek1708): ",1), sizeof(username));
	//strncpy(passwd, env_radius_password, sizeof(passwd));
	strncpy (passwd, rc_getstr(rh, "Password (ThisIsAnotherLongPassword_88): ",1), sizeof (passwd));

	send = NULL;

	/*
	 * Fill in User-Name
	 */
								/* the list of approved users are stored in radius server */
								/* refer to <URL>/etc/freeradius/3.0/users for the complete list */
								/* the prompted user name and password must match this list */
	strncpy(username_realm, username, sizeof(username_realm));
	printf("Before : username_realm = %s \n", username_realm);

	/* Append default realm */
	if ((strchr(username_realm, '@') == NULL) && default_realm &&
	    (*default_realm != '\0'))
	{
		strncat(username_realm, "@", sizeof(username_realm)-strlen(username_realm)-1);
		strncat(username_realm, default_realm, sizeof(username_realm)-strlen(username_realm)-1);
	}
	printf("After: username_realm = %s \n", username_realm);

	if (rc_avpair_add(rh, &send, PW_USER_NAME, username_realm, -1, 0) == NULL)
		return UA_FALSE; //ERROR_RC;

	/*
	 * Fill in User-Password
	 */
	if (rc_avpair_add(rh, &send, PW_USER_PASSWORD, passwd, -1, 0) == NULL)
		return UA_FALSE; //ERROR_RC;
	if (rc_avpair_add(rh, &send, PW_NAS_IP_ADDRESS, URL, -1, 0) == NULL)
		return UA_FALSE;
	uint32_t client_port = atoi(NAS_PORT);
	if (rc_avpair_add(rh, &send, PW_NAS_PORT, NAS_PORT, -1, 0) == NULL)
		return UA_FALSE;
	/*
	 * Fill in Service-Type
	 */
	service = PW_AUTHENTICATE_ONLY; //PW_AUTHENTICATE_ONLY, PW_ADMINISTRATIVE;
	if (rc_avpair_add(rh, &send, PW_SERVICE_TYPE, &service, -1, 0) == NULL)
		return UA_FALSE; //ERROR_RC;

	/*
	 * Send the authentication request to freeRadius server
	 */
	result = rc_auth(rh, client_port, send, &received, msg);
	// rc_auth() function is defined in /usr/local/src/freeradius-client/lib/buildreq.c

	if (result == OK_RC)
	{
		fprintf(stderr, "\"%s\" RADIUS Authentication OK\n", username);
		return (UA_TRUE);
	}
	else
	{
		#ifdef COMMENT
		declared in /usr/local/src/freeradius-client/include
		#define BADRESP_RC      -2
		#define ERROR_RC        -1
		#define OK_RC           0
		#define TIMEOUT_RC      1
		#define REJECT_RC       2
		#endif
		fprintf(stderr, "\"%s\" RADIUS Authentication failure (RC=%i)\n", username, result);
		return (UA_FALSE);
	}
}








#ifdef CANNOT_USED
	// initialise the client
	struct radius_client *client = fr_radius_init(env_radius_url, strlen(env_radius_url)+1);
	if (!client)
	{
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
				"--------SV_Radius.c : cannot initialise freeRadius client");
		return (UA_FALSE);
	}

printf("username : %s \n", env_radius_username);
printf("password : %s \n", env_radius_password);
printf("secret   : %s \n", env_radius_secret);

	// set the shared secret
	radius_set_secret(client, env_radius_secret);

	// create an access request packet
	struct radius_packet *request = radius_create_packet(RADIUS_ACCESS_REQUEST, 0);
	if (!request)
	{
		radius_destroy_client(client);
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                "--------SV_Radius.c : cannot create freeRadius client request");
                return (UA_FALSE);
	}

	// add user-name attribute
	radius_add_attr(request, env_radius_username);
	radius_add_attr(request, env_radius_password);
	// add secret attribute
	radius_add_attr(request, env_radius_secret);

	// send the reqesst and get the response
	struct radius_packet *response = radius_send_packet(client, request);
	if (response == NULL)
	{
                radius_destroy_client(client);
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                "--------SV_Radius.c : fail to get freeRadius response");
                return (UA_FALSE);
        }

	switch(response->code)
	{
		case RADIUS_ACCESS_ACCEPT:
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
				"--------SV_Radius.c : Authentication with RadiusServer is accepted");
			break;
		case RADIUS_ACCESS_REJECT:
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                "--------SV_Radius.c : Authentication with RadiusServer is rejected");
			break;
		case RADIUS_ACCESS_CHALLENGE:
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                "--------SV_Radius.c : Authentication clallenged by RadiusServer");
			// respond to the challenge

			break;
	}
	radius_destroy_packet(response);
	radius_destroy_client(client);

	return UA_TRUE;
}
#endif
