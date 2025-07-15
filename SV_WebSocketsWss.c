//--------------------------------------------------------------------------------------------------
// NOTE: This entire program is taken from github.com/warmcat/libwebsockets/blob/main/lwsws/main.c
// I only change main(int argc, char** argv) to wss(int argc, char **argv)
//--------------------------------------------------------------------------------------------------

#define PRODUCT_URI "http://virtualskies.com.sg"
#define APPLICATION_NAME "Airgard OPCUA-Wss client based on open62541"
#define APPLICATION_URI "urn:opc123.virtualskies.com.sg"

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

#include "SV_Misc.h"

typedef struct webSocketStruct {
                UA_Server *uaServer;
		UA_String uaServerIP;
                UA_NodeId *pdsId;
                UA_NetworkAddressUrlDataType networkAddrUrl;
                int argC;
                char** argV;
} wsThreadArgs;


/*
 * libwebsockets web server application
 *
 * Written in 2010-2020 by Andy Green <andy@warmcat.com>
 *
 * This file is made available under the Creative Commons CC0 1.0
 * Universal Public Domain Dedication.
 *
 * The person who associated a work with this deed has dedicated
 * the work to the public domain by waiving all of his or her rights
 * to the work worldwide under copyright law, including all related
 * and neighboring rights, to the extent allowed by law. You can copy,
 * modify, distribute and perform the work, even for commercial purposes,
 * all without asking permission.
 *
 * The test apps are intended to be adapted for use in your code, which
 * may be proprietary.	So unlike the library itself, they are licensed
 * Public Domain.
 */
#include "lws_config.h"

#include <stdio.h>
#include <stdlib.h>
#if defined(LWS_HAS_GETOPT_LONG) || defined(WIN32)
#include <getopt.h>
#endif
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#ifndef _WIN32
#include <dirent.h>
#include <syslog.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>
#else
#include <io.h>
#include "gettimeofday.h"
#include <uv.h>

UA_Client *createOpcUaClientWss(UA_Server *, UA_String);
int fork(void);

int fork(void)
{
	fprintf(stderr, "Sorry Windows doesn't support fork().\n");
	return 0;
}
#endif

#include <pthread.h>
#include <libwebsockets.h>
#include <uv.h>


#if defined(LWS_HAVE_MALLOC_TRIM)
#include <malloc.h>
#endif

static struct lws_context *context;
static lws_sorted_usec_list_t sul_lwsws;
static char config_dir[128], default_plugin_path = 1;
static int opts = 0, do_reload = 1;
static uv_loop_t loop;
static uv_signal_t signal_outer[2];
static int pids[32];
void lwsl_emit_stderr(int level, const char *line);

#define LWSWS_CONFIG_STRING_SIZE (64 * 1024)

static const struct lws_extension exts[] = {
#if !defined(LWS_WITHOUT_EXTENSIONS)
	{
		"permessage-deflate",
		lws_extension_callback_pm_deflate,
		"permessage-deflate"
	},
#endif
	{ NULL, NULL, NULL /* terminator */ }
};

#if defined(LWS_WITH_PLUGINS)
  #define INSTALL_DATADIR "/usr/local/share"

  static const char * const plugin_dirs[] = {
       INSTALL_DATADIR"/libwebsockets-test-server/plugins/",
	NULL};
#endif

#if defined(LWS_HAS_GETOPT_LONG) || defined(WIN32)
static struct option options[] = {
	{ "help",	no_argument,		NULL, 'h' },
	{ "debug",	required_argument,	NULL, 'd' },
	{ "configdir",  required_argument,	NULL, 'c' },
	{ "no-default-plugins",  no_argument,	NULL, 'n' },
	{ NULL, 0, 0, 0 }
};
#endif

