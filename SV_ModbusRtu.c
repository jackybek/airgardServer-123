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

UA_NodeId outSlaveId;
UA_NodeId outProtocol;
UA_NodeId outIP;
UA_NodeId outPort;
UA_NodeId outTimeout;
UA_NodeId outSerialType;
UA_NodeId outBaud;
UA_NodeId outDatabits;
UA_NodeId outDevice;
UA_NodeId outParity;
UA_NodeId outStopbit;
UA_NodeId outInterframeTol;

UA_NodeId r4_modbus_TCP_DATA_FC1_Id;
UA_NodeId r4_modbus_TCP_DATA_FC2_Id;
UA_NodeId r4_modbus_TCP_DATA_FC3_Id;
UA_NodeId r4_modbus_TCP_DATA_FC4_Id;

UA_NodeId outArrayDataFC1;
UA_NodeId outArrayDataFC2;
UA_NodeId outArrayDataFC3;
UA_NodeId outArrayDataFC4;

UA_NodeId outArraySizeFC1;
UA_NodeId outArraySizeFC2;
UA_NodeId outArraySizeFC3;
UA_NodeId outArraySizeFC4;

void *initialiseModbusSlaveInstance(void *);
modbus_t *createModusExtension(UA_Server *);
modbus_t *initialiseModbusConnectionToSlaveTCP(UA_Server *, size_t, UA_NodeId, char*, char*, short, int);
modbus_t *initialiseModbusConnectionToSlaveRTU(UA_Server *, size_t, UA_NodeId, const char *device, int serialType, long baud, char parity, int data_bit, int stop_bit, int slave);
modbus_t *initialiseModbusConnectionToSlaveASCII(UA_Server *, size_t, UA_NodeId);
modbus_t *initialiseModbusConnectionToSlaveUDP(UA_Server *, size_t, UA_NodeId);
modbus_t *initialiseModbusConnectionToSlaveRTUoverTCP(UA_Server *, size_t, UA_NodeId);

int readModbusRTU(UA_Server *, modbus_t *);
void closeModbusConnection(modbus_t *);

const int NB_BITS = 16;
const int NB_INPUT_BITS = 16;
const int NB_REGISTERS = 16;
const int NB_INPUT_REGISTERS = 16;
const uint8_t UT_INPUT_BITS_TAB[] = {0xFF, 0xDB};

void *initialiseModbusSlaveInstance(void *arg)
{
	/* As a Modbus Slave, open62541 server will read from its Modbus Nodes and return to connecting Masters */
return NULL;

	int s = -1;
	int rc = 0;
	uint8_t buf[MODBUS_TCP_MAX_ADU_LENGTH] = {};
	modbus_t *ctx;
	modbus_mapping_t *map;

	UA_Server *server = (UA_Server *)arg;

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Entering initialiseModbusSlaveInstance()");

	// create the modbus slave instance
	// listening port = 1502

	ctx = modbus_new_tcp(NULL, 1502);
	map = modbus_mapping_new_start_address(
		0, NB_BITS, 0, NB_INPUT_BITS, 0, NB_REGISTERS, 10, NB_INPUT_REGISTERS);


	if (map == NULL)
	{
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Unable to allocate modbus mapping : %s", modbus_strerror(errno));
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
				UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Unable to listen to modbus TCP connections : %s , wait for 10 secs to retry",  modbus_strerror(errno));
				sleep(10);
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
				UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "--------SV_Modbus.c : Error receiving data %s", modbus_strerror(errno));
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
				UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "--------SV_Modbus.c : Error responding back to the Master %s", modbus_strerror(errno));
				close(s);
				break;
			}

		}

	}
	// UA_free up resources
	close(s);
	modbus_free(ctx);
	modbus_mapping_free(map);
	return NULL;
}

modbus_t *initialiseModbusConnectionToSlaveRTU(UA_Server *uaServer, size_t namespaceIndex, UA_NodeId r1_modbusroot_Id, const char *device, int serialType, long baud, char parity, int data_bit, int stop_bit, int slave_id)
{

  /* As a modbus RTU Master, open62541 will read from an external Modbus Slave and update its nodes */

  modbus_t *mb=NULL;
  int rc;

#ifdef DEBUG
printf("data passed in is %s %d %ld %c %d %d %d \n", device, serialType, baud, parity, data_bit, stop_bit, slave_id);
#endif

  mb = modbus_new_rtu(device, baud, parity, data_bit, stop_bit);
  if (mb == NULL)
  {
         UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Unable to allocate libmodbus context : %s", modbus_strerror(errno));
        return NULL;
  }
  else
  {
	if (serialType == 232)
	{
        	if (modbus_rtu_set_serial_mode(mb, MODBUS_RTU_RS232) == -1)
		{
			UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot set mode to RS232 : %s", modbus_strerror(errno));
			modbus_free(mb);
			return NULL;
		}
		else
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Serial Type set is RS232");
	}
	else if (serialType == 485)
	{
		 if (modbus_rtu_set_serial_mode(mb, MODBUS_RTU_RS485) == -1)
                {
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot set mode to RS485 : %s", modbus_strerror(errno));
                        modbus_free(mb);
                        return NULL;
                }
		else
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Serial Type set is RS485");
	}

	rc = modbus_set_slave(mb, rtu_slave_id);
	if (rc == -1)
	{
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Cannot set slave id : %s\n", modbus_strerror(errno));
		modbus_free(mb);
		return NULL;
	}
	else
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Slave id set is %d", slave_id);

        if (modbus_connect(mb) == -1)
        {
                printf("SV_Modbus.c : Connection to modbus slave endpoint %s (serialtype = RS%d, baud = %ld, parity = %c, data_bit = %d, stop_bit = %d, slave_id = %d) failed : %s\n",
			device, serialType, baud, parity, data_bit, stop_bit, slave_id, modbus_strerror(errno));
                modbus_free(mb);
                return NULL;
        }
	else
	{
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c :  Successfully connected to modbus slave endpoint %s (serialtype = RS%d, baud = %ld, parity = %c, data_bit = %d, stop_bit = %d, slave_id = %d)",
			device, serialType, baud, parity, data_bit, stop_bit, slave_id);


				// Add objectnode to represent Modbus->RTU
                                UA_NodeId r2_modbus_RTU_Id;
                                UA_ObjectAttributes oAttr_r2_modbus_RTU = UA_ObjectAttributes_default;
                                oAttr_r2_modbus_RTU.description = UA_LOCALIZEDTEXT("en-US", "RTU");
                                oAttr_r2_modbus_RTU.displayName = UA_LOCALIZEDTEXT("en-US", "RTU");
                                UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "Modbus_RTU_Info"),
					r1_modbusroot_Id,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                        UA_QUALIFIEDNAME(namespaceIndex, "RTU"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                    oAttr_r2_modbus_RTU, NULL, &r2_modbus_RTU_Id);                           // attr, nodeContext, outNewNodeId assigned by the server


                                        // variable nodes : SerialType, Baud, Databits, Device, Parity, StopBit, Interframe
                                        // ---- RTU->SerialType
                                        UA_VariableAttributes vSerialTypeAttr = UA_VariableAttributes_default;
                                        vSerialTypeAttr.description = UA_LOCALIZEDTEXT("en-US", "SerialType");
                                        vSerialTypeAttr.displayName = UA_LOCALIZEDTEXT("en-US", "4.1 Serial Type");
                                        vSerialTypeAttr.dataType = UA_TYPES[UA_TYPES_INT16].typeId;
                                        vSerialTypeAttr.valueRank = UA_VALUERANK_SCALAR;
                                        vSerialTypeAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                        vSerialTypeAttr.historizing = UA_FALSE;
                                        //UA_String ip = UA_STRING(ipaddress);
                                                UA_Variant_setScalar(&vSerialTypeAttr.value, &serialType, &UA_TYPES[UA_TYPES_INT16]);
                                        	UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 106),
                                                r2_modbus_RTU_Id,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                UA_QUALIFIEDNAME(namespaceIndex, "SerialType"),
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                vSerialTypeAttr, NULL, &outSerialType);

                                        // ---- RTU->Baud
                                        UA_VariableAttributes vBaudAttr = UA_VariableAttributes_default;
                                        vBaudAttr.description = UA_LOCALIZEDTEXT("en-US", "Baud");
                                        vBaudAttr.displayName = UA_LOCALIZEDTEXT("en-US", "4.2 Baud rate");
                                        vBaudAttr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
                                        vBaudAttr.valueRank = UA_VALUERANK_SCALAR;
                                        vBaudAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                        vBaudAttr.historizing = UA_FALSE;
                                        //UA_String ip = UA_STRING(ipaddress);
                                                UA_Variant_setScalar(&vBaudAttr.value, &baud, &UA_TYPES[UA_TYPES_INT32]);
                                        	UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 107),
                                                r2_modbus_RTU_Id,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                UA_QUALIFIEDNAME(namespaceIndex, "Baud"),
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                vBaudAttr, NULL, &outBaud);

                                        // ---- RTU->Data bits
                                        UA_VariableAttributes vDatabitsAttr = UA_VariableAttributes_default;
                                        vDatabitsAttr.description = UA_LOCALIZEDTEXT("en-US", "Databits");
                                        vDatabitsAttr.displayName = UA_LOCALIZEDTEXT("en-US", "4.3 Data bit");
                                        vDatabitsAttr.dataType = UA_TYPES[UA_TYPES_INT16].typeId;
                                        vDatabitsAttr.valueRank = UA_VALUERANK_SCALAR;
                                        vDatabitsAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                        vDatabitsAttr.historizing = UA_FALSE;
                                        //UA_String ip = UA_STRING(ipaddress);
                                                UA_Variant_setScalar(&vDatabitsAttr.value, &data_bit, &UA_TYPES[UA_TYPES_INT16]);
                                        	UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 108),
                                                r2_modbus_RTU_Id,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                UA_QUALIFIEDNAME(namespaceIndex, "Databits"),
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                vDatabitsAttr, NULL, &outDatabits);


                                        // ---- RTU->Device
                                        UA_VariableAttributes vDeviceAttr = UA_VariableAttributes_default;
                                        vDeviceAttr.description = UA_LOCALIZEDTEXT("en-US", "Device");
                                        vDeviceAttr.displayName = UA_LOCALIZEDTEXT("en-US", "4.4 Device Type");
                                        vDeviceAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
                                        vDeviceAttr.valueRank = UA_VALUERANK_SCALAR;
                                        vDeviceAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                        vDeviceAttr.historizing = UA_FALSE;
                                        UA_String _device = UA_STRING((char*)device);
                                                UA_Variant_setScalar(&vDeviceAttr.value, &_device, &UA_TYPES[UA_TYPES_STRING]);
                                        	UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 109),
                                                r2_modbus_RTU_Id,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                UA_QUALIFIEDNAME(namespaceIndex, "Device"),
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                vDeviceAttr, NULL, &outDevice);

                                        // ---- RTU->Parity
					char _parity[10];
					sprintf(_parity, "%c", parity); 
                                        UA_VariableAttributes vParityAttr = UA_VariableAttributes_default;
                                        vParityAttr.description = UA_LOCALIZEDTEXT("en-US", "Parity");
                                        vParityAttr.displayName = UA_LOCALIZEDTEXT("en-US", "4.5 Parity bit");
                                        vParityAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
                                        vParityAttr.valueRank = UA_VALUERANK_SCALAR;
                                        vParityAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                        vParityAttr.historizing = UA_FALSE;
                                        UA_String _Parity = UA_STRING(_parity);
                                                UA_Variant_setScalar(&vParityAttr.value, &_Parity, &UA_TYPES[UA_TYPES_STRING]);
                                        	UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 110),
                                                r2_modbus_RTU_Id,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                UA_QUALIFIEDNAME(namespaceIndex, "Parity"),
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                vParityAttr, NULL, &outParity);

                                        // ---- RTU->Stop bit
                                        UA_VariableAttributes vStopbitAttr = UA_VariableAttributes_default;
                                        vStopbitAttr.description = UA_LOCALIZEDTEXT("en-US", "Stopbit");
                                        vStopbitAttr.displayName = UA_LOCALIZEDTEXT("en-US", "4.6 Stop bit");
                                        vStopbitAttr.dataType = UA_TYPES[UA_TYPES_INT16].typeId;
                                        vStopbitAttr.valueRank = UA_VALUERANK_SCALAR;
                                        vStopbitAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                        vStopbitAttr.historizing = UA_FALSE;
                                        //UA_String ip = UA_STRING(ipaddress);
                                                UA_Variant_setScalar(&vStopbitAttr.value, &stop_bit, &UA_TYPES[UA_TYPES_INT16]);
                                        	UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 111),
                                                r2_modbus_RTU_Id,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                UA_QUALIFIEDNAME(namespaceIndex, "Stopbit"),
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                vStopbitAttr, NULL, &outStopbit);


		return mb;
	}
  }
}

