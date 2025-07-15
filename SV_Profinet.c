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
/*
#include "sampleapp_common.h"
#include "app_gsdml.h"
#include "app_log.h"
#include "app_utils.h"
*/

#include <osal.h>
#include <osal_log.h> /* For LOG_LEVEL */
#include <pnal.h>
#include <pnal_filetools.h>
#include <pnet_api.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#if PNET_MAX_PHYSICAL_PORTS == 1
#define APP_DEFAULT_ETHERNET_INTERFACE "eth0"
#else
#define APP_DEFAULT_ETHERNET_INTERFACE "br0,eth0,eth1"
#endif

#define APP_MAIN_SLEEPTIME_US          5000 * 1000
#define APP_SNMP_THREAD_PRIORITY       1
#define APP_SNMP_THREAD_STACKSIZE      256 * 1024 /* bytes */
#define APP_ETH_THREAD_PRIORITY        10
#define APP_ETH_THREAD_STACKSIZE       4096 /* bytes */
#define APP_BG_WORKER_THREAD_PRIORITY  5
#define APP_BG_WORKER_THREAD_STACKSIZE 4096 /* bytes */

pnet_cfg_t *createProfinetExtension(UA_Server *);

pnet_cfg_t *createProfinetExtension(UA_Server *uaServer)
{
	pnet_cfg_t pnet_cfg = {0};
	pnet_if_cfg_t netif_cfg = {0};
	uint16_t number_of_ports = 1;


   UA_LOG_INFO (UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "------SV_Profibus.c : Number of slots:      %u (incl slot for DAP module", PNET_MAX_SLOTS);
   UA_LOG_INFO (UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "------SV_Profibus.c : P-net log level:      %u (TRACE=0, FATAL=600)\n", UA_LOGLEVEL);
   UA_LOG_INFO (UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "------SV_Profibus.c : App log level:        %u (TRACE=0, FATAL=600)\n", UA_LOGLEVEL);
   UA_LOG_INFO (UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "------SV_Profibus.c : Max number of ports:  %u\n", PNET_MAX_PHYSICAL_PORTS);
   UA_LOG_INFO (UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "------SV_Profibus.c : Network interfaces:   %s\n", app_args.eth_interfaces);
   UA_LOG_INFO (UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "------SV_Profibus.c : Button1 file:         %s\n", app_args.path_button1);
   UA_LOG_INFO (UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "------SV_Profibus.c : Button2 file:         %s\n", app_args.path_button2);
   UA_LOG_INFO (UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "------SV_Profibus.c : Default station name: %s\n", app_args.station_name);

   /* Prepare configuration */
   app_pnet_cfg_init_default (&pnet_cfg);
   strcpy (pnet_cfg.station_name, app_args.station_name);
   ret = app_utils_pnet_cfg_init_netifs (
      app_args.eth_interfaces,
      &netif_name_list,
      &number_of_ports,
      &netif_cfg);
   if (ret != 0)
   {
      exit (EXIT_FAILURE);
   }
   pnet_cfg.if_cfg = netif_cfg;
   pnet_cfg.num_physical_ports = number_of_ports;

   app_utils_print_network_config (&netif_cfg, number_of_ports);
   UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "------SV_Profibus.c : Interval (1/32 ms):   %u\n", pnet_cfg.min_device_interval);
   UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "------SV_Profibus.c : Tick rate (us):       %u\n", pnet_cfg.tick_us);


   /* Operating system specific settings */
   pnet_cfg.pnal_cfg.snmp_thread.prio = APP_SNMP_THREAD_PRIORITY;
   pnet_cfg.pnal_cfg.snmp_thread.stack_size = APP_SNMP_THREAD_STACKSIZE;
   pnet_cfg.pnal_cfg.eth_recv_thread.prio = APP_ETH_THREAD_PRIORITY;
   pnet_cfg.pnal_cfg.eth_recv_thread.stack_size = APP_ETH_THREAD_STACKSIZE;
   pnet_cfg.pnal_cfg.bg_worker_thread.prio = APP_BG_WORKER_THREAD_PRIORITY;
   pnet_cfg.pnal_cfg.bg_worker_thread.stack_size = APP_BG_WORKER_THREAD_STACKSIZE;

   ret = app_pnet_cfg_init_storage (&pnet_cfg, &app_args);
   if (ret != 0)
   {
      printf ("Failed to initialize storage.\n");
      printf ("Aborting application\n");
      exit (EXIT_FAILURE);
   }

	return pnet_cfg;
}


