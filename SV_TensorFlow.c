#include "open62541.h"
#include <tensorflow/c/c_api.h>

extern float TensorDataArray[2600][4]; 	/* 1 record per second over 1 hour = 3600 rows,*/
					/* 4 columns - attributes (id, speed, temperature, displacement) */
					/* 6 modbus serial vibration sensors */
					/* Total records per hour = 3600 * 6 = 21,600 */
int tensorFlow(void);

int tensorFlow()
{
#ifdef KIV
	Py_initialise();
	PyObject *pModule = PyImport_ImportModule(" ");	// e.g. my_module.py
	if (!pModule)
	{
		PyErr_Print();
		return (UA_FALSE);
	}
	// do something

	Py_Finalized();
#endif
	return UA_TRUE;
}