void signal_cb(uv_signal_t *watcher, int signum)
{
	switch (watcher->signum) {
	case SIGTERM:
	case SIGINT:
		break;

	case SIGHUP:
		if (lws_context_is_deprecated(context))
			return;
		lwsl_notice("Dropping listen sockets\n");
		lws_context_deprecate(context, NULL);
		return;

	default:
		signal(SIGABRT, SIG_DFL);
		abort();
		break;
	}
	lwsl_err("Signal %d caught\n", watcher->signum);
	uv_signal_stop(watcher);
	uv_signal_stop(&signal_outer[1]);
	lws_context_destroy(context);
}

static void
lwsws_min(lws_sorted_usec_list_t *sul)
{
	lwsl_debug("%s\n", __func__);

#if defined(LWS_HAVE_MALLOC_TRIM)
	malloc_trim(4 * 1024);
#endif

	lws_sul_schedule(context, 0, &sul_lwsws, lwsws_min, 60 * LWS_US_PER_SEC);
}

static int
context_creation(void)
{
	int cs_len = LWSWS_CONFIG_STRING_SIZE - 1;
	struct lws_context_creation_info info;
	char *cs, *config_strings;
	void *foreign_loops[1];

	cs = config_strings = malloc(LWSWS_CONFIG_STRING_SIZE);
	if (!config_strings) {
		lwsl_err("Unable to allocate config strings heap\n");
		return -1;
	}

	memset(&info, 0, sizeof(info));

	info.external_baggage_free_on_destroy = config_strings;
	info.pt_serv_buf_size = 8192;
	info.options = (uint64_t)((uint64_t)opts | LWS_SERVER_OPTION_VALIDATE_UTF8 |
			      LWS_SERVER_OPTION_EXPLICIT_VHOSTS |
			      LWS_SERVER_OPTION_LIBUV);

#if defined(LWS_WITH_PLUGINS)
	if (default_plugin_path)
		info.plugin_dirs = plugin_dirs;
#endif
	lwsl_notice("Using config dir: \"%s\"\n", config_dir);

	/*
	 *  first go through the config for creating the outer context
	 */
	if (lwsws_get_config_globals(&info, config_dir, &cs, &cs_len))
		goto init_failed;

	foreign_loops[0] = &loop;
	info.foreign_loops = foreign_loops;
	info.pcontext = &context;

	context = lws_create_context(&info);
	if (context == NULL) {
		lwsl_err("libwebsocket init failed\n");
		goto init_failed;
	}

	/*
	 * then create the vhosts... protocols are entirely coming from
	 * plugins, so we leave it NULL
	 */

	info.extensions = exts;

	if (lwsws_get_config_vhosts(context, &info, config_dir, &cs, &cs_len))
		return 1;

	lws_sul_schedule(context, 0, &sul_lwsws, lwsws_min, 60 * LWS_US_PER_SEC);

	return 0;

init_failed:
	free(config_strings);

	return 1;
}


/*
 * root-level sighup handler
 */

static void
reload_handler(int signum)
{
#ifndef _WIN32
	int m;

	switch (signum) {

	case SIGHUP: /* reload */
		fprintf(stderr, "root process receives reload\n");
		if (!do_reload) {
			fprintf(stderr, "passing HUP to child processes\n");
			for (m = 0; m < (int)LWS_ARRAY_SIZE(pids); m++)
				if (pids[m])
					kill(pids[m], SIGHUP);
			sleep(1);
		}
		do_reload = 1;
		break;
	case SIGINT:
	case SIGTERM:
	case SIGKILL:
		fprintf(stderr, "parent process waiting 2s...\n");
		sleep(2); /* give children a chance to deal with the signal */
		fprintf(stderr, "killing service processes\n");
		for (m = 0; m < (int)LWS_ARRAY_SIZE(pids); m++)
			if (pids[m])
				kill(pids[m], SIGTERM);
		exit(0);
	}
#else
	// kill() implementation needed for WIN32
#endif
}


/* -------------------------------------------------------------------------------------------------- */
/* int wss(int argc, char **argv, UA_Client *uaClient, wsThreadArgs *argv)                                                */
/* This is the actual function that runs the websocket WSS loop                                       */
/* -------------------------------------------------------------------------------------------------- */

