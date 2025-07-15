#define DEBUG
#ifdef no_almagamation
#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/nodestore_default.h>
#include <stdio.h>
#else
   #include "open62541.h"
#endif
#include <stdio.h>
#include <pthread.h>
#include <modbus/modbus.h>

extern UA_NodeId outSlaveId;
extern UA_NodeId outProtocol;
extern UA_NodeId outIP;
extern UA_NodeId outPort;
extern UA_NodeId outTimeout;
extern UA_NodeId outSerialType;
extern UA_NodeId outBaud;
extern UA_NodeId outDatabits;
extern UA_NodeId outDevice;
extern UA_NodeId outParity;
extern UA_NodeId outStopbit;
extern UA_NodeId outInterframeTol;

extern UA_NodeId r4_modbus_TCP_DATA_FC1_Id;
extern UA_NodeId r4_modbus_TCP_DATA_FC2_Id;
extern UA_NodeId r4_modbus_TCP_DATA_FC3_Id;
extern UA_NodeId r4_modbus_TCP_DATA_FC4_Id;

void *initialiseModbusSlaveRTUInstance(void *arg)
{
	/* As a Modbus Slave, open62541 server will read from its Modbus Nodes and return to connecting Masters */

        //int s = -1;
        //int rc = 0;
        //uint8_t buf[MODBUS_TCP_MAX_ADU_LENGTH] = {};
        modbus_t *ctx;
        modbus_mapping_t *mb_mapping;
        uint8_t tab_bits[MODBUS_RTU_MAX_ADU_LENGTH];
        //uint8_t tab_input_bits[MODBUS_RTU_MAX_ADU_LENGTH]; - see below : input_bits[]
        uint16_t tab_input_registers[MODBUS_RTU_MAX_ADU_LENGTH];
        uint16_t tab_holding_registers[MODBUS_RTU_MAX_ADU_LENGTH];
        //int receiveContext;

	UA_Server *server = (UA_Server *)arg;
        if (server == NULL)
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "--------SV_CreateModbusSlave.c (TCP) : Undefined error UA_Server is NULL");
                return NULL;
        }

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "--------SV_CreateModbusSlave.c (RTU) : ----Entering initialiseModbusSlaveTCPInstance()");

        // modbus mapping with 100 outcput coils, 200 input coils, 300 holding registers, 2 input registers
        mb_mapping = modbus_mapping_new(100, 200, 300, 400);
        if (mb_mapping == NULL)
        {
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c (RTU) : ----Unable to allocate modbus mapping structure : %s",
			modbus_strerror(errno));
                return NULL;
        }

        // 1. set the coils (FC1) offset 0 to true (starting address 0)
	mb_mapping->tab_bits = tab_bits;
        mb_mapping->tab_bits[0] = UA_TRUE;
        mb_mapping->tab_bits[1] = UA_FALSE;
        mb_mapping->tab_bits[2] = UA_TRUE;
	mb_mapping->tab_bits[3] = UA_FALSE;

        // 2. set the discrete inputs (FC2) offset 0 to true (starting address
        uint8_t input_bits[] = {0xFF, 0xDB}; // 0xFFDB
        modbus_set_bits_from_bytes(mb_mapping->tab_input_bits, 0, 32, input_bits);
        #ifdef DEBUG
        //for (int index = 0; index < 32; index++)
        //      printf("Discrete inputs are : %d \n", mb_mapping->tab_input_bits[index]);
        #endif


        // 3. set the holding register offsets 0 to integer value 332 (starting address 40000)
	mb_mapping->tab_registers = tab_holding_registers;
        mb_mapping->tab_registers[0] = 332;
        mb_mapping->tab_registers[1] = 144;
        mb_mapping->tab_registers[2] = 543;

        // 4. set the input register offsets 0 to integer value 122 (starting address 30000)
	mb_mapping->tab_input_registers = tab_input_registers;
        mb_mapping->tab_input_registers[0] = 122;
        mb_mapping->tab_input_registers[1] = 777;
        mb_mapping->tab_input_registers[2] = 233;

        // create the modbus rtu slave instance
        char* env_mbslave_slaveId = getenv("SVR_MB_SLAVE_RTU_SLAVEID");
        if (env_mbslave_slaveId != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c : retrieved environment variable <SVR_MB_SLAVE_RTU_SLAVEID> : %s", env_mbslave_slaveId);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c : cannot retrieve environment variable <SVR_MB_SLAVE_RTU_SLAVEID>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbusSlave.c : default to 40");
                env_mbslave_slaveId = (char*)calloc(5, sizeof(char));
                if (env_mbslave_slaveId != NULL)
                        strcpy(env_mbslave_slaveId, "40");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
				"--------SV_CreateModbusSlave.c : cannot retrieve environment variable <SVR_MB_SLAVE_RTU_SLAVEID> : out of memory");
                        exit(UA_FALSE);
                }
        }

        char* env_mbslave_device = getenv("SVR_MB_SLAVE_DEVICE");
        if (env_mbslave_device != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c : retrieved environment variable <SVR_MB_SLAVE_DEVICE> : %s", env_mbslave_device);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c : cannot retrieve environment variable <SVR_MB_SLAVE_DEVICE>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbusSlave.c : default to dev/ttyUSB0");
                env_mbslave_device = (char*)calloc(100, sizeof(char));
                if (env_mbslave_device != NULL)
                        strcpy(env_mbslave_device, "/devttyUSB0");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
				"--------SV_CreateModbusSlave.c : cannot retrieve environment variable <SVR_MB_SLAVE_DEVICE> : out of memory");
                        exit(UA_FALSE);
                }
        }

        char* env_mbslave_baud = getenv("SVR_MB_SLAVE_BAUD");
        if (env_mbslave_baud != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c : retrieved environment variable <SVR_MB_SLAVE_BAUD> : %s", env_mbslave_baud);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c : cannot retrieve environment variable <SVR_MB_SLAVE_BAUD>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbusSlave.c : default to 9600");
                env_mbslave_baud = (char*)calloc(10, sizeof(char));
                if (env_mbslave_baud != NULL)
                        strcpy(env_mbslave_baud, "9600");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
				"--------SV_CreateModbusSlave.c : cannot retrieve environment variable <SVR_MB_SLAVE_BAUD> : out of memory");
                        exit(UA_FALSE);
                }
        }

       char* env_mbslave_parity = getenv("SVR_MB_SLAVE_PARITY");
        if (env_mbslave_parity != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c : retrieved environment variable <SVR_MB_SLAVE_PARITY> : %s", env_mbslave_parity);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c : cannot retrieve environment variable <SVR_MB_SLAVE_PARITY>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbusSlave.c : default to Even");
                env_mbslave_parity = (char*)calloc(10, sizeof(char));
                if (env_mbslave_parity != NULL)
                        strcpy(env_mbslave_parity, "E");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
				"--------SV_CreateModbusSlave.c : cannot retrieve environment variable <SVR_MB_SLAVE_PARITY> : out of memory");
                        exit(UA_FALSE);
                }
        }

       char* env_mbslave_databit = getenv("SVR_MB_SLAVE_DATABIT");
        if (env_mbslave_databit != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c : retrieved environment variable <SVR_MB_SLAVE_DATABIT> : %s", env_mbslave_databit);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c : cannot retrieve environment variable <SVR_MB_SLAVE_DATABIT>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbusSlave.c : default to 7");
                env_mbslave_databit = (char*)calloc(10, sizeof(char));
                if (env_mbslave_databit != NULL)
                        strcpy(env_mbslave_databit, "7");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
				"--------SV_CreateModbusSlave.c : cannot retrieve environment variable <SVR_MB_SLAVE_DATABIT> : out of memory");
                        exit(UA_FALSE);
                }
        }

       char* env_mbslave_stopbit = getenv("SVR_MB_SLAVE_STOPBIT");
        if (env_mbslave_stopbit != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c : retrieved environment variable <SVR_MB_SLAVE_STOPBIT> : %s", env_mbslave_stopbit);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c : cannot retrieve environment variable <SVR_MB_SLAVE_STOPBIT>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbusSlave.c : default to 0");
                env_mbslave_stopbit = (char*)calloc(10, sizeof(char));
                if (env_mbslave_stopbit != NULL)
                        strcpy(env_mbslave_stopbit, "0");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
				"--------SV_CreateModbusSlave.c : cannot retrieve environment variable <SVR_MB_SLAVE_STOPBIT> : out of memory");
                        exit(UA_FALSE);
                }
        }


        ctx = modbus_new_rtu(env_mbslave_device, atoi(env_mbslave_baud), env_mbslave_parity[0], atoi(env_mbslave_databit), atoi(env_mbslave_stopbit));
        if (ctx == NULL)
                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c (TCP) : ----Unable to initialise a new modbus Rtu context : %s", modbus_strerror(errno));
        else
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c (TCP): ----Successfully initialised new modbus Rtu context");

        // set the Modbus address of this slave to 40
        modbus_set_slave(ctx, atoi(env_mbslave_slaveId));

        // set auto-recovery
        modbus_set_error_recovery(ctx, MODBUS_ERROR_RECOVERY_LINK);
        uint32_t old_response_to_sec;
        uint32_t old_response_to_usec;
        /* Save original timeout */
        modbus_get_response_timeout(ctx, &old_response_to_sec, &old_response_to_usec);
        /* Define a new timeout of 200ms */
        modbus_set_response_timeout(ctx, 0, 200000);
        /*
        if (modbus_connect(ctx) == -1)
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c (TCP): ----Unable to connect: %s", modbus_strer>
                modbus_free(ctx);
                return (NULL);
        }
        else
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c (TCP): ---- modbus connect to context successful>
        */

