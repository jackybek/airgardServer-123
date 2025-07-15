//#include <modbus/modbus.h>
//#include <osal.h>
//#include <osal_log.h> /* For LOG_LEVEL */
#//include <pnal.h>
//#include <pnal_filetools.h>
//#include <pnet_api.h>
#include <modbus/modbus.h>

//pnet_cfg_t *createProfinetExtension(UA_Server *);
modbus_t **createModbusExtension(UA_Server *);

void createRepeatedCallbacks(UA_Server *);
void Callback_1(UA_Server *, void *);
void Callback_2(UA_Server *, void *);
void Callback_3(UA_Server *, void *);
void Callback_4(UA_Server *, void *);
void readModbusTCPData(UA_Server *, modbus_t *);