modbus_t *initialiseModbusConnectionToSlaveTCP(UA_Server *uaServer, size_t namespaceIndex, UA_NodeId r1_modbusroot_Id, char* protocol, char* ipaddress, short port, int slave_id)
{

  /* As a modbus TCP Master, open62541 will read from an external Modbus Slave and update its nodes */

  modbus_t *mb=NULL;

#ifdef DEBUG
printf("data passed in is %s %d %d \n", ipaddress, port, slave_id);
#endif

  mb = modbus_new_tcp(ipaddress, port);
  if (mb == NULL)
  {
	printf("Unable to allocate libmodbus context : %s \n", modbus_strerror(errno));
	return NULL;
  }
  else
  {
	int retval;
	retval = modbus_connect(mb);
#ifdef DEBUG
printf("modbus_connect(mb) return value is %d \n", retval);
#endif
  	if (retval == -1)
	{
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Error connecting to modbus slave endpoint %s:%d : %s", ipaddress, port, modbus_strerror(errno));
		modbus_free(mb);
		return NULL;
	}
	else
	{
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Succesfully connected to modbus slave endpoint %s:%d, slave_id:%d : %s", ipaddress, port, slave_id, modbus_strerror(errno));
		modbus_set_slave(mb, rtu_slave_id);


				// Add objectnode to represent Modbus->TCP
				UA_NodeId r2_modbus_TCP_Id;
				UA_ObjectAttributes oAttr_r2_modbus_TCP = UA_ObjectAttributes_default;
				oAttr_r2_modbus_TCP.description = UA_LOCALIZEDTEXT("en-US", "TCP");
				oAttr_r2_modbus_TCP.displayName = UA_LOCALIZEDTEXT("en-US", "3. TCP");
                               	retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "Modbus_TCP_Info"),
					r1_modbusroot_Id,
					UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
					UA_QUALIFIEDNAME(namespaceIndex, "TCP"),
					UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                    oAttr_r2_modbus_TCP, NULL, &r2_modbus_TCP_Id);                           // attr, nodeContext, outNewNodeId assigned by the server

					// variable nodes : IP, port, ConnectionTimeOut
					// ---- TCP->IP
	                                UA_VariableAttributes vIPAttr = UA_VariableAttributes_default;
        	                        vIPAttr.description = UA_LOCALIZEDTEXT("en-US", "IP");
                	                vIPAttr.displayName = UA_LOCALIZEDTEXT("en-US", "3.1 IP");
                        	        vIPAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
                                	vIPAttr.valueRank = UA_VALUERANK_SCALAR;
	                                vIPAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
        	                        vIPAttr.historizing = UA_FALSE;
                	                UA_String ip = UA_STRING(ipaddress);
                        	                UA_Variant_setScalar(&vIPAttr.value, &ip, &UA_TYPES[UA_TYPES_STRING]);
                                	retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 103),
                                                r2_modbus_TCP_Id,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                UA_QUALIFIEDNAME(namespaceIndex, "IP"),
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                vIPAttr, NULL, &outIP);

					// ---- TCP->Port
	                                /*char *_Port;
        	                        _Port = (char *)calloc(10, sizeof(char));
                	                if (_Port == NULL)
                        	        {
                                	        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                        	        "--------SV_Modbus.c : Error allocating memory for Port");
	                                        exit(UA_FALSE);
        	                        }
                	                else
                        	        {*/
	                                        UA_VariableAttributes vPortAttr = UA_VariableAttributes_default;
        	                                vPortAttr.description = UA_LOCALIZEDTEXT("en-US", "Port");
                	                        vPortAttr.displayName = UA_LOCALIZEDTEXT("en-US", "3.2 Port");
                        	                vPortAttr.dataType = UA_TYPES[UA_TYPES_INT16].typeId;
                                	        vPortAttr.valueRank = UA_VALUERANK_SCALAR;
                                        	vPortAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
	                                        vPortAttr.historizing = UA_FALSE;
	                                        //sprintf(_Port, "%hd", port);
        	                                //UA_String Port = UA_STRING(_Port);
                	                                UA_Variant_setScalar(&vPortAttr.value, &port, &UA_TYPES[UA_TYPES_INT16]);
                        	                retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 104),
                                	                r2_modbus_TCP_Id,
                                        	        UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                	UA_QUALIFIEDNAME(namespaceIndex, "Port"),
	                                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
        	                                        vPortAttr, NULL, &outPort);
					//}
		//#ifdef ERROR_HERE
					// ---- ConnectionTimeOut
					/*
                                        char *_ConnectionTimeOut; // 1 to 3600 seconds
                                        _ConnectionTimeOut = (char *)calloc(10, sizeof(char));
                                        if (_ConnectionTimeOut == NULL)
                                        {
                                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                        "--------SV_Modbus.c : Error allocating memory for ConnectionTimeOut");
                                                exit(UA_FALSE);
                                        }
                                        else
                                        {*/
				                char* env_MBTimeout = getenv("SVR_MB_TIMEOUT");
                				if (env_MBTimeout != NULL)
                        				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : retrieved environment variable : SVR_MB_TIMEOUT : %s", env_MBTimeout);
                				else
                				{
                        				UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MB_TIMEOUT : %s>", env_MBTimeout);
                        				UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : default to 3600");
                        				env_MBTimeout = (char*)calloc(10, sizeof(char));
                        				if (env_MBTimeout != NULL)
                                				strcpy(env_MBTimeout, "3600");
                        				else
                        				{
                                				UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_BAUD> : out of memory");
                                				return UA_FALSE;
                        				}
                				}

                                                UA_VariableAttributes vTimeoutAttr = UA_VariableAttributes_default;
                                                vTimeoutAttr.description = UA_LOCALIZEDTEXT("en-US", "ConnectionTimeout");
                                                vTimeoutAttr.displayName = UA_LOCALIZEDTEXT("en-US", "3.3 Connection Time Out");
                                                vTimeoutAttr.dataType = UA_TYPES[UA_TYPES_INT16].typeId;
                                                vTimeoutAttr.valueRank = UA_VALUERANK_SCALAR;
                                                vTimeoutAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                                vTimeoutAttr.historizing = UA_FALSE;
                                                //sprintf(_Timeout, "%hd", port);
                                                short timeOut = atoi(env_MBTimeout);
                                                        UA_Variant_setScalar(&vTimeoutAttr.value, &timeOut, &UA_TYPES[UA_TYPES_INT16]);
                                                retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 105),
                                                        r2_modbus_TCP_Id,
                                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                        UA_QUALIFIEDNAME(namespaceIndex, "ConnectionTimeout"),
                                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                        vTimeoutAttr, NULL, &outTimeout);
                                       // }
		//#endif

				// add an object node to represent Modbus-TCP->DATA
                                UA_NodeId r3_modbus_TCP_DATA_Id;
                                UA_ObjectAttributes oAttr_r3_modbus_TCP_DATA = UA_ObjectAttributes_default;
                                oAttr_r3_modbus_TCP_DATA.description = UA_LOCALIZEDTEXT("en-US", "Data");
                                oAttr_r3_modbus_TCP_DATA.displayName = UA_LOCALIZEDTEXT("en-US", "3.4. Data");
                                retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "Modbus_TCP_DATA_Info"),
                                        r2_modbus_TCP_Id,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                        UA_QUALIFIEDNAME(namespaceIndex, "TCP_DATA"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                    oAttr_r3_modbus_TCP_DATA, NULL, &r3_modbus_TCP_DATA_Id);                           // attr, nodeContext, outNewNodeId assigned by the server

				// add an object node to represent Modbus-TCP->DATA->FC1
                                //UA_NodeId r4_modbus_TCP_DATA_FC1_Id; - move to top as global variable
                                UA_ObjectAttributes oAttr_r4_modbus_TCP_DATA_FC1 = UA_ObjectAttributes_default;
                                oAttr_r4_modbus_TCP_DATA_FC1.description = UA_LOCALIZEDTEXT("en-US", "FC1");
                                oAttr_r4_modbus_TCP_DATA_FC1.displayName = UA_LOCALIZEDTEXT("en-US", "3.4.1. FC1 (Read coils)");
                                retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "Modbus_TCP_DATA_FC1_Info"),
                                        r3_modbus_TCP_DATA_Id,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                        UA_QUALIFIEDNAME(namespaceIndex, "TCP_DATA_FC1"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                    oAttr_r4_modbus_TCP_DATA_FC1, NULL, &r4_modbus_TCP_DATA_FC1_Id);                           // attr, nodeContext, outNewNodeId assigned by the server

					// ----------------remaining variable nodes created separately in readModbusTCPData()

                                // add an object node to represent Modbus-TCP->DATA->FC2
                                //UA_NodeId r4_modbus_TCP_DATA_FC2_Id; - move to top as global variable
                                UA_ObjectAttributes oAttr_r4_modbus_TCP_DATA_FC2 = UA_ObjectAttributes_default;
                                oAttr_r4_modbus_TCP_DATA_FC2.description = UA_LOCALIZEDTEXT("en-US", "FC2");
                                oAttr_r4_modbus_TCP_DATA_FC2.displayName = UA_LOCALIZEDTEXT("en-US", "3.4.2. FC2 (Read discrete inputs)");
                                retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "Modbus_TCP_DATA_FC2_Info"),
                                        r3_modbus_TCP_DATA_Id,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                        UA_QUALIFIEDNAME(namespaceIndex, "TCP_DATA_FC2"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                    oAttr_r4_modbus_TCP_DATA_FC2, NULL, &r4_modbus_TCP_DATA_FC2_Id);                           // attr, nodeContext, outNewNodeId assigned by the server

					// ----------------remaining variable nodes created separately in readModbusTCPData()

                                // add an object node to represent Modbus-TCP->DATA->FC3
                                //UA_NodeId r4_modbus_TCP_DATA_FC3_Id; - move to top as global variable
                                UA_ObjectAttributes oAttr_r4_modbus_TCP_DATA_FC3 = UA_ObjectAttributes_default;
                                oAttr_r4_modbus_TCP_DATA_FC3.description = UA_LOCALIZEDTEXT("en-US", "FC3");
                                oAttr_r4_modbus_TCP_DATA_FC3.displayName = UA_LOCALIZEDTEXT("en-US", "3.4.3. FC3 (Read holding registers)");
                                retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "Modbus_TCP_DATA_FC3_Info"),
                                        r3_modbus_TCP_DATA_Id,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                        UA_QUALIFIEDNAME(namespaceIndex, "TCP_DATA_FC3"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                    oAttr_r4_modbus_TCP_DATA_FC3, NULL, &r4_modbus_TCP_DATA_FC3_Id);                           // attr, nodeContext, outNewNodeId assigned by the server

					// ----------------remaining variable nodes created separately in readModbusTCPData()

                                // add an object node to represent Modbus-TCP->DATA->FC4
                                //UA_NodeId r4_modbus_TCP_DATA_FC4_Id; - move to top as global variable
                                UA_ObjectAttributes oAttr_r4_modbus_TCP_DATA_FC4 = UA_ObjectAttributes_default;
                                oAttr_r4_modbus_TCP_DATA_FC4.description = UA_LOCALIZEDTEXT("en-US", "FC4");
                                oAttr_r4_modbus_TCP_DATA_FC4.displayName = UA_LOCALIZEDTEXT("en-US", "3.4.4. FC4 (Read input registers)");
                                retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "Modbus_TCP_DATA_FC4_Info"),
                                        r3_modbus_TCP_DATA_Id,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                        UA_QUALIFIEDNAME(namespaceIndex, "TCP_DATA_FC4"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                    oAttr_r4_modbus_TCP_DATA_FC4, NULL, &r4_modbus_TCP_DATA_FC4_Id);                           // attr, nodeContext, outNewNodeId assigned by the server

					// ----------------remaining variable nodes created separately in readModbusTCPData()
  		return mb;
	}

  }
}