return NULL;
}

void *initialiseModbusSlaveTCPInstance(void *arg)
{
// refer to this : https://www.youtube.com/watch?v=Q0fldVY3s6c

	/* As a Modbus Slave, open62541 server will read from its Modbus Nodes and return to connecting Masters */

	//int s = -1;
	//int rc = 0;
	//uint8_t buf[MODBUS_TCP_MAX_ADU_LENGTH] = {};
	modbus_t *ctx;
	modbus_mapping_t *mb_mapping;
	uint8_t tab_bits[MODBUS_TCP_MAX_ADU_LENGTH];
	//uint8_t tab_input_bits[MODBUS_TCP_MAX_ADU_LENGTH]; - see below : input_bits
	uint16_t tab_input_registers[MODBUS_TCP_MAX_ADU_LENGTH];
	uint16_t tab_holding_registers[MODBUS_TCP_MAX_ADU_LENGTH];
	int receiveContext;
	int modbus_slave_socket;

	UA_Server *server = (UA_Server *)arg;
	if (server == NULL)
	{
		UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
		"--------SV_CreateModbusSlave.c (TCP) : Undefined error UA_Server is NULL");
		return NULL;
	}

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
		"--------SV_CreateModbusSlave.c (TCP) : ----Entering initialiseModbusSlaveTCPInstance()");

	// modbus mapping with 100 outcput coils, 200 input coils, 300 holding registers, 2 input registers
	mb_mapping = modbus_mapping_new(100, 200, 300, 400);
	if (mb_mapping == NULL)
	{
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c (TCP) : ----Unable to allocate modbus mapping structure : %s", modbus_strerror(errno));
		return NULL;
	}

	// 1. set the coils (FC1) offset 0 to true (starting address 0)
	mb_mapping->tab_bits = tab_bits;
	mb_mapping->tab_bits[0] = UA_TRUE;
	mb_mapping->tab_bits[1] = UA_FALSE;
	mb_mapping->tab_bits[2] = UA_TRUE;
	mb_mapping->tab_bits[3] = UA_FALSE;

	// 2. set the discrete inputs (FC2) offset 0 to true (starting address
	uint8_t input_bits[] = {0xFF, 0xDB}; // 0xFFDB
	modbus_set_bits_from_bytes(mb_mapping->tab_input_bits, 0, 32,input_bits);
	#ifdef DEBUG
	//for (int index = 0; index < 32; index++)
	//	printf("Discrete inputs are : %d \n", mb_mapping->tab_input_bits[index]);
	#endif

	// 3. set the holding register offsets 0 to integer value 332 (starting address 40000)
	mb_mapping->tab_registers = tab_holding_registers;
	mb_mapping->tab_registers[0] = 332;
	mb_mapping->tab_registers[1] = 144;
	mb_mapping->tab_registers[2] = 543;

	// 4. set the input register offsets 0 to integer value 122 (starting address 30000)
	mb_mapping->tab_input_registers = tab_input_registers;
	mb_mapping->tab_input_registers[0] = 122;
	mb_mapping->tab_input_registers[1] = 777;
	mb_mapping->tab_input_registers[2] = 233;

	// create the modbus tcp slave instance
	// listening to any addresses on port 1502


               char* env_mbslave_slaveId = getenv("SVR_MB_SLAVE_TCP_SLAVEID");
                if (env_mbslave_slaveId != NULL)
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
				"--------SV_CreateModbusSlave.c : retrieved environment variable <SVR_MB_SLAVE_TCP_SLAVEID> : %s", env_mbslave_slaveId);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
				"--------SV_CreateModbusSlave.c : cannot retrieve environment variable <SVR_MB_SLAVE_TCP_SLAVEID>");
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbusSlave.c : default to 45");
                        env_mbslave_slaveId = (char*)calloc(5, sizeof(char));
                        if (env_mbslave_slaveId != NULL)
                                strcpy(env_mbslave_slaveId, "45");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
					"--------SV_CreateModbusSlave.c : cannot retrieve environment variable <SVR_MB_SLAVE_SLAVEID> : out of memory");
                                exit(UA_FALSE);
                        }
                }

               char* env_mbslave_port = getenv("SVR_MB_SLAVE_PORT");
                if (env_mbslave_port != NULL)
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
				"--------SV_CreateModbusSlave.c : retrieved environment variable <SVR_MB_SLAVE_PORT> : %s", env_mbslave_port);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
				"--------SV_CreateModbusSlave.c : cannot retrieve environment variable <SVR_MB_SLAVE_PORT>");
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbusSlave.c : default to 1502");
                        env_mbslave_port = (char*)calloc(5, sizeof(char));
                        if (env_mbslave_port != NULL)
                                strcpy(env_mbslave_port, "1502");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
					"--------SV_CreateModbusSlave.c : cannot retrieve environment variable <SVR_MB_SLAVE_SLAVEPORT> : out of memory");
                                exit(UA_FALSE);
                        }
                }

	ctx = modbus_new_tcp(NULL, atoi(env_mbslave_slaveId));
	if (ctx == NULL)
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c (TCP) : ----Unable to initialise a new modbus tcp context : %s", modbus_strerror(errno));
	else
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c (TCP): ----Successfully initialised new modbus tcp context");

	// set the Modbus address of this slave to 45
	printf("--------SV_CreateModbusSlave.c (???) : modbus_set_slave(port) is %d \n", atoi(env_mbslave_port));
	modbus_set_slave(ctx, atoi(env_mbslave_port));

	// set auto-recovery
	modbus_set_error_recovery(ctx, MODBUS_ERROR_RECOVERY_LINK);
	uint32_t old_response_to_sec;
	uint32_t old_response_to_usec;
	/* Save original timeout */
	modbus_get_response_timeout(ctx, &old_response_to_sec, &old_response_to_usec);
	/* Define a new timeout of 200ms */
	modbus_set_response_timeout(ctx, 0, 200000);
	/*
	if (modbus_connect(ctx) == -1)
	{
		UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c (TCP): ----Unable to connect: %s", modbus_strerror(errno));
		modbus_free(ctx);
		return (NULL);
	}
	else
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c (TCP): ---- modbus connect to context successful");
	*/
	while (UA_TRUE)
	{
	// listen for modbus requests
		modbus_slave_socket = modbus_tcp_listen(ctx, 1);	// handle up to 1 established connections
		if (modbus_slave_socket == -1)
		{
			UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
				"--------SV_CreateModbusSlave.c (TCP) : ----Unable to create a listening TCP socket : %s", modbus_strerror(errno));
			sleep(1);
			continue;
		}
		else
		{
		//FD_ZERO(&refset);	/* clear the reference set of socket */
		//FD_SET(mb_slave_socket, &refset);	// add the server socket */
	    //if (select(mb_slave_socket +1, &refset, NULL, NULL, NULL) == -1)
	    //{
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
				"--------SV_CreateModbusSlave.c (TCP) : ----Successfully created a listening TCP socket for modbus slave : %d",
				modbus_slave_socket);
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
				"--------SV_CreateModbusSlave.c (TCP) : ----Start listening for incoming modbus TCP requests ...");
			modbus_tcp_accept(ctx, &modbus_slave_socket);
			//UA_Byte queryCOIL[MODBUS_TCP_MAX_ADU_LENGTH];

			if (modbus_slave_socket < 0)
			{
				UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
					"--------SV_CreateModbusSlave.c (TCP) : ----FATAL error encountered in modbus_tcp_accept()");
				sleep(1);
				continue;
			}
			else
			{
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
					"--------SV_CreateModbusSlave.c (TCP) : ----Connection request from a Master is accepted");
				break;
			}
		} // ---if (mb_slave_socket == -1)
	} // ---while (UA_TRUE)

                struct mb_tcp {
                        uint16_t transact;
                        uint16_t protocol;
                        uint16_t length;
                        uint8_t unit;
                        uint8_t func;
                        uint8_t data[];
                };

		uint8_t buf[MODBUS_TCP_MAX_ADU_LENGTH];
                for (;;)
                {
                        do
                        {
                                receiveContext = modbus_receive(ctx, buf);
                        } while (receiveContext == 0);

                        if (receiveContext < 0) {
                                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
					"--------SV_CreateModbusSlave.c (TCP) : ----Error receiving data %s", modbus_strerror(errno));
                                close(modbus_slave_socket);
                                break;
                        }

			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
				"--------SV_CreateModbusSlave.c (TCP) : ----Received a new connection request from Master");
                        struct mb_tcp *frame = (struct mb_tcp *) buf;
                        printf("Function code is : %d \n", frame->func);
                        printf("Protocol is      : %d \n", frame->protocol);
                        printf("Transaction is   : %d \n", frame->transact);
                        printf("-----------------------\n");

                        // respond back to the Master
                        receiveContext = modbus_reply(ctx, buf, receiveContext, mb_mapping);
                        if (receiveContext< 0)
                        {
                                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
					"--------SV_CreateModbusSlave.c (TCP) : ----Error responding back to the Master %s",
					modbus_strerror(errno));
                                close(modbus_slave_socket);
                                break;
                        }

                }
        // free up resources
        close(modbus_slave_socket);
        modbus_free(ctx);
        modbus_mapping_free(mb_mapping);

	return NULL;
}

