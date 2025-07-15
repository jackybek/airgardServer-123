
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
#include <stdio.h>

        typedef struct webSocketStruct {
                UA_Server *uaServer;
                UA_NodeId *pdsId;
                UA_NetworkAddressUrlDataType networkAddrUrl;
                int argC;
                char** argV;
        } wsThreadArgs;

void createWebSocketsOption(void *);
void createWebSocketsServerHttps(void *);
void createWebSocketsServerWss(void *);

void createWebSocketsOption(void *input)
{
        wsThreadArgs *arg = (wsThreadArgs *)input;

	printf("inspecting the pthread parameters \n");
	printf("================================= \n");
	printf("input->pdsId 		= %d      \n", arg->pdsId->identifier.numeric);
	printf("input->networkAddrUrl 	= %s      \n", arg->networkAddrUrl.url.data);
	printf("input->argC		= %d      \n", arg->argC);
	printf("input->argV		= %s      \n", arg->argV[0]);

        int response;
        printf("Choose the type of server instance to create \n");
        printf("1 : Http Secure instance (https)\n");
        printf("2 : Web Sockets Secure (wss) \n");
        scanf("%d", &response);

        switch (response)
        {
                case 1 : createWebSocketsServerHttps(input); break;
                case 2 : createWebSocketsServerWss(input); break;
                default : createWebSocketsServerWss(input); break;
        }
}
