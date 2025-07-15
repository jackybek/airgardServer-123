//#define DEBUG
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

extern UA_NodeId r4_modbus_TCP_DATA_FC1_Id;
extern UA_NodeId r4_modbus_TCP_DATA_FC2_Id;
extern UA_NodeId r4_modbus_TCP_DATA_FC3_Id;
extern UA_NodeId r4_modbus_TCP_DATA_FC4_Id;

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
modbus_t *initialiseModbusConnectionToSlaveRTU(UA_Server *, size_t, UA_NodeId, const char *device, int serialType, long baud, char parity, int data_bit, int stop_bit, int slave);
modbus_t *initialiseModbusConnectionToSlaveASCII(UA_Server *, size_t, UA_NodeId);
modbus_t *initialiseModbusConnectionToSlaveUDP(UA_Server *, size_t, UA_NodeId);
modbus_t *initialiseModbusConnectionToSlaveRTUoverTCP(UA_Server *, size_t, UA_NodeId);

int readModbusTCPData(UA_Server *, modbus_t *);

char *env_MBStartFC1AddrTCP, *env_MBStartFC2AddrTCP, *env_MBStartFC3AddrTCP, *env_MBStartFC4AddrTCP;
int firstTimeAddModbusToUAServer = UA_TRUE;
int modbusTCPSlaveIsConnected;

UA_Byte *FC1tab_reg=NULL;	// uint8_t read modbus coils
UA_Byte *FC2tab_reg=NULL; 	// uint8_t read modbus discrete inputs
UA_UInt16 *FC3tab_reg=NULL;	// uint16_t read modbus holding registers
UA_UInt16 *FC4tab_reg=NULL;	// uint16_t read modbus input registers
char *env_MBNoOfFC1RegisterTCP=NULL, *env_MBNoOfFC2RegisterTCP=NULL, *env_MBNoOfFC3RegisterTCP=NULL, *env_MBNoOfFC4RegisterTCP=NULL;
UA_Variant arrayVariantFC1, arrayVariantFC2, arrayVariantFC3, arrayVariantFC4;
UA_NodeId arrayNodeIdFC1, arrayNodeIdFC2, arrayNodeIdFC3, arrayNodeIdFC4;