modbus_t *initialiseModbusConnectionToSlaveASCII(UA_Server *uaServer, size_t namespaceIndex, UA_NodeId r1_modbusroot_Id)
{
  modbus_t *mb=NULL;


				// Add objectnode to represent Modbus->ASCII
                                UA_NodeId r2_modbus_ASCII_Id;
                                UA_ObjectAttributes oAttr_r2_modbus_ASCII = UA_ObjectAttributes_default;
                                oAttr_r2_modbus_ASCII.description = UA_LOCALIZEDTEXT("en-US", "ASCII");
                                oAttr_r2_modbus_ASCII.displayName = UA_LOCALIZEDTEXT("en-US", "ASCII");
                                UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "Modbus_ASCII_Info"),
					r1_modbusroot_Id,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                        UA_QUALIFIEDNAME(namespaceIndex, "ASCII"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                    oAttr_r2_modbus_ASCII, NULL, &r2_modbus_ASCII_Id);                           // attr, nodeContext, outNewNodeId assigned by the server


	return (mb);
}

modbus_t *initialiseModbusConnectionToSlaveUDP(UA_Server *uaServer, size_t namespaceIndex, UA_NodeId r1_modbusroot_Id)
{
  modbus_t *mb=NULL;


				// Add objectnode to represent Modbus->UDP
                                UA_NodeId r2_modbus_UDP_Id;
                                UA_ObjectAttributes oAttr_r2_modbus_UDP = UA_ObjectAttributes_default;
                                oAttr_r2_modbus_UDP.description = UA_LOCALIZEDTEXT("en-US", "UDP");
                                oAttr_r2_modbus_UDP.displayName = UA_LOCALIZEDTEXT("en-US", "UDP");
                                UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "Modbus_UDP_Info"),
					r1_modbusroot_Id,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                        UA_QUALIFIEDNAME(namespaceIndex, "UDP"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                    oAttr_r2_modbus_UDP, NULL, &r2_modbus_UDP_Id);

	return (mb);
}

modbus_t *initialiseModbusConnectionToSlaveRTUoverTCP(UA_Server *uaServer, size_t namespaceIndex, UA_NodeId r1_modbusroot_Id)
{
  modbus_t *mb=NULL;

				// Add objectnode to represent Modbus->RTUOverTCP
                                UA_NodeId r2_modbus_RTUoverTCP_Id;
                                UA_ObjectAttributes oAttr_r2_modbus_RTUoverTCP = UA_ObjectAttributes_default;
                                oAttr_r2_modbus_RTUoverTCP.description = UA_LOCALIZEDTEXT("en-US", "RTUoverTCP");
                                oAttr_r2_modbus_RTUoverTCP.displayName = UA_LOCALIZEDTEXT("en-US", "RTUoverTCP");
                                UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, "Modbus_RTUoverTCP_Info"),
					r1_modbusroot_Id,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASSUBTYPE),
                                        UA_QUALIFIEDNAME(namespaceIndex, "RTUoverTCP"),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),
                                    oAttr_r2_modbus_RTUoverTCP, NULL, &r2_modbus_RTUoverTCP_Id);

	return (mb);
}


char *env_MBStartFC1AddrTCP, *env_MBStartFC2AddrTCP, *env_MBStartFC3AddrTCP, *env_MBStartFC4AddrTCP;
int firstTimeAddModbusToUAServer = UA_TRUE;
UA_Byte *FC1tab_reg=NULL;	// uint8_t read modbus coils
UA_Byte *FC2tab_reg=NULL; 	// uint8_t read modbus discrete inputs
UA_UInt16 *FC3tab_reg=NULL;	// uint16_t read modbus holding registers
UA_UInt16 *FC4tab_reg=NULL;	// uint16_t read modbus input registers
char *env_MBNoOfFC1RegisterTCP, *env_MBNoOfFC2RegisterTCP, *env_MBNoOfFC3RegisterTCP, *env_MBNoOfFC4RegisterTCP;
UA_Variant arrayVariantFC1, arrayVariantFC2, arrayVariantFC3, arrayVariantFC4;
UA_NodeId arrayNodeIdFC1, arrayNodeIdFC2, arrayNodeIdFC3, arrayNodeIdFC4;

