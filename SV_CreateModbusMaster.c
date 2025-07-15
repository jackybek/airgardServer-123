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

modbus_t *initialiseModbusConnectionToSlaveTCP(UA_Server *, size_t, UA_NodeId, char*, char*, short, int);
modbus_t *initialiseModbusConnectionToSlaveRTU(UA_Server *, size_t, UA_NodeId, const char *device, int serialType, long baud, char parity, int data_bit, int stop_bit, int slave_id);
modbus_t *initialiseModbusConnectionToSlaveASCII(UA_Server *, size_t, UA_NodeId);
modbus_t *initialiseModbusConnectionToSlaveUDP(UA_Server *, size_t, UA_NodeId);
modbus_t *initialiseModbusConnectionToSlaveRTUoverTCP(UA_Server *, size_t, UA_NodeId);

extern const int NB_BITS; // = 16;
extern const int NB_INPUT_BITS; // = 16;
extern const int NB_REGISTERS; // = 16;
extern const int NB_INPUT_REGISTERS; // = 16;
extern const uint8_t UT_INPUT_BITS_TAB[]; // = {0xFF, 0xDB};

extern int tcp_slave_id;
extern int rtu_slave_id;

modbus_t *initialiseModbusConnectionToSlaveRTU(UA_Server *uaServer, size_t namespaceIndex, UA_NodeId r1_modbusroot_Id, const char *device, int serialType, long baud, char parity, int data_bit, int stop_bit, int slave_id)
{
#ifdef COMMENT
  refer to https://support.embeddedts.com/support/solutions/articles/22000230769-creating-a-libmodbus-slave-server-to-improve-halt-test-data-collection-
  As a modbus RTU Master, open62541 will read from an external Modbus Slave and update its nodes
#endif

  modbus_t *mb=NULL;
  int rc;

#ifdef DEBUG
printf("------------SV_CreateModbusMaster.c : data passed in is %s %d %ld %c %d %d %d \n", device, serialType, baud, parity, data_bit, stop_bit, slave_id);
#endif

  mb = modbus_new_rtu(device, baud, parity, data_bit, stop_bit);
  if (mb == NULL)
  {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbusMaster.c (RTU) : Unable to allocate libmodbus context : %s", modbus_strerror(errno));
        return NULL;
  }
  else
  {
	if (serialType == 232)
	{
        	if (modbus_rtu_set_serial_mode(mb, MODBUS_RTU_RS232) == -1)
		{
			UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbusMaster.c (RTU) : cannot set mode to RS232 : %s", modbus_strerror(errno));
			modbus_free(mb);
			return NULL;
		}
		else
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbusMaster.c (RTU) : Serial Type set is RS232");
	}
	else if (serialType == 485)
	{
		 if (modbus_rtu_set_serial_mode(mb, MODBUS_RTU_RS485) == -1)
                {
                        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbusMaster.c (RTU) : cannot set mode to RS485 : %s", modbus_strerror(errno));
                        modbus_free(mb);
                        return NULL;
                }
		else
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbusMaster.c (RTU) : Serial Type set is RS485");
	}

	rc = modbus_set_slave(mb, slave_id);
	if (rc == -1)
	{
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbusMaster.c (RTU) : Cannot set slave id : %s\n", modbus_strerror(errno));
		modbus_free(mb);
		return NULL;
	}
	else
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbusMaster.c (RTU) : Slave id set is %d", slave_id);

        if (modbus_connect(mb) == -1)
        {
                printf("SV_CreateModbusMaster.c (RTU) : Connection to modbus slave endpoint %s (serialtype = RS%d, baud = %ld, parity = %c, data_bit = %d, stop_bit = %d, slave_id = %d) failed : %s\n",
			device, serialType, baud, parity, data_bit, stop_bit, slave_id, modbus_strerror(errno));
                modbus_free(mb);
                return NULL;
        }
	else
	{
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbusMaster.c (RTU) :  Successfully connected to modbus slave endpoint %s (serialtype = RS%d, baud = %ld, parity = %c, data_bit = %d, stop_bit = %d, slave_id = %d)",
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

modbus_t *initialiseModbusConnectionToSlaveTCP(UA_Server *uaServer, size_t namespaceIndex, UA_NodeId r1_modbusroot_Id, char* protocol, char* ipaddress, short port, int _slave_id)
{

  /* As a modbus TCP Master, open62541 will read from an external Modbus Slave and update its nodes */

  modbus_t *mb=NULL;

#ifdef DEBUG
printf("---------SV_CreateModbusMaster.c : data passed in is %s %d %d \n", ipaddress, port, _slave_id);
#endif

  mb = modbus_new_tcp(ipaddress, port);
  if (mb == NULL)
  {
	printf("SV_CreateModbusMaster.c : Unable to allocate libmodbus context : %s \n", modbus_strerror(errno));
	return NULL;
  }
  else
  {
	printf("SV_CreateModbusMaster.c : libmodbus context is created \n");
	int retval;
  	//while (UA_TRUE)
	//{
	// This is the very first time modbus_connect(mb) is invoked.
	retval = modbus_connect(mb);
		#ifdef DEBUG
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusMaster.c (TCP) : modbus_connect(mb) return value is %d", retval);
		#endif
	if (retval != 0)
		//	break;
		//else
	{
		UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusMaster.c (TCP) : Error connecting (initial) to modbus slave endpoint %s:%d : %s",
			ipaddress, port, modbus_strerror(errno));
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusMaster.c (TCP) : System will attempt to reconnect at the earliest possibility");
		//sleep(30);
	}
	else
	{
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
			"--------SV_CreateModbusMaster.c (TCP) : Succesfully connected (initial) to modbus slave endpoint %s:%d, slave_id:%d : %s",
			ipaddress, port, _slave_id, modbus_strerror(errno));
		tcp_slave_id = _slave_id; // save to global variable, to be used in SV_ModbusTcp.c
		modbus_set_slave(mb, tcp_slave_id);
	}
	//}
	if (UA_TRUE)	//(retval == 0)	// if it is not successful here (startup), then system will try to reconnect during every callback
	{

// regardless whether modbus_connect() is successful, we still want to create the OPC nodes
// nevertheless, before we use modbus functions, we will check and reconnect if necessary
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
                                        	        "--------SV_CreateModbus.c : Error allocating memory for Port");
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
                                                        "--------SV_CreateModbus.c : Error allocating memory for ConnectionTimeOut");
                                                exit(UA_FALSE);
                                        }
                                        else
                                        {*/
				                char* env_MBTimeout = getenv("SVR_MB_TIMEOUT");
                				if (env_MBTimeout != NULL)
                        				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
								"--------SV_CreateModbusMaster.c (TCP) : retrieved environment variable : SVR_MB_TIMEOUT : %s", env_MBTimeout);
                				else
                				{
                        				UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
								"--------SV_CreateModbusMaster.c (TCP) : cannot retrieve environment variable <SVR_MB_TIMEOUT>");
                        				UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
								"--------SV_CreateModbusMaster.c (TCP) : default to 3600");
                        				env_MBTimeout = (char*)calloc(10, sizeof(char));
                        				if (env_MBTimeout != NULL)
                                				strcpy(env_MBTimeout, "3600");
                        				else
                        				{
                                				UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
									"--------SV_CreateModbusMaster.c (TCP) : cannot retrieve environment variable <SVR_MODBUS_BAUD> : out of memory");
                                				return NULL;
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