#ifdef FOR_REFERENCE
static const size_t usernamePasswordsSize = 2;
static UA_UsernamePasswordLogin logins[2] = {
       // {UA_STRING_STATIC("jackybek"), UA_STRING_STATIC("thisisatestpassword24")},
        {UA_STRING_STATIC("jackybek"),UA_STRING_STATIC("8ebe744e41fa3494536e9648093ab4f4ae156071eb54274c1dbf4c320c2023e0")},
       //{UA_STRING_STATIC("admin"), UA_STRING_STATIC("defaultadminpassword")}
        {UA_STRING_STATIC("admin"), UA_STRING_STATIC("e10351222ba08fce9d5867d676e8af9a5bef18807d40dea7f048eaf0a3f8a738")}
};
#endif

int wss(int argc, char **argv, UA_Client *uaClient, wsThreadArgs* arg)
{

#ifdef COMMENT
 here, an instance of uaClient has been created.
 It is not yet connected to the uaServer. So we can now connect using UA_Client_connectUsername()
 Use the uaClient to extract information from the uaServer using the monitoreditems callback
#endif

	UA_StatusCode retval;
	//UA_ClientConfig *clientConfig = UA_Client_getConfig(uaClient);
	char* login = "jackybek";
	char* password = "8ebe744e41fa3494536e9648093ab4f4ae156071eb54274c1dbf4c320c2023e0";

	// connect uaClient to uaServer
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
		"--------SV_WebSocketsWss.c : Entering wss() function and calling UA_Client_connectUsername()");

	char fullIP[255];
        sprintf(fullIP, "opc.tcp://192.168.1.123:4840");
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_WebSocketsWss.c : Attempt to call UA_Client_connectUsername() at line 321 : %s", fullIP);
	retval = UA_Client_connectUsername(uaClient, (const char*)fullIP, login, password);
        if (retval != UA_STATUSCODE_GOOD)
        {
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_WebSocketsWss.c : Cannot login securely to OPCUAServer using userid <%s>:<%s>, server IP= <%s>, errcode : %s",
			login, password, arg->uaServerIP.data, UA_StatusCode_name(retval) );
                //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Re-trying in 10 seconds");
		//sleep(10);
        }
        else
        {
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_WebSocketsWss.c : UA_Client_connectUsername() : success");
        }


	int n = 0, budget = 100, debug_level = 1024 + 7;
#ifndef _WIN32
	int m;
	int status;//, syslog_options = LOG_PID | LOG_PERROR;
#endif

	strcpy(config_dir, "/etc/lwsws");
	while (n >= 0) {
#if defined(LWS_HAS_GETOPT_LONG) || defined(WIN32)
		n = getopt_long(argc, argv, "hd:c:n", options, NULL);
#else
		n = getopt(argc, argv, "hd:c:n");
#endif
		if (n < 0)
			continue;
		switch (n) {
		case 'd':
			debug_level = atoi(optarg);
			break;
		case 'n':
			default_plugin_path = 0;
			break;
		case 'c':
			lws_strncpy(config_dir, optarg, sizeof(config_dir));
			break;
		case 'h':
			fprintf(stderr, "Usage: lwsws [-c <config dir>] "
					"[-d <log bitfield>] [--help] "
					"[-n]\n");
			exit(1);
		}
	}
#ifndef _WIN32
	/*
	 * We leave our original process up permanently, because that
	 * suits systemd.
	 *
	 * Otherwise we get into problems when reload spawns new processes and
	 * the original one dies randomly.
	 */

	signal(SIGHUP, reload_handler);
	signal(SIGINT, reload_handler);

	fprintf(stderr, "Root process is %u\n", (unsigned int)getpid());

	while (1) {
		if (do_reload) {
			do_reload = 0;
			n = fork();
			if (n == 0) /* new */
				break;
			/* old */
			if (n > 0)
				for (m = 0; m < (int)LWS_ARRAY_SIZE(pids); m++)
					if (!pids[m]) {
						pids[m] = n;
						break;
					}
		}
#ifndef _WIN32
		sleep(2);

		n = waitpid(-1, &status, WNOHANG);
		if (n > 0)
			for (m = 0; m < (int)LWS_ARRAY_SIZE(pids); m++)
				if (pids[m] == n) {
					pids[m] = 0;
					break;
				}
#else
// !!! implemenation needed
#endif
	}
