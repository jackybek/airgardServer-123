//#define DEBUG
#ifdef no_almagamation
//#include <open62541/network_tcp.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/client_config_default.h>
#include <open62541/plugin/create_certificate.h>
#include <open62541/plugin/securitypolicy.h>
#include <open62541/plugin/accesscontrol_default.h>
#include <open62541/plugin/nodestore_default.h>
#include <open62541/plugin/pki_default.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#else
   #define UA_ENABLE_DISCOVERY_MULTICAST
   #include "open62541.h"
#endif

#define DEBUG
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>

#include <lely/co/co.h>
#include <lely/co/dcf.h>
#include <lely/co/dev.h>
#include <lely/co/detail/obj.h>
#include "SV_Misc.h"
#include "SV_CanopenEdsDcf.h"

//extern co_dev_t **CanopenDcfDevices;

int CreateCanOpenDeviceFromEdsDcf(co_dev_t **CanopenDcfDevices)
{
	// This will retrieve the directory where the EDS files are kept
	char* env_canopenEdsDcfLoc;
	env_canopenEdsDcfLoc = getenv("SVR_CANOPEN_EDSDCFLOC");
        if (env_canopenEdsDcfLoc != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CanopenEdsDcf.c :  retrieved environment variable : SVR_CANOPEN_EDSDCFLOC : %s", env_canopenEdsDcfLoc);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CanopenEdsDcf.c : cannot retrieve environment variable <SVR_CANOPEN_EDSDCFLOC>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CanopenEdsDcf.c : default to /home/pi/OPCProject/Lely/canopen/edsdcf/");
                env_canopenEdsDcfLoc = (char*)calloc(100, sizeof(char));
                if (env_canopenEdsDcfLoc != NULL)
                        strcpy(env_canopenEdsDcfLoc, "/home/pi/OPCProject/Lely/canopen/edsdcf/");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CanopenEdsDcf.c : cannot retrieve environment variable <SVR_CANOPEN_EDSDCFLOC : out of memory");
                        return -1;
                }
        }
	// Next, load the EDS and DCF file list
        size_t EdsDcfFileListSize = 0;
        char **EdsDcfFileList = NULL;
        int total_eds_files=0, total_dcf_files=0, counter=0;                    // count the number of files with .pem and .der extension

        struct dirent *de;      // pointer for directory entry
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CanopenEdsDcf.c : canopenEdsDcfLoc data = %s", env_canopenEdsDcfLoc);
        DIR *dr = opendir((char *)env_canopenEdsDcfLoc);
        if (dr == NULL)
                 UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CanopenEdsDcf.c : cannot find Canopen EDS DCF directory %s", env_canopenEdsDcfLoc);
        else
        {
                int retval;
                counter = 0;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,("--------SV_CanopenEdsDcf.c : Start while loop ") );

                while ((de = readdir(dr)) != NULL) // read the list of files in the directory
                {
                        #ifdef DEBUG
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CanopenEdsDcf.c :  iterating through the Canopen EDS DCF directory : pass %d", counter);
                        #endif
                        //char* file_name = de->d_name;
                        // process only the files with .eds extension
                        retval = stringEndsWith((char*)de->d_name, "eds");              // first count the total number .eds files
                        if (retval == 0) // file name ends with .eds
                        {
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CanopenEdsDcf.c : Found EDS filename : <%s>", de->d_name);
                                total_eds_files++;
                        }
                        else
                        {
                                retval = stringEndsWith((char*)de->d_name, "dcf");              // also count the total number of .dcf files
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CanopenEdsDcf.c : Found DCF filename : <%s>", de->d_name);
                                if (retval == 0) // file name ends with .dcf
                                {
                                        total_dcf_files++;
                                }
                        }

                }

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CanopenEdsDcf.c :  pass 1 iterating through the Canopen EDS DCF directory for .eds and .dcf files : %d %d", total_eds_files, total_dcf_files);
                // next, based on the total number of files with .eds .dcf extension, allocate memory to EdsDcfFileList
                EdsDcfFileList = (char **)UA_malloc(total_eds_files+total_dcf_files * sizeof(char*));
		for (int i=0; i < total_eds_files+total_dcf_files; i++)
			EdsDcfFileList[i] = (char*)UA_malloc(100 * sizeof(char));	// actual path/filename

		rewinddir(dr);	// now, rewind the directory pointer to the start
		counter=0;
		while ((de = readdir(dr)) != NULL)	// finally process the .eds and .dcf files and save into EdsDcfFileList
		{
			// get the file
			retval = stringEndsWith((char*)de->d_name, ".eds");
                        char fullpathname[100];
                        if (retval == 0) // file name ends with .eds
                        {
                                strcpy(fullpathname, env_canopenEdsDcfLoc);
                                strcat(fullpathname, de->d_name);
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CanopenEdsDcf.c : .eds file found is <%s>", fullpathname);
				strcpy(EdsDcfFileList[counter], fullpathname);	// save the filename .eds to the array of char*

                                // loadFile needs the full path
                                //trustcertificate = loadFile(fullpathname);
                                //UA_ByteString_copy(&trustcertificate, &trustList[counter]);
                                counter++;
                                //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : contents of the .pem certificate is <%s>", trustcertificate.data); // display the contents >

                        }

                        // get the file (.dcf)
                        retval = stringEndsWith((char*)de->d_name, ".dcf");
                        if (retval == 0) // file name ends with .dcf
                        {
                                strcpy(fullpathname, env_canopenEdsDcfLoc);
                                strcat(fullpathname, de->d_name);
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : .dcf file found is <%s>", fullpathname);
				strcpy(EdsDcfFileList[counter], fullpathname);	// save the filename .dcf to the array of char*

                                // loadFile needs the full path
                                //trustcertificate = loadFile((char*)fullpathname);
                                //UA_ByteString_copy(&trustcertificate,&trustList[counter]);
                                counter++;
                                //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_Encrypt.c : contents of the certificate is %s", trustcertificate.data);       // NULL????????
                        }
                        //skip to the next file

		}
	}

	co_dev_t **dcf_device;
	CanopenDcfDevices  = (co_dev_t **)UA_malloc(counter * sizeof(co_dev_t **));
	dcf_device = CanopenDcfDevices;
	for (int i=0; i<counter; i++)
		dcf_device[i] = UA_malloc(1 * sizeof(co_dev_t *));

	for (int i=0; i<counter; i++)
	{
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "-------------------------------------------------------");
		UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : Processing %d of %d files ", i+1, counter);
		dcf_device[i] = co_dev_create_from_dcf_file(EdsDcfFileList[i]);
		if (dcf_device[i] == NULL)
		{

			UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : fail to get handle to file <%s>", EdsDcfFileList[i]);
		}
		else
		{
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : canopen device for %s is created", EdsDcfFileList[i]);
			#ifdef DEBUG
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : listing the fields of the Canopen EDS/DCF file");
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : netId           = %d ", co_dev_get_netid(dcf_device[i]));
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : Id              = %d ", co_dev_get_id(dcf_device[i]));
			//UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : tree->num_nodes = %d \n", co_dev_get_tree(dcf_device[i]->tree->num_nodes));
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : name            = %s ", co_dev_get_name(dcf_device[i]));
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : vendor_name     = %s ", co_dev_get_vendor_name(dcf_device[i]));
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : vendor_id       = %d ", co_dev_get_vendor_id(dcf_device[i]));
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : product_name    = %s ", co_dev_get_product_name(dcf_device[i]));
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : product_code    = %d ", co_dev_get_product_code(dcf_device[i]));
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : revision        = %d ", co_dev_get_revision(dcf_device[i]));
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : order_code      = %s ", co_dev_get_order_code(dcf_device[i]));
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : baud            = 0x%x ", co_dev_get_baud(dcf_device[i]));
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : rate (kbit/s)   = %d ", co_dev_get_rate(dcf_device[i]));
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : LSS flag        = %d ", co_dev_get_lss(dcf_device[i]));
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : dummy           = %d ", co_dev_get_dummy(dcf_device[i]));
			#endif

			co_unsigned16_t *arrayOfIndices=UA_malloc(100 * sizeof(co_unsigned16_t));
			co_unsigned16_t totalObjectIndices = co_dev_get_idx(dcf_device[i], 100, arrayOfIndices);
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : Total Objects   = %d ", totalObjectIndices);
			#ifdef DEBUG
			for (int i=0; i < totalObjectIndices; i++)
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CanopenEdsDcf.c : index of Object[%d] = %d ", i, arrayOfIndices[i]);
			#endif

			// objects are defined as [nnnn] in eds file
			co_obj_t *canObj;
			co_unsigned8_t *subidx = UA_malloc(10 * sizeof(co_unsigned8_t));
			co_unsigned8_t noOfSubidx;
			canObj = co_dev_first_obj(dcf_device[i]);
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------First CANObject : name               = %s ", co_obj_get_name(canObj));
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------First CANObject : code		  = %d ", co_obj_get_code(canObj));
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------First CANObject : index              = %d ", co_obj_get_idx(canObj));
			noOfSubidx = co_obj_get_subidx(canObj,10, subidx);
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------First CANObject : Total no of sub-indces = %d ", noOfSubidx);
			#ifdef DEBUG
			for (int i=0; i< noOfSubidx; i++)
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------subindex[%d] = %d ", i, subidx[i]);
			#endif
			while (1)
			{
				canObj = co_obj_next(canObj);
				if (canObj == NULL) break;
				#ifdef DEBUG
				UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------Next CANObject : name               = %s ", co_obj_get_name(canObj));
	                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------Next CANObject : code               = %d ", co_obj_get_code(canObj));
	                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------Next CANObject : index              = %d ", co_obj_get_idx(canObj));
				#endif

	                        noOfSubidx = co_obj_get_subidx(canObj,10, subidx);
        	                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------First CANObject : Total no of sub-indces = %d ", noOfSubidx);
				#ifdef DEBUG
                	        for (int i=0; i< noOfSubidx; i++)
                        	        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------subindex[%d] = %d ", i, subidx[i]);
				#endif
			}

			/* not required as the while(1) loop will get all the CANobjects including the last one
			canObj = co_dev_last_obj(dcf_device[i]);
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------Last CANObject : name               = %s ", co_obj_get_name(canObj));
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------Last CANObject : code		 = %d ", co_obj_get_code(canObj));
			*/


		}
	}
	return counter;	// number of Eds/ Dcf files processed
}
