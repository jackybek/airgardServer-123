#ifdef almagamation
  #include <open62541/websockets.h>
#else
  #include "open62541.h"
#endif



void createWebSocketsServerWss(void *);
void createWebSocketsServerHttps(void *);
void initialiseWebSocketsWssHttps(UA_Server *);

void initialiseWebSocketsWssHttps(UA_Server *uaServer)
{
        #ifdef COMMENT
                Here we are creating a thread to launch websockets server using pthread
                It is an alternate implementation to open62541 websockets implementation
        #endif

        pthread_t webSocketServerthreadHttps, webSocketServerthreadWss;
        //int createWebSocketsServer(UA_NodeId *publishedDataSetidentifier, UA_NetworkAddressUrlDataType *networkAddressUrl, int, char**);
        typedef struct webSocketStruct {
                UA_Server *uaServer;
                UA_String uaServerIP;
                UA_NodeId *pdsId;
                UA_NetworkAddressUrlDataType networkAddrUrl;
                int argC;
                char** argV;
        } wsThreadArgs;

        #ifdef COMMENT
                "Usage: lwsws [-c <config dir>] "
                "[-d <log bitfield>] [--help] "
                "[-n]");
        #endif

        wsThreadArgs webSocketArgs1, webSocketArgs2, webSocketArgs3, webSocketArgs4;
        // websocket HTTPS
        webSocketArgs1.uaServer = uaServer;
        //webSocketArgs1.pdsId = &publishedDataSetIdentifier;
        //webSocketArgs1.networkAddrUrl = networkAddressUrl;

	char *env_SVR_WSS_IP = getenv("SVR_WSS_IP");
	char env_wss_ip[255];
	if (env_SVR_WSS_IP == NULL)
		strcpy(env_wss_ip, "192.168.1.123");
	else
		strcpy(env_wss_ip, env_SVR_WSS_IP);

        webSocketArgs1.uaServerIP = UA_STRING(env_wss_ip);
        // websocket WSS
        webSocketArgs2.uaServer = uaServer;
        //webSocketArgs2.pdsId = &publishedDataSetIdentifier;
        //webSocketArgs2.networkAddrUrl = networkAddressUrl;
        webSocketArgs2.uaServerIP = UA_STRING(env_wss_ip);
        // websocket HTTPS & WSS
        webSocketArgs3.uaServer = uaServer;
        //webSocketArgs3.pdsId = &publishedDataSetIdentifier;
        //webSocketArgs3.networkAddrUrl = networkAddressUrl;
        webSocketArgs3.uaServerIP = UA_STRING(env_wss_ip);
        webSocketArgs4.uaServer = uaServer;
        //webSocketArgs4.pdsId = &publishedDataSetIdentifier;
        //webSocketArgs4.networkAddrUrl = networkAddressUrl;
        webSocketArgs4.uaServerIP = UA_STRING(env_wss_ip);