#endif
	/* child process */

	lws_set_log_level(debug_level, lwsl_emit_stderr_notimestamp);

	lwsl_notice("lwsws libwebsockets web server - license CC0 + MIT\n");
	lwsl_notice("(C) Copyright 2010-2020 Andy Green <andy@warmcat.com>\n");

#if (UV_VERSION_MAJOR > 0) // Travis...
	uv_loop_init(&loop);
#else
	fprintf(stderr, "Your libuv is too old!\n");
	return 0;
#endif
	uv_signal_init(&loop, &signal_outer[0]);
	uv_signal_start(&signal_outer[0], signal_cb, SIGINT);
	uv_signal_init(&loop, &signal_outer[1]);
	uv_signal_start(&signal_outer[1], signal_cb, SIGHUP);

	if (context_creation()) {
		lwsl_err("Context creation failed\n");
		return 1;
	}

	lws_service(context, 0);

	lwsl_err("%s: closing\n", __func__);

	for (n = 0; n < 2; n++) {
		uv_signal_stop(&signal_outer[n]);
		uv_close((uv_handle_t *)&signal_outer[n], NULL);
	}

	/* cancel the per-minute sul */
	lws_sul_cancel(&sul_lwsws);

	lws_context_destroy(context);
	(void)budget;
#if (UV_VERSION_MAJOR > 0) // Travis...
	while ((n = uv_loop_close(&loop)) && --budget)
		uv_run(&loop, UV_RUN_ONCE);
#endif

	fprintf(stderr, "lwsws exited cleanly: %d\n", n);

#ifndef _WIN32
	closelog();
#endif

	context = NULL;

	return 0;
}