extern int tcp_slave_id;	// delcared in SV_CreateModbus.c - Modbus TCP Master
extern int slave_tcp_slave_id; 	// declared in SV_CreateModbus.c - Modbus TCP Slave
int readModbusTCPData(UA_Server *uaServer, modbus_t *mb)
{
#define TEST_WITHOUT_SOURCE
#define DEBUG

  //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : readModbusTCPData() function ");
  //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : firstTimeAddModbusToUAServer is %d ", firstTimeAddModbusToUAServer);
  if (mb)
  {
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : in if(mb) section");
	if (firstTimeAddModbusToUAServer == UA_TRUE)
	{
		firstTimeAddModbusToUAServer = UA_FALSE;

	//-- FC1
	        env_MBStartFC1AddrTCP = getenv("SVR_MB_COILS_STARTADDR_TCP"); //getenv("SVR_MB_STARTFC1ADDRTCP");
        	if (env_MBStartFC1AddrTCP != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c :  retrieved environment variable : SVR_MB_COILS_STARTADDR_TCP : %s", env_MBStartFC1AddrTCP);
	        else
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot retrieve environment variable <SVR_MB_COILS_STARTADDR_TCP>");
	                env_MBStartFC1AddrTCP = (char*)calloc(10, sizeof(char));
        	        if (env_MBStartFC1AddrTCP != NULL)
			{
                        	strcpy(env_MBStartFC1AddrTCP, "1"); // if set to 0, crash: modbus slave address will be 1
		                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : default to %s", env_MBStartFC1AddrTCP);
			}
                	else
	                {
        	                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot retrieve environment variable <SVR_MB_COILS_STARTADDR_TCP> :  out of memory");
                	        return UA_FALSE;
	                }
	        }

	        env_MBNoOfFC1RegisterTCP = getenv("SVR_MB_COILS_REGISTERCOUNT_TCP");//getenv("SVR_MB_NUMFC1REGISTERTCP");
        	if (env_MBNoOfFC1RegisterTCP != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c :  retrieved environment variable : SVR_MB_COILS_REGISTERCOUNT_TCP : %s", env_MBNoOfFC1RegisterTCP);
	        else
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot retrieve environment variable <SVR_MB_COILS_REGISTERCOUNT_TCP>");
	                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : default to 32");
	                env_MBNoOfFC1RegisterTCP = (char*)calloc(32, sizeof(char));
        	        if (env_MBNoOfFC1RegisterTCP != NULL)
                	        strcpy(env_MBNoOfFC1RegisterTCP, "32");
	                else
        	        {
                	        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot retrieve environment variable <SVR_MB_COILS_REGISTERCOUNT_TCP> :  out of memory");
	                        return UA_FALSE;
        	        }
        	}

	//--- FC2
	        env_MBStartFC2AddrTCP = getenv("SVR_MB_DISCRETEINPUT_STARTADDR_TCP"); //getenv("SVR_MB_STARTFC2ADDRTCP");
        	if (env_MBStartFC2AddrTCP != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c :  retrieved environment variable : SVR_MB_DISCRETEINPUT_STARTADDR_TCP : %s", env_MBStartFC2AddrTCP);
	        else
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot retrieve environment variable <SVR_MB_DISCRETEINPUT_STARTADDR_TCP>");
	                env_MBStartFC2AddrTCP = (char*)calloc(10, sizeof(char));
        	        if (env_MBStartFC2AddrTCP != NULL)
			{
                        	strcpy(env_MBStartFC2AddrTCP, "10000"); // modbus slave address will be 10001
		                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : default to %s", env_MBStartFC2AddrTCP);
			}
                	else
	                {
        	                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot retrieve environment variable <SVR_MB_DISCRETEINPUT_STARTADDR_TCP> :  out of memory");
                	        return UA_FALSE;
                	}
        	}

	        env_MBNoOfFC2RegisterTCP = getenv("SVR_MB_DISCRETEINPUT_REGISTERCOUNT_TCP"); //getenv("SVR_MB_NUMFC2REGISTERTCP");
        	if (env_MBNoOfFC2RegisterTCP != NULL)
	                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c :  retrieved environment variable : SVR_MB_DISCRETEINPUT_REGISTERCOUNT_TCP : %s", env_MBNoOfFC2RegisterTCP);
        	else
	        {
        	        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot retrieve environment variable <SVR_MB_DISCRETEINPUT_REGISTERCOUNT_TCP>");
                	env_MBNoOfFC2RegisterTCP = (char*)calloc(32, sizeof(char));
	                if (env_MBNoOfFC2RegisterTCP != NULL)
			{
                	        strcpy(env_MBNoOfFC2RegisterTCP, "32");
		                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : default to %s", env_MBNoOfFC2RegisterTCP);
			}
                	else
	                {
        	                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot retrieve environment variable <SVR_MB_DISCRETEINPUT_REGISTERCOUNT_TCP> :  out of memory");
                	        return UA_FALSE;
                	}
        	}

	//--- FC3
		env_MBStartFC3AddrTCP = getenv("SVR_MB_HOLDREGISTER_STARTADDR_TCP"); // getenv("SVR_MB_STARTFC3ADDRTCP");
		if (env_MBStartFC3AddrTCP != NULL)
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c :  retrieved environment variable : SVR_MB_HOLDREGISTER_STARTADDR_TCP : %s", env_MBStartFC3AddrTCP);
		else
		{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot retrieve environment variable <SVR_MB_HOLDREGISTER_STARTADDR_TCP>");
	                env_MBStartFC3AddrTCP = (char*)calloc(10, sizeof(char));
        	        if (env_MBStartFC3AddrTCP != NULL)
			{
                        	strcpy(env_MBStartFC3AddrTCP, "40000"); // modbus slave address will be 40001
		                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : default to %s", env_MBStartFC3AddrTCP);
			}
                	else
	                {
        	                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot retrieve environment variable <SVR_MB_STARTFC3ADDRTCP> :  out of memory");
                	        return UA_FALSE;
                	}
		}

	        env_MBNoOfFC3RegisterTCP = getenv("SVR_MB_HOLDREGISTER_REGISTERCOUNT_TCP"); // getenv("SVR_MB_NUMFC3REGISTERTCP");
        	if (env_MBNoOfFC3RegisterTCP != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c :  retrieved environment variable : SVR_MB_HOLDREGISTER_REGISTERCOUNT_TCP : %s", env_MBNoOfFC3RegisterTCP);
	        else
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot retrieve environment variable <SVR_MB_HOLDREGISTER_REGISTERCOUNT_TCP>");
	                env_MBNoOfFC3RegisterTCP = (char*)calloc(32, sizeof(char));
        	        if (env_MBNoOfFC3RegisterTCP != NULL)
			{
                        	strcpy(env_MBNoOfFC3RegisterTCP, "32");
		                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : default to %s", env_MBNoOfFC3RegisterTCP);
			}
                	else
	                {
        	                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot retrieve environment variable <SVR_MB_HOLDREGISTER_REGISTERCOUNT_TCP> :  out of memory");
                	        return UA_FALSE;
	                }
        	}

	//--FC4
	        env_MBStartFC4AddrTCP = getenv("SVR_MB_HOLDREGISTER_REGISTERCOUNT_TCP");
        	if (env_MBStartFC4AddrTCP != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c :  retrieved environment variable : SVR_MB_HOLDREGISTER_REGISTERCOUNT_TCP : %s", env_MBStartFC4AddrTCP);
	        else
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot retrieve environment variable <SVR_MB_HOLDREGISTER_REGISTERCOUNT_TCP>");
	                env_MBStartFC4AddrTCP = (char*)calloc(10, sizeof(char));
        	        if (env_MBStartFC4AddrTCP != NULL)
			{
                        	strcpy(env_MBStartFC4AddrTCP, "30000"); // modbus slave address will be 30001
		                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : default to %s", env_MBStartFC4AddrTCP);
			}
                	else
                	{
                        	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot retrieve environment variable <SVR_MB_HOLDREGISTER_REGISTERCOUNT_TCP> :  out of memory");
	                        return UA_FALSE;
        	        }
        	}

		env_MBNoOfFC4RegisterTCP = getenv("SVR_MB_INPUTREGISTER_REGISTERCOUNT_TCP"); //getenv("SVR_MB_NUMFC4REGISTERTCP");
        	if (env_MBNoOfFC4RegisterTCP != NULL)
                	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c :  retrieved environment variable : SVR_MB_INPUTREGISTER_REGISTERCOUNT_TCP : %s", env_MBNoOfFC4RegisterTCP);
	        else
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot retrieve environment variable <SVR_MB_INPUTREGISTER_REGISTERCOUNT_TCP>");
	                env_MBNoOfFC4RegisterTCP = (char*)calloc(32, sizeof(char));
        	        if (env_MBNoOfFC4RegisterTCP != NULL)
			{
                        	strcpy(env_MBNoOfFC4RegisterTCP, "32");
		                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : default to %s", env_MBNoOfFC4RegisterTCP);
			}
                	else
	                {
        	                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot retrieve environment variable <SVR_MB_INPUTREGISTER_REGISTERCOUNT_TCP> :  out of memory");
                	        return UA_FALSE;
	                }
        	}

	// ---Modbus function code 0x01 (read output coils) - FC1 {201, 202}
	#ifdef DEBUG
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : FC1 : Start register %d, No Of Registers %d", atoi(env_MBStartFC1AddrTCP), atoi(env_MBNoOfFC1RegisterTCP));
        #endif
        #ifdef TEST_WITHOUT_SOURCE
	        FC1tab_reg = (UA_Byte*)calloc(atoi(env_MBNoOfFC1RegisterTCP), sizeof(uint8_t));
        	if (!FC1tab_reg)
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot allocate memory for FC1 (discrete output coils)");
	                exit(UA_FALSE);
        	}
		// need to subtract 1 from env_MBStartFC1AddrTCP when passing into modbus_read_bits
       		modbus_read_bits(mb, atoi(env_MBStartFC1AddrTCP) - 1, atoi(env_MBNoOfFC1RegisterTCP), FC1tab_reg);
	        //for (int i=0; i< atoi(env_MBNoOfFC1RegisterTCP); i++)
		#ifdef DEBUG
               	int i=0;     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : 1st element in FC1 register[%d] = %d", i, FC1tab_reg[i]);
		#endif
		//UA_free(FC1tab_reg);
        #endif

		// ---Modbus function code 0x02 (read discrete inputs) - FC2 {203, 204}
	#ifdef DEBUG
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : FC2 : Start register %d, No Of Registers %d", atoi(env_MBStartFC2AddrTCP), atoi(env_MBNoOfFC2RegisterTCP));
        #endif
	#ifdef TEST_WITHOUT_SOURCE
	        FC2tab_reg = (UA_Byte*)calloc(atoi(env_MBNoOfFC2RegisterTCP), sizeof(uint8_t));
        	if (!FC2tab_reg)
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot allocate memory for FC2 (discrete inputs)");
	                exit(UA_FALSE);
        	}
		// need to subtract 1 from env_MBStartFC2AddrTCP when passing into modbus_read_bits
	        modbus_read_bits(mb, atoi(env_MBStartFC2AddrTCP - 1), atoi(env_MBNoOfFC2RegisterTCP), FC2tab_reg);
	        //for (int i=0; i< atoi(env_MBNoOfFC2RegisterTCP); i++)
		#ifdef DEBUG
        	i=0;     UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : 1st element in FC2 register[%d] = %d", i, FC2tab_reg[i]);
		#endif
		//UA_free(FC2tab_reg);
        #endif

	// ---Modbus function code 0x03 (read multiple holding registers) - FC3 {205, 206}
	#ifdef DEBUG
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : FC3 : Start register %d, No Of Registers %d", atoi(env_MBStartFC3AddrTCP), atoi(env_MBNoOfFC3RegisterTCP));
	#endif
        #ifdef TEST_WITHOUT_SOURCE
		FC3tab_reg = (UA_UInt16*)calloc(atoi(env_MBNoOfFC3RegisterTCP), sizeof(uint16_t));
		if (!FC3tab_reg)
		{
			UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot allocate memory for FC3 (holding registers)");
			exit(UA_FALSE);
		}
		// need to subtract 1 from env_MBStartFC3AddrTCP when passing into modbus_read_registers
  		modbus_read_registers(mb, atoi(env_MBStartFC3AddrTCP) - 1, atoi(env_MBNoOfFC3RegisterTCP), FC3tab_reg);
		#ifdef DEBUG
		//for (int i=0; i< atoi(env_MBNoOfFC3RegisterTCP); i++)
		i=0;	 UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : 1st element in FC3 register[%d] = %d", i, FC3tab_reg[i]);
		#endif
		//UA_free(FC3tab_reg);
	#endif

	// ---Modbus function code 0x04 (read input registers) - FC4 {207, 208}
        #ifdef DEBUG
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : FC4 : Start register %d, No Of Registers %d", atoi(env_MBStartFC4AddrTCP), atoi(env_MBNoOfFC4RegisterTCP));
        #endif
        #ifdef TEST_WITHOUT_SOURCE
        	FC4tab_reg = (UA_UInt16*)calloc(atoi(env_MBNoOfFC4RegisterTCP), sizeof(uint16_t));
	        if (!FC4tab_reg)
        	{
                	UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot allocate memory for FC4 (input registers)");
	                exit(UA_FALSE);
        	}
		// need to subtract 1 from env_MBStartFC4AddrTCP when passing into modbus_read_input_registers
        	modbus_read_input_registers(mb, atoi(env_MBStartFC4AddrTCP) - 1, atoi(env_MBNoOfFC4RegisterTCP), FC4tab_reg);
		#ifdef DEBUG
	        //for (int i=0; i< atoi(env_MBNoOfFC4RegisterTCP); i++)
        	i=0;    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : 1st element in FC4 register[%d] = %d", i, FC4tab_reg[i]);
		#endif
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

		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : Adding FC1 ArrayData nodes");
		// -- 1st node - ArrayData
                UA_VariableAttributes vArrayDataFC1Attr = UA_VariableAttributes_default;
                vArrayDataFC1Attr.displayName = UA_LOCALIZEDTEXT("en-US", "3.4.1.1 FC1 ArrayData");
                vArrayDataFC1Attr.description = UA_LOCALIZEDTEXT("en-US", "FC1 ArrayData");
		//vArrayDataFC1Attr.writeMask = ??
		// vArrayDataFC1Attr.userWriteMask = ??
		/* set the variable value constraints */
                vArrayDataFC1Attr.dataType = UA_TYPES[UA_TYPES_BYTE].typeId;
                vArrayDataFC1Attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
		UA_UInt32 arrayFC1Dims[atoi(env_MBNoOfFC1RegisterTCP)]; //arrayFC1Dims[32];
		vArrayDataFC1Attr.arrayDimensionsSize = 1; //1;
		vArrayDataFC1Attr.arrayDimensions = arrayFC1Dims;
                vArrayDataFC1Attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
		//vArrayDataFC1Attr.userAccessLevel = ??;
		//vArrayDataFC1Attr.minimumSamplingInterval = ??;
                vArrayDataFC1Attr.historizing = UA_TRUE;

			/* set the value (UA_Variant).  The array dimensions need to be the same for the value */
			UA_Byte *arrayFC1Data;	// eg {true, false, true, false, true, false, ...}
			size_t arrayFC1Size = atoi(env_MBNoOfFC1RegisterTCP);	// eg 32
			arrayFC1Data = (UA_Byte *)UA_malloc(arrayFC1Size * sizeof (UA_Byte));
			if (!arrayFC1Data)
			{
				UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot allocate memory for arrayFC1 (coils)");
				return UA_FALSE;
			}
			else
			{	// initialise the COIL array to 0
				for (size_t index = 0; index < arrayFC1Size; index++)
					arrayFC1Data[index] = (UA_Byte)0;

				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : arrayFC1Size is %ld", arrayFC1Size);
				printf("arrayFC1Data[] is  ");
				for (size_t index = 0; index < arrayFC1Size; index++)
					printf("%d ", arrayFC1Data[index]);
				printf("\n");
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

			retval = UA_Server_writeValue(uaServer, outArrayDataFC1, arrayVariantFC1);
			if (retval != UA_STATUSCODE_GOOD)
				UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : Failure to initialise arrayFC1Data[] : %s ", UA_StatusCode_name(retval));
			else
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : UA_Server_writeValue() initialising arrayNodeIdFC1 is successful");
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
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : Adding FC2 ArrayData nodes");
                UA_VariableAttributes vArrayDataFC2Attr = UA_VariableAttributes_default;
                vArrayDataFC2Attr.description = UA_LOCALIZEDTEXT("en-US", "FC2 ArrayData");
                vArrayDataFC2Attr.displayName = UA_LOCALIZEDTEXT("en-US", "3.4.2.1 FC2 ArrayData");

                /* set the variable value constraints */
                vArrayDataFC2Attr.dataType = UA_TYPES[UA_TYPES_BYTE].typeId;
                vArrayDataFC2Attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
                UA_UInt32 arrayFC2Dims[atoi(env_MBNoOfFC2RegisterTCP)]; // UA_UInt32 arrayFC2Dims[32]
                vArrayDataFC2Attr.arrayDimensionsSize = 1;
                vArrayDataFC2Attr.arrayDimensions = arrayFC2Dims;	// UA_UInt32 *)
                vArrayDataFC2Attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
                vArrayDataFC2Attr.historizing = UA_TRUE;

                        /* set the value.  The array dimensions need to be the same for the value */
                        UA_Byte *arrayFC2Data;       // eg {true, false, true, false, true, false, ...}
                        size_t arrayFC2Size = atoi(env_MBNoOfFC2RegisterTCP);   // eg 32
                        arrayFC2Data = (UA_Byte *)UA_malloc(arrayFC2Size * sizeof(UA_Byte));
                        if (!arrayFC2Data)
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot allocate memory for arrayFC2 (discrete inputs)");
                                return UA_FALSE;
                        }
                        else
                        {       // initialise the DISCRETE Inputs array to FALSE
                                for (size_t index = 0; index < arrayFC2Size; index++)
                                        arrayFC2Data[index] = (UA_Byte)0; //UA_FALSE;

                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : arrayFC2Size is %ld", arrayFC2Size);
                                printf("arrayFC2Data[] is  ");
                                for (size_t index = 0; index < arrayFC2Size; index++)
                                        printf("%d ", arrayFC2Data[index]);
                                printf("\n");
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

                        retval = UA_Server_writeValue(uaServer, outArrayDataFC2, arrayVariantFC2);
                        if (retval != UA_STATUSCODE_GOOD)
                                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : Failure to initialise arrayFC2Data[] : %s ", UA_StatusCode_name(retval));
                        else
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : UA_Server_writeValue() initialising arrayNodeIdFC2 is successful");

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

                //-------------------------------------------------- ---- TCP->DATA->FC3 (array of WORD i.e. 2 BYTES per register => uint16_t)
                // Function code 3 - Read Holding Registers
                // follow the example in https://github.com/open62541/open62541/blob/master/examples/tutorial_server_variable.c
                // refer to static void addMatrixVariable(UA_Server *server)

		// Data Format:
		// The register values in the response message are packed as two bytes per register, with the first byte containing the high-order bits and the second byte containing the low-order bits.
		// variable nodes : ArrayData, ArraySize
                // ---- TCP->DATA->FC3 (array of int16_t)
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : Adding FC3 ArrayData nodes");
                UA_VariableAttributes vArrayDataFC3Attr = UA_VariableAttributes_default;
		//vArrayDataFC3Attr.specifiedAttributes = ???
                vArrayDataFC3Attr.displayName = UA_LOCALIZEDTEXT("en-US", "3.4.3.1 FC3 ArrayData");
                vArrayDataFC3Attr.description = UA_LOCALIZEDTEXT("en-US", "FC3 ArrayData");
		//vArrayDataFC3Attr.writeMask = ???
		//vArrayDataFC3Attr.userWriteMask = ???
                /* set the variable value constraints */
                vArrayDataFC3Attr.dataType = UA_TYPES[UA_TYPES_UINT16].typeId;
                vArrayDataFC3Attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
                UA_UInt32 arrayFC3Dims[atoi(env_MBNoOfFC3RegisterTCP)]; // UA_UInt32 arrayFC3Dims[32];
                vArrayDataFC3Attr.arrayDimensionsSize = 1;
                vArrayDataFC3Attr.arrayDimensions = arrayFC3Dims;
                vArrayDataFC3Attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
		//vArrayDataFC3Attr.userAccessLevel = ???;
		//vArrayDataFC3Attr.minimumSamplingInterval = ???;
                vArrayDataFC3Attr.historizing = UA_TRUE;

                        /* set the value.  The array dimensions need to be the same for the value */
                        UA_UInt16 *arrayFC3Data;       // eg {true, false, true, false, true, false, ...}
                        size_t arrayFC3Size = atoi(env_MBNoOfFC3RegisterTCP);   // eg 32
                        arrayFC3Data = (UA_UInt16 *)UA_malloc(arrayFC3Size * sizeof(UA_UInt16));
                        if (!arrayFC3Data)
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot allocate memory for arrayFC3 (holding registers)");
                                return UA_FALSE;
                        }
                        else
                        {       // initialise the holding registers array to 0
                                for (size_t index = 0; index < arrayFC3Size; index++)
                                        arrayFC3Data[index] = (UA_UInt16)0;

                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : arrayFC3Size is %ld", arrayFC3Size);
                                printf("arrayFC3Data[] is  ");
                                for (size_t index = 0; index < arrayFC3Size; index++)
                                        printf("%d ", arrayFC3Data[index]);
                                printf("\n");
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
        	        /* vAttr.value is left empty, the server instantiates with the default value */
	                UA_Server_addVariableNode(uaServer, arrayNodeIdFC3,
                	          r4_modbus_TCP_DATA_FC3_Id,
                        	  UA_NODEID_NUMERIC(0, UA_NS0ID_HASPROPERTY), // parent Reference NodeId
                                  UA_QUALIFIEDNAME(namespaceIndex, "FC3_ArrayData"),
                                  UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                  vArrayDataFC3Attr, NULL, &outArrayDataFC3);
                        retval = UA_Server_writeValue(uaServer, outArrayDataFC3, arrayVariantFC3);
                        if (retval != UA_STATUSCODE_GOOD)
                                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : Failure to initialise arrayFC3Data[] : %s ", UA_StatusCode_name(retval));
                        else
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : UA_Server_writeValue() arrayNodeIdFC3 is successful");

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

		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : Adding FC4 ArrayData nodes");
                UA_VariableAttributes vArrayDataFC4Attr = UA_VariableAttributes_default;
                vArrayDataFC4Attr.description = UA_LOCALIZEDTEXT("en-US", "FC4 ArrayData");
                vArrayDataFC4Attr.displayName = UA_LOCALIZEDTEXT("en-US", "3.4.4.1 FC4 ArrayData");

                /* set the variable value constraints */
                vArrayDataFC4Attr.dataType = UA_TYPES[UA_TYPES_UINT16].typeId;
                vArrayDataFC4Attr.valueRank = UA_VALUERANK_ONE_DIMENSION;
                UA_UInt32 arrayFC4Dims[atoi(env_MBNoOfFC4RegisterTCP)]; // = {1};
                vArrayDataFC4Attr.arrayDimensionsSize = 1;
                vArrayDataFC4Attr.arrayDimensions = arrayFC4Dims;
                vArrayDataFC4Attr.accessLevel = UA_ACCESSLEVELMASK_READ;
                vArrayDataFC4Attr.historizing = UA_TRUE;

                        /* set the value.  The array dimensions need to be the same for the value */
                        UA_UInt16 *arrayFC4Data;       // eg {true, false, true, false, true, false, ...}
                        size_t arrayFC4Size = atoi(env_MBNoOfFC4RegisterTCP);   // eg 32
                        arrayFC4Data = (UA_UInt16 *)UA_malloc(arrayFC4Size * sizeof(UA_UInt16));
                        if (!arrayFC4Data)
                        {
                                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : cannot allocate memory for arrayFC4 (input registers)");
                                return UA_FALSE;
                        }
                        else
                        {       // initialise the DISCRETE Inputs array to FALSE
                                for (size_t index = 0; index < arrayFC4Size; index++)
                                        arrayFC4Data[index] = (UA_UInt16)0;

                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : arrayFC4Size is %ld", arrayFC2Size);
                                printf("arrayFC4Data[] is  ");
                                for (size_t index = 0; index < arrayFC4Size; index++)
                                        printf("%d ", arrayFC4Data[index]);
                                printf("\n");

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
                        arrayVariantFC4.type =  &UA_TYPES[UA_TYPES_UINT16];
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
                                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : Failure to initialise arrayFC4Data[] : %s ", UA_StatusCode_name(retval));
                        else
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : UA_Server_writeValue() arrayNodeIdFC4 is successful");

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
		// we do not assume that the connection to slave is still valid
		// check if the connection to slave is still valid
		int retval;

		//printf("SV_ModbusTcp.c : modbusTCPSlaveIsConnected = %d \n", modbusTCPSlaveIsConnected);

		// close and reconnect mb
		modbus_close(mb);
     		retval = modbus_connect(mb);
	                #ifdef DEBUG
        	        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : modbus_connect(mb) return value is %d \n", retval);
                	#endif
		if (UA_TRUE) //if (modbusTCPSlaveIsConnected == UA_FALSE)
		{
                	//retval = modbus_connect(mb);

	                if (retval == 0)	// connected
			{
				if (modbusTCPSlaveIsConnected == UA_FALSE) // if it is originally FALSE, then print out the message, else don't print
					UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : connection to modbus TCP slave is established");
				modbusTCPSlaveIsConnected = UA_TRUE;
			}
                	else
	                {
        	                UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : Error connecting to modbus TCP slave endpoint : %s", modbus_strerror(errno));
                	        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : retry in progress ...");
				modbusTCPSlaveIsConnected = UA_FALSE;
                	}
		}
		modbus_set_slave(mb, tcp_slave_id);

		// just update the values to open62541->modbus nodes

		#ifdef DEBUG
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : inspecting variables : ");
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c :  %d %d",  atoi(env_MBStartFC1AddrTCP), atoi(env_MBNoOfFC1RegisterTCP));
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c :  %d %d",  atoi(env_MBStartFC2AddrTCP), atoi(env_MBNoOfFC2RegisterTCP));
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c :  %d %d",  atoi(env_MBStartFC3AddrTCP), atoi(env_MBNoOfFC3RegisterTCP));
	 	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c :  %d %d",  atoi(env_MBStartFC4AddrTCP), atoi(env_MBNoOfFC4RegisterTCP));
		#endif

	    if (modbusTCPSlaveIsConnected == UA_TRUE)
	    {
		if (FC1tab_reg != NULL)
		{
			// need to subtract 1 from env_MBStartFC1AddrTCP when passing into modbus_read_bits
                	modbus_read_bits(mb, atoi(env_MBStartFC1AddrTCP) - 1, atoi(env_MBNoOfFC1RegisterTCP), FC1tab_reg);
			arrayVariantFC1.data = FC1tab_reg;
	                retval = UA_Server_writeValue(uaServer, arrayNodeIdFC1, arrayVariantFC1);
        	        if (retval != UA_STATUSCODE_GOOD)
                	        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : Failure to update arrayFC1Data[] : %s ", UA_StatusCode_name(retval));
			#ifdef DEBUG
                	else
                        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : Updating data changes in Slave (arrayNodeIdFC1) is successful");
			#endif
		}
		if (FC2tab_reg != NULL)
		{
			// need to subtract 1 from env_MBStartFC2AddrTCP when passing into modbus_read_bits
                	modbus_read_bits(mb, atoi(env_MBStartFC2AddrTCP) - 1, atoi(env_MBNoOfFC2RegisterTCP), FC2tab_reg);
			arrayVariantFC2.data = FC2tab_reg;
	                retval = UA_Server_writeValue(uaServer, arrayNodeIdFC2, arrayVariantFC2);
        	        if (retval != UA_STATUSCODE_GOOD)
                	        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : Failure to update arrayFC2Data[] : %s ", UA_StatusCode_name(retval));
			#ifdef DEBUG
                	else
                        	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : Updating data changes in Slave (arrayNodeIdFC2) is successful");
			#endif
		}
		if (FC3tab_reg != NULL)
		{
			// need to subtract 1 from env_MBStartFC3AddrTCP when passing into modbus_read_registers
                	modbus_read_registers(mb, atoi(env_MBStartFC3AddrTCP) - 1, atoi(env_MBNoOfFC3RegisterTCP), FC3tab_reg);
			arrayVariantFC3.data = FC3tab_reg;
			retval = UA_Server_writeValue(uaServer, arrayNodeIdFC3, arrayVariantFC3);
        	        if (retval != UA_STATUSCODE_GOOD)
                	        UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : Failure to update arrayFC3Data[] : %s ", UA_StatusCode_name(retval));
			#ifdef DEBUG
	                else
        	                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : UA_Server_writeValue() - Updating arrayNodeIdFC3 is successful");
			#endif
		}
		if (FC4tab_reg != NULL)
		{
			// need to subtract 1 from env_MBStartFC3AddrTCP when passing into modbus_read_input_registers
                	modbus_read_input_registers(mb, atoi(env_MBStartFC4AddrTCP) - 1, atoi(env_MBNoOfFC4RegisterTCP), FC4tab_reg);
			arrayVariantFC4.data = FC4tab_reg;
			retval = UA_Server_writeValue(uaServer, arrayNodeIdFC4, arrayVariantFC4);
        	        if (retval != UA_STATUSCODE_GOOD)
                		UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : Failure to update arrayFC4Data[] : %s ", UA_StatusCode_name(retval));
			#ifdef DEBUG
	                else
        	                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : UA_Server_writeValue() - Updating arrayNodeIdFC4 is successful");
			#endif
		}
		#ifdef DEBUG
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : Successfully read modbus register from Slave");
		#endif
	   } // if (modbusTCPSlaveIsConnected==TRUE)
	}

	return 0;
  } // if (mb)
  else 
  {
	UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_ModbusTcp.c : Failure to get a handle to MB object");
	return -1;
  }
}

