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
#include <Python.h>


void x()
{
    PbMaster *master = NULL;
    int result = 0;

    if (confdir == NULL) {
        confdir = ".";
    }

    try {
        // Parse the config file.
        PbConf *config = PbConf_fromFile(confdir + "/example_dummy_oneslave.conf");

        // Create a DP master.
        master = PbConf_makeDPM(config);

        // Create the slave descriptions.
        byte outData[MAX_SLAVES][2]; // Assuming a maximum number of slaves
        memset(outData, 0, sizeof(outData));

        for (int i = 0; i < config->slaveCount; i++) {
            PbSlaveConf *slaveConf = &config->slaveConfs[i];
            PbSlaveDesc *slaveDesc = PbSlaveConf_makeDpSlaveDesc(slaveConf);

            // Set User_Prm_Data
            byte dp1PrmMask[3] = {DPV1PRM0_FAILSAFE, DPV1PRM1_REDCFG, 0x00};
            byte dp1PrmSet[3] = {DPV1PRM0_FAILSAFE, DPV1PRM1_REDCFG, 0x00};
            PbSlaveDesc_setUserPrmData(slaveDesc, PbGsd_getUserPrmData(slaveConf->gsd, dp1PrmMask, dp1PrmSet));

            // Register the slave at the DPM
            PbMaster_addSlave(master, slaveDesc);

            // Set initial output data.
            outData[i][0] = 0x42;
            outData[i][1] = 0x24;
        }

        // Initialize the DPM
        PbMaster_initialize(master);

        // Run the slave state machine.
        while (1) {
            // Write the output data.
            for (int i = 0; i < config->slaveCount; i++) {
                PbSlaveDesc *slaveDesc = PbMaster_getSlaveList(master)[i];
                PbSlaveDesc_setMasterOutData(slaveDesc, outData[i]);
            }

            // Run slave state machines.
            PbSlaveDesc *handledSlaveDesc = PbMaster_run(master);

            // Get the in-data (receive)
            if (handledSlaveDesc) {
                byte *inData = PbSlaveDesc_getMasterInData(handledSlaveDesc);
                if (inData != NULL) {
                    // In our example the output data shall be the inverted input.
                    outData[0][0] = inData[1];
                }
            }

            // Feed the system watchdog, if it is available.
            if (watchdog != NULL) {
                watchdog();
            }
        }

    } catch (ProfibusError e) {
        printf("Terminating: %s\n", e.message);
        result = 1;
    } finally {
        if (master) {
            PbMaster_destroy(master);
        }
    }
    return result;
}

void createProfibusMasterExtension(UA_Server *server)
{
	Py_Initialize();

	PyObject *sys = PyImport_ImportModule("sys"); //import sys
	//sys.path.insert(0, "..")
	PyObject *pyprofibus = PyImport_ImportModule("pyprofibus"); //import pyprofibus

	def main(confdir=".", watchdog=None):
	master = None
	try:
		# Parse the config file.
		config = pyprofibus.PbConf.fromFile(confdir + "/example_dummy_oneslave.conf")

		# Create a DP master.
		master = config.makeDPM()

		# Create the slave descriptions.
		outData = {}
		for slaveConf in config.slaveConfs:
			slaveDesc = slaveConf.makeDpSlaveDesc()

			# Set User_Prm_Data
			dp1PrmMask = bytearray((pyprofibus.dp.DpTelegram_SetPrm_Req.DPV1PRM0_FAILSAFE,
						pyprofibus.dp.DpTelegram_SetPrm_Req.DPV1PRM1_REDCFG,
						0x00))
			dp1PrmSet  = bytearray((pyprofibus.dp.DpTelegram_SetPrm_Req.DPV1PRM0_FAILSAFE,
						pyprofibus.dp.DpTelegram_SetPrm_Req.DPV1PRM1_REDCFG,
						0x00))
			slaveDesc.setUserPrmData(slaveConf.gsd.getUserPrmData(dp1PrmMask=dp1PrmMask,
									      dp1PrmSet=dp1PrmSet))


			# Register the slave at the DPM
			master.addSlave(slaveDesc)

			# Set initial output data.
			outData[slaveDesc.name] = bytearray((0x42, 0x24))

		# Initialize the DPM
		master.initialize()

		# Run the slave state machine.
		while True:
			# Write the output data.
			for slaveDesc in master.getSlaveList():
				slaveDesc.setMasterOutData(outData[slaveDesc.name])

			# Run slave state machines.
			handledSlaveDesc = master.run()

			# Get the in-data (receive)
			if handledSlaveDesc:
				inData = handledSlaveDesc.getMasterInData()
				if inData is not None:
					# In our example the output data shall be the inverted input.
					outData["first"][0] = inData[1]

			# Feed the system watchdog, if it is available.
			if watchdog is not None:
				watchdog()

	except pyprofibus.ProfibusError as e:
		print("Terminating: %s" % str(e))
		return 1
	finally:
		if master:
			master.destroy()
	return 0

	Py_Finalize();
}
