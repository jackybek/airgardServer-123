#ifdef no_almagamation
#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/nodestore_default.h>
#include <stdio.h>
#else
   #define UA_ENABLE_DISCOVERY_MULTICAST
   #include "open62541.h"
#endif
#include "SV_Callbacks.h"
#include <modbus/modbus.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>  // inet_addr
#include <unistd.h>
#include <pthread.h>

#define PORT 20004	// this is the listening port of Airgard server
#define ADDR "192.168.1.109"

UA_Boolean firsttimeSendData = UA_TRUE;
UA_Boolean firsttimeReceiveData = UA_TRUE;
static UA_Boolean callbackmodbusTCPFirstTime = UA_TRUE;
static UA_Boolean callbackmodbusRTUFirstTime = UA_TRUE;
static UA_Boolean callbackTwinCATAdsFirstTime = UA_TRUE;
static UA_Boolean callbackCanopenFirstTime = UA_TRUE;

void Callback_modbusTCP(UA_Server *server, void *data)
{
	if (callbackmodbusTCPFirstTime == UA_TRUE)
	{
		// only print the debug message once, otherwise it will flood the screen
		// to check if the callback is invoked every cycle, go to D:\apps\diagslave\win\runTCP.bat and see the output
		UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Callbacks.c : Callback_modbusTCP is invoked");
		callbackmodbusTCPFirstTime = UA_FALSE;
	}
	readModbusTCPData(server, (modbus_t *)data);

	// results are save to OPC_Server address e.g.
	// server->field_1 = data[0];
	// server->field_2 = data[1];

}

void Callback_modbusRTU(UA_Server *server, void *data)
{
 	if (callbackmodbusRTUFirstTime == UA_TRUE)
        {
		// only print the debug message once, otherwise it will flood the screen
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Callbacks.c : Callback_modbusRTU is invoked");
                callbackmodbusRTUFirstTime = UA_FALSE;
        }
	#ifdef KIV_until_EL6022_is_installed_in_CX9020
	readModbusRTUData(server, (modbus_t *)data);
	#endif

}

void Callback_TwinCATAds(UA_Server *server, void *data)
{
	if (callbackTwinCATAdsFirstTime == UA_TRUE)
	{
		// only print the debug message once, otherwise it will flood the screen
		UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Callbacks.c : Callback_TwinCATAds is invoked");
		callbackTwinCATAdsFirstTime = UA_FALSE;
	}
//	CB_TwinCATAdsUpdate(); //(twincat_ads *)data);
}

void Callback_profinet(UA_Server *server, void *data)
{
        //UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "callback 3 is invoked");
}

void Callback_Canopen(UA_Server *server, void *data)
{
	if (callbackCanopenFirstTime == UA_TRUE)
        {
                // only print the debug message once, otherwise it will flood the screen
	        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Callback_Canopen is invoked");
		callbackCanopenFirstTime = UA_FALSE;
	}
	#ifdef KIV_until_powersupply_is_installed
	readCanopenData(server);
	#endif
}


