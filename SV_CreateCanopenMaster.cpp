#include <stdio.h>
#ifdef REPLACED
#include <lely/co/co.h>
#include <lely/co/dcf.h>
#include <lely/co/dev.h>
#include <lely/co/detail/obj.h>
#include <lely/coapp/master.hpp>
#endif

#include <lely/ev/loop.hpp>
#include <lely/io2/linux/can.hpp>
#include <lely/io2/posix/poll.hpp>
#include <lely/io2/sys/io.hpp>
#include <lely/io2/sys/sigset.hpp>
#include <lely/io2/sys/timer.hpp>
#include <lely/coapp/fiber_driver.hpp>
#include <lely/coapp/master.hpp>

#include <iostream>

using namespace std::chrono_literals;
using namespace lely;


extern "C" {
#include <stdio.h>
#include "lely_pro_top.h"
#include "SV_CreateCanopenMaster.h"
// This driver inhertis from FiberDriver, which means all CANopen event callbacks, such as OnBoot, run as a task insde a "fiber" (or stackful coroutine)
class MyDriver : public canopen::FiberDriver {
	public:
		using FiberDriver::FiberDriver;

	private:
	// This function gets called when the boot-up process of the slave completes.

	void OnBoot(canopen::NmtState /*st*/, char es, const std::string& what) noexcept override
	{
		if (!es || es == 'L')
			std::cout << "slave " << static_cast<int>(id()) << " booted successfully" << std::endl;
		else
			std::cout << "slave " << static_cast<int>(id()) << " failed to boot: " << what << std::endl;
	}

	void OnConfig(std::function<void(std::error_code ec)> res) noexcept override
	{
   	  try
	  {
      	// Perform a few SDO write requests to configure the slave. The
      	// AsyncWrite() function returns a future which becomes ready once the
      	// request completes, and the Wait() function suspends the coroutine for
      	// this task until the future is ready.

      	// Configure the slave to monitor the heartbeat of the master (node-ID 1)
      	// with a timeout of 2000 ms.
      		Wait(AsyncWrite<uint32_t>(0x1016, 1, (1 << 16) | 2000));
      	// Configure the slave to produce a heartbeat every 1000 ms.
      		Wait(AsyncWrite<uint16_t>(0x1017, 0, 1000));
      	// Configure the heartbeat consumer on the master.
      		ConfigHeartbeat(2000ms);

      	// Report success (empty error code).
      		res({});
    	  }
	  catch (canopen::SdoError& e)
	  {
      	// If one of the SDO requests resulted in an error, abort the
      	// configuration and report the error code.
      		res(e.code());
    	  }
	}

	void OnDeconfig(std::function<void(std::error_code ec)> res) noexcept override
	{
	  try
	  {
      	// Disable the heartbeat consumer on the master.
      		ConfigHeartbeat(0ms);
      	// Disable the heartbeat producer on the slave.
      		Wait(AsyncWrite<uint16_t>(0x1017, 0, 0));
      	// Disable the heartbeat consumer on the slave.
      		Wait(AsyncWrite<uint32_t>(0x1016, 1, 0));
      		res({});
    	  }
	  catch (canopen::SdoError& e)
	  {
      		res(e.code());
    	  }
  	}

	void OnRpdoWrite(uint16_t idx, uint8_t subidx) noexcept override {

	}
};

int CreateCanOpenMaster(co_dev_t **CanopenDcfDevices, int NumberOfDevices)
{

	printf("Entering SV_CreateCanopenMaster.cpp : CreateCanOpenMaster() function \n");

	// step 2 : create a canopen master
	//https://opensource.lely.com/canopen/assets/files/master.cpp
	// Initialize the I/O library - required for Windows but no-op on Linux

//printf("1 \n");
	io::IoGuard io_guard;
	// Create an I/O context to synchronize I/O services during shutdown
//printf("2 \n");
	io::Context ctx;
	// Create a platform-specific I/O polling instance to monitor the CAN bus, as well as timers and signals
//printf("3 \n");
	io::Poll poll(ctx);
	// Create a polling event loop and pass it the platform-independent polling interface.
	// If no tasks are pending, the event loop will poll for I/O events
//printf("4 \n");
	ev::Loop loop(poll.get_poll());

	// I/O devices only need access to the executor interface of the event loop
//printf("5 \n");
	auto exec = loop.get_executor();

	// Create a timer using a monotonic clock, i.e. a clock that is not affected
	// by discontinuous jumps in the system time
//printf("6 \n");
	io::Timer timer(poll, exec, CLOCK_MONOTONIC);

	// create a virtual SocketCAN CAN controller and channel, and do not
	// modify the current CAN bus state or bitrate
//printf("7 \n");
#ifdef COMMENT
	sudo modprobe vcan
 	sudo ip link add dev vcan0 type vcan
 	sudo ip link set up vcan0
	ip addr | grep "can"
		6: vcan0: <NOARP,UP,LOWER_UP> mtu 72 qdisc noqueue state UNKNOWN group default qlen 1000
    			link/can
		7: can0: <NOARP> mtu 72 qdisc noop state DOWN group default qlen 1000
    			link/can

	lsmod | grep "can"
		vcan                   12288  0
		can_dev                49152  1 vcan
		can_raw                20480  0
		can                    24576  1 can_raw

	sudo ip link set vcan0 down
	sudo ip link set vcan0 txqueuelen 1000
	sudo ip link set vcan0 up

	# the following only for physical CAN IO board
	sudo ip link set can0 type can bitrate 500000 restart-ms 100
#endif
	io::CanController ctrl("vcan0");
//printf("8 \n");
	io::CanChannel chan(poll, exec);
//printf("9 \n");
	chan.open(ctrl);
	printf("CanController vcan0 is created \n");

	// Create a CANopen Master with node-ID 1.  The master is asynchronous,
	// which means every user-defined callback for a CANopen event will be
	// posted as a task on the event loop, instead of being invoked during
	// the event processing by the stack
	canopen::AsyncMaster master(timer, chan, "/home/pi/OPCProject/Lely/canopen/edsdcf/master.dcf", "", 1);
	// Note: canopen_master.yml file contains references to slaves in the form of a directory e.g.
	// slave_2:
  	//     dcf: "pro_com_can_open__pro_top.eds"
  	//     bin: "/home/pi/OPCProject/Lely/canopen/edsdcf/"
  	//     node_id: 2
	// generate master.dcf from canopen_master.yml

	// Create a driver for the slave with node_ID 2
	// TODO modify to use the list of CanopenDcfDevices passed in from SV_main.c
	MyDriver driver(exec, master, 2);	// 1st device, node_id = 2
	for (int i =0; i < NumberOfDevices-1; i++)	// subsequent devices, node_id start from 3
	{
		int start = 2+i+1;
		MyDriver driver(exec, master, start);
	}
	// Create a signal handler
	io::SignalSet sigset(poll, exec);
	// Watch for Ctrl+C or process termination
	sigset.insert(SIGHUP);
	sigset.insert(SIGINT);
	sigset.insert(SIGTERM);

	sigset.submit_wait([&](int /*signo*/)
	{
		// If the signal is raised again, terminate immediately.
    		sigset.clear();
    		// Tell the master to start the deconfiguration process for all nodes, and
    		// submit a task to be executed once that process completes.
    		master.AsyncDeconfig().submit(exec, [&]()
		{
      			// Perform a clean shutdown.
      			ctx.shutdown();
    		});
	});

  	// Start the NMT service of the master by pretending to receive a 'reset
  	// node' command.
  	master.Reset();
	// Run the event loop until no tasks remain (or the I/O context is shut down).
  	loop.run();


        // step 3 :


        // finally, create OPCUA nodes representing canopen devices

	return 0;











#ifdef REPLACED
	const io_timer_vtbl* const *tBase;
printf("A \n");
	lely::io::TimerBase myCanopenTimer;
	myCanopenTimer::TimerBase(&tBase);
	//auto myCanopenTimer = lely::io::TimerBase(1, tBase);
printf("B \n");
	io_can_chan_vtbl* const *cBase;
printf("C \n");
	auto myCanopenChannel = lely::io::CanChannelBase(cBase);
printf("D \n");
	std::string mydcf_txt = "/home/pi/OPCProject/canopen/edsdcf/";
	std::string mydcf_bin = "";
	uint8_t myid=0xff;
#endif


#ifdef LINK_ERROR
/usr/bin/ld: /usr/local/lib//liblely-co.a(liblely_co_la-emcy.o): in function `co_emcy_send':
/usr/local/src/lely-core/build/src/co/../../../src/co/emcy.c:891: undefined reference to `can_buf_reserve'
/usr/bin/ld: /usr/local/lib//liblely-co.a(liblely_co_la-emcy.o): in function `__co_emcy_init':
/usr/local/src/lely-core/build/src/co/../../../src/co/emcy.c:307: undefined reference to `can_buf_fini'
/usr/bin/ld: /usr/local/lib//liblely-co.a(liblely_co_la-emcy.o): in function `__co_emcy_fini':
/usr/local/src/lely-core/build/src/co/../../../src/co/emcy.c:325: undefined reference to `can_buf_fini'
/usr/bin/ld: /usr/local/lib//liblely-co.a(liblely_co_la-nmt.o): in function `__co_nmt_init':
/usr/local/src/lely-core/build/src/co/../../../src/co/nmt.c:1135: undefined reference to `can_buf_fini'
/usr/bin/ld: /usr/local/lib//liblely-co.a(liblely_co_la-nmt.o): in function `__co_nmt_fini':
/usr/local/src/lely-core/build/src/co/../../../src/co/nmt.c:1223: undefined reference to `can_buf_fini'
/usr/bin/ld: /usr/local/lib//liblely-co.a(liblely_co_la-nmt.o): in function `co_nmt_cs_req':
/usr/local/src/lely-core/build/src/co/../../../src/co/nmt.c:1853: undefined reference to `can_buf_reserve'
#endif

#ifdef LINK_ERROR
        auto myCanopenMaster = lely::canopen::BasicMaster(NULL, myCanopenTimer,
					myCanopenChannel, mydcf_txt, mydcf_bin, myid);
	myCanopenMaster::reset();
#endif

	// step 3 :


	// finally, create OPCUA nodes representing canopen devices

	return 0;
 
} // CreateCanOpenMaster()
} // extern "C"