UA_Client *createOpcUaClientWss(UA_Server *uaServer, UA_String OPCUAServerIP)
{
#ifdef COMMAND
---------------------------------------------------------------------------------------
This is a common function to be called by WebSockets WSS and WebSockets HTTPS pthreads
---------------------------------------------------------------------------------------
#endif

	UA_Client *uaClient;
	UA_ClientConfig *config1;
//	UA_StatusCode retval;

	UA_ByteString certificate = UA_BYTESTRING_NULL;
	UA_ByteString privateKey = UA_BYTESTRING_NULL;
	char sslcert[255], sslprivatekey[255];
	struct dirent *de;      // pointer for directory entry

        UA_ByteString *trustList=NULL;
        size_t trustListSize = 0;
        int total_pem_files=0, total_der_files=0, counter=0;                    // count the number of files with .pem and .der extension
        UA_ByteString trustcertificate;
//        UA_ByteString *issuerList = NULL;
//        size_t issuerListSize = 0;
        UA_ByteString *revocationList = NULL;
        size_t revocationListSize = 0;

	strcpy(sslcert, "/etc/ssl/certs/");
	strcat(sslcert, "Opccert123.pem");
	strcpy(sslprivatekey, "/etc/ssl/private/");
	strcat(sslprivatekey, "Opcprivate-key123.pem");

	certificate = loadFile(sslcert);
	privateKey = loadFile(sslprivatekey);
	char* env_trustlistloc;

	if ((certificate.length == 0) || (privateKey.length == 0))
	{
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"SV_WebSocketsWss.c : cannot find SSL Certificate : %s", sslcert );
		exit(0);
	}
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_WebSocketsWss.c : Successfully loaded certificate <%s> and privatekey %s>", sslcert, sslprivatekey);

	// load the trustlist.  Load revocationlist is not supported now
	env_trustlistloc = getenv("SVR_TRUSTLISTLOC");
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_WebSocketsWss.c : trustlistloc.data = %s", env_trustlistloc);
	if (env_trustlistloc == NULL)
	{
		env_trustlistloc = (char*)malloc(255);
		strcpy(env_trustlistloc, "/usr/local/ssl/trustlist/");
	}

        DIR *dr = opendir((char *)env_trustlistloc);
        if (dr == NULL)
                 UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_WebSocketsWss.c : cannot find trustlist directory %s", env_trustlistloc);
        else
        {
                int retval;
                counter = 0;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,("--------SV_WebSocketsWss.c : Start while loop ") );

                while ((de = readdir(dr)) != NULL) // read the list of files in the directory
                {
                        #ifdef DEBUG
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_WebSocketsWss.c :  iterating through the trustlist directory : pass %d", count>
                        #endif
                        //char* file_name = de->d_name;
                        // process only the files with .pem extension
                        retval = stringEndsWith((char*)de->d_name, "pem");               // first count the total number .pem files
                        if (retval == 0) // file name ends with .pem
                        {
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_WebSocketsWss.c : Found PEM filename : <%s>", de->d_name);
                                total_pem_files++;
                        }
                        else
                        {

                                retval = stringEndsWith((char*)de->d_name, "der");               // also count the total number of .der files (e.g. UAEx>
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_WebSocketsWss.c : Found DER filename : <%s>", de->d_name);
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
                                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_WebSocketsWss.c : converted der filename is <%s>", derToPemfile);

                                        // run the command to convert .der to .pem
                                        // sudo openssl x509 -inform der -in uaexpert.der -out uaexpert.pem

                                        char sys_command[255];
                                        strcpy(sys_command, "sudo openssl x509 -inform der -in ");
                                        strcat(sys_command, env_trustlistloc);
                                        strcat(sys_command, de->d_name);
                                        strcat(sys_command, " -out ");
                                        strcat(sys_command, env_trustlistloc);
                                        strcat(sys_command, derToPemfile);
                                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_WebSocketsWss.c : command to execute <%s>", sys_command);
                                        //sprintf(sys_command, "sudo openssl x509 -inform der -in %s -out %s", de->d_name, derfile);
                                        system(sys_command);
                                        // alternative, look for a C function in openssl to do the conversion
                                }
                        }
                }
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_WebSocketsWss.c :  pass 1 iterating through the trustlist directory for .pem and .der  files : %d %d",
					total_pem_files, total_der_files);


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
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_WebSocketsWss.c : .pem certificate found is <%s>", fullpathname);

                                // loadFile needs the full path
                                trustcertificate = loadFile(fullpathname);
                                UA_ByteString_copy(&trustcertificate, &trustList[counter]);
                                counter++;
                                //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_WebSocketsWss.c : contents of the .pem certificate is <%s>", trustce>

                        }

                        #ifdef DER_FILES_ARE_NOT_USED_IN_OPEN62541
                        // get the file (.der)
                        retval = stringEndsWith((char*)de->d_name, ".der");
                        if (retval == 0) // file name ends with .der
                        {
                                strcpy(fullpathname, env_trustlistloc);
                                strcat(fullpathname, de->d_name);
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_WebSocketsWss.c : .der certificate found is <%s>", fullpathname);
                                // loadFile needs the full path
                                trustcertificate = loadFile((char*)fullpathname);
                                UA_ByteString_copy(&trustcertificate,&trustList[counter]);
                                counter++;
                                //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_WebSocketsWss.c : contents of the certificate is %s", trustcertific>
                        }
                        #endif
                        //skip to the next file
                }
                closedir(dr);
        }
        trustListSize = counter;
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_WebSocketsWss.c : Successfully loaded trustlist : no of trustlist loaded : %d", counter);

        #ifdef DEBUG
        for (counter=0; counter < trustListSize; counter++)
        {
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_WebSocketsWss.c :  Display the trustlist contents %d of %d", counter+1, trustListSize);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"%s", trustList[counter].data);
        }
        #endif
        // ------------------------------------------------------------------------- finish loading trustlist

        // Loading of a issuer list, not used in this application
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_WebSocketsWss.c : issuerList is not supported");
	//issuerList = NULL;
	//issuerListSize = 0;
        // Loading of a revocation list currently unsupported
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_WebSocketsWss.c : revocationList is not supported");
	revocationList = NULL;

        // new capability : Cert Rejected Directory
        // #define UA_ENABLE_CERT_REJECTED_DIR


	#ifdef COMMENT
	refer to open62541/tests/client/check_client_encryption.c
	#endif

	// ------------------------------------------------------------------------- create a UA_Client instance and connect to the uaServer
	uaClient = UA_Client_new();

	#ifdef COMMENT
	struct UA_Client is defined in open62541.c or /src/client/ua_client_internal.h
	#endif

	// -------first populate uaClient parameters

	//uaClient.housekeepingCallbackId = ???
	//uaClient.connectStatus = ???

	//uaClient.oldChannelState = ???
	//uaClient.oldSessionState = ???
	//uaClient.oldConnectStatus = ??

	//uaClient.findServersHandshake = ???
	//uaClient.endpointsHandshake = ???

	//uaClient->discoveryUrl = "opc.tcp://192.168.1.44:4840";		// error: invalid use of incomplete typedef ‘UA_Client’

        //uaClient->endpoint.endpointUrl = "opc.tcp://192.168.1.109:4840";	// error: invalid use of incomplete typedef ‘UA_Client’
        //uaClient->endpoint.server = "opc.tcp://192.168.1.109:4840";		// error: invalid use of incomplete typedef ‘UA_Client’
        //uaClient->endpoint.serverCerificate = ???
        //uaClient->endpoint.securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;	// error: invalid use of incomplete typedef ‘UA_Client’
											// UA_MESSAGESECURITYMODE_INVALID, UA_MESSAGESECURITYMODE_NONE,
											// UA_MESSAGESECURITYMODE_SIGN, UA_MESSAGESECURITYMODE_SIGNANDENCRYPT, __UA_MESSAGESECURITYMODE_FORCE32BIT
	#ifdef IMPORTANT_NOTICE
	//----error: invalid use of incomplete typedef ‘UA_Client’
	//---- without this being set, initConnect() will fail
	uaClient->endpoint.securityPolicyUri = UA_String_fromChars("http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep");
	#endif

        //uaClient->endpoint.UserIdentityTokenSize = ???
        //uaClient->endpoint.transportProfileUri = ???
        //uaClient->endpoint.securityLevel = ???

	//uaClient.channel = ???
	//uaClient.requestId = ???
	//uaClient.nextChannelRenewal = ???

	//uaClient.reverseConnectionCM = ??
	//uaClient.reverseConnectionIds = ???

	//uaClient.sessionState = ???
	//uaClient.authenticationToken = ???
	//uaClient.requestHandle = ???

	//uaClient.serverSessionNonce = ???
	//uaClient.clientSessionNonce = ???

	//uaClient.lastConnectivityCheck = ???
	//uaClient.pendingConnectivityCheck = ???

	//uaClient.asyncServiceCalls = ???

	//uaClient.monitoredItemHandles = ???
	//uaClient.currentlyOutStandingPublishRequests = ???

	//uaClient.clientMutex = ???

	// -----now populate <config> parameters::
	config1 = UA_Client_getConfig(uaClient);
	UA_ClientConfig_setDefault(config1);	// initialise with default values
	UA_ClientConfig_setDefaultEncryption(config1, certificate, privateKey,
                                         trustList, trustListSize,
                                         revocationList, revocationListSize);

	/* Accept all certificates */
	//UA_CertificateGroup_AcceptAll(&config1->certificateVerification);
	config1->securityPolicyUri = UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep");

	// try to connect

                //config1->stateCallback = stateCallback;
		config1->localConnectionConfig = UA_ConnectionConfig_default;
		//config1->context = ???;
                //config1->logging = ???;
		config1->timeout = 60000;   // no timeout, it is to resolve the runtime error : The connection has timed out before it could be fully opened

                UA_ApplicationDescription_clear(&config1->clientDescription);
                config1->clientDescription.applicationUri = UA_STRING_ALLOC(APPLICATION_URI);
                config1->clientDescription.productUri = UA_STRING_ALLOC(PRODUCT_URI);
                config1->clientDescription.applicationName = UA_LOCALIZEDTEXT_ALLOC("en", APPLICATION_NAME);
                config1->clientDescription.applicationType = UA_APPLICATIONTYPE_CLIENT;

    /**
     * Connection configuration
     * ~~~~~~~~~~~~~~~~~~~~~~~~
     */
                //config1->userIdentityToken = ???
		config1->securityMode = UA_MESSAGESECURITYMODE_SIGNANDENCRYPT;  // require encryption
		config1->securityPolicyUri = UA_STRING_ALLOC("http://opcfoundation.org/UA/SecurityPolicy#Aes128_Sha256_RsaOaep");
                config1->noSession = UA_FALSE;
                config1->noReconnect = UA_FALSE;
                config1->noNewSession = UA_FALSE;
                //config1->endpoint = ???
                //config1->userTokenPolicy.policyId = ???
                //config1->userTokenPolicy.tokenType = ???
                //config1->issuedTokenType = ???
                //config1->issuerEndpointUrl = ???
                //config1->securityPolicyUrl = ???
		config1->applicationUri = UA_STRING_ALLOC(APPLICATION_URI);

    /**
     * Advanced Client Configuration
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
		config1->secureChannelLifeTime = 10 * 60 * 1000;        // 10 minutes
                //config1->requestedSessionTimeout = ???
                //config1->localConnectionConfig = ???
                //config1->connectivityCheckInterval = ???

    /* EventLoop */
                //config1->eventLoop =???
                //config1->externalEventLoop=???

	#ifdef COMMENT
	: to be continued after resolving the error message:
	error/client       The connection has timed out before it could be fully opened
	#endif

	char fullIP[255];
	sprintf(fullIP, "opc.tcp://%s:4840", OPCUAServerIP.data);
	OPCUAServerIP = UA_STRING(fullIP);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_WebSocketsWss.c : Attempt to call UA_Client_connect() at line 775 : %s", OPCUAServerIP.data);
	int retval = UA_Client_connect(uaClient, (const char*)OPCUAServerIP.data);

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_WebSocketsWss.c : UA_Client_connect status is %s", UA_StatusCode_name(retval));

