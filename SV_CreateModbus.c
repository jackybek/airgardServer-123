#ifdef COMMENT
Refer to ~/Downloads/ModbusDesign.txt

SV_CreateModbus.c
        createModbusExtension() calls
                initialiseModbusConnectionToSlaveTCP()
                initialiseModbusConnectionToSlaveRTU()
                initialiseModbusConnectionToSlaveASCII()
                initialiseModbusConnectionToSlaveUDP()
                initialiseModbusConnectionToSlaveRTUoverTCP()

                pthread_create(modbusSlaveTCPThread);
                pthread_create(modbusSlaveRTUThread();

# -------open62541 as a Modbus Master related functions
SV_CreateModbusMaster.c
        initialiseModbusConnectionToSlaveRTU();
        initialiseModbusConnectionToSlaveTCP();
        initialiseModbusConnectionToSlaveASCII();
        initialiseModbusConnectionToSlaveUDP();
        initialiseModbusConnectionToSlaveRTUoverTCP();

SV_ModbusTcp.c
        readModbusTcpData();

SV_ModbusRtu.c
        readModbusRTUData(); - needs modification



SV_Callbacks.c
        createRepeatedCallbacks() calls
        createModbusExtension();
        addRepeatedCallback(Callback_modbusTCP);
                readModbusTcpData();
        addRepeatedCallback(Callback_modbusRTU);
                readModbusRtuData();


# ------open62541 as a Modbus Slave related functions
SV_CreateModbusSlave.c
        initialiseModbusSlaveRTUInstance();
        initialiseModbusSlaveTCPInstance();

#endif

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

UA_NodeId outTcpSlaveId;
UA_NodeId outRtuSlaveId;
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

void *initialiseModbusSlaveTCPInstance(void *);
void *initialiseModbusSlaveRTUInstance(void *);
modbus_t *createModusExtension(UA_Server *);
modbus_t *initialiseModbusConnectionToSlaveTCP(UA_Server *, size_t, UA_NodeId, char*, char*, short, int);
modbus_t *initialiseModbusConnectionToSlaveRTU(UA_Server *, size_t, UA_NodeId, const char *device, int serialType, long baud, char parity, int data_bit, int stop_bit, int slave);
modbus_t *initialiseModbusConnectionToSlaveASCII(UA_Server *, size_t, UA_NodeId);
modbus_t *initialiseModbusConnectionToSlaveUDP(UA_Server *, size_t, UA_NodeId);
modbus_t *initialiseModbusConnectionToSlaveRTUoverTCP(UA_Server *, size_t, UA_NodeId);

void closeModbusConnection(modbus_t *);

const int NB_BITS = 16;
const int NB_INPUT_BITS = 16;
const int NB_REGISTERS = 16;
const int NB_INPUT_REGISTERS = 16;
const uint8_t UT_INPUT_BITS_TAB[] = {0xFF, 0xDB};

int tcp_slave_id;
int rtu_slave_id;
int slave_tcp_slave_id;
int slave_rtu_slave_id;

void closeModbusConnection(modbus_t *mb)
{
  modbus_close(mb);
  modbus_free(mb);
}

modbus_t **createModbusExtension(UA_Server *uaServer)
{
#ifdef COMMENT
				* add the 4 types of registers to open62541
				 * BaseObjectType
				 * |
				 * + (V) Tcp Slave Id - 100
				 * + (V) Rtu Slave Id - 101
		drop Protocol	 * + (V) Protocol -102 i.e. TCP, RTU, ASCII, UDP, RTUoverTCP
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
	modbus_t *mbMasterTcp=NULL, *mbMasterRtu=NULL, *mbMasterAscii=NULL, *mbMasterUdp=NULL, *mbMasterRtuOverTcp=NULL;
	modbus_t **arrayOfModbusMasters;

	arrayOfModbusMasters = (modbus_t**)calloc(5, sizeof(modbus_t **));
	for (int i = 0; i < 5; i++)
		arrayOfModbusMasters[i] = (modbus_t*)calloc(1, sizeof(modbus_t *));

	//short port;
	//char device[100], parity;
	//int baud, data_bit, stop_bit, slave_id;
	//int mode;

	char *env_MBProtocol, *env_MBIpV4, *env_MBPort, *env_MBTcpSlaveId;
	char *env_MBDevice, *env_MBBaud, *env_MBSerialType,  *env_MBParity, *env_MBDataBit, *env_MBStopBit, *env_MBRtuSlaveId;

	// OPC Server acting as a Modbus Master

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : ==========================================================");
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : Entering createModbusExtension()");

        // add root node (r1_modbusroot_Id); to be reference in initialiseModbusConnectionToSlaveTCP(), initialiseModbusConnectionToSlaveRTU()
        size_t namespaceIndex;
        int retval;
        char sNameSpace[] = "ModbusMaster";
        char sNameSpaceEx[] = "virtualskies.com.sg/MKS/";
        UA_NodeId r1_modbusroot_Id;  /* get the nodeid assigned by the server */

        UA_LOG_INFO(UA_Log_Stdout,UA_LOGCATEGORY_USERLAND,
                  ("--------SV_CreateModbus.c : Begin the process of adding to OPC UA Tree structure"));
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
                	"--------SV_CreateModbus.c : Error creating object node : Root Node ModbusMaster : %s", UA_StatusCode_name(retval));
	}
        else
        {
        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "--------SV_CreateModbus.c : Root Node Modbus added successfully : %d", r1_modbusroot_Id.identifier.numeric);
        }

	// read the environment variables
	// ----------------------------------------------------------------------Modbus Master, TCP
	env_MBTcpSlaveId = getenv("SVR_MB_TCP_SLAVEID");
                if (env_MBTcpSlaveId != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : retrieved environment variable : SVR_MB_TCP_SLAVEID : %s", env_MBTcpSlaveId);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MB_TCP_SLAVEID>");
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : default to 43");
                        env_MBTcpSlaveId = (char*)calloc(15, sizeof(char));
                        if (env_MBTcpSlaveId != NULL)
                                strcpy(env_MBTcpSlaveId, "43");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MB_TCP_SLAVEID> :  out of memory");
                                return NULL;
                        }

				// variable
				// ----- Tcp Slave Id
				UA_VariableAttributes vTcpSlaveIdAttr = UA_VariableAttributes_default;
				vTcpSlaveIdAttr.description = UA_LOCALIZEDTEXT("en-US", "Tcp Slave Id");
				vTcpSlaveIdAttr.displayName = UA_LOCALIZEDTEXT("en-US", "1. Tcp Slave Id");
				vTcpSlaveIdAttr.dataType = UA_TYPES[UA_TYPES_INT16].typeId;
				vTcpSlaveIdAttr.valueRank = UA_VALUERANK_SCALAR;
				vTcpSlaveIdAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
				vTcpSlaveIdAttr.historizing = UA_FALSE;

				/*
				char *_SlaveId;
				_SlaveId = (char *)calloc(10, sizeof(char));
				if (_SlaveId == NULL)
				{
					UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
						"--------SV_CreateModbus.c : Error allocating memory for SlaveId");
					exit(UA_FALSE);
				}
				else
				{*/
					//sprintf(_SlaveId, "%d", slave_id);
					//UA_String SlaveId = UA_STRING(_SlaveId);
					tcp_slave_id = atoi(env_MBTcpSlaveId);
						UA_Variant_setScalar(&vTcpSlaveIdAttr.value, &tcp_slave_id, &UA_TYPES[UA_TYPES_INT16]);
					retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 100),
						r1_modbusroot_Id,
						UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
            					UA_QUALIFIEDNAME(namespaceIndex, "Tcp Slave Id"),
						UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
						vTcpSlaveIdAttr, NULL, &outTcpSlaveId);
					if (retval != UA_STATUSCODE_GOOD)
					   UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                				"--------SV_CreateModbus.c : Error creating node : Modbus->Tcp Slave Id (100): %s", UA_StatusCode_name(retval));
				        #ifdef DEBUG
	        			else
        	    			   UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                				"--------SV_CreateModbus.c : Attribute Variable created: Modbus->Tcp Slave Id (100) : %d", outTcpSlaveId.identifier.numeric);
        				#endif
					//free(_SlaveId);
				//}
                }

        // ---------------------------------------------------------------------------------Modbus Master, RTU
        env_MBRtuSlaveId = getenv("SVR_MB_RTU_SLAVEID");
                if (env_MBRtuSlaveId != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : retrieved environment variable : SVR_MB_RTU_SLAVEID : %s", env_MBRtuSlaveId);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MB_RTU_SLAVEID>");
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : default to 143");
                        env_MBRtuSlaveId = (char*)calloc(15, sizeof(char));
                        if (env_MBRtuSlaveId != NULL)
                                strcpy(env_MBRtuSlaveId, "143");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MB_RTU_SLAVEID> :  out of memory");
                                return NULL;
                        }

                                // variable
                                // ----- Rtu Slave Id
                                UA_VariableAttributes vRtuSlaveIdAttr = UA_VariableAttributes_default;
                                vRtuSlaveIdAttr.description = UA_LOCALIZEDTEXT("en-US", "Rtu Slave Id");
                                vRtuSlaveIdAttr.displayName = UA_LOCALIZEDTEXT("en-US", "2. Rtu Slave Id");
                                vRtuSlaveIdAttr.dataType = UA_TYPES[UA_TYPES_INT16].typeId;
                                vRtuSlaveIdAttr.valueRank = UA_VALUERANK_SCALAR;
                                vRtuSlaveIdAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                                vRtuSlaveIdAttr.historizing = UA_FALSE;

                                /*
                                char *_SlaveId;
                                _SlaveId = (char *)calloc(10, sizeof(char));
                                if (_SlaveId == NULL)
                                {
                                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                "--------SV_CreateModbus.c : Error allocating memory for SlaveId");
                                        exit(UA_FALSE);
                                }
                                else
                                {*/
                                        //sprintf(_SlaveId, "%d", slave_id);
                                        //UA_String SlaveId = UA_STRING(_SlaveId);
                                        rtu_slave_id = atoi(env_MBRtuSlaveId);
                                                UA_Variant_setScalar(&vRtuSlaveIdAttr.value, &rtu_slave_id, &UA_TYPES[UA_TYPES_INT16]);
                                        retval = UA_Server_addVariableNode(uaServer, UA_NODEID_NUMERIC(namespaceIndex, 101),
                                                r1_modbusroot_Id,
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                                UA_QUALIFIEDNAME(namespaceIndex, "Rtu Slave Id"),
                                                UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                                vRtuSlaveIdAttr, NULL, &outRtuSlaveId);
                                        if (retval != UA_STATUSCODE_GOOD)
                                           UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                "--------SV_CreateModbus.c : Error creating node : Modbus->Rtu Slave Id (101): %s", UA_StatusCode_name(retval));
                                        #ifdef DEBUG
                                        else
                                           UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                                                "--------SV_CreateModbus.c : Attribute Variable created: Modbus->Rtu Slave Id (101) : %d", outRtuSlaveId.identifier.numeric);
                                        #endif
                                        //free(_SlaveId);
                                //}
                }


	env_MBProtocol = getenv("SVR_MB_PROTOCOL");
        if (env_MBProtocol != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : retrieved environment variable : SVR_MB_PROTOCOL : %s", env_MBProtocol);
        else
        {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MB_PROTOCOL>");
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : default to TCP");
                        env_MBProtocol = (char*)calloc(50, sizeof(char));
                        if (env_MBProtocol != NULL)
                                strcpy(env_MBProtocol, "TCP");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MB_PROTOCOL> :  out of memory");
                                return NULL;
                        }
        }

	// - check the protocol type i.e. TCP, SERIAL(RTU), ASCII, UDP, RTUoverTCP
	//if (strcmp(env_MBProtocol, "TCP") == 0) - no need to check anymore
	//{
	// populate the ua tree with Modbus Master TCP information
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"=====================================================================");
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : Creating an instance of Modbus Master Tcp");
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"=====================================================================");

		env_MBIpV4 = getenv("SVR_MB_IPV4");
		if (env_MBIpV4 != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : retrieved environment variable : SVR_MB_IPV4 : %s", env_MBIpV4);
        	else
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MODBUS_IPV4>");
                	UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : default to 192.168.1.157");
                	env_MBIpV4 = (char*)calloc(50, sizeof(char));
                	if (env_MBIpV4 != NULL)
                        	strcpy(env_MBIpV4, "192.168.1.157");
                	else
                	{
                        	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MODBUS_IPV4> :  out of memory");
                        	return NULL;
                	}
        	}

		env_MBPort = getenv("SVR_MB_PORT");
                if (env_MBPort != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : retrieved environment variable : SVR_MODBUS_PORT : %s", env_MBPort);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MODBUS_PORT>");
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : default to 502");
                        env_MBPort = (char*)calloc(15, sizeof(char));
                        if (env_MBPort != NULL)
                                strcpy(env_MBPort, "502");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MODBUS_PORT> :  out of memory");
                                return NULL;
                        }
                }

		printf("------SV_CreateModbus.c : data to pass into initialiseModbusConnectionToSlaveTCP() are %s, %d, %d", env_MBIpV4, (short)atoi(env_MBPort), atoi(env_MBTcpSlaveId));
		mbMasterTcp = initialiseModbusConnectionToSlaveTCP(uaServer, namespaceIndex, r1_modbusroot_Id, env_MBProtocol, env_MBIpV4, (short)atoi(env_MBPort), atoi(env_MBTcpSlaveId));
		//mb = initialiseModbusConnectionTCP("192.168.1.157", 502, 43);

	//} finish populate ua tree with Modbus Master TCP information

	//else if (strcmp(env_MBProtocol, "SERIAL") == 0) // equivalent to Modbus RTU protocol - no need to check anymore
	//{
	// populate the ua tree with Modbus Master RTU information
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"=====================================================================");
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : Creating an instance of Modbus Master Rtu");
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"=====================================================================");

		env_MBSerialType = getenv("SVR_MB_SERIAL_TYPE");
                if (env_MBSerialType != NULL)
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : retrieved environment variable : SVR_MODBUS_SERIAL_TYPE : %s", env_MBSerialType);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MODBUS_SERIAL_TYPE>");
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : default to RS232");
                        env_MBSerialType = (char*)calloc(15, sizeof(char));
                        if (env_MBSerialType != NULL)
                                strcpy(env_MBSerialType, "232");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MODBUS_SERIAL_TYPE> :  out of memory");
                                return NULL;
                        }
                }

		env_MBDevice = getenv("SVR_MB_DEVICE");
		if (env_MBDevice != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : retrieved environment variable : SVR_MODBUS_DEVICE : %s", env_MBDevice);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MODBUS_DEVICE>");
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : default to /dev/ttyUSB0");
                        env_MBDevice = (char*)calloc(15, sizeof(char));
                        if (env_MBDevice != NULL)
                                strcpy(env_MBDevice, "/dev/ttyUSB0");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MODBUS_DEVICE> : out of memory");
                                return NULL;
                        }
                }

		env_MBBaud = getenv("SVR_MB_BAUD");
                if (env_MBBaud != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : retrieved environment variable : SVR_MODBUS_BAUD : %s", env_MBBaud);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MODBUS_BAUD>");
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : default to 115200");
                        env_MBBaud = (char*)calloc(15, sizeof(char));
                        if (env_MBBaud != NULL)
                                strcpy(env_MBBaud, "115200");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MODBUS_BAUD> : out of memory");
                                return NULL;
                        }
                }

		env_MBParity = getenv("SVR_MB_PARITY");
                if (env_MBParity != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : retrieved environment variable : SVR_MODBUS_PARITY : %s", env_MBParity);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MODBUS_PARITY>");
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : default to N");
                        env_MBParity = (char*)calloc(15, sizeof(char));
                        if (env_MBParity != NULL)
                                strcpy(env_MBParity, "N");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MODBUS_BAUD> : out of memory");
                                return NULL;
                        }
                }

		env_MBDataBit = getenv("SVR_MB_DATABIT");
                if (env_MBDataBit != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : retrieved environment variable : SVR_MODBUS_DATABIT : %s", env_MBDataBit);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MODBUS_DATABIT>");
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : default to 8");
                        env_MBDataBit = (char*)calloc(15, sizeof(char));
                        if (env_MBDataBit != NULL)
                                strcpy(env_MBDataBit, "8");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MODBUS_DATABIT> : out of memory");
                                return NULL;
                        }
                }

		env_MBStopBit = getenv("SVR_MB_STOPBIT");
                if (env_MBStopBit != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : retrieved environment variable : SVR_MODBUS_STOPBIT : %s", env_MBStopBit);
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MODBUS_STOPBIT>");
                        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : default to 1");
                        env_MBStopBit = (char*)calloc(15, sizeof(char));
                        if (env_MBStopBit != NULL)
                                strcpy(env_MBStopBit, "1");
                        else
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_MODBUS_STOPBIT> : out of memory");
                                return NULL;
                        }
                }

		printf("data to pass into initialiseModbusConnectionToSlaveRTU() are %s, %d, %ld, %c, %d, %d, %d \n", env_MBDevice, atoi(env_MBSerialType), atol(env_MBBaud),
				(char)env_MBParity[0], atoi(env_MBDataBit), atoi(env_MBStopBit), atoi(env_MBRtuSlaveId));
		mbMasterRtu = initialiseModbusConnectionToSlaveRTU(uaServer, namespaceIndex, r1_modbusroot_Id, env_MBDevice, atoi(env_MBSerialType), atol(env_MBBaud),
				(char)env_MBParity[0], atoi(env_MBDataBit), atoi(env_MBStopBit), atoi(env_MBRtuSlaveId));
	//} finish populate ua tree with Modbus Master RTU information

	//else if (strcmp(env_MBProtocol, "ASCII") == 0)
	//{
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"=======================================================================");
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : Creating an instance of Modbus Master Ascii");
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"=======================================================================");

		mbMasterAscii = initialiseModbusConnectionToSlaveASCII(uaServer, namespaceIndex, r1_modbusroot_Id);
	//}
	//else if (strcmp(env_MBProtocol, "UDP") == 0)
	//{
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"=====================================================================");
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : Creating an instance of Modbus Master UDP");
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"=====================================================================");

		mbMasterUdp = initialiseModbusConnectionToSlaveUDP(uaServer, namespaceIndex, r1_modbusroot_Id);
	//}
	//else if (strcmp(env_MBProtocol, "RTUoverTCP") == 0)
	//{
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"==============================================================================");
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : Creating an instance of Modbus Master RTU over UDP");
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"==============================================================================");

		mbMasterRtuOverTcp = initialiseModbusConnectionToSlaveRTUoverTCP(uaServer, namespaceIndex, r1_modbusroot_Id);
	//}



	#ifdef COMMENT
	printf(" ------------------------------------------------------------------------------ extra capability : Modbus Slave Tcp");
	printf("OPC Server acting as a Modbus Slave");
	#endif
        char *env_SVRAsModbusSlaveTcp = getenv("SVR_CAP_ASMODBUSSLAVE_TCP");
        if (env_SVRAsModbusSlaveTcp != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : retrieved environment variable : <SVR_CAP_ASMODBUSSLAVE_TCP> : %s", env_SVRAsModbusSlaveTcp);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_CAP_ASMODBUSSLAVE_TCP>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : default to <NO>");
                env_SVRAsModbusSlaveTcp = (char*)calloc(5,sizeof(char));
                if (env_SVRAsModbusSlaveTcp != NULL)
                        strcpy(env_SVRAsModbusSlaveTcp, "NO");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_CAP_ASMODBUSSLAVE_TCP> :  out of memory");
                        exit(UA_FALSE);
                }
		if (strncmp(env_SVRAsModbusSlaveTcp, "YES", 3)==0)
		{
                	// create a Modbus Slave thread that listens for modbus request at TCP port 1502
	                pthread_t modbusSlaveTCPThread;
        	        int ret = pthread_create(&modbusSlaveTCPThread, NULL, initialiseModbusSlaveTCPInstance, (void *)uaServer);
			printf("ret from pthread_create(modbusSlaveTCPThread) is %d \n", ret);
	                if (ret != 0)
        	                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "--------SV_CreateModbus.c (Slave) : Fail to create a Modbus TCP Slave instance");
                	else
	                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "--------SV_CreateModbus.c (Slave) : Modbus TCP Slave instance started successfully");
		}
        }

	#ifdef COMMENT
	printf(" ------------------------------------------------------------------------------ extra capability : Modbus Slave Rtu");
	printf("OPC Server acting as a Modbus Slave");
	#endif
        char *env_SVRAsModbusSlaveRtu = getenv("SVR_CAP_ASMODBUSSLAVE_RTU");
        if (env_SVRAsModbusSlaveRtu != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : retrieved environment variable : <SVR_CAP_ASMODBUSSLAVE_RTU> : %s", env_SVRAsModbusSlaveRtu);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_CAP_ASMODBUSSLAVE_RTU>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : default to <NO>");
                env_SVRAsModbusSlaveRtu = (char*)calloc(5,sizeof(char));
                if (env_SVRAsModbusSlaveRtu != NULL)
                        strcpy(env_SVRAsModbusSlaveRtu, "NO");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateModbus.c : cannot retrieve environment variable <SVR_CAP_ASMODBUSSLAVE_RTU> :  out of memory");
                        exit(UA_FALSE);
		}
		if (strncmp(env_SVRAsModbusSlaveRtu, "YES", 3)==0)
		{
			// create a Modbus Slave thread that listens for modbus rerquest at SERIAL port /dev/ttyUSB0
			pthread_t modbusSlaveRTUThread;
			int ret = pthread_create(&modbusSlaveRTUThread, NULL, initialiseModbusSlaveRTUInstance, (void *)uaServer);
			printf("ret from pthread_create(modbusSlaveRTUThread) is %d \n", ret);
			if (ret != 0)
                	        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "--------SV_CreateModbus.c (Slave RTU) : Fail to create a Modbus RTU Slave instance");
	                else
        	                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "--------SV_CreateModbus.c (Slave RTU) : Modbus RTU Slave instance started successfully");

		}
        }

	// need to change to an array of modbus_t *mb[4];
	arrayOfModbusMasters[0] = mbMasterTcp;
	arrayOfModbusMasters[1] = mbMasterRtu;
	arrayOfModbusMasters[2] = mbMasterAscii;
	arrayOfModbusMasters[3] = mbMasterUdp;
	arrayOfModbusMasters[4] = mbMasterRtuOverTcp;


	//return mbMasterTcp; // return Modbus Master object back to createRepeatedCallbacks()
	return arrayOfModbusMasters;
}

