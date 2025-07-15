#define DEBUG

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
#include "SV_Misc.h"
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
#include <lely/co/detail/obj.h>

int CreateCanOpenMaster(void);

int CreateCanOpenMaster()
{
	// This will retrieve the directory where the EDS files are kept
	char* env_canopenEdsDcfLoc;
	env_canopenEdsDcfLoc = getenv("SVR_CANOPEN_EDSDCFLOC");
        if (env_canopenEdsDcfLoc != NULL)
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateCanopenMaster.c :  retrieved environment variabe : SVR_CANOPEN_EDSDCFLOC : %s", env_canopenEdsDcfLoc);
        else
        {
                UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateCanopenMaster.c : cannot retrieve environment variable <SVR_CANOPEN_EDSDCFLOC>");
                UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateCanopenMaster.c : default to /home/pi/OPCProject/canopen/edsdcf/");
                env_canopenEdsDcfLoc = (char*)calloc(100, sizeof(char));
                if (env_canopenEdsDcfLoc != NULL)
                        strcpy(env_canopenEdsDcfLoc, "/home/pi/OPCProject/canopen/edsdcf/");
                else
                {
                        UA_LOG_FATAL(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateCanopenMaster.c : cannot retrieve environment variable <SVR_CANOPEN_EDSDCFLOC : out of memory");
                        return UA_FALSE;
                }
        }
	// Next, load the EDS and DCF file list
        size_t EdsDcfFileListSize = 0;
        char **EdsDcfFileList = NULL;
        int total_eds_files=0, total_dcf_files=0, counter=0;                    // count the number of files with .pem and .der extension

        struct dirent *de;      // pointer for directory entry
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateCanopenMaster.c : canopenEdsDcfLoc data = %s", env_canopenEdsDcfLoc);
        DIR *dr = opendir((char *)env_canopenEdsDcfLoc);
        if (dr == NULL)
                 UA_LOG_WARNING(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateCanopenMaster.c : cannot find Canopen EDS DCF directory %s", env_canopenEdsDcfLoc);
        else
        {
                int retval;
                counter = 0;
                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,("--------SV_CreateCanopenMaster.c : Start while loop ") );

                while ((de = readdir(dr)) != NULL) // read the list of files in the directory
                {
                        #ifdef DEBUG
                        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateCanopenMaster.c :  iterating through the Canopen EDS DCF directory : pass %d", counter);
                        #endif
                        //char* file_name = de->d_name;
                        // process only the files with .eds extension
                        retval = stringEndsWith((char*)de->d_name, "eds");              // first count the total number .eds files
                        if (retval == 0) // file name ends with .eds
                        {
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateCanopenMaster.c : Found EDS filename : <%s>", de->d_name);
                                total_eds_files++;
                        }
                        else
                        {
                                retval = stringEndsWith((char*)de->d_name, "dcf");              // also count the total number of .dcf files
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateCanopenMaster.c : Found DCF filename : <%s>", de->d_name);
                                if (retval == 0) // file name ends with .dcf
                                {
                                        total_dcf_files++;
                                }
                        }

                }

                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateCanopenMaster.c :  pass 1 iterating through the Canopen EDS DCF directory for .eds and .dcf files : %d %d", total_eds_files, total_dcf_files);
                // next, based on the total number of files with .eds .dcf extension, allocate memory to EdsDcfFileList
                EdsDcfFileList = (char **)malloc(total_eds_files+total_dcf_files * sizeof(char*));
		for (int i=0; i < total_eds_files+total_dcf_files; i++)
			EdsDcfFileList[i] = (char*)malloc(100 * sizeof(char));	// actual path/filename

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
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_CreateCanopenMaster.c : .eds file found is <%s>", fullpathname);
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
                                UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CreateCanopenMaster.c : .dcf file found is <%s>", fullpathname);
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

	co_dev_t *dcf_handle;
	for (int i=0; i<counter; i++)
	{
		dcf_handle = co_dev_create_from_dcf_file(EdsDcfFileList[i]);
		if (dcf_handle != NULL)
		{
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CreateCanopenMaster.c : canopen device for %s is created", EdsDcfFileList[i]);
			#ifdef DEBUG
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CreateCanopenMaster.c : listing the fields of the Canopen EDS/DCF file");
			UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CreateCanopenMaster.c : netId = %d", dcf_handle->netId);
			#endif


		}
		else
			UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "--------SV_CreateCanopenMaster.c : fail to get handle to file <%s>", EdsDcfFileList[i]);

	}
}