#ifdef CANNOT_SET
    /* EventLoop */
		//config1->eventLoop =???
		//config1->externalEventLoop=???

    /* Available SecurityPolicies */
    		config1->securityPoliciesSize = 4;	// Basic128Rsa15, Basic256, Basic256Sha256, Aes128Sha256RsaOaep
    		config1->securityPolicies = (UA_SecurityPolicy*)UA_malloc(sizeof(UA_SecurityPolicy));
    		if (!config1->securityPolicies)
		{
			UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_WebSocketsWss.c : Fail to allocate memory for securityPolicies");
        		exit(-1);
		}
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_WebSocketsWss.c : config->securityPolicies : success");

    /* Certificate Verification Plugin */
		//config1->certificateVerification = ???
		//config1->authSecurityPoliciesSize = ???
		//config1->authSecurityPolicies = ???
		//config1->authSecurityPolicyUri = ???


    		//config1->initConnectionFunc = UA_ClientConnectionTCP_init; // for async client
    		//UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,config1->pollConnectionFunc = UA_ClientConnectionTCP_poll; // for async connection
    		config1->customDataTypes = NULL;

		// Set stateCallback : https://raw.githubusercontent.com/open62541/open62541/master/examples/client_method_async.c
		// static void stateCallback(UA_Client *client, UA_SecureChannelState channelState, UA_SessionState sessionState, UA_StatusCode connectStatus)
		//config1->timeout = 60000;
    		//config1->stateCallback = stateCallback;	// function implemented in AG_NewMethod.c

		config1->connectivityCheckInterval = 0;
    		config1->requestedSessionTimeout = 1200000; // requestedSessionTimeout
    		config1->inactivityCallback = NULL;
    		config1->clientContext = NULL;
		config1->connectivityCheckInterval = 3000; // in milliseconds

		#ifdef UA_ENABLE_SUBSCRIPTIONS
			config1->outStandingPublishRequests = 10;
    			config1->subscriptionInactivityCallback = NULL;
		#endif

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_WebSocketsWss.c : Calling setDefaultEncryption");
                UA_ClientConfig_setDefaultEncryption(config1, certificate, privateKey,
                                                        trustList, trustListSize,
                                                        revocationList, revocationListSize);
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_WebSocketsWss.c : Finished calling setDefaultEncryption");
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_WebSocketsWss.c : createOpcUaClientWss() : successfully setDefaultEncryption");

                UA_ByteString_clear(&certificate);
                UA_ByteString_clear(&privateKey);
                for(size_t deleteCount = 0; deleteCount < trustListSize; deleteCount++)
                        UA_ByteString_clear(&trustList[deleteCount]);

		#ifdef NO_NEED_TO_CONNECT
		--- do it in wss() - see above

		retval = UA_STATUSCODE_GOOD;
		printf("OPCUAServerIP is %s \n", OPCUAServerIP.data);

		here we only create the UA_Client instance within the pthread.  Reason the UA_Server instance has not been created yet.
		we can only call the connectUsername function in the main server thread, passing the client obj into the callback to connect and retrieve data after the main thread is running.
		Using a callback function of the main server thread ensures that the call to connectUsername(server) happens after the main server is up and running.
		while (1)
		{
			retval = UA_Client_connectUsername(uaClient, OPCUAServerIP.data, "admin", "defaultadminpassword24");
			if (retval != UA_STATUSCODE_GOOD)
			{
                        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
					"--------SV_WebSocketsWss.c : Cannot login securely to OPCUAServer : %s, errcode : %s",
							OPCUAServerIP, UA_StatusCode_name(retval) );
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
					"--------SV_WebSocketsWss.c : Re-trying in 3 seconds");
				sleep(3);

               		}
			else
			{
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SSV_WebSocketsWss.c : UA_Client_connectUsername() : success");
				break;
			}
		}
		#endif	// ifdef NO_NEED_TO_CONNECT
