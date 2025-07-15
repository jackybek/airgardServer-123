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

#include <pthread.h>

//static UA_NodeId publishedDataSetIdentifierWS;


#ifdef COMMENT
https://libwebsockets.org/git/libwebsockets/tree/minimal-examples-lowlevel/ws-server/minimal-ws-server/minimal-ws-server.c
#endif

#define LWS_WITH_TLS 1
#define LWS_WITH_PLUGINS 1
//int createWebSocketsServer(UA_NodeId *, UA_NetworkAddressUrlDataType *, int, const char **);

        typedef struct webSocketStruct {
                UA_Server *uaServer;
		UA_String uaServerIP;
                UA_NodeId *pdsId;
                UA_NetworkAddressUrlDataType networkAddrUrl;
                int argC;
                char** argV;
        } wsThreadArgs;



void createWebSocketsServerHttps(wsThreadArgs *);
UA_Client *createOpcUaClientWss(UA_Server *uaServer, UA_String uaServerIP);

/*
 * lws-minimal-ws-server
 *
 * Written in 2010-2019 by Andy Green <andy@warmcat.com>
 *
 * This file is made available under the Creative Commons CC0 1.0
 * Universal Public Domain Dedication.
 *
 * This demonstrates the most minimal http server you can make with lws,
 * with an added websocket chat server using a ringbuffer.
 *
 * To keep it simple, it serves stuff in the subdirectory "./mount-origin" of
 * the directory it was started in.
 * You can change that by changing mount.origin.
 */

#include <libwebsockets.h>
#include <string.h>
#include <signal.h>

#define https

#ifdef https
#define LWS_PLUGIN_STATIC
#include "protocol_lws_minimal.c"

static struct lws_protocols protocols[] = {
        { "http", lws_callback_http_dummy, 0, 0, 0, NULL, 0},
        LWS_PLUGIN_PROTOCOL_MINIMAL,
        LWS_PROTOCOL_LIST_TERM
};

static const lws_retry_bo_t retry = {
        .secs_since_valid_ping = 3,
        .secs_since_valid_hangup = 10,
};

static int interrupted;

static const struct lws_http_mount mount = {
        /* .mount_next */		NULL,		/* linked-list "next" */
        /* .mountpoint */		"/",		/* mountpoint URL */
        /* .origin */			"./mount-origin",  /* serve from dir */
        /* .def */			"index1.html",	/* default filename */
        /* .protocol */			NULL, /* if you are using your own http protocol, to
                                               * serve the mount, give the protocol's name here */
        /* .cgienv */			NULL,
        /* .extra_mimetypes */		NULL,
        /* .interpret */		NULL,
        /* .cgi_timeout */		0,
        /* .cache_max_age */		0,
        /* .auth_mask */		0,
        /* .cache_reusable */		0,
        /* .cache_revalidate */		0,
        /* .cache_intermediaries */	0,
        /* .cache_no */			0,
        /* .origin_protocol */		LWSMPRO_FILE,	/* files in a dir */
        /* .mountpoint_len */		1,		/* char count */
        /* .basic_auth_login_file */	NULL,
        /* .cgi_chroot_path */		NULL,
        /* .cgi_wd */			NULL,
        /* .headers */			//NULL,
};

#if defined(LWS_WITH_PLUGINS)
/* if plugins enabled, only protocols explicitly named in pvo bind to vhost */
static struct lws_protocol_vhost_options pvo = { NULL, NULL, "lws-minimal", "" };
#endif

#endif //#ifdef https

void sigint_handler(int sig)
{
        interrupted = 1;
}