int readModbusTCPData(UA_Server *uaServer, modbus_t *mb)
{
#define TEST_WITHOUT_SOURCE

  if (mb)
  {
	if (firstTimeAddModbusToUAServer == UA_TRUE)
	{
		firstTimeAddModbusToUAServer = UA_FALSE;

	//-- FC1
	        env_MBNoOfFC1RegisterTCP = getenv("SVR_MB_NUMFC1REGISTERTCP");
        	if (env_MBNoOfFC1RegisterTCP != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c :  retrieved environment variable : SVR_MB_NUMFC1REGISTERTCP : %s", env_MBNoOfFC1RegisterTCP);
	        else
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MB_NUMFC1REGISTERTCP : %s>", env_MBNoOfFC1RegisterTCP);
	                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : default to 32");
	                env_MBNoOfFC1RegisterTCP = (char*)calloc(32, sizeof(char));
        	        if (env_MBNoOfFC1RegisterTCP != NULL)
                	        strcpy(env_MBNoOfFC1RegisterTCP, "32");
	                else
        	        {
                	        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MB_NUMFC3REGISTERTCP> :  out of memory");
	                        return UA_FALSE;
        	        }
        	}

	        env_MBStartFC1AddrTCP = getenv("SVR_MB_STARTFC1ADDRTCP");
        	if (env_MBStartFC1AddrTCP != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c :  retrieved environment variable : SVR_MB_STARTFC1ADDRTCP : %s", env_MBStartFC1AddrTCP);
	        else
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MB_STARTFC1ADDRTCP : %s>", env_MBStartFC1AddrTCP);
	                env_MBStartFC1AddrTCP = (char*)calloc(10, sizeof(char));
        	        if (env_MBStartFC1AddrTCP != NULL)
			{
                        	strcpy(env_MBStartFC1AddrTCP, "1"); // if set to 0, crash: modbus slave address will be 1
		                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : default to %s", env_MBStartFC1AddrTCP);
			}
                	else
	                {
        	                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MB_STARTFC1ADDRTCP> :  out of memory");
                	        return UA_FALSE;
	                }
	        }

	//--- FC2
	        env_MBNoOfFC2RegisterTCP = getenv("SVR_MB_NUMFC2REGISTERTCP");
        	if (env_MBNoOfFC2RegisterTCP != NULL)
	                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c :  retrieved environment variable : SVR_MB_NUMFC2REGISTERTCP : %s", env_MBNoOfFC2RegisterTCP);
        	else
	        {
        	        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MB_NUMFC2REGISTERTCP : %s>", env_MBNoOfFC2RegisterTCP);
                	env_MBNoOfFC2RegisterTCP = (char*)calloc(32, sizeof(char));
	                if (env_MBNoOfFC2RegisterTCP != NULL)
			{
                	        strcpy(env_MBNoOfFC2RegisterTCP, "32");
		                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : default to %s", env_MBNoOfFC2RegisterTCP);
			}
                	else
	                {
        	                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MB_NUMFC2REGISTERTCP> :  out of memory");
                	        return UA_FALSE;
                	}
        	}

	        env_MBStartFC2AddrTCP = getenv("SVR_MB_STARTFC2ADDRTCP");
        	if (env_MBStartFC2AddrTCP != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c :  retrieved environment variable : SVR_MB_STARTFC2ADDRTCP : %s", env_MBStartFC2AddrTCP);
	        else
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MB_STARTFC2ADDRTCP : %s>", env_MBStartFC2AddrTCP);
	                env_MBStartFC2AddrTCP = (char*)calloc(10, sizeof(char));
        	        if (env_MBStartFC2AddrTCP != NULL)
			{
                        	strcpy(env_MBStartFC2AddrTCP, "10001"); // modbus slave address will be 10001
		                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : default to %s", env_MBStartFC2AddrTCP);
			}
                	else
	                {
        	                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MB_STARTFC2ADDRTCP> :  out of memory");
                	        return UA_FALSE;
                	}
        	}

	//--- FC3
	        env_MBNoOfFC3RegisterTCP = getenv("SVR_MB_NUMFC3REGISTERTCP");
        	if (env_MBNoOfFC3RegisterTCP != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c :  retrieved environment variable : SVR_MB_NUMFC3REGISTERTCP : %s", env_MBNoOfFC3RegisterTCP);
	        else
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MB_NUMFC3REGISTERTCP : %s>", env_MBNoOfFC3RegisterTCP);
	                env_MBNoOfFC3RegisterTCP = (char*)calloc(32, sizeof(char));
        	        if (env_MBNoOfFC3RegisterTCP != NULL)
			{
                        	strcpy(env_MBNoOfFC3RegisterTCP, "32");
		                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : default to %s", env_MBNoOfFC3RegisterTCP);
			}
                	else
	                {
        	                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MB_NUMFC3REGISTERTCP> :  out of memory");
                	        return UA_FALSE;
	                }
        	}

		env_MBStartFC3AddrTCP = getenv("SVR_MB_STARTFC3ADDRTCP");
		if (env_MBStartFC3AddrTCP != NULL)
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c :  retrieved environment variable : SVR_MB_STARTFC3ADDRTCP : %s", env_MBStartFC3AddrTCP);
		else
		{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MB_STARTFC3ADDRTCP : %s>", env_MBStartFC3AddrTCP);
	                env_MBStartFC3AddrTCP = (char*)calloc(10, sizeof(char));
        	        if (env_MBStartFC3AddrTCP != NULL)
			{
                        	strcpy(env_MBStartFC3AddrTCP, "40001"); // modbus slave address will be 40001
		                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : default to %s", env_MBStartFC3AddrTCP);
			}
                	else
	                {
        	                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MB_STARTFC3ADDRTCP> :  out of memory");
                	        return UA_FALSE;
                	}
		}

	//--FC4
		env_MBNoOfFC4RegisterTCP = getenv("SVR_MB_NUMFC4REGISTERTCP");
        	if (env_MBNoOfFC4RegisterTCP != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c :  retrieved environment variable : SVR_MB_NUMFC4REGISTERTCP : %s", env_MBNoOfFC4RegisterTCP);
	        else
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MB_NUMFC4REGISTERTCP : %s>", env_MBNoOfFC4RegisterTCP);
	                env_MBNoOfFC4RegisterTCP = (char*)calloc(32, sizeof(char));
        	        if (env_MBNoOfFC4RegisterTCP != NULL)
			{
                        	strcpy(env_MBNoOfFC4RegisterTCP, "32");
		                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : default to %s", env_MBNoOfFC4RegisterTCP);
			}
                	else
	                {
        	                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MB_NUMFC4REGISTERTCP> :  out of memory");
                	        return UA_FALSE;
	                }
        	}

	        env_MBStartFC4AddrTCP = getenv("SVR_MB_STARTFC4ADDRTCP");
        	if (env_MBStartFC4AddrTCP != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c :  retrieved environment variable : SVR_MB_STARTFC4ADDRTCP : %s", env_MBStartFC4AddrTCP);
	        else
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MB_STARTFC4ADDRTCP : %s>", env_MBStartFC4AddrTCP);
	                env_MBStartFC4AddrTCP = (char*)calloc(10, sizeof(char));
        	        if (env_MBStartFC4AddrTCP != NULL)
			{
                        	strcpy(env_MBStartFC4AddrTCP, "30001"); // modbus slave address will be 30001
		                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : default to %s", env_MBStartFC4AddrTCP);
			}
                	else
                	{
                        	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MB_STARTFC4ADDRTCP> :  out of memory");
	                        return UA_FALSE;
        	        }
        	}


	// ---Modbus function code 0x01 (read output coils) - FC1 {201, 202}
	#ifdef DEBUG
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : FC1 : Start register %d, No Of Registers %d", atoi(env_MBStartFC1AddrTCP), atoi(env_MBNoOfFC1RegisterTCP));
        #endif
        #ifdef TEST_WITHOUT_SOURCE
	        FC1tab_reg = calloc(atoi(env_MBNoOfFC1RegisterTCP), sizeof(uint8_t));
        	if (!FC1tab_reg)
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot allocate memory for FC1 (discrete output coils)");
	                exit(UA_FALSE);
        	}
       		modbus_read_bits(mb, atoi(env_MBStartFC1AddrTCP), atoi(env_MBNoOfFC1RegisterTCP), FC1tab_reg);
	        //for (int i=atoi(env_MBStartFC1AddrTCP); i< atoi(env_MBNoOfFC1RegisterTCP); i++)
	        for (int i=0; i< atoi(env_MBNoOfFC1RegisterTCP); i++)
               		printf("FC1 register[%d] = %d \n", i, FC1tab_reg[i]);
		//UA_free(FC1tab_reg);
        #endif

		// ---Modbus function code 0x02 (read discrete inputs) - FC2 {203, 204}
	#ifdef DEBUG
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : FC2 : Start register %d, No Of Registers %d", atoi(env_MBStartFC2AddrTCP), atoi(env_MBNoOfFC2RegisterTCP));
        #endif
	#ifdef TEST_WITHOUT_SOURCE
	        FC2tab_reg = calloc(atoi(env_MBNoOfFC2RegisterTCP), sizeof(uint8_t));
        	if (!FC2tab_reg)
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot allocate memory for FC2 (discrete inputs)");
	                exit(UA_FALSE);
        	}
	        modbus_read_bits(mb, atoi(env_MBStartFC2AddrTCP), atoi(env_MBNoOfFC2RegisterTCP), FC2tab_reg);
	        //for (int i=atoi(env_MBStartFC2AddrTCP); i< atoi(env_MBNoOfFC2RegisterTCP); i++)
	        for (int i=0; i< atoi(env_MBNoOfFC2RegisterTCP); i++)
        	       	printf("FC2 register[%d] = %d \n", i, FC2tab_reg[i]);
		//UA_free(FC2tab_reg);
        #endif

	// ---Modbus function code 0x03 (read multiple holding registers) - FC3 {205, 206}
	#ifdef DEBUG
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : FC3 : Start register %d, No Of Registers %d", atoi(env_MBStartFC3AddrTCP), atoi(env_MBNoOfFC3RegisterTCP));
	#endif
        #ifdef TEST_WITHOUT_SOURCE
		FC3tab_reg = calloc(atoi(env_MBNoOfFC3RegisterTCP), sizeof(uint16_t));
		if (!FC3tab_reg)
		{
			UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot allocate memory for FC3 (holding registers)");
			exit(UA_FALSE);
		}
  		modbus_read_registers(mb, atoi(env_MBStartFC3AddrTCP), atoi(env_MBNoOfFC3RegisterTCP), FC3tab_reg);
		//for (int i=atoi(env_MBStartFC3AddrTCP); i< atoi(env_MBNoOfFC3RegisterTCP); i++)
		for (int i=0; i< atoi(env_MBNoOfFC3RegisterTCP); i++)
			printf("FC3 register[%d] = %d \n", i, FC3tab_reg[i]);
		//UA_free(FC3tab_reg);
	#endif

	// ---Modbus function code 0x04 (read input registers) - FC4 {207, 208}
        #ifdef DEBUG
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : FC4 : Start register %d, No Of Registers %d", atoi(env_MBStartFC4AddrTCP), atoi(env_MBNoOfFC4RegisterTCP));
        #endif
        #ifdef TEST_WITHOUT_SOURCE
        	FC4tab_reg = calloc(atoi(env_MBNoOfFC4RegisterTCP), sizeof(uint16_t));
	        if (!FC4tab_reg)
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot allocate memory for FC4 (input registers)");
	                exit(UA_FALSE);
        	}
        	modbus_read_input_registers(mb, atoi(env_MBStartFC4AddrTCP), atoi(env_MBNoOfFC4RegisterTCP), FC4tab_reg);
	        //for (int i=atoi(env_MBStartFC4AddrTCP); i< atoi(env_MBNoOfFC4RegisterTCP); i++)
	        for (int i=0; i< atoi(env_MBNoOfFC4RegisterTCP); i++)
        	       	printf("FC4 register[%d] = %d \n", i, FC4tab_reg[i]);
		//UA_free(FC4tab_reg);
        #endif

		size_t namespaceIndex;
		int retval;

		UA_Server_getNamespaceByName(uaServer, UA_STRING("virtualskies.com.sg/MKS/"), &namespaceIndex);

		// add variable nodes under FC1->FC4, nodeid = {201, 202}, {203, 204}, {205, 206}, {207, 208}
		// variable nodes : ArrayData, ArraySize
                //-------------------------------------------------- ---- TCP->DATA->FC1 (array of boolean)
		// Function Code 1 - Read Coils (discrete Outputs)
		// follow the example in https://github.com/open62541/open62541/blob/master/examples/tutorial_server_variable.c
		// does not work; use example in google
		// refer to static void addMatrixVariable(UA_Server *server)

		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Adding FC1 ArrayData nodes");
		// -- 1st node - ArrayData
                UA_VariableAttributes vArrayDataFC1Attr = UA_VariableAttributes_default;
                vArrayDataFC1Attr.description = UA_LOCALIZEDTEXT("en-US", "FC1 ArrayData");
                vArrayDataFC1Attr.displayName = UA_LOCALIZEDTEXT("en-US", "3.4.1.1 FC1 ArrayData");

		/* set the variable value constraints */
                vArrayDataFC1Attr.dataType = UA_TYPES[UA_TYPES_BYTE].typeId;
                vArrayDataFC1Attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
		UA_UInt32 arrayFC1Dims[atoi(env_MBNoOfFC1RegisterTCP)];                   //arrayFC1Dims[1] = {1};
		vArrayDataFC1Attr.arrayDimensions = arrayFC1Dims;
		vArrayDataFC1Attr.arrayDimensionsSize = 1; //1;
                vArrayDataFC1Attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                vArrayDataFC1Attr.historizing = UA_TRUE;

			/* set the value.  The array dimensions need to be the same for the value */
			UA_Byte *arrayFC1Data;	// eg {true, false, true, false, true, false, ...}
			size_t arrayFC1Size = atoi(env_MBNoOfFC1RegisterTCP);	// eg 32
			arrayFC1Data = (UA_Byte *)UA_malloc(arrayFC1Size);
			if (!arrayFC1Data)
			{
				UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot allocate memory for arrayFC1 (coils)");
				return UA_FALSE;
			}
			else
			{	// initialise the COIL array to 0
				for (size_t index = 0; index < arrayFC1Size; index++)
					arrayFC1Data[index] = (UA_Byte)0;

				printf("arrayFC1Size is %ld \n", arrayFC1Size);
				//for (size_t index = 0; index < arrayFC1Size; index++)
				//	printf("arrayFC1Data[%lu] is %d \n", index, arrayFC1Data[index]);
			}

			// example in github - does not work
			//UA_Variant_setArray(&vArrayDataFC1Attr.value, arrayFC1Data, arrayFC1Size, &UA_TYPES[UA_TYPES_BOOLEAN]);
			//vArrayDataFC1Attr.value.arrayDimensions = arrayFC1Dims;
			//vArrayDataFC1Attr.value.arrayDimensionsSize = 2;
			/* vArrayDataFC1.value is left empty, the server instantiates with the default value */
			// following example from google
			//UA_Variant arrayVariantFC1;
			arrayNodeIdFC1 = UA_NODEID_NUMERIC(namespaceIndex, 201);
			UA_Variant_init(&arrayVariantFC1);
			arrayVariantFC1.type =  &UA_TYPES[UA_TYPES_BYTE];
			arrayVariantFC1.data = arrayFC1Data;
			arrayVariantFC1.arrayLength = arrayFC1Size;

	                UA_Server_addVariableNode(uaServer, arrayNodeIdFC1,
        	        	r4_modbus_TCP_DATA_FC1_Id,
                		UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                UA_QUALIFIEDNAME(namespaceIndex, "FC1_ArrayData"),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                vArrayDataFC1Attr, NULL, &outArrayDataFC1);

			retval = UA_Server_writeValue(uaServer, arrayNodeIdFC1, arrayVariantFC1);
			if (retval != UA_STATUSCODE_GOOD)
				UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Failure to initialise arrayFC1Data[] : %s ", UA_StatusCode_name(retval));
			else
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : UA_Server_writeValue() arrayNodeIdFC1 is successful");
		// end example


		//-- 2nd node - ArraySize
                UA_VariableAttributes vArraySizeFC1Attr = UA_VariableAttributes_default;
                vArraySizeFC1Attr.description = UA_LOCALIZEDTEXT("en-US", "FC1 ArraySize");
                vArraySizeFC1Attr.displayName = UA_LOCALIZEDTEXT("en-US", "3.4.1.2 FC1 ArraySize");
                vArraySizeFC1Attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;	// up to 64K registers
                vArraySizeFC1Attr.valueRank = UA_VALUERANK_SCALAR;
                vArraySizeFC1Attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                vArraySizeFC1Attr.historizing = UA_FALSE;
                UA_Variant_setScalar(&vArraySizeFC1Attr.value, &arrayFC1Size, &UA_TYPES[UA_TYPES_INT32]);
                UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 202),
                        r4_modbus_TCP_DATA_FC1_Id,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                  UA_QUALIFIEDNAME(namespaceIndex, "FC1_ArraySize"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  vArraySizeFC1Attr, NULL, &outArraySizeFC1);
		//UA_free(arrayFC1Data);

                //-------------------------------------------------- ---- TCP->DATA->FC2 (array of uint8_t)
		// Function code 2 - Read Discrete Inputs
                // follow the example in https://github.com/open62541/open62541/blob/master/examples/tutorial_server_variable.c
                // refer to static void addMatrixVariable(UA_Server *server)
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Adding FC2 ArrayData nodes");
                UA_VariableAttributes vArrayDataFC2Attr = UA_VariableAttributes_default;
                vArrayDataFC2Attr.description = UA_LOCALIZEDTEXT("en-US", "FC2 ArrayData");
                vArrayDataFC2Attr.displayName = UA_LOCALIZEDTEXT("en-US", "3.4.2.1 FC2 ArrayData");

                /* set the variable value constraints */
                vArrayDataFC2Attr.dataType = UA_TYPES[UA_TYPES_BYTE].typeId;
                vArrayDataFC2Attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
                UA_UInt32 arrayFC2Dims[atoi(env_MBNoOfFC2RegisterTCP)]; // = {1};
                vArrayDataFC2Attr.arrayDimensions = arrayFC2Dims;
                vArrayDataFC2Attr.arrayDimensionsSize = 1;
                vArrayDataFC2Attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                vArrayDataFC2Attr.historizing = UA_TRUE;

                        /* set the value.  The array dimensions need to be the same for the value */
                        UA_Byte *arrayFC2Data;       // eg {true, false, true, false, true, false, ...}
                        size_t arrayFC2Size = atoi(env_MBNoOfFC2RegisterTCP);   // eg 32
                        arrayFC2Data = (UA_Byte *)UA_malloc(arrayFC2Size);
                        if (!arrayFC2Data)
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot allocate memory for arrayFC2 (discrete inputs)");
                                return UA_FALSE;
                        }
                        else
                        {       // initialise the DISCRETE Inputs array to FALSE
                                for (size_t index = 0; index < arrayFC2Size; index++)
                                        arrayFC2Data[index] = (UA_Byte)0; //UA_FALSE;

                                printf("arrayFC2Size is %ld \n", arrayFC2Size);
                                //for (size_t index = 0; index < arrayFC2Size; index++)
                                //        printf("arrayFC2Data[%lu] is %d \n", index, arrayFC2Data[index]);
                        }

			// following example from github- does not work
                        //UA_Variant_setArray(&vArrayDataFC2Attr.value, arrayFC2Data, arrayFC2Size, &UA_TYPES[UA_TYPES_BOOLEAN]);
                        //vArrayDataFC2Attr.value.arrayDimensions = arrayFC2Dims;
                        //vArrayDataFC2Attr.value.arrayDimensionsSize = 2;
                        // following example from google
                        //UA_Variant arrayVariantFC2;
                        arrayNodeIdFC2 = UA_NODEID_NUMERIC(namespaceIndex, 203);
                        UA_Variant_init(&arrayVariantFC2);
                        arrayVariantFC2.type =  &UA_TYPES[UA_TYPES_BYTE];
                        arrayVariantFC2.data = arrayFC2Data;
                        arrayVariantFC2.arrayLength = arrayFC2Size;
                // end example

                	/* vAttr.value is left empty, the server instantiates with the default value */
                	UA_Server_addVariableNode(uaServer, arrayNodeIdFC2,
                        	r4_modbus_TCP_DATA_FC2_Id,
                        	UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                UA_QUALIFIEDNAME(namespaceIndex, "FC2_ArrayData"),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                vArrayDataFC2Attr, NULL, &outArrayDataFC2);

                        retval = UA_Server_writeValue(uaServer, arrayNodeIdFC2, arrayVariantFC2);
                        if (retval != UA_STATUSCODE_GOOD)
                                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Failure to initialise arrayFC2Data[] : %s ", UA_StatusCode_name(retval));
                        else
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : UA_Server_writeValue() arrayNodeIdFC2 is successful");

		//-- 2nd node - ArraySize
                UA_VariableAttributes vArraySizeFC2Attr = UA_VariableAttributes_default;
                vArraySizeFC2Attr.description = UA_LOCALIZEDTEXT("en-US", "FC2 ArraySize");
                vArraySizeFC2Attr.displayName = UA_LOCALIZEDTEXT("en-US", "3.4.2.2 FC2 ArraySize");
                vArraySizeFC2Attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;   // up to 64K registers
                vArraySizeFC2Attr.valueRank = UA_VALUERANK_SCALAR;
                vArraySizeFC2Attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                vArraySizeFC2Attr.historizing = UA_FALSE;
                UA_Variant_setScalar(&vArraySizeFC2Attr.value, &arrayFC2Size, &UA_TYPES[UA_TYPES_INT32]);
                UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 204),
                        r4_modbus_TCP_DATA_FC2_Id,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                  UA_QUALIFIEDNAME(namespaceIndex, "FC2_ArraySize"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  vArraySizeFC2Attr, NULL, &outArraySizeFC2);
		//UA_free(arrayFC2Data);

                //-------------------------------------------------- ---- TCP->DATA->FC3 (array of WORD i.e. 2 BYTES per register => int16_t)
                // Function code 3 - Read Holding Registers
                // follow the example in https://github.com/open62541/open62541/blob/master/examples/tutorial_server_variable.c
                // refer to static void addMatrixVariable(UA_Server *server)

		// Data Format:
		// The register values in the response message are packed as two bytes per register, with the first byte containing the high-order bits and the second byte containing the low-order bits.
		// variable nodes : ArrayData, ArraySize
                // ---- TCP->DATA->FC3 (array of int16_t)
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Adding FC3 ArrayData nodes");
                UA_VariableAttributes vArrayDataFC3Attr = UA_VariableAttributes_default;
                vArrayDataFC3Attr.description = UA_LOCALIZEDTEXT("en-US", "FC3 ArrayData");
                vArrayDataFC3Attr.displayName = UA_LOCALIZEDTEXT("en-US", "3.4.3.1 FC3 ArrayData");

                /* set the variable value constraints */
                vArrayDataFC3Attr.dataType = UA_TYPES[UA_TYPES_UINT16].typeId;
                vArrayDataFC3Attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
                UA_UInt32 arrayFC3Dims[atoi(env_MBNoOfFC3RegisterTCP)]; // = {1};
                vArrayDataFC3Attr.arrayDimensions = arrayFC3Dims;
                vArrayDataFC3Attr.arrayDimensionsSize = 1;
                vArrayDataFC3Attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                vArrayDataFC3Attr.historizing = UA_TRUE;

                        /* set the value.  The array dimensions need to be the same for the value */
                        UA_UInt16 *arrayFC3Data;       // eg {true, false, true, false, true, false, ...}
                        size_t arrayFC3Size = atoi(env_MBNoOfFC3RegisterTCP);   // eg 32
                        arrayFC3Data = (UA_UInt16 *)UA_malloc(arrayFC3Size);
                        if (!arrayFC3Data)
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot allocate memory for arrayFC3 (holding registers)");
                                return UA_FALSE;
                        }
                        else
                        {       // initialise the DISCRETE Inputs array to FALSE
                                for (size_t index = 0; index < arrayFC3Size; index++)
                                        arrayFC3Data[index] = (UA_UInt16)0;

                                printf("arrayFC3Size is %ld \n", arrayFC3Size);
                                //for (size_t index = 0; index < arrayFC3Size; index++)
                                //        printf("arrayFC3Data[%lu] is %d \n", index, (int16_t)arrayFC3Data[index]);
                        }

			// following example from github- does not work
                        //UA_Variant_setArray(&vArrayDataFC3Attr.value, arrayFC3Data, arrayFC3Size, &UA_TYPES[UA_TYPES_INT16]);
                        //vArrayDataFC3Attr.value.arrayDimensions = arrayFC3Dims;
                        //vArrayDataFC3Attr.value.arrayDimensionsSize = 2;
			// following example from google
                        //UA_Variant arrayVariantFC3;
                        arrayNodeIdFC3 = UA_NODEID_NUMERIC(namespaceIndex, 205);
                        UA_Variant_init(&arrayVariantFC3);
                        arrayVariantFC3.type =  &UA_TYPES[UA_TYPES_UINT16];
                        arrayVariantFC3.data = arrayFC3Data;
                        arrayVariantFC3.arrayLength = arrayFC3Size;
                // end example

			printf("here 1\n");
        	        /* vAttr.value is left empty, the server instantiates with the default value */
	                UA_Server_addVariableNode(uaServer, arrayNodeIdFC3,
                	          r4_modbus_TCP_DATA_FC3_Id,
                        	  UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                  UA_QUALIFIEDNAME(namespaceIndex, "FC3_ArrayData"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  vArrayDataFC3Attr, NULL, &outArrayDataFC3);
			printf("here 2\n");
                        retval = UA_Server_writeValue(uaServer, arrayNodeIdFC3, arrayVariantFC3);
                        if (retval != UA_STATUSCODE_GOOD)
                                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Failure to initialise arrayFC3Data[] : %s ", UA_StatusCode_name(retval));
                        else
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : UA_Server_writeValue() arrayNodeIdFC3 is successful");


		//-- 2nd node - ArraySize
                UA_VariableAttributes vArraySizeFC3Attr = UA_VariableAttributes_default;
                vArraySizeFC3Attr.description = UA_LOCALIZEDTEXT("en-US", "FC3 ArraySize");
                vArraySizeFC3Attr.displayName = UA_LOCALIZEDTEXT("en-US", "3.4.3.2 FC3 ArraySize");
                vArraySizeFC3Attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;   // up to 64K registers
                vArraySizeFC3Attr.valueRank = UA_VALUERANK_SCALAR;
                vArraySizeFC3Attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                vArraySizeFC3Attr.historizing = UA_FALSE;
                UA_Variant_setScalar(&vArraySizeFC3Attr.value, &arrayFC3Size, &UA_TYPES[UA_TYPES_INT32]);
                UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 206),
                        r4_modbus_TCP_DATA_FC3_Id,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                  UA_QUALIFIEDNAME(namespaceIndex, "FC3_ArraySize"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  vArraySizeFC3Attr, NULL, &outArraySizeFC3);
		//UA_free(arrayFC3Data);

                //-------------------------------------------------- ---- TCP->DATA->FC4 (array of WORD i.e. 2 BYTES per register => int16_t)
                // Function code 4 - Read Input Registers (readonly, so cannot use writeNode in OPCUA and expect it to update to Input registers
                // follow the example in https://github.com/open62541/open62541/blob/master/examples/tutorial_server_variable.c
                // refer to static void addMatrixVariable(UA_Server *server)

                // Data Format:
                // The register values in the response message are packed as two bytes per register, with the first byte containing the high-order bits and the second byte containing the low-order bits.
                // variable nodes : ArrayData, ArraySize
                // ---- TCP->DATA->FC3 (array of int16_t)

		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Adding FC4 ArrayData nodes");
                UA_VariableAttributes vArrayDataFC4Attr = UA_VariableAttributes_default;
                vArrayDataFC4Attr.description = UA_LOCALIZEDTEXT("en-US", "FC4 ArrayData");
                vArrayDataFC4Attr.displayName = UA_LOCALIZEDTEXT("en-US", "3.4.4.1 FC4 ArrayData");

                /* set the variable value constraints */
                vArrayDataFC4Attr.dataType = UA_TYPES[UA_TYPES_UINT16].typeId;
                vArrayDataFC4Attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
                UA_UInt32 arrayFC4Dims[atoi(env_MBNoOfFC4RegisterTCP)]; // = {1};
                vArrayDataFC4Attr.arrayDimensions = arrayFC4Dims;
                vArrayDataFC4Attr.arrayDimensionsSize = 1;
                vArrayDataFC4Attr.accessLevel = UA_ACCESSLEVELMASK_READ;
                vArrayDataFC4Attr.historizing = UA_TRUE;

                        /* set the value.  The array dimensions need to be the same for the value */
                        UA_UInt16 *arrayFC4Data;       // eg {true, false, true, false, true, false, ...}
                        size_t arrayFC4Size = atoi(env_MBNoOfFC4RegisterTCP);   // eg 32
                        arrayFC4Data = (UA_Int16 *)UA_malloc(arrayFC4Size);
                        if (!arrayFC4Data)
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot allocate memory for arrayFC4 (input registers)");
                                return UA_FALSE;
                        }
                        else
                        {       // initialise the DISCRETE Inputs array to FALSE
                                for (size_t index = 0; index < arrayFC4Size; index++)
                                        arrayFC4Data[index] = (UA_UInt16)0;

                                printf("arrayFC4Size is %ld \n", arrayFC4Size);
                                //for (size_t index = 0; index < arrayFC4Size; index++)
                                //        printf("arrayFC4Data[%lu] is %d \n", index, (int16_t)arrayFC4Data[index]);
                        }

			// following example from github- does not work
                        //UA_Variant_setArray(&vArrayDataFC4Attr.value, arrayFC4Data, arrayFC4Size, &UA_TYPES[UA_TYPES_INT16]);
                        //vArrayDataFC4Attr.value.arrayDimensions = arrayFC4Dims;
                        //vArrayDataFC4Attr.value.arrayDimensionsSize = 2;
                       	// following example from google
                        //UA_Variant arrayVariantFC4;
                        arrayNodeIdFC4 = UA_NODEID_NUMERIC(namespaceIndex, 207);
                        UA_Variant_init(&arrayVariantFC4);
                        arrayVariantFC4.type =  &UA_TYPES[UA_TYPES_INT16];
                        arrayVariantFC4.data = arrayFC4Data;
                        arrayVariantFC4.arrayLength = arrayFC4Size;

	                /* vAttr.value is left empty, the server instantiates with the default value */
        	        UA_Server_addVariableNode(uaServer, arrayNodeIdFC4,
                	        r4_modbus_TCP_DATA_FC4_Id,
                        	UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
				UA_QUALIFIEDNAME(namespaceIndex, "FC4_ArrayData"),
                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                vArrayDataFC4Attr, NULL, &outArrayDataFC4);

                        retval = UA_Server_writeValue(uaServer, arrayNodeIdFC4, arrayVariantFC4);
                        if (retval != UA_STATUSCODE_GOOD)
                                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Failure to initialise arrayFC4Data[] : %s ", UA_StatusCode_name(retval));
                        else
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : UA_Server_writeValue() arrayNodeIdFC4 is successful");

                //-- 2nd node - ArraySize
                UA_VariableAttributes vArraySizeFC4Attr = UA_VariableAttributes_default;
                vArraySizeFC4Attr.description = UA_LOCALIZEDTEXT("en-US", "FC4 ArraySize");
                vArraySizeFC4Attr.displayName = UA_LOCALIZEDTEXT("en-US", "3.4.4.2 FC4 ArraySize");
                vArraySizeFC4Attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;   // up to 64K registers
                vArraySizeFC4Attr.valueRank = UA_VALUERANK_SCALAR;
                vArraySizeFC4Attr.accessLevel = UA_ACCESSLEVELMASK_READ;
                vArraySizeFC4Attr.historizing = UA_FALSE;
                UA_Variant_setScalar(&vArraySizeFC4Attr.value, &arrayFC4Size, &UA_TYPES[UA_TYPES_INT32]);
                UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 208),
                        r4_modbus_TCP_DATA_FC4_Id,
                        UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                  UA_QUALIFIEDNAME(namespaceIndex, "FC4_ArraySize"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  vArraySizeFC4Attr, NULL, &outArraySizeFC4);
		//UA_free(arrayFC4Data);
	} // if (firstTimeAddModbusToUAServer == UA_TRUE)
	else
	{
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Entering the ELSE portion (firstTimeAddModbusToUAServer)");
		// just update the values to open62541->modbus nodes

		int retval;
		#ifdef DEBUG
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : inspecting variables : ");
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c :  %d %d",  atoi(env_MBStartFC1AddrTCP), atoi(env_MBNoOfFC1RegisterTCP));
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c :  %d %d",  atoi(env_MBStartFC2AddrTCP), atoi(env_MBNoOfFC2RegisterTCP));
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c :  %d %d",  atoi(env_MBStartFC3AddrTCP), atoi(env_MBNoOfFC3RegisterTCP));
	 	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c :  %d %d",  atoi(env_MBStartFC4AddrTCP), atoi(env_MBNoOfFC4RegisterTCP));
		#endif

		if (FC1tab_reg != NULL) // somehow this is NULL !!!
		{
                	modbus_read_bits(mb, atoi(env_MBStartFC1AddrTCP), atoi(env_MBNoOfFC1RegisterTCP), FC1tab_reg);
			arrayVariantFC1.data = FC1tab_reg;
	                retval = UA_Server_writeValue(uaServer, arrayNodeIdFC1, arrayVariantFC1);
        	        if (retval != UA_STATUSCODE_GOOD)
                	        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Failure to update arrayFC1Data[] : %s ", UA_StatusCode_name(retval));
                	else
                        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : UA_Server_writeValue() - Updating arrayNodeIdFC1 is successful");
		}
		if (FC2tab_reg != NULL) // somehow this is NULL !!!
		{
                	modbus_read_bits(mb, atoi(env_MBStartFC2AddrTCP), atoi(env_MBNoOfFC1RegisterTCP), FC2tab_reg);
			arrayVariantFC2.data = FC2tab_reg;
	                retval = UA_Server_writeValue(uaServer, arrayNodeIdFC2, arrayVariantFC2);
        	        if (retval != UA_STATUSCODE_GOOD)
                	        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Failure to update arrayFC2Data[] : %s ", UA_StatusCode_name(retval));
                	else
                        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : UA_Server_writeValue() - Updating arrayNodeIdFC2 is successful");
		}
		if (FC3tab_reg != NULL)
		{
                	modbus_read_registers(mb, atoi(env_MBStartFC3AddrTCP), atoi(env_MBNoOfFC1RegisterTCP), FC3tab_reg);
			arrayVariantFC3.data = FC3tab_reg;
			retval = UA_Server_writeValue(uaServer, arrayNodeIdFC3, arrayVariantFC3);
        	        if (retval != UA_STATUSCODE_GOOD)
                	        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Failure to update arrayFC3Data[] : %s ", UA_StatusCode_name(retval));
	                else
        	                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : UA_Server_writeValue() - Updating arrayNodeIdFC3 is successful");
		}
		if (FC4tab_reg != NULL)
		{
                	modbus_read_input_registers(mb, atoi(env_MBStartFC4AddrTCP), atoi(env_MBNoOfFC1RegisterTCP), FC4tab_reg);
			arrayVariantFC4.data = FC4tab_reg;
			retval = UA_Server_writeValue(uaServer, arrayNodeIdFC4, arrayVariantFC4);
        	        if (retval != UA_STATUSCODE_GOOD)
                		UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Failure to update arrayFC4Data[] : %s ", UA_StatusCode_name(retval));
	                else
        	                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : UA_Server_writeValue() - Updating arrayNodeIdFC4 is successful");
		}
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Successfully read modbus register from Slave");
	}

	return 0;
  } // if (mb)
  else return -1;
}