#endif // #ifdef WAIT

		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SSV_WebSocketsWss.c : Leaving createOpcUaClientWss()");
		return uaClient;
}

void createWebSocketsServerWss(void *input)
{
        wsThreadArgs *arg = (wsThreadArgs *)input;

        char*argumentV[] = {"./lwsws", "/etc/lwsws", "-d"};	// "-d 1151" cannot have flag "-c" otherwise default plugins will not be loaded
        int argumentC = sizeof(argumentV) / sizeof(char*);
	UA_Client *uaClient;
	//UA_StatusCode retval;


	// create a UA Client object that connects to uaServer
	// UA Client will subscribe for monitoreditems
	//
	printf("arg->uaServerIP is %s \n", arg->uaServerIP.data);
	uaClient = createOpcUaClientWss(arg->uaServer, arg->uaServerIP);


	//try to get test data from the server
	//UA_Variant vSoftwareVersion;
	//retval = UA_Client_readValueAttribute(uaClient, UA_NODEID_NUMERIC(1, 10001), &vSoftwareVersion);
	//char *SoftwareVersion = (char *)vSoftwareVersion.data;
	//printf("SoftwareVersion : %s , retval is %s \n", SoftwareVersion, UA_StatusCode_name(retval));
	//printf("pausing here for 15 seconds to display the nodes using UA_Client_readValueAttribute() \n");
	//sleep(15);

	// calls the websocket WSS loop
        wss(argumentC, argumentV, uaClient, arg);

        exit(0);
}