printf("inspecting the structure elements \n");
printf("webSocketArgs1.uaServerIP = %s \n", webSocketArgs1.uaServerIP.data);
printf("webSocketArgs2.uaServerIP = %s \n", webSocketArgs2.uaServerIP.data);
printf("webSocketArgs3.uaServerIP = %s \n", webSocketArgs3.uaServerIP.data);
printf("webSocketArgs4.uaServerIP = %s \n", webSocketArgs4.uaServerIP.data);


        bool result, result1, result2;
        char* env_WebSocketsServerType = getenv("SVR_WEBSOCKETS_SERVERTYPE");

        if (env_WebSocketsServerType != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_WebSocketsInitialise.c : retrieved environment variable <SVR_WEBSOCKETS_SERVERTYPE> : %s", env_WebSocketsServerType);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_WebSocketsInitialise.c : cannot retrieve environment variable <SVR_WEBSOCKETS_SERVERTYPE>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_WebSocketsInitialise.c : default to HTTPS");
                env_WebSocketsServerType = (char*)calloc(255, sizeof(char));
                if (env_WebSocketsServerType != NULL)
                        strcpy(env_WebSocketsServerType, "HTTPS");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_WebSocketsInitialise.c : cannot retrieve environment variable <SVR_WEBSOCKETS_SERVERTYPE> : out of memory");
                        exit(UA_FALSE);
                }
        }

        int response;
        while (1)
        {
	        printf("Choose the type of server instance to create \n");
        	printf("1 : Http Secure instance (https)\n");
	        printf("2 : Web Sockets Secure (wss) \n");
        	printf("3 : Both https and wss intances \n");
	        printf("> "); scanf("%d", &response);

        	switch (response)
        	{
                case 1 : // Websocket HTTPS
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
				"--------SV_WebSocketsInitialise.c : creating thread for WebSocketsServer Https");
                        char *argumentV1[] = {"./lws-minimal-ws-server", "-s"};         // -s means "serve using TLS selfsigned cert
                        int argumentC1 = sizeof(argumentV1) / sizeof(char*);
                        webSocketArgs1.argC = argumentC1;
                        webSocketArgs1.argV = argumentV1;
                        result = pthread_create(&webSocketServerthreadHttps, NULL, (void *)createWebSocketsServerHttps, &webSocketArgs1);
                        break;

                case 2 : // Websocket WSS
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                "--------SV_WebSocketsInitialise.c : creating thread for WebSocketsServer Wss");
                        char *argumentV2[] = {"./lwsws", "/etc/lwsws/conf.d", "-d", "1151"};             // -s "serve using TLS selfsigned cert, -d debug
                        int argumentC2 = sizeof(argumentV2) / sizeof(char*);
                        webSocketArgs2.argC = argumentC2;
                        webSocketArgs2.argV = argumentV2;
                        result = pthread_create(&webSocketServerthreadWss, NULL, (void *)createWebSocketsServerWss, &webSocketArgs2);
			break;

                case 3 : // Websocket HTTPS
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                "--------SV_WebSocketsInitialise.c : creating threads for WebSocketsServer Https and Wss");
                        char *argumentV3[] = {"./lws-minimal-ws-server", "-s"};                // -s means "serve using TLS selfsigned cert
                        int argumentC3 = sizeof(argumentV3) / sizeof(char*);
                        webSocketArgs3.argC = argumentC3;
                        webSocketArgs3.argV = argumentV3;
                        result1 = pthread_create(&webSocketServerthreadHttps, NULL, (void *)createWebSocketsServerHttps, &webSocketArgs3);
                        // Websocket WSS
                        char *argumentV4[] = {"./lwsws", "/etc/lwsws/conf.d", "-d", "1151"};             // -s "serve using TLS selfsigned cert, -d debug
                        int argumentC4 = sizeof(argumentV4) / sizeof(char*);
                        webSocketArgs4.argC = argumentC4;
                        webSocketArgs4.argV = argumentV4;
                        result2 = pthread_create(&webSocketServerthreadWss, NULL, (void *)createWebSocketsServerWss, &webSocketArgs4);

                        result = result1 | result2;
                        break;

                default : printf("Unknown response.. please try again \n"); continue;
        	}
        } // while (1)

        //result = pthread_create(&webSocketServerthread, NULL, (void *)createWebSocketsOption, &webSocketArgs);        // argC is corrupted
        if (result != 0)
        {
                printf("-----------------------------------------------------------------------------------------------------------------\n");
                printf("------SV_WebSocketsInitialise.c : Fail to create websocket server thread as a separate process             \n");
                printf("------SV_WebSocketsInitialise.c : Websockets server functions will not be available until the next restart \n");
                printf("-----------------------------------------------------------------------------------------------------------------\n");
        }
        else
        {
           switch (response)
           {
             case 1 : //
                result = pthread_detach(webSocketServerthreadHttps);
                if (result != 0)
                {
                        printf("-----SV_WebSocketsInitialise.c : cannot detach Websockets server HTTPS thread from main thread \n");
                        exit(0);
                }
                else
                {
                        printf("---------------------------------------------------------------------------------------------------------\n");
                        printf("------SV_WebSocketsInitialise.c : websocket server HTTPS thread is created successfully                  \n");
                        printf("------SV_WebSocketsInitialise.c : starting Websockets server HTTPS thread and waiting for request  \n");
                        printf("------SV_WebSocketsInitialise.c : data will be made available in https://<UAServer url>:8443 \n");
                        printf("---------------------------------------------------------------------------------------------------------\n");
                }
                break;
             case 2 : //
                result = pthread_detach(webSocketServerthreadWss);
                if (result != 0)
                {
                        printf("-----SV_WebSocketsInitialise.c : cannot detach Websockets server WSS thread from main thread \n");
                        exit(0);
                }
                else
                {
                        printf("---------------------------------------------------------------------------------------------------------\n");
                        printf("------SV_WebSocketsInitialise.c : websocket server WSS thread is created successfully                  \n");
                        printf("------SV_WebSocketsInitialise.c : starting Websockets server WSS thread and waiting for request    \n");
                        printf("------SV_WebSocketsInitialise.c : data will be made available in https://<UAServer url>:7681 \n");
                        printf("---------------------------------------------------------------------------------------------------------\n");
                }
                break;
            case 3 : //
                result1 = pthread_detach(webSocketServerthreadHttps);
                result2 = pthread_detach(webSocketServerthreadWss);
                if (result1 || result2 != 0)
                {
                        printf("-----SV_WebSocketsInitialise.c : cannot detach Websockets server HTTPS and WSS threads from main thread \n");
                        exit(0);
                }
                else
                {
                        printf("---------------------------------------------------------------------------------------------------------\n");
                        printf("------SV_WebSocketsInitialise.c : websocket server HTTPS and WSS threads are created successfully                  \n");
                        printf("------SV_WebSocketsInitialise.c : starting Websockets server HTTPS and WSS threads and waiting for request    \n");
                        printf("------SV_WebSocketsInitialise.c : data will be made available in https://<UAServer url>:8443 and https://<UAServer url>:7681\n");
                        printf("---------------------------------------------------------------------------------------------------------\n");
                }
                break;
           }
        }
}