void closeModbusConnection(modbus_t *mb)
{
  modbus_close(mb);
  modbus_free(mb);
}

modbus_t *createModbusExtension(UA_Server *uaServer)
{
#ifdef COMMENT
				* add the 4 types of registers to open62541
				 * BaseObjectType
				 * |
				 * + (V) Slave Id - 101
				 * + (V) Protocol -102 i.e. TCP, RTU, ASCII, UDP, RTUoverTCP
				 * +-(OT) TCP
				 *    + (V) IP - 103
				 *    + (V) Port - 104
                                 *    + (V) Connection Timeout - 105
				 *    +----(OT) DATA
				 *    +-----+ (OT) FC1 (Read Coils)
				 *	       +-------- ArrayData - 201
				 *             +-------- ArraySize - 202
				 *    +-----+ (OT) FC2 (Read Discrete Inputs)
                                 *             +-------- ArrayData - 203
                                 *             +-------- ArraySize - 204
				 *    +-----+ (OT) FC3 (Read holding registers)
                                 *             +-------- ArrayData - 205
                                 *             +-------- ArraySize - 206
                                 *    +-----+ (OT) FC4 (Read input registers)
                                 *             +-------- ArrayData - 207
                                 *             +-------- ArraySize - 208
				 * +-(OT) RTU
				 *    + (V) Serial Type - 106 i.e 232, 485
				 *    + (V) Baud - 107
				 *    + (V) Databits - 108
				 *    + (V) Device - 109
				 *    + (V) Parity - 110
				 *    + (V) Stop bit - 111
				 *    + (V) Inter-frame tolerance (ms) - 112
 				 *    +----(OT) DATA
                                 *    +-----+ (OT) FC1 (Read Coils)
                                 *             +-------- ArrayData - 301
                                 *             +-------- ArraySize - 302
                                 *    +-----+ (OT) FC2 (Read Discrete Inputs)
                                 *             +-------- ArrayData - 303
                                 *             +-------- ArraySize - 304
                                 *    +-----+ (OT) FC3 (Read holding registers)
                                 *             +-------- ArrayData -305
                                 *             +-------- ArraySize - 306
                                 *    +-----+ (OT) FC4 (Read input registers)
                                 *             +-------- ArrayData - 307
                                 *             +-------- ArraySize - 308
				 * +-(OT) ASCII
                                 *    + (V) Serial Type i.e 232, 485 - 113
                                 *    + (V) Baud - 114
                                 *    + (V) Databits - 115
                                 *    + (V) Device - 116
                                 *    + (V) Parity - 117
                                 *    + (V) Stop bit - 118
				 *    +----(OT) DATA
                                 *    +-----+ (OT) FC1 (Read Coils)
                                 *             +-------- ArrayData - 401
                                 *             +-------- ArraySize - 402
                                 *    +-----+ (OT) FC2 (Read Discrete Inputs)
                                 *             +-------- ArrayData - 403
                                 *             +-------- ArraySize - 404
                                 *    +-----+ (OT) FC3 (Read holding registers)
                                 *             +-------- ArrayData - 405
                                 *             +-------- ArraySize - 406
                                 *    +-----+ (OT) FC4 (Read input registers)
                                 *             +-------- ArrayData - 407
                                 *             +-------- ArraySize - 408
				 * +-(OT) UDP
                                 *    + (V) Port - 119
				 *    + (V) Connection Timeout - 120
				 * +-(OT) RTUoverTCP
                                 *    + (V) Port - 121
                                 *    + (V) Connection Timeout - 122
				*
#endif

	//char ipaddress[100];
	modbus_t *mb=NULL;
	//short port;
	//char device[100], parity;
	//int baud, data_bit, stop_bit, slave_id;
	//int mode;

	char *env_MBProtocol, *env_MBIpV4, *env_MBPort, *env_MBSlaveId;
	char *env_MBDevice, *env_MBBaud, *env_MBSerialType,  *env_MBParity, *env_MBDataBit, *env_MBStopBit;

	// OPC Server acting as a Modbus Master

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : ==========================================================");
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : Entering createModbusExtension()");

        // add root node (r1_modbusroot_Id); to be reference in initialiseModbusConnectionToSlaveTCP(), initialiseModbusConnectionToSlaveRTU()
        size_t namespaceIndex;
        int retval;
        char sNameSpace[] = "ModbusMaster";
        char sNameSpaceEx[] = "virtualskies.com.sg/MKS/";
        UA_NodeId r1_modbusroot_Id;  /* get the nodeid assigned by the server */

        UA_LOG_INFO(UA_Log_Stdout,UA_LOGCATEGORY_USERLAND,
                  ("--------SV_Modbus.c : Begin the process of adding to OPC UA Tree structure"));
        UA_Server_getNamespaceByName(uaServer, UA_STRING(sNameSpaceEx), &namespaceIndex);
        // Add new object called Modbus in OPCUA tree
        UA_ObjectAttributes oAttr = UA_ObjectAttributes_default;
        retval = UA_Server_addObjectNode(uaServer, UA_NODEID_STRING(namespaceIndex, sNameSpace),
                UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),               // parentNodeId
                UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),                // referenceTypeId original UA_NS0ID_ORGANIZES
                UA_QUALIFIEDNAME(namespaceIndex, sNameSpace),               // variable
                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEOBJECTTYPE),              // typeDefinition this refers to the Object Type identifier
                oAttr, NULL, &r1_modbusroot_Id);                           // attr, nodeContext, outNewNodeId assigned by the server
        if (retval != UA_STATUSCODE_GOOD)
        {
         	UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                	"--------SV_Modbus.c : Error creating object node : Root Node ModbusMaster : %s", UA_StatusCode_name(retval));
	}
        else
        {
        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_Modbus.c : Root Node Modbus added successfully : %d", r1_modbusroot_Id.identifier.numeric);
        }

	// read the environment variables
	// common fields (SlaveId, Protocol)
	env_MBSlaveId = getenv("SVR_MB_SLAVEID");
                if (env_MBSlaveId != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : retrieved environment variable : SVR_MODBUS_SLAVEID : %s", env_MBSlaveId);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_SLAVEID : %s>", env_MBSlaveId);
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : default to 43");
                        env_MBSlaveId = (char*)calloc(15, sizeof(char));
                        if (env_MBSlaveId != NULL)
                                strcpy(env_MBSlaveId, "43");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_SLAVEID> :  out of memory");
                                return UA_FALSE;
                        }

				// variable
				// ----- Slave Id
				UA_VariableAttributes vSlaveIdAttr = UA_VariableAttributes_default;
				vSlaveIdAttr.description = UA_LOCALIZEDTEXT("en-US", "Slave Id");
				vSlaveIdAttr.displayName = UA_LOCALIZEDTEXT("en-US", "1. Slave Id");
				vSlaveIdAttr.dataType = UA_TYPES[UA_TYPES_INT16].typeId;
				vSlaveIdAttr.valueRank = UA_VALUERANK_SCALAR;
				vSlaveIdAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
				vSlaveIdAttr.historizing = UA_FALSE;

				/*
				char *_SlaveId;
				_SlaveId = (char *)calloc(10, sizeof(char));
				if (_SlaveId == NULL)
				{
					UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
						"--------SV_Modbus.c : Error allocating memory for SlaveId");
					exit(UA_FALSE);
				}
				else
				{*/
					//sprintf(_SlaveId, "%d", slave_id);
					//UA_String SlaveId = UA_STRING(_SlaveId);
					int slave_id = atoi(env_MBSlaveId);
						UA_Variant_setScalar(&vSlaveIdAttr.value, &slave_id, &UA_TYPES[UA_TYPES_INT16]);
					retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 101),
						r1_modbusroot_Id,
						UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
            					UA_QUALIFIEDNAME(namespaceIndex, "Slave Id"),
						UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
						vSlaveIdAttr, NULL, &outSlaveId);
					if (retval != UA_STATUSCODE_GOOD)
					   UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                				"--------SV_Modbus.c : Error creating node : Modbus->Slave Id (101): %s", UA_StatusCode_name(retval));
				        #ifdef DEBUG
	        			else
        	    			   UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                				"--------SV_Modbus.c : Attribute Variable created: Modbus->Slave Id (101) : %d", outSlaveId.identifier.numeric);
        				#endif
					//UA_free(_SlaveId);
				//}


                }

	env_MBProtocol = getenv("SVR_MB_PROTOCOL");
	if (env_MBProtocol != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : retrieved environment variable : SVR_MB_PROTOCOL : %s", env_MBProtocol);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MB_PROTOCOL : %s>", env_MBProtocol);
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : default to SERIAL");
                env_MBProtocol = (char*)calloc(15, sizeof(char));
                if (env_MBProtocol != NULL)
                        strcpy(env_MBProtocol, "TCP");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_PROTOCOL> :  out of memory");
                        return UA_FALSE;
                }

				// ---- Protocol
				UA_VariableAttributes vProtocolAttr = UA_VariableAttributes_default;
                                vProtocolAttr.description = UA_LOCALIZEDTEXT("en-US", "Protocol");
                                vProtocolAttr.displayName = UA_LOCALIZEDTEXT("en-US", "2. Protocol");
                                vProtocolAttr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
                                vProtocolAttr.valueRank = UA_VALUERANK_SCALAR;
                                vProtocolAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                vProtocolAttr.historizing = UA_FALSE;

				UA_String Protocol = UA_STRING(env_MBProtocol);
					UA_Variant_setScalar(&vProtocolAttr.value, &Protocol, &UA_TYPES[UA_TYPES_STRING]);
				retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 102),
                                                r1_modbusroot_Id,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                UA_QUALIFIEDNAME(namespaceIndex, "Protocol"),
						UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
						vProtocolAttr, NULL, &outProtocol);
				if (retval != UA_STATUSCODE_GOOD)
				   UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                "--------SV_Modbus.c : Error creating node : Modbus->Protocol (102): %s", UA_StatusCode_name(retval));
				#ifdef DEBUG
                                else
                                   UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                "--------SV_Modbus.c : Attribute Variable created: Modbus->Protocol (102) : %d", outProtocol.identifier.numeric);
                                #endif


        }

	// - check the protocol type i.e. TCP, SERIAL(RTU), ASCII, UDP, RTUoverTCP
	if (strcmp(env_MBProtocol, "TCP") == 0)
	{
		env_MBIpV4 = getenv("SVR_MB_IPV4");
		if (env_MBIpV4 != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : retrieved environment variable : SVR_MODBUS_IPV4 : %s", env_MBIpV4);
        	else
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_IPV4 : %s>", env_MBIpV4);
                	UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : default to 192.168.1.157");
                	env_MBIpV4 = (char*)calloc(50, sizeof(char));
                	if (env_MBIpV4 != NULL)
                        	strcpy(env_MBIpV4, "192.168.1.157");
                	else
                	{
                        	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_IPV4> :  out of memory");
                        	return UA_FALSE;
                	}
        	}

		env_MBPort = getenv("SVR_MB_PORT");
                if (env_MBPort != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : retrieved environment variable : SVR_MODBUS_PORT : %s", env_MBPort);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_PORT : %s>", env_MBPort);
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : default to 502");
                        env_MBPort = (char*)calloc(15, sizeof(char));
                        if (env_MBPort != NULL)
                                strcpy(env_MBPort, "502");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_PORT> :  out of memory");
                                return UA_FALSE;
                        }
                }

		//printf("data to pass into initialiseModbusConnectionToSlaveTCP() are %s, %d, %d", env_MBIpV4, (short)atoi(env_MBPort), atoi(env_MBSlaveId));
		mb = initialiseModbusConnectionToSlaveTCP(uaServer, namespaceIndex, r1_modbusroot_Id, env_MBProtocol, env_MBIpV4, (short)atoi(env_MBPort), atoi(env_MBSlaveId));
		//mb = initialiseModbusConnectionTCP("192.168.1.157", 502, 43);

	}
	else if (strcmp(env_MBProtocol, "SERIAL") == 0) // equivalent to Modbus RTU protocol
	{
		env_MBSerialType = getenv("SVR_MB_SERIAL_TYPE");
                if (env_MBSerialType != NULL)
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : retrieved environment variable : SVR_MODBUS_SERIAL_TYPE : %s", env_MBSerialType);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_SERIAL_TYPE : %s>", env_MBSerialType);
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : default to RS232");
                        env_MBSerialType = (char*)calloc(15, sizeof(char));
                        if (env_MBSerialType != NULL)
                                strcpy(env_MBSerialType, "232");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_SERIAL_TYPE> :  out of memory");
                                return UA_FALSE;
                        }
                }

		#ifdef NOT_NEEDED
		env_MBSlaveId = getenv("SVR_MB_SLAVEID");
		if (env_MBSlaveId != NULL)
	                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : retrieved environment variable : SVR_MODBUS_SLAVEID : %s", env_MBSlaveId);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_SLAVEID : %s>", env_MBSlaveId);
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : default to 43");
                        env_MBSlaveId = (char*)calloc(15, sizeof(char));
                        if (env_MBSlaveId != NULL)
                                strcpy(env_MBSlaveId, "43");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_SLAVEID> :  out of memory");
                                return UA_FALSE;
                        }

				// variable
				// ----- Slave Id
				UA_VariableAttributes vSlaveIdAttr = UA_VariableAttributes_default;
				vSlaveIdAttr.description = UA_LOCALIZEDTEXT("en-US", "Slave Id");
				vSlaveIdAttr.displayName = UA_LOCALIZEDTEXT("en-US", "1. Slave Id");
				vSlaveIdAttr.dataType = UA_TYPES[UA_TYPES_INT16].typeId;
				vSlaveIdAttr.valueRank = UA_VALUERANK_SCALAR;
				vSlaveIdAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
				vSlaveIdAttr.historizing = UA_FALSE;

				/*
				char *_SlaveId;
				_SlaveId = (char *)calloc(10, sizeof(char));
				if (_SlaveId == NULL)
				{
					UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
						"--------SV_Modbus.c : Error allocating memory for SlaveId");
					exit(UA_FALSE);
				}
				else
				{*/
					int slave_id = atoi(env_MBSlaveId);
					//sprintf(_SlaveId, "%d", slave_id);
					//UA_String SlaveId = UA_STRING(_SlaveId);
						UA_Variant_setScalar(&vSlaveIdAttr.value, &slave_id, &UA_TYPES[UA_TYPES_INT16]);
					retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 101),
						r1_modbusroot_Id,
						UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
            					UA_QUALIFIEDNAME(namespaceIndex, "Slave Id"),
						UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
						vSlaveIdAttr, NULL, &outSlaveId);
					if (retval != UA_STATUSCODE_GOOD)
					   UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                				"--------SV_Modbus.c : Error creating node : Modbus->Slave Id (101): %s", UA_StatusCode_name(retval));
				        #ifdef DEBUG
	        			else
        	    			   UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                				"--------SV_Modbus.c : Attribute Variable created: Modbus->Slave Id (101) : %d", outSlaveId.identifier.numeric);
        				#endif
					//UA_free(_SlaveId);
				//}

                }
		#endif

		env_MBDevice = getenv("SVR_MB_DEVICE");
		if (env_MBDevice != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : retrieved environment variable : SVR_MODBUS_DEVICE : %s", env_MBDevice);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_DEVICE : %s>", env_MBDevice);
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : default to /dev/ttyUSB0");
                        env_MBDevice = (char*)calloc(15, sizeof(char));
                        if (env_MBDevice != NULL)
                                strcpy(env_MBDevice, "/dev/ttyUSB0");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_DEVICE> : out of memory");
                                return UA_FALSE;
                        }
                }

		env_MBBaud = getenv("SVR_MB_BAUD");
                if (env_MBBaud != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : retrieved environment variable : SVR_MODBUS_BAUD : %s", env_MBBaud);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_BAUD : %s>", env_MBBaud);
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : default to 115200");
                        env_MBBaud = (char*)calloc(15, sizeof(char));
                        if (env_MBBaud != NULL)
                                strcpy(env_MBBaud, "115200");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_BAUD> : out of memory");
                                return UA_FALSE;
                        }
                }

		env_MBParity = getenv("SVR_MB_PARITY");
                if (env_MBParity != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : retrieved environment variable : SVR_MODBUS_PARITY : %s", env_MBParity);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_PARITY : %s>", env_MBParity);
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : default to N");
                        env_MBParity = (char*)calloc(15, sizeof(char));
                        if (env_MBParity != NULL)
                                strcpy(env_MBParity, "N");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_BAUD> : out of memory");
                                return UA_FALSE;
                        }
                }

		env_MBDataBit = getenv("SVR_MB_DATABIT");
                if (env_MBDataBit != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : retrieved environment variable : SVR_MODBUS_DATABIT : %s", env_MBDataBit);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_DATABIT : %s>", env_MBDataBit);
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : default to 8");
                        env_MBDataBit = (char*)calloc(15, sizeof(char));
                        if (env_MBDataBit != NULL)
                                strcpy(env_MBDataBit, "8");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_DATABIT> : out of memory");
                                return UA_FALSE;
                        }
                }

		env_MBStopBit = getenv("SVR_MB_STOP_BIT");
                if (env_MBStopBit != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : retrieved environment variable : SVR_MODBUS_STOPBIT : %s", env_MBStopBit);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_STOPBIT : %s>", env_MBStopBit);
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : default to 1");
                        env_MBStopBit = (char*)calloc(15, sizeof(char));
                        if (env_MBStopBit != NULL)
                                strcpy(env_MBStopBit, "1");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Modbus.c : cannot retrieve environment variable <SVR_MODBUS_STOPBIT> : out of memory");
                                return UA_FALSE;
                        }
                }

		printf("data to pass into initialiseModbusConnectionToSlaveRTU() are %s, %ud, %ld, %c, %d, %d, %d \n", env_MBDevice, (short)atoi(env_MBSerialType), atol(env_MBBaud),
				(char)env_MBParity[0], atoi(env_MBDataBit), atoi(env_MBStopBit), atoi(env_MBSlaveId));
		mb = initialiseModbusConnectionToSlaveRTU(uaServer, namespaceIndex, r1_modbusroot_Id, env_MBDevice, atoi(env_MBSerialType), atol(env_MBBaud),
				(char)env_MBParity[0], atoi(env_MBDataBit), atoi(env_MBStopBit), atoi(env_MBSlaveId));
	}
	else if (strcmp(env_MBProtocol, "ASCII") == 0)
	{
		mb = initialiseModbusConnectionToSlaveASCII(uaServer, namespaceIndex, r1_modbusroot_Id);
	}
	else if (strcmp(env_MBProtocol, "UDP") == 0)
	{
		mb = initialiseModbusConnectionToSlaveUDP(uaServer, namespaceIndex, r1_modbusroot_Id);
	}
	else if (strcmp(env_MBProtocol, "RTUoverTCP") == 0)
	{
		mb = initialiseModbusConnectionToSlaveRTUoverTCP(uaServer, namespaceIndex, r1_modbusroot_Id);
	}

	// OPC Server acting as a Modbus Slave
                // create a Modbus Slave thread that listens for modbus request at TCP port 1502
                pthread_t modbusSlaveThread;
                int ret = pthread_create(&modbusSlaveThread, NULL, initialiseModbusSlaveInstance, (void *)uaServer);
		printf("ret is %d \n", ret);

                if (ret != 0)
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "--------SV_main.c : Fail to create a Modbus Slave instance");
                else
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "--------SV_main.c : Modbus Slave instance started successfully");

		return mb; // return Modbus Master object back to createRepeatedCallbacks()
}