#ifdef NO_USE
			else if (receiveContext >= 0)
			{
				// update modbus slave data into mb_mapping
				mb_mapping->nb_bits = 0;
				mb_mapping->nb_input_bits = 0;
				mb_mapping->nb_input_registers = 0;
				mb_mapping->nb_registers = 0;
				mb_mapping->tab_bits = tab_bits;
				mb_mapping->tab_input_bits = tab_input_bits;
				mb_mapping->tab_input_registers = tab_input_registers;
			us.c : ---- modbus slave stopped unexpectedly.
				// reply back to MB_Master
				modbus_reply(ctx, queryCOIL, receiveContext, mb_mapping);
			}
		}
		UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c (TCP) : ---- modbus slave stopped unexpectedly.");
		if (mb_slave_socket != -1)
			close(mb_slave_socket);
		modbus_mapping_free(mb_mapping);
		modbus_close(ctx);
		modbus_free(ctx);
		return NULL;
	}
#endif
#ifdef WAIT
	if (select(mb_slave_socket + 1, &refset, NULL, NULL, NULL) == -1
	{

	}

	map = modbus_mapping_new_start_address(0, NB_BITS, 0, NB_INPUT_BITS, 0, NB_REGISTERS, 10, NB_INPUT_REGISTERS);


	if (map == NULL)
	{
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusSlave.c (TCP) : ----Unable to allocate modbus mapping : %s", modbus_strerror(errno));
	}
	else
	{
		// assign values to input registers (simulate sensor data)
		// instead read from opcuA server nodes
		map->tab_input_registers[0] = 0;
		map->tab_input_registers[1] = 1;
		map->tab_input_registers[2] = 2;

		// assign values to discrete input registers
		modbus_set_bits_from_bytes(
			map->tab_input_bits, 0, NB_INPUT_REGISTERS, UT_INPUT_BITS_TAB);

		while (UA_TRUE)
		{
			s = modbus_tcp_listen(ctx, 100);	// 100 = number of concurrent modbus connections
			modbus_tcp_accept(ctx, &s);

			if (s < 0)
			{
				UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
					"--------SV_CreateModbusSlave.c (TCP) : ----Unable to listen to modbus TCP connections : %s , wait for 5 secs to retry",
					modbus_strerror(errno));
				sleep(5);
				continue;
			}
			else
			{
				// listening socket is created
				break;
			}
		}

		struct mb_tcp {
			uint16_t transact;
			uint16_t protocol;
			uint16_t length;
			uint8_t unit;
			uint8_t func;
			uint8_t data[];
		};

		for (;;)
		{
			do
			{
				rc = modbus_receive(ctx, buf);
			} while (rc == 0);

			if (rc < 0) {
				UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
					"--------SV_CreateModbus.c (TCP) : ----Error receiving data %s", modbus_strerror(errno));
				close(s);
				break;
			}

			printf("Received polling request from Master \n");
			struct mb_tcp *frame = (struct mb_tcp *) buf;
			printf("Function code is : %d \n", frame->func);
			printf("Protocol is 	 : %d \n", frame->protocol);
			printf("Transaction is	 : %d \n", frame->transact);
			printf("-----------------------\n");

			// respond back to the Master
			rc = modbus_reply(ctx, buf, rc, map);
			if (rc < 0)
			{
				UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
					"--------SV_CreateModbusSlave.c (TCP) : ----Error responding back to the Master %s",
					modbus_strerror(errno));
				close(s);
				break;
			}

		}

	}
	// free up resources
	close(s);
	modbus_free(ctx);
	modbus_mapping_free(map);
	return NULL;
}
#endif

