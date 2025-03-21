#ifdef almagamation
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#else
   #include "open62541.h"
#endif
#include "SV_ReverseConnect.h"

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>


static void reverseConnectStateCallback(UA_Server *server, UA_UInt64 handle,
                                        UA_SecureChannelState state, void *context)
{
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Reverse connect state callback for %lu with context %p: State %d",
                (unsigned long)handle, context,  state);
}

void reverseConnect(UA_Server *uaServer, char *svrAddressRaw)
{
        const char *env_SVRport = getenv("SVR_REVERSE_CONNECT_PORT");
	int reverse_connect_port = atoi(env_SVRport);
	UA_UInt64 handle;

        char svr_endpoint[100];
	char new_connect_port[6];
	int index = strlen(svrAddressRaw)+10;	// add 9 is because addr start with opc.udp:// or opc.tcp://

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "==========================================================");
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_reverseConnect.c : entering reverseConnect()");

	#ifdef UA_PUBSUB
	strcpy(svr_endpoint, "opc.udp://");
	#else
	strcpy(svr_endpoint, "opc.tcp://");
	#endif

        strncpy(&svr_endpoint[10], svrAddressRaw, strlen(svrAddressRaw));
        svr_endpoint[index]= ':';
	itoa(reverse_connect_port, new_connect_port, 10);
        strcpy(&svr_endpoint[index+1], new_connect_port); //env_SVRport);
        //printf("2 : %s, %d \n", lds_endpoint, strlen(lds_endpoint));


        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_reverseConnect.c : successfully assembled SVR endpoint url : <%s>", svr_endpoint);

	int retval = UA_Server_addReverseConnect(uaServer, UA_STRING(svr_endpoint),
			reverseConnectStateCallback, (void *)123456, &handle);

	if (retval == UA_STATUSCODE_GOOD)
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_reverseConnect.c : add ReverseConnect success");
	else
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_reverseConnect.c : Failure to add ReverseConnect <%s>", UA_StatusCode_name(retval));
}