void createRepeatedCallbacks(UA_Server *server)
{

/* moving the following to SV_main() cause it to crash */
	//modbus_t *mb;
	modbus_t **arrayOfModbusMasters;

	char *env_CallbackModbusTcp, *env_CallbackModbusRtu, *env_CallbackTwinCATAds;
	char *env_CallbackCanopen;
	//pnet_cfg_t *pfnet;
	// the following function creates
	// a modbus master (TCP or SERIAL,depending on the configuration set in ENV variables
	// a modbus slave listening to port 1502

	env_CallbackModbusTcp = getenv("SVR_CALLBACK_MODBUS_TCP");
        if (env_CallbackModbusTcp != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_Callbacks.c : retrieved environment variable <SVR_CALLBACK_MODBUS_TCP> : %s", env_CallbackModbusTcp);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Callbacks.c : cannot retrieve environment variable <SVR_CALLBACK_MODBUS_TCP>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Callbacks.c : default to <NO>");
                env_CallbackModbusTcp = (char*)calloc(10, sizeof(char));
                if (env_CallbackModbusTcp != NULL)
                        strcpy(env_CallbackModbusTcp, "YES");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
				"--------SV_Callbacks.c : cannot retrieve environment variable <SVR_CALLBACK_MODBUS_TCP> : out of memory");
			printf("Please enter  YES or NO for SVR_CALLBACK_MODBUS_TCP : "); scanf("%s", env_CallbackModbusTcp);
                        //exit(UA_FALSE);
                }
        }

	env_CallbackModbusRtu = getenv("SVR_CALLBACK_MODBUS_RTU");
        if (env_CallbackModbusRtu != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Callbacks.c : retrieved environment variable <SVR_CALLBACK_MODBUS_RTU> : %s", env_CallbackModbusRtu);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Callbacks.c : cannot retrieve environment variable <SVR_CALLBACK_MODBUS_RTU>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Callbacks.c : default to <NO>");
                env_CallbackModbusRtu = (char*)calloc(10, sizeof(char));
                if (env_CallbackModbusRtu != NULL)
                        strcpy(env_CallbackModbusRtu, "NO");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                "--------SV_Callbacks.c : cannot retrieve environment variable <SVR_CALLBACK_MODBUS_RTU> : out of memory");
			printf("Please enter  YES or NO for SVR_CALLBACK_MODBUS_RTU : "); scanf("%s", env_CallbackModbusRtu);
                        //exit(UA_FALSE);
                }
        }

	env_CallbackTwinCATAds = getenv("SVR_CALLBACK_TWINCAT_ADS");
        if (env_CallbackTwinCATAds != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Callbacks.c : retrieved environment variable <SVR_CALLBACK_TWINCAT_ADS> : %s", env_CallbackTwinCATAds);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Callbacks.c : cannot retrieve environment variable <SVR_CALLBACK_TWINCAT_ADS>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Callbacks.c : default to <NO>");
                env_CallbackTwinCATAds = (char*)calloc(10, sizeof(char));
                if (env_CallbackTwinCATAds != NULL)
                        strcpy(env_CallbackTwinCATAds, "NO");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                "--------SV_Callbacks.c : cannot retrieve environment variable <SVR_CALLBACK_TWINCAT_ADS> : out of memory");
			printf("Please enter  YES or NO for SVR_CALLBACK_TWINCAT_ADS : "); scanf("%s", env_CallbackTwinCATAds);
                        //exit(UA_FALSE);
                }
        }

	env_CallbackCanopen = getenv("SVR_CALLBACK_TWINCAT_CANOPEN");
        if (env_CallbackCanopen != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Callbacks.c : retrieved environment variable <SVR_CALLBACK_CANOPEN> : %s", env_CallbackCanopen);

        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Callbacks.c : cannot retrieve environment variable <SVR_CALLBACK_CANOPEN>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Callbacks.c : default to <NO>");
                env_CallbackCanopen = (char*)calloc(10, sizeof(char));
                if (env_CallbackCanopen != NULL)
                        strcpy(env_CallbackCanopen, "NO");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                "--------SV_Callbacks.c : cannot retrieve environment variable <SVR_CALLBACK_CANOPEN> : out of memory");
			printf("Please enter  YES or NO for SVR_CALLBACK_CANOPEN : "); scanf("%s", env_CallbackCanopen);
                        //exit(UA_FALSE);
                }
        }

	#ifdef COMMENT
	arrayOfModbusMasters[0] = modbusTcp;
	arrayOfModbusMasters[1] = modbusRtu;
	arrayOfModbusMasters[2] = modbusAscii; // not implemented
	arrayOfModbusMasters[3] = modbusUdp; // not implemented
	arrayOfModbusMasters[4] = modbusRtuOverTcp; // not implemented
	#endif

	// mb = createModbusExtension(server); // initial code only create 1 modbus master
	// new code creates 4 modbus masters in this order : tcp, rtu, ascii, udp, rtu over tcp
	arrayOfModbusMasters = createModbusExtension(server);

	//if (!mb)
	//	 UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_Callbacks.c : cannot get handle to modbus object");

	if (strncmp(env_CallbackModbusTcp, "YES", 3) == 0)
	{
		printf("about to call addRepeatedCallback - modbusTCP \n");
		//UA_Server_addRepeatedCallback(server, Callback_modbusTCP, mb, 2000, NULL); // every 2,000 msec callback is invoked
		if (!arrayOfModbusMasters[0])
			UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_Callbacks.c : cannot get handle to modbusTcp object");
		else
			UA_Server_addRepeatedCallback(server, Callback_modbusTCP, arrayOfModbusMasters[0], 2000, NULL);
	}
	if (strncmp(env_CallbackModbusRtu, "YES", 3) == 0)
	{
		//UA_Server_addRepeatedCallback(server, Callback_modbusRTU, mb, 2000, NULL); // every 2,000 msec callback is invoked
		if (!arrayOfModbusMasters[1])
			UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_Callbacks.c : cannot get handle to modbusRtu object");
		else
			UA_Server_addRepeatedCallback(server, Callback_modbusRTU, arrayOfModbusMasters[1], 2000, NULL);
	}
	#ifdef NO_IMPLEMENTATION
        if (strncmp(env_CallbackModbusAscii, "YES", 3) == 0)
        {
                //UA_Server_addRepeatedCallback(server, Callback_modbusRTU, mb, 2000, NULL); // every 2,000 msec callback is invoked
                if (!arrayOfModbusMasters[2])
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_Callbacks.c : cannot get handle to modbusAscii object");
                else
                        UA_Server_addRepeatedCallback(server, Callback_modbusASCII, arrayOfModbusMasters[2], 2000, NULL);
        }
        if (strncmp(env_CallbackModbusUdp, "YES", 3) == 0)
        {
                //UA_Server_addRepeatedCallback(server, Callback_modbusRTU, mb, 2000, NULL); // every 2,000 msec callback is invoked
                if (!arrayOfModbusMasters[3])
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_Callbacks.c : cannot get handle to modbusUdp object");
                else
                        UA_Server_addRepeatedCallback(server, Callback_modbusUDP, arrayOfModbusMasters[3], 2000, NULL);
        }
        if (strncmp(env_CallbackModbusRtuOverTcp, "YES", 3) == 0)
        {
                //UA_Server_addRepeatedCallback(server, Callback_modbusRTU, mb, 2000, NULL); // every 2,000 msec callback is invoked
                if (!arrayOfModbusMasters[4])
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "SV_Callbacks.c : cannot get handle to modbusRtuOverTcp object");
                else
                        UA_Server_addRepeatedCallback(server, Callback_modbusRTUOverTCP, arrayOfModbusMasters[4], 2000, NULL);
        }
	#endif
	if (strncmp(env_CallbackTwinCATAds, "YES", 3) == 0)
		UA_Server_addRepeatedCallback(server, Callback_TwinCATAds, NULL, 10, NULL); // every 10 msec callback is invoked : to match TwinCAT realtime cycle time

	if (strncmp(env_CallbackCanopen, "YES", 3) == 0)
		UA_Server_addRepeatedCallback(server, Callback_Canopen, NULL, 10000, NULL); // evvery 10,000 msec callback is invoked


	// placeholders for future use of callbacks
	//pfnet = createProfinetExtension(server);
	//UA_Server_addRepeatedCallback(server, Callback_profinet, pfnet, 2000, NULL);
	//UA_Server_addRepeatedCallback(server, Callback_3, NULL, 6000, NULL);
	//UA_Server_addRepeatedCallback(server, Callback_4, NULL, 8000, NULL);
}