//--------------------------------------------------------------------------------------------------------------
void createWebSocketsServerHttps(wsThreadArgs *input)
{

	wsThreadArgs *arg = (wsThreadArgs *)input;

	UA_Client *uaClient;
	UA_StatusCode retval;

//#ifdef https
        struct lws_context_creation_info info;
        struct lws_context *context;
        const char *p;
        int n = 0, logs = LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE
                        /* for LLL_ verbosity above NOTICE to be built into lws,
                         * lws must have been configured and built with
                         * -DCMAKE_BUILD_TYPE=DEBUG instead of =RELEASE */
                        /* | LLL_INFO */ /* | LLL_PARSER */ /* | LLL_HEADER */
                        /* | LLL_EXT */ /* | LLL_CLIENT */ /* | LLL_LATENCY */
                        /* | LLL_DEBUG */;

        signal(SIGINT, sigint_handler);

	// create a UA Client object that connects to uaServer
        // UA Client will subscribe for monitoreditems
        uaClient = createOpcUaClientWss(arg->uaServer, arg->uaServerIP);
        //try to get test data from the server
        UA_Variant vSoftwareVersion;
        retval = UA_Client_readValueAttribute(uaClient, UA_NODEID_NUMERIC(1, 10001), &vSoftwareVersion);
        char *SoftwareVersion = (char *)vSoftwareVersion.data;
        printf("SoftwareVersion : %s ", SoftwareVersion);



printf("Here 0 : %s \n", (const char*)arg->argV[0]);
        if ((p = lws_cmdline_option(arg->argC, (const char**)arg->argV, "-d")))  //(argc1, argv1, "-d")))
	{
                logs = atoi(p);
		printf("Here 1.0 : logs = %d \n", logs);
	}
printf("Here 1.1 \n");
        lws_set_log_level(logs, NULL);
        lwsl_user("LWS minimal ws server | visit http://localhost:8443 (-s = use TLS / https)\n");

        memset(&info, 0, sizeof info); /* otherwise uninitialized garbage */
        info.port = 8443;
        info.mounts = &mount;
        info.protocols = protocols;
        info.vhost_name = "192.168.1.123";
printf("Here 2 \n");

#if defined(LWS_WITH_PLUGINS)
        info.pvo = &pvo;
#endif
        info.options =
                LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;
printf("Here 3 \n");
printf("=============================================================================\n");
printf("Inspecting the thread parameters passed into createWebSocketsServerHttps() : \n");
printf("=============================================================================\n");
printf("argC = %d \n", arg->argC);
for (int i=0; i< arg->argC; i++)
	printf("argV[%d] = %s \n", i, arg->argV[i]);				// ./lws-minimal-ws-server -s
printf("=============================================\n");
#if defined(LWS_WITH_TLS)
	printf("Here 3.1 : LWS_WITH_TLS \n");
        if (lws_cmdline_option(arg->argC, (const char**)arg->argV, "-s")) {	//(argc1, argv1, "-s")) {
		printf("Here 3.2 : lws_cmdline_option(-s)  \n");
                lwsl_user("Server using TLS\n");
                info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
                info.ssl_cert_filepath = "/usr/local/ssl/certs/Opccert123.pem"; //"localhost-100y.cert";
                info.ssl_private_key_filepath = "/usr/local/ssl/private/Opcprivate-key123.pem"; //"localhost-100y.key";
        }
#endif // #if defined(LWS_WITH_TLS)

printf("Here 4 \n");
        if (lws_cmdline_option(arg->argC, (const char**)arg->argV, "-h"))	//(argc1, argv1, "-h"))
                info.options |= LWS_SERVER_OPTION_VHOST_UPG_STRICT_HOST_CHECK;

        if (lws_cmdline_option(arg->argC, (const char**)arg->argV, "-v"))	//(argc1, argv1, "-v"))
                info.retry_and_idle_policy = &retry;

        context = lws_create_context(&info);
        if (!context) {
                lwsl_err("lws init failed\n");
                exit(-1);
        }

        while (n >= 0 && !interrupted)
                n = lws_service(context, 0);

        lws_context_destroy(context);
//#endif // #ifdef http

	exit(0);
}

#ifdef KIV_FIRST
/* OPCUA Server is acting as the Web Socket Client */
/* any wss client can request for data, just like mqtt client or uadp client */

    const char* env_wss_port = getenv("SVR_WSS_PORT");
    const char* env_wss_ip = getenv("SVR_WSS_IP");
    char URI_wss[100];

    if ( (env_wss_port == NULL) || (env_wss_ip == NULL) )
    {
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"==========================================================");
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_WebSockets.c : Error reading environment variables");
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_WebSockets.c : SVR_WSS_IP = %s", env_wss_ip);
        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_WebSockets.c : SVR_WSS_PORT = %s", env_wss_port);

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_WebSockets.c : Setting to default values");

	env_wss_ip = "192.168.1.123";
	env_wss_port = "7681";
    }


                // set the WSS_Enable global flag
  //              UA_Boolean WSS_Enable = UA_TRUE;

                //similar to sprintf(URI_wss, "opc.wss://%s:7681", wssbrokeraddress);

		sprintf(URI_wss, "opc.ws://%s:%s", env_wss_ip,env_wss_port);
/*
                strcpy(URI_wss, "opc.wss://");
                strcat(URI_wss, wssaddress);
                strcat(URI_wss, ":");
		strcat(URI_wss, env_wss_port);	// 7681
*/

                printf("URI_ws : <%s> \n", URI_wss);

	#ifdef UA_ENABLE_JSON_ENCODING
                UA_String transportProfile = UA_STRING(TRANSPORT_PROFILE_URI_WSSJSON);
	#else
		UA_String transportProfile = UA_STRING(TRANSPORT_PROFILE_URI_WSSBIN);
	#endif


                networkAddressUrl->url = UA_STRING(URI_wss);
		networkAddressUrl->networkInterface = UA_STRING("eth0"); // name of the interface defined in /etc/dhcpcd.conf

		// send avail publishedDataSetIdentifer to port opc.wss://192.168.1.165:7681
		//TODO


		return UA_STATUSCODE_GOOD;
}
#endif

