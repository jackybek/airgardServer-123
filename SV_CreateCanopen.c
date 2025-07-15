#include <stdio.h>
#include <lely/co/co.h>
#include <lely/co/dcf.h>
#include <lely/co/dev.h>
#include <lely/co/detail/obj.h>
#include "SV_CreateCanopenMaster.h"

int CreateCanOpenMaster(int);

void *initialiseCanopen(void *arg)
{

        /* Canopen related functions
        1. read the list of slave Eds/ Dcf files
        2. Create a canopen Master instance
        3. Setup a polling cycle using open62541 callbacks (SV_Callbacks.c)
        4. Every polling cycle, the canopen Master will read the slaves (defined in step 1)
        */
        co_dev_t **CanopenDcfDevices=NULL;
        int NumberOfFilesRead = CreateCanOpenDeviceFromEdsDcf(CanopenDcfDevices);
        printf("-----SV_CreateCanopen.c : No of canopen Eds/ Dcf files read : %d \n", NumberOfFilesRead);
        printf("-----SV_CreateCanopen.c : pointer to the list of canopenDcfDevices is *CanopenDcfDevices \n");

	// get the number of devices defined on yaml; extract from Eds/ Dcf file
	int NumberOfDevices = 1;	// TODO
        int retval = CreateCanOpenMaster(NumberOfDevices);
        printf("-----SV_CreateCanopen.c : retval from CreateCanOpenMaster() : %d \n", retval);
        /* end Canopen related functions */
}
