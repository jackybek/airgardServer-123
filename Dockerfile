###########################################################################################################
#  Dockerfile : creates a docker image that sudos a open62541 instance called airgardServer-33 #
###########################################################################################################

#########################
# -- get the OS 
#########################
FROM debian:bookworm
LABEL maintainer="jacky81100@yahoo.com"
SHELL ["/bin/bash", "-c"]

########################################
# -- update and upgrade OS patches
########################################
sudo apt-get update && apt-get upgrade -y && apt-get dist-upgrade -y 
# if cannot update legacy key do the following
  # apt-key list | grep -A4 "trusted.gpg$"
  # -----get the last 8 characters (excluding the space) under the line in /etc/apt/trusted.gpg
  # sudo apt-key export 90FDDD2E | sudo gpg --dearmor -o /tmp/raspi.gpg
  # sudo apt-key del 90FDDD2E
  # sudo mv /tmp/raspi.gpg /etc/apt/trusted.gpg.d/
  
sudo DEBIAN_FRONTEND="noninteractive" apt-get install -y apt-utils
# -- prepare the build environment for OPC62541
sudo DEBIAN_FRONTEND="noninteractive" apt-get install build-essential pkg-config python3 net-tools iputils-ping -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install cmake-curses-gui -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install check libsubunit-dev -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install libmbedtls-dev -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install wget -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install libreadline-dev -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install gnome -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install libncurses5-dev libncursesw5-dev -y # to detect Ctrl+Key
sudo DEBIAN_FRONTEND="noninteractive" apt-get install ninja-build -y

####################################
# -- install ntp client
####################################
sudo apt-get install ntp -y
cd /etc/ntpsec/ntp.conf
sudo nano ntp.conf
server 192.168.1.157 (NTP server IP address)
sudo service ntp start
sudo service ntp status
<To verify with my notes in SG>

####################################
setup Windows as ntp server
####################################
go to regedit - Computer\HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\W32Time\Parameters
LocalNTP = 1 REG_DWORD

setup linux as ntp server

#########################################################
# -- build GCC from source to get the latest version
# -- https://iq.opengenus.org/build-gcc-from-source/
#########################################################
WORKDIR /root
sudo DEBIAN_FRONTEND="noninteractive" apt-get install git -y
# -------DO NOT remove GCC otherwise git-clone will be very slow
#sudo DEBIAN_FRONTEND="noninteractive" apt-get remove gcc -y

###############################
# -- pre-requisites for GCC
###############################
sudo DEBIAN_FRONTEND="noninteractive" apt-get install flex -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install zlib1g-dev -y

#############################################################################
# -- get gcc from source using git-clone : https://gcc.gnu.org/gcc-14/
#############################################################################
sudo git clone git://gcc.gnu.org/git/gcc.git
WORKDIR /root/gcc
sudo git checkout releases/gcc-14.2.0
sudo ./contrib/download_prerequisites
WORKDIR /root
sudo mkdir objdir
WORKDIR /root/objdir
sudo DEBIAN_FRONTEND="noninteractive" apt-get install g++
sudo ../gcc/configure --target=aarch64-linux-gnu --prefix=/usr/local/gcc14.2.0 --disable-multilib --with-system-zlib --enable-languages=c,c++ --program-suffix=14.2.0
ulimit -m unlimited
ulimit -v unlimited
sudo make -j4                     
sudo make install
WORKDIR /etc
su -
sudo echo "PATH=/usr/local/gcc14.2.0/bin:"$PATH > environment 
# original environment : PATH=/root/cmake-3.31.6/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
sudo source /etc/environment
sudo echo $PATH
WORKDIR /usr/local/gcc14.2.0/bin
sudo ln -s gcc14.2.0 gcc
sudo gcc --version -a

cd /bin
sudo ln -s /usr/local/gcc14.2.0/bin/aarch64-linux-gnu-gcc14.2.0 gcc 
sudo ln -s /usr/local/gcc14.2.0/bin/aarch64-linux-gnu-c++14.2.0 cpp
sudo ln -s /usr/local/gcc14.2.0/bin/aarch64-linux-gnu-g++14.2.0 g++
sudo ln -s /usr/local/gcc14.2.0/bin/aarch64-linux-gnu-c++14.2.0 c++
which gcc
which cpp
which g++
which c++

WORKDIR /etc/profile.d
#binaries are installed in:/usr/local/gcc14.2.0/bin
#Libraries have been installed in: /usr/local/gcc14.2.0/lib
#If you ever happen to want to link against installed libraries
#in a given directory, LIBDIR, you must either use libtool, and
#specify the full pathname of the library, or use the `-LLIBDIR'
#flag during linking and do at least one of the following:
#   - add LIBDIR to the `LD_LIBRARY_PATH' environment variable
#     during execution
#   - add LIBDIR to the `LD_RUN_PATH' environment variable
#     during linking
#   - use the `-Wl,-rpath -Wl,LIBDIR' linker flag
#   - have your system administrator add LIBDIR to `/etc/ld.so.conf'

###################################################
# -- remove default openssl libraries
###################################################
sudo DEBIAN_FRONTEND="noninteractive" apt-get remove openssl -y

#######################
# -- install zlib
#######################
sudo DEBIAN_FRONTEND="noninteractive" apt-get install zlib1g-dev -y
# -- alternate - compile from source
#WORKDIR /root
#sudo wget https://github.com/madler/zlib/releases/download/v1.2.13/zlib-1.2.13.tar.gz
#sudo tar -xvf zlib-1.2.13.tar.gz
#WORKDIR /root/zlib-2.2.13
#sudo ./configure --prefix=/usr/local/zlib
#sudo make
#sudo make install

####################################
# -- reinstall openssl libraries
####################################
sudo apt-get remove openssl
WORKDIR /usr/local/src/
sudo wget https://www.openssl.org/source/openssl-3.5.1.tar.gz
sudo tar -xf openssl-3.5.1.tar.gz
sudo pwd
WORKDIR /usr/local/src/openssl-3.5.1/
sudo ./config --prefix=/usr/local/ssl --openssldir=/usr/local/ssl/ shared zlib
sudo make -j4
# sudo make test
sudo make install
WORKDIR /etc/ld.so.conf.d/
sudo touch openssl-open62541.conf 
sudo echo "/usr/local/ssl/" | sudo tee -a /etc/ld.so.conf.d/openssl-open62541.conf
export LD_LIBRARY_PATH=/usr/local/ssl/lib/
WORKDIR /etc/profile.d
sudo echo "export LD_LIBRARY_PATH=/usr/local/ssl/lib; ldconfig" | sudo tee -a ssl_export_ld_library_path.sh
sudo chmod 744 ssl_export_ld_library_path.sh

sudo ldconfig -v
WORKDIR /etc/
sudo echo ":/usr/local/ssl/bin" | sudo tee -a environment 
source /etc/environment
sudo echo $PATH
sudo /usr/local/ssl/bin/openssl version -a
sudo openssl version

$$$$$$$$$$$$$$$$$$$$$$$$$$
How to resolve this error
$$$$$$$$$$$$$$$$$$$$$$$$$$
openssl: /lib/aarch64-linux-gnu/libssl.so.3: version `OPENSSL_3.4.0' not found (required by openssl)
openssl: /lib/aarch64-linux-gnu/libssl.so.3: version `OPENSSL_3.2.0' not found (required by openssl)
openssl: /lib/aarch64-linux-gnu/libcrypto.so.3: version `OPENSSL_3.3.0' not found (required by openssl)
openssl: /lib/aarch64-linux-gnu/libcrypto.so.3: version `OPENSSL_3.5.0' not found (required by openssl)
openssl: /lib/aarch64-linux-gnu/libcrypto.so.3: version `OPENSSL_3.4.0' not found (required by openssl)
openssl: /lib/aarch64-linux-gnu/libcrypto.so.3: version `OPENSSL_3.2.0' not found (required by openssl)

cd /lib/aarch64-linux-gnu/
sudo mv libssl.so.3 libssl.so.3.original
sudo mv libcrypto.so.3 libcrypto.so.3.original
sudo ln -s /usr/local/ssl/lib/libssl.so.3 libssl.so.3
sudo ln -s /usr/local/ssl/lib/libcrypto.so.3 libcrypto.so.3

OR

sudo apt-get install libssl-dev

############################################################################
# -- build CMAKE from source to get the latest version : https://cmake.org
# -- https://markusthill.github.io/blog/2024/installing-cmake/
############################################################################
WORKDIR /root
sudo DEBIAN_FRONTEND="noninteractive" apt-get remove --purge --autoremove cmake -y
WORKDIR /usr/local/src/
sudo wget https://cmake.org/files/v3.31/cmake-3.31.6.tar.gz
sudo tar -xvf cmake-3.31.6.tar.gz
cd cmake-3.31.6/

export CXX=/usr/local/gcc14.2.0/bin/aarch64-linux-gnu-c++14.2.0
export CXXFLAGS="-std=c++11"

sudo ./configure
# if the above step fail do the following
  # export OPENSSL_INCLUDE=/usr/local/ssl/include/
  # export OPENSSL_LIBRARIES=/usr/local/ssl/lib/
  # export OPENSSL_INCLUDE=/usr/local/ssl/include/
  # in /etc/profile.d, add the above 3 lines
  # sudo apt-get install libssl-dev
sudo gmake
sudo make install
WORKDIR /etc/profile.d
sudo echo "export CMAKE_ROOT=/usr/local/src/cmake-3.31.6/bin/; sudo ldconfig" | sudo tee -a cmake_export_CMAKE_root_path.sh
sudo ldconfig -v
WORKDIR /etc
#sudo echo "export PATH=export PATH=/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin:/usr/X11/bin:/root/cmake-3.31.6/bin:$PATH" | tee -a environment
sudo echo "PATH=/usr/local/src/cmake-3.31.6/bin:"$PATH > environment
source /etc/environment
sudo echo $PATH
sudo cmake --version -a

OR
# -- alternative - use apt-get
# DEBIAN_FRONTEND="noninteractive" apt-get install cmake



######################################
# -- add emscripten capability
######################################
cd /usr/local/src
sudo git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
sudo ./emsdk install latest
sudo ./emsdk activate latest
source ./emsdk_env.sh
#note - refer to .bash_profile

Output
Setting up EMSDK environment (suppress these messages with EMSDK_QUIET=1)
Adding directories to PATH:
PATH += /usr/local/src/emsdk
PATH += /usr/local/src/emsdk/upstream/emscripten

Setting environment variables:
PATH = /usr/local/src/emsdk:/usr/local/src/emsdk/upstream/emscripten:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/local/games:/usr/games
EMSDK = /usr/local/src/emsdk
EMSDK_NODE = /usr/local/src/emsdk/node/22.16.0_64bit/bin/node


Next steps:
- To conveniently access emsdk tools from the command line,
  consider adding the following directories to your PATH:
    /usr/local/src/emsdk
    /usr/local/src/emsdk/upstream/emscripten
- This can be done for the current shell by running:
    source "/usr/local/src/emsdk/emsdk_env.sh"
- Configure emsdk in your shell startup scripts by running:
    echo 'source "/usr/local/src/emsdk/emsdk_env.sh"' >> $HOME/.bash_profile

To turn off CSP,
1. install 'Allows CSP:Content-Security Policy' from Chrome webstore
2. Click the P: icon on the top right corner and 'Toggle ON'
3. Click the button to the right and choose P: to anchor it

######################################
# -- add websockets capability
######################################
libuv is required to build the lwsws (used as SV_Warmcat.c):
# https://github.com/libuv/libuv
go to https://dist.libuv.org/dist/v1.51.0/
download libuv-v1.51.0.tar.gz to /usr/local/src
cd /usr/local/src/
sudo tar -xvf libuv-v1.51.0.tar.gz
cd libuv-v1.51.0
sudo apt-get install autogen autoconf automake libtool
sudo sh autogen.sh
sudo ./configure
sudo make
make check && sudo make install

#if the above does not work, the try the following 
    sudo apt-get install libuv1 libuv1-dev, then rebuild libwebsockets

# -- library is installed  to /usr/local/include
WORKDIR /usr/local/src/
sudo DEBIAN_FRONTEND="noninteractive" apt-get install git -y
sudo git clone https://libwebsockets.org/repo/libwebsockets
WORKDIR /usr/local/src/libwebsockets
sudo mkdir build
WORKDIR /usr/local/src/libwebsockets/build
sudo cmake .. -DLWS_WITH_LWSWS=1 -DLWS_WITH_LIBUV=1 -DLWS_WITH_PLUGINS=1 -DLWS_WITH_PLUGINS_API=1 -DCMAKE_BUILD_TYPE=DEBUG

#sudo DEBIAN_FRONTEND="noninteractive" apt-get remove git -y
# -- alternative - use apt-get
#sudo echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections
#sudo DEBIAN_FRONTEND="noninteractive" apt-get install libwebsockets-dev -y

#in /usr/local/src/libwebsockets/lib/core/context.c line 9292 comment out :
If you don't care about the GID you're running, just comment out this
bit on /usr/local/src/libwebsockets/lib/core/context.c line 929.

         /* if he gave us names, set the uid / gid */
         if (lws_plat_drop_app_privileges(context, 0) ||
             lws_fi(&context->fic, "ctx_createfail_privdrop"))
                 goto free_context_fail2;
				 
cd /usr/local/src/libwebsockets/build/
sudo make -j4
sudo make install
sudo ldconfig
sudo pkg-config --modversion libwebsockets

#in github.com/warmcat/libwebsockets/blob/main/lwsws/main.c (which is adopted as SV_WebSocketsWss.c / SV_Warmcat.c[defunct] ), 
	under #if defined(LWS_WITH_PLUGINS)
	add #define INSTALL_DATADIR "/usr/local/src"

Error during make:
====================
In file included from /usr/local/src/libuv-v1.51.0/include/uv.h:72,
                 from /usr/local/include/libwebsockets.h:252,
                 from SV_WebSocketsWss.c:54: [which is actually github.com/warmcat/libwebsockets/blob/main/lwsws/main.c]
/usr/local/src/libuv-v1.51.0/include/uv/unix.h:135:9: error: unknown type name ‘pthread_rwlock_t’; did you mean ‘pthread_cond_t’?
  135 | typedef pthread_rwlock_t uv_rwlock_t;

===> to resolve : in Makefile change -std=c99 to -std=gnu99 in CFLAGS1
====================

#to change connectivity-check sources, go to /usr/local/src/libwebsockets/lib/core/context.c
#look for captive_portal_detect and update 
a. endpoint to "\"captive.apple.com\"."
b. http_url to "\"detect.html\","
OR 
a. endpoint to "\"connectivitycheck\"gstatic.com\","
b. http_url to "\"generate_204\","

!!!!
Note: During open62541 startup, make sure that the file /etc/lwsws/conf.d/svr.virtualskies.com.sg is NOT open, otherwise open62541 will fail to run libwebsockets
Refer to README.lwsws.md at github.com/warmcat/libwebsockets on the options to be configured in svr.virtualskies.com.sg
!!!
check the value of lwsws in /etc/passwd; 
	update conf 
		uid : "???"
		gid : "???"
/etc/lwsws => copy conf
/etc/lwsws/conf.d => copy svr.virtualskies.com.sg
/var/www/svr.virtualskies.com.sg/ => copy index1.html, 404.html

#install websocket client for testing
sudo apt-get install npm
sudo npm install -g wscat
To use: wscat -c wss://192.168.1.109:7681 --cert /usr/local/ssl/certs/Opccert109.pem --key /usr/local/ssl/private/Opcprivate-key109.pem --no-check -s lws-minimal

#install wscat on windows
1. launch powershell in administrator mode
2. powershell -c "irm https://community.chocolatey.org/install.ps1|iex"
3. choco install nodejs --version="20.19.3"
4. wscat -c wss://192.168.1.109:7681 --cert "C:\Users\Jacky Bek\Temp\_temp\AirgardServer-109\certs\Opccert109.pem" --key "C:\Users\Jacky Bek\Temp\_temp\AirgardServer-109\private\Opcprivate-key109.pem" --no-check -s lws-minimal

#create a new user lwsws and associated directories
sudo useradd lwsws
sudo passwd lwsws
sudo visudo 
	lwsws ALL=(ALL:ALL) ALL

mkdir /etc/lwsws
mkdir /etc/lwsws/config.d

refer to lws mounts config.txt

For websockets wss setup 
1. Do not use soft links for files /var/www/svr2.virtualskies.com.sg that references /home/pi/OPCProject/mounts/*
2. Just copy the required files over.
3. websockets https can continue to use soft links

** Note
protocols are stored in /usr/local/share/virtualskies-test-server/plugins/XXX.so

// create my own streaming protocol - lws-stream
1. add protocol to /etc/lwsws/conf.d/svr2.virtualskies.com.sg
		"lws-stream": {
            "status": "ok"
         },

2. in update_arigard_stream.js, document.addEventListener function()
	var ws = new_ws(get_appropriate_ws_url(""), "lws-stream-opc");

 3. Create the c program : protocol_lws_stream_opc.c, taking into these argument:
 a. server url e.g. 192.168.1.109
 b. server port e.g. 4840
 	assembled server url = opc.tcp://192.168.1.109:4840
 c. namespaceindex e.g. 2
 d. creates a client and connects to the server
 e. returns a structure containing all the fields in opcua server

int main(int argv, char **argv)
e.g. compile as : lws-stream-opc --ip=192.168.1.109 --p=4840 --ns=2
 
 4. compile as libprotocol_lws_stream_opc.so
 	deploy to /usr/local/share/virtualskies-test-server/plugins/
  
######################################
# -- add modbus capability
######################################
To configure the RS485 vibration sensor, download : http://www.liyuanz.com/filedownload/144463

WORKDIR /usr/local/src/
sudo wget "https://github.com/stephane/libmodbus/releases/download/v3.1.11/libmodbus-3.1.11.tar.gz"
sudo tar -xvf libmodbus-3.1.11.tar.gz
WORKDIR /usr/local/src/libmodbus-3.1.11/
sudo ./configure
sudo make
sudo make install
<files are generated in /usr/local/lib and /usr/local/include>

sudo raspi-config
Disable login shell
Enable serial port hardware
Enabe SPI interface
Interface options -> Serial Port -> Choose 'No' for
	a. login shell to be accessible over serial
 	b. serial port hardware to be enabled
	
sudo chmod 777 /dev/ttyUSB0
To debug:
dmesg | grep ttyUSB0

Need to get a rs332/rs485 RPI HAT from waveshare to overcome this error: Inappropriate ioctl for device

Modbus TCP : Simulate multiple modbus TCP slaves on Windows using diagslave.exe D:\apps\diagslave\win

diagslave -m tcp -a 43 -p 502
diagslave -m tcp -a 44 -p 503
diagslave -m tcp -a 45 -p 504
diagslave -m tcp -a 46 -p 505

Then in windows -> network connections -> Ethernet NIC -> IpV4 -> Properties -> Advanced -> add IP address
192.168.1.157
192.168.1.158
192.168.1.159
192.168.1.160

go SV_CreateModbus.c and 
1. update SVR_MB_IPV4 = "192.168.1.157 192.168.1.158 192.168.1.159 192.168.1.160"
2. update SVR_MB_PORT = "502 504 504 505"
3. update  env_MBTcpSlaveId = "43 44 45 46"
######################################
# -- add openldap : https://www.openldap.org/doc/admin26/quickstart.html
######################################
cd /usr/local/src/
sudo git clone https://git.openldap.org/openldap/openldap.git/
cd openldap
sudo ./configure --prefix=/usr/local/src/openldap
sudo make depend
sudo make -j4
sudo apt-get install slapd ldap-utils <admin passwd = m...h..24>
sudo dpkg-reconfigure slapd
sudo make test (ignore)
sudo make install

*files are installed to /usr/local/src/openldap/libraries/libldap/.lib

*To install the service (https://www.digitalocean.com/community/tutorials/how-to-install-and-configure-openldap-and-phpldapadmin-on-ubuntu-16-04
sudo apt-get install slapd ldap-utils
sudo dpkg-reconfigure slapd
	omit : no
	domain : bookworm.virtualskies.com
	org : virtualskies
	remove database ? no
	move database ? yes
	admin passwwd : mol..24

######################################
# -- add phpldapadmin
######################################
sudo apt-get install phpldapadmin

To configure:
sudo nano /etc/phpldapadmin/config.php 
servers->setvalue('server', 'name', 'My Ldap server');
servers->setValue('server', 'host', '192.168.1.112');
servers->setValue('server', 'port', 389);
servers->setvalue('server', 'base', array('dc=virtualskies,dc=com'));
uncomment bind_id : servers->setValue('login','bind_id','cn=admin,dc=virtualskies,dc=com');

To login via browser: http://192.168.1.112/phpldapadmin/		# ------------- change this URL if there is a change in IP address
login phpldapadmin : cn=admin,dc=virtualskies,dc=com
password : mol..24

create a child entry->posix group
	Group = users
	GID number = 500
create a child entry->generic:user account
	userid : jbek
	passwd : m..24

Set A: phpldapmin on virtualbox : 192.168.1.102 or 192.168.0.174
login phpldapadmin : cn=admin,dc=bookworm,dc=virtualskies,dc=com
password : mol..24
adjust the settings in /etc/phpldapmin/config.php to match the url and CN

Set B: phpldapmin on RPI : 192.168.1.112 or 192.168.1.115
login phpldapadmin : cn=admin, dc=virtualskies,dc=com
password : mol..24
adjust the settings in /etc/phpldapmin/config.php to match the url and CN

Set C: phpldapadmin on RPI :
login phpldapadmin : cn=jackybek, dc=virtualskies,dc=com
password (1st entry) : mol..24 
password (2nd entry) : 8ebe744e41fa3494536e9648093ab4f4ae156071eb54274c1dbf4c320c2023e0 (same as uid/password for UA_Server)

############################################
# add freeradius-client
############################################
cd /usr/local/src/
sudo git clone https://github.com/freeradius/freeradius-client
cd /usr/local/src/freeradius-client
sudo ./configure
sudo make
sudo make install
cd /usr/local/src/freeradius-client/include
sudo ln -s ../config.h config.h

# libfreeradius-client.a is installed to /usr/local/src/freeradius-client/lib/.libs/
# freeradius-client.h is installed to /usr/local/src/freeradius-client/include/
# source codes are installed to /usr/local/src/freeradius-client/lib/
# program return -1 in buildreq.c

# FreeRadius server is installed at 192.168.1.33 / .109 / .155
# update the configuration 
1. /usr/local/src/freeradius-client/etc/radiusclient.conf
	authserver 	192.168.1.33
	acctserver 	192.168.1.33
2. /usr/local/etc/radiusclient/servers
	192.168.1.33	mysecret

=============
Architecture:
=============
cisco -> FreeRadiusSvr (.33) -> openLdap <- phpldapadmin
               |
	    freeradius-client
=========================================
Setup segment : FreeRadiusSvr -> openLdap
=========================================
#A. define a list of users in /etc/freeradius/3.0/users
myuser Cleartext-Password := "ThisIsALongPassword_24"
        Service-Type = Administrative-User,
        Reply-Message = "cisco-avpair = shell:priv-lvl=15"

#if ldap is configured for freeRadius, then comment out the following
jackybek Cleartext-Password := "ThisIsAnotherLongPassword_88"
        Service-Type = Administrative-User,
        Reply-Message = "cisco-avpair = shell:priv-lvl=15"

sybek1708 Cleartext-Password := "ThisIsAnotherLongPassword_88"
       Service-Type = Administrative-User,
       Reply-Message = "cisco-avpair = shell:priv-lvl=15"

admin Cleartext-Password := "ILoveSnoopyOscar#24"
        Service-Type = Administrative-User,
        Reply-Message = "cisco-avpair = shell:priv-lvl=15"

testuser ClearText-Password := "SimplePassword#24"
        Service-Type = Administrative-User,
        Reply-Message = "cisco-avpair = shell:priv-lvl=15"


#B. in 192.168.1.33 /etc/freeradius/3.0/clients.conf define a list of client machines connecting to freeRadius e.g
client sg200-26 {
        ipaddr = 192.168.1.254
        nastype = cisco
        shortname = smartswitch
        secret = mysecret			// this is used when configuring sg200-26 to use Radius as the authentication server
        require_message_authenticator = no
        proto = udp
}
client APP-subnet1 {
        ipaddr = 192.168.0.0/24
        #netmask = 24
        secret = strongsecret
        #shortname = APP-subnet1
}
client APP-subnet2 {
        ipaddr = 192.168.1.0/24
        #netmask = 24
        secret = strongsecret
        #shortname = APP-subnet2
}
client bookworm-102 {				// this will allow bookworm-102 as a radius-client to connect to RadiusServer-33
        ipaddr = 192.168.1.102
        nastype = other
        shortname = bookworm-102
        secret = strongsecret
        require_message_authenticator = no
        proto = *
}
client myldap-server-88 {
        ipaddr = 192.168.1.88
        nastype = other
        shortname = myldap-88
        secret = strongsecret
        require_message_authenticator = no
        proto = *
}
client myldap-server-112 {
        ipaddr = 192.168.1.112
        nastype = other
        shortname = myldap-112
        secret = strongsecret
        require_message_authenticator = no
        proto = *
}
client myldap-server-115 {
        ipaddr = 192.168.1.115
        nastype = other
        shortname = myldap-115
        secret = strongsecret
        require_message_authenticator = no
        proto = *

#C. in /etc/freeradius/3.0/sites-enabled, <my_server> file
server my_server {
listen {
        type = auth
        ipaddr = *
        port = 1812
}
authorize {
        ldap
        if (ok || updated) {
                update control {
                        Auth-Type := ldap
                }
        }
        # fall back in case something is wrong
        #if (notfound || fail) {
        #       update control {
        #               Auth-Type := ACCEPT
        #       }
        #}
}
authenticate {
        Auth-Type LDAP {
                ldap
        }
}
post-auth {
        update reply {
                Service-Type := Administrative-User,
                Reply-Message := "cisco-avpair = shell:priv-lvl=15"
                }
        }
}

#D. in /etc/freeradius/3.0/mods-enabled, <ldap> file
ldap {
        # URI of the target ldap server
        server = "ldap://192.168.1.33"
        server = "ldap://192.168.1.112"
        server = "ldap://192.168.1.115"

        # the dn from which all searches will start from
        base_dn = 'dc=virtualskies,dc=com'
        #base_dn = 'cn=users,dc=virtualskies,dc=com'

        # Port to connect on, defaults to 389, will be ignored for LDAP URIs
        port = 389

        # Administrator account for searching
        identity = 'cn=admin,dc=virtualskies,dc=com'
        password = 'molekhaven24'

        user {
                # Where to start searching in the tree for users
                base_dn = 'cn=users,dc=virtualskies,dc=com'

                # for Active Directory only
                #filter = "(sAMAccountName=%{%{Stripped-User-Name}:-%{User-Name}})"

                # for openlap :
                filter = "(uid=%{%{Stripped-User-Name}:-%{User-Name}})"

        }

        update reply {
                Service-Type := "Administrative-User"
                Reply-Message := "cisco-avpair = shell:priv-lvl=15"
        }
}

=============
Setup segment : cisco 192.168.1.254
=============
1. Administration -> User Accounts : add an account 'admin/ molekhaven24' => http login
2. Security -> RADIUS -> Add
Server IP Address/ Name : 192.168.1.33
Priority : 1
Key String-> User Defined (PlainText) : mysecret
Authentication port : 1812
Usage Type : Login

Repeat for 192.168.1.109 and 192.168.1.155

3. Security->Management Access Authentication
HTTP : RADIUS
HTTPS : local (meaning use admin/ molekhaven24 as login)

############################################
# add PTPD IEEE 1588 capability - DEFUNCT, refer to the next section
############################################
refer to : https://raw.githubusercontent.com/ptpd/ptpd/master/INSTALL
sudo apt-get install libpcap-dev
sudo apt-get install snmpd libsnmp-dev
sudo apt-get install autoconf automake libtool
cd /usr/local/src/
sudo git clone https://github.com/ptpd/ptpd.git
cd ptpd
sudo autoreconf -vi
sudo ./configure --disable-statistics --with-max-unicast-destinations=2048 --disable-so-timestamping
sudo make

> update test/client-e2e-socket.conf so that is "ptpengine:interface = " settings points to a network interface that is connected to a GMC
> to test: sudo ../src/ptpd2 -c test/client-e2e-socket.conf
> check the log output in /var/run/ptpd2.event.log
> check the statistics output in /var/run/ptpd2.stats.log (without the --disable-statistics flag)
> once verified okay

cd /usr/local/src/ptpd/
sudo make install

############################################
# add LinuxPTP - only for SBC with PTP-ready NIC (e.g. beaglebone black)
############################################
cd /usr/local/src/
sudo git clone git://git.code.sf.net/p/linuxptp/code linuxptp
cd linuxptp
sudo make
sudo make install
# the following executables are installed to /usr/local/sbin
hwstamp_ctl
nsm
phc2sys
phc_ctl
pmc
ptp4l
timemaster
ts2phc
tz2alt

sudo ethtool -T eth0

To integrate PTP capabilities directly into open62541:
------------------
Step 1 : Map out the steps required to perform time sync using command line tools
------------------
Concept:
1. ptp4l daemon synchronises the PTP hardware clock (PHC) on the network card
2. phc2sys utility then synchronises the Linux system time to PHC

1.1 Sychronise the system clock : 
1.1.1 configure UTC-TAI offset : 
sudo pmc -u -b -0 -t 1 "SET GRANDMASTER_SETTINGS_NP clockClass 248 clockAccuracy 0xfe offsetScaledLogVariance 0xffff currentUtcOffset 37 leap61 0 leap59 0 currentUtcOffsetValid 1 ptpTimescale 1 timeTraceable 1 frequencyTraceable 0 timeSource 0xa0"

1.1.2 Synchronise the Master System Clock with PHC : 
======
MASTER 192.168.1.71
======
1. sudo phc2sys -s eth0 -c CLOCK_REALTIME --step_threshold=1 --transportSpecific=0 -w -m -l 7 -r
	Note: --transportSpecific=0 must match the same parameter in /configs/default.cfg settings
2. sudo ptp4l -4 -i eth0 -H --step_threshold=1 -m -f ./configs/default.cfg 

	To manually check if the clocks are synchronise, 
 		a. open a new terminal and run : sudo ptp4l -i eth0
 			Verify if rms is < 100
		b. open a new terminal and run : phc2sys
  			verify if phc offset  < 100

1.1.3 Sychronise the Slave system clock to Master Clock
======
Slave 192.168.1.127
======
1. sudo phc2sys -s eth0 -c CLOCK_REALTIME --step_threshold=1 --transportSpecific=0 -w -m -l 7 -r
2. sudo ptp4l -A -f /home/debian/linuxptp-4.2/configs/default.cfg --step_threshold=1 -4 -H -i eth0 -m -s -f /home/debian/linuxptp-4.2/configs/default.cfg --step_threshold=1 -4

The log message : 
1. new foreign master f045da.fffe.77a12a-1 : GMC master clock on 192.168.1.71 is selected
2. LISTENING to UNCALIBRATED on RS_SLAVE : initial handshake
3. UNCALIBRATED to SLAVE on MASTER_CLOCK_SELECTED : final handshake : indicating successful synchronization with a PTP master clock

-------------
Step 2 : Download all the source codes : git clone http://git.code.sf.net/p/linuxptp/code linuxptp
			cd linuxptp
   			sudo make
	  		sudo make install

  			* files are installed to /usr/local/sbin/.
-------------
Step 3 : Update the main() function of the following files to a new name.  This is to facilitate the main open62541 program to call these functions via function calls (instead of command line)
 	a.  pmc.c => main() to pmc()
 	b.  ptp4l.c => main() to ptp4l()
 	c.  phc2sys.c => main() to phc2sys()
    -----not used
    c.  hwstamp_ctl.c => main() to hwstampCtl()
	d.  nsm.c => main() to nsm()
	e.  phc_ctl.c => main() to phcCtl()
  	g.  timemaster.c => main() to timemaster()
    h.  ts2phc.c => main() to ts2phc()
	i.  tz2alt.c => main() to tz2alt()
-------------
Step 4 : Update the Makefile to include the above files


To test the synchronisation effects:
1. install linuxptp to beaglebone black x 2
	192.168.1.71 - GMC
 		output of step 1.1


   
 	192.168.1.127 - Slave
2. Run the above commands

References:
1. https://tsn.readthedocs.io/timesync.html#checking-clocks-synchronization
  
#############################################
# add a RPI5 compatible PTP 1588 HAT - TimeHat
############################################
https://www.tindie.com/products/timeappliances/timehat-i226-nic-with-pps-inout-for-rpi5/
1. Pre-setup steps
# Install linuxptp
cd /usr/local/src/
sudo apt-get install linuxptp
# download testptp
sudo mkdir testptp
cd testptp
sudo wget https://raw.githubusercontent.com/torvalds/linux/refs/heads/master/tools/testing/selftests/ptp/testptp.c
sudo wget https://raw.githubusercontent.com/torvalds/linux/refs/heads/master/include/uapi/linux/ptp_clock.h
sudo ln-s /usr/include/linux/ptp_clock.h ptp_clock.h
# compile testptp
sudo gcc -Wall -lrt testptp.c -o testptp
# install testptp
sudo cp testptp /usr/bin/
# verify testptp works
sudo testptp -d /dev/ptp0 -l
> name SDP0 index 0 func 0 chan 0
> name SDP1 index 1 func 0 chan 0
> name SDP2 index 2 func 0 chan 0
>name SDP3 index 3 func 0 chan 0

2. configure 1PPS output
# Setup SDP0 (SMA1, closest to HAT header) as periodic output
sudo testptp -d /dev/ptp0 -L0,2
sudo testptp -d /dev/ptp0 -p 1000000000

3. Read 1 PPS SMA Input
# Setup SDP1 (SMA2 , furthest from HAT header) as timestamp input
sudo testptp -d /dev/ptp0 -L1,1
# Read timestamps, use -1 to read forever and ctrl+C to stop, using 5 here as demo. 
# Note: I226 driver passes both edges to Linux, so both rising and falling edges will be listed. 
# A fix for this is listed below, requires patching and building kernel

4. Discipline to 1PPS SMA input
# Fix 1PPS input to only use rising edge, New Method, much easier
Note : Procedure documented on Github: https://github.com/Time-Appliances-Project/Products/tree/main/TimeHAT

############################################
# add TwinCAT ADS capability - v113.0.30-1
############################################
cd /bin
sudo mv c++ c++.original
sudo ln -s /usr/local/gcc14.2.0/bin/c++14.2.0 c++

cd /usr/local/src
sudo git clone https://github.com/Beckhoff/ADS.git
cd ADS
sudo meson setup build
sudo ninja -C build
<libAdsLib.a is saved to /usr/local/src/ADS/build/>
<header files are saved to /usr/local/src/ADS/AdsLib>

To link cpp with c, need to add extern "C" to cpp file.
#######################################################################################
# -- install other libraries needed for user-defined application e.g. open62541lds
#######################################################################################
WORKDIR /root
sudo DEBIAN_FRONTEND="noninteractive" apt-get install libjson-c-dev -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install libxml2-dev -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install mariadb-client -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install libmariadb3 libmariadb-dev -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install mosquitto-clients -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install net-tools proftpd nano -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install python3-sphinx graphviz -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install python3-sphinx-rtd-theme -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install texlive-latex-recommended -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install libavahi-client-dev libavahi-common-dev -y

################################################
# -- get the open62541 source from github
compile open62541 - steps to follow

option 1
1. version : 1.4.13
2. git clone https://github.com/open62541/open62541.git --branch v1.4.13 -c advice.detachedHead=FALSE
2.1 cd open62541
2.2 sudo git submodule update --init --recursive
3. During make, errors related to MQTT (almagamation=ON), but open62541.h is produced (along with open62541.c)
4. Next, set: almagamation=OFF, make = passed
5. libopen62541.a is produced
6. use libopen62541.a and open62541.h in application Makefile and application source codes (but don't use open62541.c in Makefile)
7. Successfully build application!!!

################################################
WORKDIR /root 
#sudo DEBIAN_FRONTEND="noninteractive" apt-get install git -y
sudo git clone https://github.com/open62541/open62541.git --branch v1.4.13 -c advice.detachedHead=FALSE
WORKDIR /root/open62541
sudo git submodule update --init --recursive

##################################
# -- install options for cmake
##################################
sudo DEBIAN_FRONTEND="noninteractive" apt-get install biber -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install clang-format -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install clang-tidy -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install latex2html -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install texlive-xetex -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install xindy -y

###########################################
# -- build the base open62541 libraries
###########################################
WORKDIR /root/open62541
sudo mkdir build
WORKDIR /root/open62541/build
# -- clears cache
sudo rm CMakeCache.txt
sudo rm *.cmake
sudo rm -r CMakeFiles
sudo rm -r doc

# once gcc is updated to v14.2.0, need to modify the following
# -DCMAKE_C_COMPILER=/usr/local/gcc14.2.0/bin/gcc14.2.0 
# -DCMAKE_C_COMPILER_AR=/usr/local/gcc14.2.0/bin/gcc-ar14.2.0
# -DCMAKE_C_COMPILER_RANLIB=/usr/local/gcc14.2.0/bin/gcc-ranlib14.2.0

# change loglevel = 500 if you want to reduce the onscreen system debug messages
# change loglevel = 100 if you want to debug
# KIV step 1B: almagamation = ON (not required)
# KIV sudo cmake -S .. -DCMAKE_C_COMPILER=/usr/local/gcc14.2.0/bin/gcc14.2.0 -DCMAKE_C_COMPILER_AR=/usr/local/gcc14.2.0/bin/gcc-ar14.2.0 -DCMAKE_C_COMPILER_RANLIB=/usr/local/gcc14.2.0/bin/gcc-ranlib14.2.0 -DOPENSSL_CRYPTO_LIBRARY=/usr/local/ssl/lib64/libcrypto.so -DOPENSSL_INCLUDE_DIR=/usr/local/ssl/include -DOPENSSL_SSL_LIBRARY=/usr/local/ssl/lib64/libssl.so -DUA_ARCHITECTURE=posix -DUA_DEBUG_FILE_LINE_INFO=ON -DUA_ENABLE_AMALGAMATION=OFF -DBUILD_SHARED_LIBS=OFF -DUA_ENABLE_DA=ON -DUA_ENABLE_DATATYPES_ALL=ON -DUA_ENABLE_DEBUG_SANITIZER=ON -DUA_ENABLE_DIAGNOSTICS=ON -DUA_ENABLE_DISCOVERY=ON -DUA_ENABLE_DISCOVERY_MULTICAST=ON -DUA_ENABLE_DISCOVERY_SEMAPHORE=ON -DUA_ENABLE_ENCRYPTION=OPENSSL -DUA_ENABLE_ENCRYPTION_OPENSSL=ON -DUA_ENABLE_HISTORIZING=ON -DUA_ENABLE_JSON_ENCODING=ON -DUA_ENABLE_METHODCALLS=ON -DUA_ENABLE_MQTT=ON -DUA_ENABLE_NODEMANAGEMENT=ON -DUA_ENABLE_NODESETLOADER=OFF -DUA_ENABLE_NODESET_COMPILER_DESCRIPTIONS=ON -DUA_ENABLE_PARSING=ON -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_ENCRYPTION=ON -DUA_ENABLE_PUBSUB_FILE_CONFIG=ON -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON -UA_ENABLE_PUBSUB_MONITORING=ON -UA_ENABLE_PUBSUB_SKS=ON -DUA_ENABLE_STATUSCODE_DESCRIPTIONS=ON -DUA_ENABLE_SUBSCRIPTIONS=ON -DUA_ENABLE_SUBSCRIPTIONS_ALARMS=OFF -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON -DUA_ENABLE_TYPEDESCRIPTION=ON -DUA_ENABLE_XML_ENCODING=ON -DUA_FORCE_WERROR=ON -DUA_LOGLEVEL=100 -DUA_MULTITHREADING=100 -DUA_NAMESPACE_ZERO=FULL -DUA_NODESET_DIR=/home/pi/open62541/deps/ua-nodeset
step 1A: almagamation = OFF

Try:
##sudo cmake -G Ninja -S .. -DCMAKE_C_COMPILER=/bin/gcc -DCMAKE_C_COMPILER_AR=/bin/gcc-ar -DCMAKE_C_COMPILER_RANLIB=/bin/gcc-ranlib12 -DOPENSSL_CRYPTO_LIBRARY=/usr/local/ssl/lib/libcrypto.so -DOPENSSL_INCLUDE_DIR=/usr/local/ssl/include -DOPENSSL_SSL_LIBRARY=/usr/local/ssl/lib/libssl.so -DMDNSD_LOGLEVEL=300 -DUA_AMALGAMATION_MULTIARCH=OFF -DUA_ARCHITECTURE=posix -DUA_BUILD_EXAMPLES=ON -DUA_BUILD_TOOLS=ON -DUA_BUILD_UNIT_TESTS=ON -DUA_DEBUG=ON -DUA_DEBUG_FILE_LINE_INFO=ON -DUA_ENABLE_AMALGAMATION=OFF -DBUILD_SHARED_LIBS=OFF -DUA_ENABLE_DA=ON -DUA_ENABLE_DATATYPES_ALL=ON -DUA_ENABLE_DEBUG_SANITIZER=ON -DUA_ENABLE_DIAGNOSTICS=ON -DUA_ENABLE_DISCOVERY=ON -DUA_ENABLE_DISCOVERY_MULTICAST=ON -DUA_ENABLE_DISCOVERY_SEMAPHORE=ON -DUA_ENABLE_ENCRYPTION=OPENSSL -DUA_ENABLE_ENCRYPTION_OPENSSL=ON -DUA_ENABLE_HISTORIZING=ON -DUA_ENABLE_JSON_ENCODING=ON -DUA_ENABLE_METHODCALLS=ON -DUA_ENABLE_MQTT=ON -DUA_ENABLE_NODEMANAGEMENT=ON -DUA_ENABLE_NODESETLOADER=OFF -DUA_ENABLE_NODESET_COMPILER_DESCRIPTIONS=ON -DUA_ENABLE_PARSING=ON -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_ENCRYPTION=ON -DUA_ENABLE_PUBSUB_FILE_CONFIG=ON -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON -DUA_ENABLE_PUBSUB_MONITORING=ON -DUA_ENABLE_PUBSUB_SKS=ON -DUA_ENABLE_STATUSCODE_DESCRIPTIONS=ON -DUA_ENABLE_SUBSCRIPTIONS=ON -DUA_ENABLE_SUBSCRIPTIONS_ALARMS=OFF -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON -DUA_ENABLE_TYPEDESCRIPTION=ON -DUA_ENABLE_XML_ENCODING=ON -DUA_FORCE_WERROR=ON -DUA_LOGLEVEL=100 -DUA_MULTITHREADING=100 -DUA_NAMESPACE_ZERO=FULL -DUA_NODESET_DIR=/home/pi/open62541/deps/ua-nodeset
sudo cmake -G Ninja -S .. -DCMAKE_C_COMPILER=/usr/local/gcc14.2.0/bin/aarch64-linux-gnu-gcc-14.2.0 -DCMAKE_C_COMPILER_AR=/usr/local/gcc14.2.0/bin/aarch64-linux-gnu-gcc-ar14.2.0 -DCMAKE_C_COMPILER_RANLIB=/usr/local/gcc14.2.0/bin/aarch64-linux-gnu-gcc-ranlib14.2.0 -DOPENSSL_CRYPTO_LIBRARY=/usr/local/ssl/lib/libcrypto.so -DOPENSSL_INCLUDE_DIR=/usr/local/ssl/include -DOPENSSL_SSL_LIBRARY=/usr/local/ssl/lib/libssl.so -DMDNSD_LOGLEVEL=300 -DUA_AMALGAMATION_MULTIARCH=OFF -DUA_ARCHITECTURE=posix -DUA_BUILD_EXAMPLES=ON -DUA_BUILD_TOOLS=ON -DUA_BUILD_UNIT_TESTS=ON -DUA_DEBUG=ON -DUA_DEBUG_FILE_LINE_INFO=ON -DUA_ENABLE_AMALGAMATION=OFF -DBUILD_SHARED_LIBS=OFF -DUA_ENABLE_DA=ON -DUA_ENABLE_DATATYPES_ALL=ON -DUA_ENABLE_DEBUG_SANITIZER=ON -DUA_ENABLE_DIAGNOSTICS=ON -DUA_ENABLE_DISCOVERY=ON -DUA_ENABLE_DISCOVERY_MULTICAST=ON -DUA_ENABLE_DISCOVERY_SEMAPHORE=ON -DUA_ENABLE_ENCRYPTION=OPENSSL -DUA_ENABLE_ENCRYPTION_OPENSSL=ON -DUA_ENABLE_HISTORIZING=ON -DUA_ENABLE_JSON_ENCODING=ON -DUA_ENABLE_METHODCALLS=ON -DUA_ENABLE_MQTT=ON -DUA_ENABLE_NODEMANAGEMENT=ON -DUA_ENABLE_NODESETLOADER=OFF -DUA_ENABLE_NODESET_COMPILER_DESCRIPTIONS=ON -DUA_ENABLE_PARSING=ON -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_ENCRYPTION=ON -DUA_ENABLE_PUBSUB_FILE_CONFIG=ON -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON -DUA_ENABLE_PUBSUB_MONITORING=ON -DUA_ENABLE_PUBSUB_SKS=ON -DUA_ENABLE_STATUSCODE_DESCRIPTIONS=ON -DUA_ENABLE_SUBSCRIPTIONS=ON -DUA_ENABLE_SUBSCRIPTIONS_ALARMS=OFF -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON -DUA_ENABLE_TYPEDESCRIPTION=ON -DUA_ENABLE_XML_ENCODING=ON -DUA_FORCE_WERROR=ON -DUA_LOGLEVEL=100 -DUA_MULTITHREADING=100 -DUA_NAMESPACE_ZERO=FULL -DUA_NODESET_DIR=/home/pi/open62541/deps/ua-nodeset

sudo ninja
sudo ninja install


for : v1.4.13 (use Ninja)
git clone https://github.com/open62541/open62541.git --branch v1.4.13 -c advice.detachedHead=FALSE
cd open62541
sudo git submodule update --init --recursive
sudo mkdir build
cd build
sudo cmake -G Ninja .. -DCMAKE_C_COMPILER=/usr/local/gcc14.2.0/bin/gcc14.2.0 -DCMAKE_C_COMPILER_AR=/usr/local/gcc14.2.0/bin/gcc-ar14.2.0 -DCMAKE_C_COMPILER_RANLIB=/usr/local/gcc14.2.0/bin/gcc-ranlib14.2.0 -DOPENSSL_CRYPTO_LIBRARY=/usr/local/ssl/lib64/libcrypto.so -DOPENSSL_INCLUDE_DIR=/usr/local/ssl/include -DOPENSSL_SSL_LIBRARY=/usr/local/ssl/lib64/libssl.so -DUA_ARCHITECTURE=posix -DUA_DEBUG_FILE_LINE_INFO=ON -DUA_ENABLE_AMALGAMATION=OFF -DBUILD_SHARED_LIBS=OFF -DUA_ENABLE_DA=ON -DUA_ENABLE_DATATYPES_ALL=ON -DUA_ENABLE_DEBUG_SANITIZER=ON -DUA_ENABLE_DIAGNOSTICS=ON -DUA_ENABLE_DISCOVERY=ON -DUA_ENABLE_DISCOVERY_MULTICAST=ON -DUA_ENABLE_DISCOVERY_SEMAPHORE=ON -DUA_ENABLE_ENCRYPTION=OPENSSL -DUA_ENABLE_ENCRYPTION_OPENSSL=ON -DUA_ENABLE_HISTORIZING=ON -DUA_ENABLE_JSON_ENCODING=ON -DUA_ENABLE_METHODCALLS=ON -DUA_ENABLE_MQTT=ON -DUA_ENABLE_NODEMANAGEMENT=ON -DUA_ENABLE_NODESETLOADER=OFF -DUA_ENABLE_NODESET_COMPILER_DESCRIPTIONS=ON -DUA_ENABLE_PARSING=ON -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_ENCRYPTION=ON -DUA_ENABLE_PUBSUB_FILE_CONFIG=ON -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON -UA_ENABLE_PUBSUB_MONITORING=ON -UA_ENABLE_PUBSUB_SKS=ON -DUA_ENABLE_STATUSCODE_DESCRIPTIONS=ON -DUA_ENABLE_SUBSCRIPTIONS=ON -DUA_ENABLE_SUBSCRIPTIONS_ALARMS=OFF -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON -DUA_ENABLE_TYPEDESCRIPTION=ON -DUA_ENABLE_XML_ENCODING=ON -DUA_FORCE_WERROR=ON -DUA_LOGLEVEL=100 -DUA_MULTITHREADING=100 -DUA_NAMESPACE_ZERO=FULL -DUA_NODESET_DIR=/home/pi/open62541/deps/ua-nodeset
sudo ninja
sudo ninja install

for : v1.4.14 (use Ninja)
git clone https://github.com/open62541/open62541.git --branch v1.4.14 -c advice.detachedHead=FALSE
cd open62541
sudo git submodule update --init --recursive
sudo mkdir build
cd build
sudo cmake -G Ninja .. -DCMAKE_C_COMPILER=/usr/local/gcc14.2.0/bin/gcc14.2.0 -DUA_BUILD_EXAMPLES=ON -DCMAKE_C_COMPILER_AR=/usr/local/gcc14.2.0/bin/gcc-ar14.2.0 -DCMAKE_C_COMPILER_RANLIB=/usr/local/gcc14.2.0/bin/gcc-ranlib14.2.0 -DOPENSSL_CRYPTO_LIBRARY=/usr/local/ssl/lib/libcrypto.so -DOPENSSL_INCLUDE_DIR=/usr/local/ssl/include -DOPENSSL_SSL_LIBRARY=/usr/local/ssl/lib/libssl.so -DUA_ARCHITECTURE=posix -DUA_ENABLE_AMALGAMATION=OFF -DBUILD_SHARED_LIBS=OFF -DUA_DEBUG=ON -DUA_DEBUG_FILE_LINE_INFO=ON -DUA_ENABLE_DA=ON -DUA_ENABLE_DATATYPES_ALL=ON -DUA_ENABLE_DEBUG_SANITIZER=ON -DUA_ENABLE_DIAGNOSTICS=ON -DUA_ENABLE_DISCOVERY=ON -DUA_ENABLE_DISCOVERY_MULTICAST=ON -DUA_ENABLE_DISCOVERY_SEMAPHORE=ON -DUA_ENABLE_ENCRYPTION=OPENSSL -DUA_ENABLE_ENCRYPTION_OPENSSL=ON -DUA_ENABLE_HISTORIZING=ON -DUA_ENABLE_JSON_ENCODING=ON -DUA_ENABLE_METHODCALLS=ON -DUA_ENABLE_MQTT=ON -DUA_ENABLE_NODEMANAGEMENT=ON -DUA_ENABLE_NODESETLOADER=OFF -DUA_ENABLE_NODESET_COMPILER_DESCRIPTIONS=ON -DUA_ENABLE_PARSING=ON -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_ENCRYPTION=ON -DUA_ENABLE_PUBSUB_FILE_CONFIG=ON -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON -UA_ENABLE_PUBSUB_MONITORING=ON -UA_ENABLE_PUBSUB_SKS=ON -DUA_ENABLE_STATUSCODE_DESCRIPTIONS=ON -DUA_ENABLE_SUBSCRIPTIONS=ON -DUA_ENABLE_SUBSCRIPTIONS_ALARMS=OFF -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON -DUA_ENABLE_TYPEDESCRIPTION=ON -DUA_ENABLE_XML_ENCODING=ON -DUA_FORCE_WERROR=ON -DUA_LOGLEVEL=100 -DUA_MULTITHREADING=100 -DUA_NAMESPACE_ZERO=FULL -DUA_NODESET_DIR=/home/pi/open62541/deps/ua-nodeset
sudo ninja
sudo ninja install

(KIV)
for : v1.5.1-rc1
sudo cmake -S .. -DCMAKE_C_COMPILER=/usr/local/gcc14.2.0/bin/gcc14.2.0 -DCMAKE_C_COMPILER_AR=/usr/local/gcc14.2.0/bin/gcc-ar14.2.0 -DCMAKE_C_COMPILER_RANLIB=/usr/local/gcc14.2.0/bin/gcc-ranlib14.2.0 -DOPENSSL_CRYPTO_LIBRARY=/usr/local/ssl/lib64/libcrypto.so -DOPENSSL_INCLUDE_DIR=/usr/local/ssl/include -DOPENSSL_SSL_LIBRARY=/usr/local/ssl/lib64/libssl.so -DUA_ARCHITECTURE=posix -DUA_DEBUG_FILE_LINE_INFO=ON -DUA_ENABLE_AMALGAMATION=OFF -DBUILD_SHARED_LIBS=OFF -DUA_ENABLE_DA=ON -DUA_ENABLE_DATATYPES_ALL=ON -DUA_ENABLE_DEBUG_SANITIZER=ON -DUA_ENABLE_DIAGNOSTICS=ON -DUA_ENABLE_DISCOVERY=ON -DUA_ENABLE_DISCOVERY_MULTICAST=MDNSD UA_ENABLE_DISCOVERY_MULTICAST_AVAHI=OFF UA_ENABLE_DISCOVERY_MULTICAST_MDNSD=ON -DUA_ENABLE_DISCOVERY_SEMAPHORE=ON -DUA_ENABLE_ENCRYPTION=OPENSSL -DUA_ENABLE_ENCRYPTION_OPENSSL=ON -DUA_ENABLE_ENCRYPTION_TPM2=OFF -DUA_ENABLE_GDS_PUSHMANAGEMENT=ON -DUA_ENABLE_HISTORIZING=ON -DUA_ENABLE_JSON_ENCODING=ON DUA_ENABLE_JSON_ENCODING_LEGACY=ON -DUA_ENABLE_METHODCALLS=ON -DUA_ENABLE_MQTT=ON -DUA_ENABLE_NODEMANAGEMENT=ON -DUA_ENABLE_NODESETLOADER=OFF -DUA_ENABLE_NODESET_COMPILER_DESCRIPTIONS=ON -DUA_ENABLE_PARSING=ON -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_FILE_CONFIG=ON -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON -UA_ENABLE_PUBSUB_SKS=ON -DUA_ENABLE_STATUSCODE_DESCRIPTIONS=ON -DUA_ENABLE_SUBSCRIPTIONS=ON -DUA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS=OFF -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON -DUA_ENABLE_TYPEDESCRIPTION=ON -DUA_ENABLE_XML_ENCODING=ON -DUA_FORCE_WERROR=ON -DUA_LOGLEVEL=100 -DUA_MULTITHREADING=100 -DUA_NAMESPACE_ZERO=FULL -DUA_NODESET_DIR=/home/pi/open62541/deps/ua-nodeset


<v1.4.13>
# somehow <pubsub.h> cannot be found in <home/pi/open62541/include/open62541/>
from github: copy <pubsub.h> from  github <github.com/open62541/include/open62541/> to </home/pi/open62541/include/open62541/>

# somehow <log.h> cannot be found in <home/pi/open62541/plugins/include>/open62541/plugin
from github: copy <log.h> from github <github.com/open62541/include/open62541/plugin/log.h> to </home/pi/open62541/plugins/include/open62541/plugin/>

cd /home/pi/open62541/build/
sudo make
# SV_main.c:631:25: error: implicit declaration of function ‘UA_Server_enableAllPubSubComponents’ [-Wimplicit-function-declaration]
# somehow server_pubsub.h in ~/open62541/include/open62541/ is different from github.
# in github, the LOC is 1126, whereas local copy the LOC is only 983 
from github: copy server_pubsub.h from <github.com/open62541/include/open62541/server_pubsub.h> to /home/pi/open62541/include/open62541/ 
# after copying, rebuild open62541.a but encounters error during make
<Stuck>

go to ~/open62541/plugins/ua_config_default.c and change the following

#define MANUFACTURER_NAME "Virtual Skies"
#define PRODUCT_NAME "Virtual Skies OPC UA Server"
#define PRODUCT_URI "http://www.virtualskies.com"
#define APPLICATION_NAME "OPC UA Server based on open62541"
#define APPLICATION_URI "urn:opc109.virtualskies.com.sg"
#define APPLICATION_URI_SERVER "urn:opc109.virtualskies.com.sg"


WORKDIR /root/open62541/build/
sudo make -j4
sudo make doc
sudo make doc_pdf
sudo make latexpdf
export open62541_NODESET_DIR='/root/open62541/deps/ua-nodeset/Schema/'

# check for reverse connect
1. in UAExpert
	Ednpoint Url - opc.tcp://192.168.1.109:4840
 	ReverseConnect - Yes
  	Client Endpoint URL -opc.tcp://192.168.1.157:4839
   	Server certificate - load <Svrcert109.der>
2. go to windows defender and create a rule <ReverseConnect>: allow incoming port 4839	
3. on Linux, timeout 1 bash -c '</dev/tcp/192.168.1.157/4839 && echo PORT OPEN || echo PORT CLOSED'
PORT OPEN => means 192.168.1.157 where UaExpert resides is listening for incoming connections from OPCUA Server@192.168.1.109

3.
##########################################
# -- creates the volume in container
# 1. svrcerts33.pem are kept in /usr/local/ssl/certs/ softlink to /etc/ssl/certs/
# 2. svrprivate-key.pem are kept in /usr/local/ssl/private/ softlink to /etc/ssl/private/
# 3. trustlist.crl is not used
##########################################
VOLUME /usr/local/ssl/certs
VOLUME /usr/local/ssl/private
# VOLUME /usr/local/ssl/trustlist/trustlist.crl

##########################################
# -- build the user-defined application
# -- saved to /etc/profile.d/open62541-export.sh
# list in order : env -0 | sort -z | tr '\0' '\n'
# access using myenv (myenv located in /usr/local/bin)
##########################################
# the following is used to create a OPCUA Server
export SVR_PRODUCT_URI="svr.virtualskies.com.sg"
export SVR_MANUFACTURER_NAME="Virtual Skies"
export SVR_PRODUCT_NAME="Virtual Skies OPC UA Server"
export SVR_APPLICATION_URI_SERVER="urn:svr.virtualskies.com.sg"
export SVR_APPLICATION_NAME="OPC UA Server based on open62541"
export SVR_PRIVATEKEYLOC="/usr/local/ssl/private/Svrprivate-key109.pem"
export SVR_SSLCERTIFICATELOC="/usr/local/ssl/certs/Svrcert109.pem"
export SVR_TRUSTLISTLOC="/usr/local/ssl/trustlist/"
export SVR_PORT="4840"
export SVR_REVERSE_CONNECT_PORT="4839"
export SVR_SQL_CONNECTION_IP="192.168.1.127"
export SVR_SQL_USERNAME="debian"
export SVR_SQL_PASSWORD="molekhaven24"
export SVR_SQL_DATABASE="HistoryAirgard"
export SVR_SQL_PORT="3306"
export SVR_SQL_HISTORY_OPTION="Poll"
# possible values are Poll, ValueSet, UserDefine
export SVR_LDS_SSLCERTIFICATELOC="/usr/local/ssl/certs/ldscert44.pem"
export SVR_LDS_PRIVATEKEYLOC="/usr/local/ssl/private/ldsprivate-key44.pem"
export SVR_LDS_APPLICATION_URI=urn:lds.virtualskies.com.sg
export SVR_LDS_USERNAME="admin"
export SVR_LDS_PASSWORD="defaultadminpassword24"
export SVR_LDS_PORT="4841"
export SVR_NETWORK_INTERFACE="eth0"
export SVR_WSS_PORT="7681"
export SVR_WSS_IP="192.168.1.109"

# the following is used to connect to a TWINCAT server using AMS protocol
export SVR_AMS_REMOTEIPV4="192.168.1.110"
export SVR_AMS_REMOTENETID="5.40.64.12.1.1"
export SVR_AMS_PORT="852"
export SVR_AMS_TCPPORT="48898"

# the following i# the following is used to initiate a modbus Master instance
#export SVR_MB_PROTOCOL="TCP" - not used anymore
export SVR_MB_TCP_SLAVEID="43"
export SVR_MB_IPV4="192.168.1.157"
export SVR_MB_PORT="502"

# the following is for 6 x vibration sensors (8N1, 9600, 11-16, RS485)
#export SVR_MB_PROTOCOL_SERIAL="YES" - not used anymore
export SVR_MB_RTU_SLAVEID="11 12 13 14 15 16"
#export SVR_MB_SERIAL_TYPE="232"
export SVR_MB_SERIAL_TYPE="485"
export SVR_MB_DEVICE="dev/ttyUSB0"
#export SVR_MB_DEVICE="COM1"
export SVR_MB_BAUD="9600"
export SVR_MB_PARITY="N"
export SVR_MB_DATABIT="8"
export SVR_MB_STOPBIT="1"

export SVR_MB_COILS_STARTADDR_TCP="1"
export SVR_MB_COILS_REGISTERCOUNT_TCP="32"
export SVR_MB_DISCRETEINPUT_STARTADDR_TCP="10000"
export SVR_MB_DISCRETEINPUT_REGISTERCOUNT_TCP="32"
export SVR_MB_HOLDREGISTER_STARTADDR_TCP="40000"
export SVR_MB_HOLDREGISTER_REGISTERCOUNT_TCP="32"
export SVR_MB_INPUTREGISTER_STARTADDR_TCP="30000"
export SVR_MB_INPUTREGISTER_REGISTERCOUNT_TCP="10"

# the following is used to initiate a modbus Slave instance
export SVR_MB_SLAVE_TCP_SLAVEID="45"
export SVR_MB_SLAVE_SLAVEPORT="1502"

export SVR_MB_SLAVE_RTU_SLAVEID="40"
export SVR_MB_SLAVE_DEVICE="/devttyUSB0"
export SVR_MB_SLAVE_BAUD="9600"
export SVR_MB_SLAVE_PARITY="E"
export SVR_MB_SLAVE_DATABIT="7"
export SVR_MB_SLAVE_STOPBIT="0"

# the following is used for connecting to a LDAP server
export SVR_LDAP_PORT="389"
export SVR_LDAP_URL="ldap://192.168.1.112:389"
export SVR_LDAP_BIND_DN="cn=admin,dc=bookworm,dc=virtualskies,dc=com"
export SVT_LDAP_BASE="dc=bookworm,dc=virtualskies,dc=com"

# the following is used for connecting to a RADIUS server
export SVR_RADIUS_PORT="1812"
export SVR_RADIUS_URL="192.168.0.174:1812"
export SVR_RADIUS_SECRET=""
export SVR_RADIUS_USERNAME="jackybek"
export SVR_RADIUS_PASSWORD="molekhaven24"

# the following is used to instantiate a CANOPEN Master
export SVR_CANOPEN_EDSDCFLOC="/home/pi/OPCProject/Lely/canopen/edsdcf/"

# the following is used to turn on/off callbacks in open62541
export SVR_CALLBACK_MODBUS_TCP="YES"
export SVR_CALLBACK_MODBUS_RTU="NO"
export SVR_CALLBACK_TWINCAT_ADS="NO"
export SVR_CALLBACK_CANOPEN="NO"

# the following is used to turn on/off Modbus Slave capability
export SVR_CAP_ASMODBUSSLAVE_TCP="NO"
export SVR_CAP_ASMODBUSSLAVE_RTU="NO"

sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0

########################################################################## 
Additional steps B : generate SSL certificates and keys for OPCServers
##########################################################################
1.  Generate a Private Key
a. sudo openssl genrsa -out Svrprivate-key109.pem 4096 => pass phrase :: this is my pass phrase
=> generated Svrprivate-key109.pem

b. copy to /usr/local/ssl/private
c. in /etc/ssl/private create symbolic link: 
	sudo ln -s /usr/local/ssl/private/Svrprivate-key109.pem Svrprivate-key109.pem

2. Generate a self signed certificate
a.1 sudo openssl req -x509 -new -nodes -key Svrprivate-key109.pem -days 3650 -sha512 -config openssl-svr109.cnf -out Svrcert109.pem
trustlist - for UAExpert (needs DER format)
a.2 sudo openssl req -x509 -new -nodes -key Svrprivate-key109.pem -days 3650 -sha512 -config openssl-svr109.cnf -outform DER -out Svrcert109.der

Country Name (2 letter code) [AU]:SG                                                                                                                                              
State or Province Name (full name) [Some-State]:SG                                                                                                                                
Locality Name (eg, city) []:SG                                                                                                                                                    
Organization Name (eg, company) [Internet Widgits Pty Ltd]:Virtual Skies                                                                                                          
Organizational Unit Name (eg, section) []:IT  
Common Name (e.g. server FQDN or YOUR name) []: svr.virtualskies.com.sg
Email Address []:jacky81100@yahoo.com

=> generated Svrcert109.pem OR Svrcert109.der

b. copy to /usr/local/ssl/certs/ 
c. in /etc/ssl/certs create symbolic link:
	sudo ln -s /usr/local/ssl/certs/Svrcert109.pem Svrcert109.pem

3a. create a config file for SAN

[ req ]
x509_extensions = v3_req

[ v3_req ]
authorityKeyIdentifier  = keyid:always, issuer:always,                                                                                                                            
subjectKeyIdentifier    = hash                                                                                                                                                    
basicConstraints        = CA:FALSE                                                                                                                                                 
keyUsage 		= nonRepudiation,digitalSignature,keyEncipherment,dataEncipherment,keyAgreement,keyCertSign                                                          
subjectAltName          = @san                                                                                                                                    

[ san ]
#if running on lds server use DNS2 and DNS3
#else use DNS.1
#DNS.1=urn:svr.virtualskies.com.sg
URI=urn:svr.virtualskies.com.sg
IP=192.168.1.109
DNS=OPCSvr-109

2. Generate a CSR
openssl req -new -key virtualskies.com.key -out virtualskies.com.csr

#####################################
# setup openldap libraries
#####################################
cd /usr/local/src
sudo git clone git.openldap.org/openldap/openldap
cd openldap
sudo ./configure --prefix=/usr/local/src/openldap/
sudo make depend
cd /usr/local/src/openldap/libraries/
sudo make

* files are generated in
a. /usr/local/src/openldap/include/
b. /usr/local/src/openldap/libraries/libldap/.libs
c. /usr/local/src/openldap/libraries/liblber/.libs and so on
* list of functions can be found in https://www.openldap.org/software/man.cgi?query=ldap
SV_Configure.c -> calls SV_Ldap.c

sample in 
a. https://gitlab.inf.ufsm.br/ncc/openvpn-auth-ldap/blob/13622f21b578d6a9a778d20b2430bc2329fb7159/src/LFLDAPConnection.m
b. https://docs.oracle.com/cd/E19957-01/817-6707/writing.html#wp27040
c. https://www.openldap.org/software/man.cgi?query=ldap_initialize&sektion=3&apropos=0&manpath=OpenLDAP+2.6-Release
#####################################
# setup mosquitto broker
#####################################
1. in mosquitto.conf
per_listener_settings true
allow_anonymous false
persistence true
persistence_file persist.txt
persistence_location D:\Apps\mosquitto\
autosave_interval 1000
allow_zero_length_clientid true
auto_id_prefix open62541
password_file D:\Apps\mosquitto\password.txt
port 1883 0.0.0.0
# bind_interface eth0 - not available on windows
protocol mqtt

2. To generate password file, mosquitto_passwd -U <password.txt>
   contents of password.txt : jackybek:molekhaven24
   
3. To start the mosquitto broker : mosquitto -c mosquitto.conf
4. To publish on mosquitto broker : mosquitto_pub -h 192.168.1.157 -p 1883 -u jackybek -P molekhaven24 -t AirgardTopicEdgeToCloud -m "This is my message"
5. To subscribe : mosquitto_sub -h 192.168.1.157 -p 1883 -u jackybek -P molekhaven24 -t AirgardTopicEdgeToCloud

######################################
# setup Modbus Slave - https://www.modbusdriver.com/diagslave.html
######################################
On Windows OS do the following
TCP:
D:\Apps\diagslave-3.5\win\diagslave.exe -m tcp -a 43 -p 502
SERIAL PORT:
For usb serial converter, go to Windows->Device Manager and look under Universal Serial Bus Controllers.
Make sure that the entry 'USB Serial Converter' is present and is working properly.
SERIAL PORT - RS232
D:\Apps\diagslave-3.5/win/diagslave.exe -m rtu -a 43 -b 19200 -d 8 -s 1 p none 
SERIAL PORT - RS485
D:\Apps\diagslave-3.5/win/diagslave.exe -m rtu -a 43 -b 19200 -d 8 -s 1 p none -4

On Linux OS do the following
1. sudo raspi-config
2. Interface options -> Serial Port -> Choose 'No' for
	a. login shell to be accessible over serial
 	b. serial port hardware to be enabled
3. Reboot
############################################
# TwinCAT ADS
############################################
cd /usr/local/src
sudo git clone https://github.com/Beckhoff/ADS.git
cd ADS
sudo meson setup build
sudo ninja -C build
<libAdsLib.a is saved to /usr/local/src/ADS/build/>
<header files are saved to /usr/local/src/ADS/AdsLib>

Note: if during runtime cannot find GLIBCXX_3.4.32,
	go to /etc/ld.so.conf.d/
 	update aarch64-linux-gnu.conf:
		# Multiarch support
		/usr/local/lib/aarch64-linux-gnu
		#/lib/aarch64-linux-gnu
		/usr/local/gcc14.2.0/lib64
		#/usr/lib/aarch64-linux-gnu
 	sudo ldconfig -v

> Access CX9020 via device manager
https://192.168.1.110/config
Create a route for RPI 192.168.1.109
go to TwinCAT
	Route Name = RPI-109
 	AMS Net Id = 192.168.1.110.1.1
  	Transport_type = TCP
   	Address = 192.168.1.110
    	Connection timeout = 60000
     	Flags = Static, IP Address

> Access CX9020 via cerhost.exe
192.168.1.110/mxxxx24
go to Start->Control Panel->CX Configuration->Firewall
Enable Firewall, Persistant Settings
Allow UDP port 48899 (ADS UDP)
Allow TCP port 48898 (ADS TCP)
Allow TCP port 8016 (SecureADS TCP)
Allow TCP port 4852 (IPCDiagUA)
Allow TCP port 987 (CERHost)
Allow TCP port 443 (HTTPS)
Allow UDP port 68 (DHCP Unicast Response)
Allow ICMP type 8, code 0 (Allow ICMP echo request)

############################################
# Codesys www.codesys.com
############################################
Free download : Codesys Development System 64bit
Free download : CODESYS Control for Raspberry PI
Installation steps:
- Open CODESYS: Open the CODESYS Development System. 
- Open the Deploy Tool: Go to Tools -> Deploy Control SL //(Update Raspberry Pi or Update Linux ARM64) 
- Enter IP Address and Credentials: Enter the IP address of your Raspberry Pi and the login credentials (username and password). 
- Select the Runtime Package: Choose the CODESYS Runtime Package you downloaded from the CODESYS Store. 
- Install: Click the "Install" button to initiate the installation process. 
- Follow the instructions: Follow the on-screen prompts during the installation process. 

Configure codesys:
After installation, you might need to configure CODESYS Control, such as setting the PLC IP address and other network settings. 
You can also configure the CODESYS runtime environment to automatically start after each reboot. 

Device User: codesys / Sn**&O*#*4

EtherCAT_Master NIC Settings:
- Choose the 2nd and 3rd NIC

Project is stored to C:\Users\Jacky Bek\Documents\codesys
Download Beckhoff Device Description File from www.beckhoff.com/
Go to Tools->Device Repository->Install->Choose the Device (e.g. EK11xx)

Programming and Deployment:
- Once installed and configured, you can program your CODESYS project and deploy it to your Raspberry Pi.
- Use the CODESYS Development System to create your PLC program.
-Connect to the Raspberry Pi via the CODESYS Deploy Tool and download the program.
- Start the program on the Raspberry Pi to execute your PLC logic. 

############################################
# igH EtherLab : EtherCAT Master 
############################################
uname -a 
	=> Linux OPCSvr-109 6.12.34+rpt-rpi-v8 #1 SMP PREEMPT Debian 1:6.12.34-1+rpt1~bookworm (2025-06-26) aarch64 GNU/Linux
kernel : 6.12.34

https://gitlab.com/etherlab.org/ethercat#hardware-requirements
On 192.168.1.109, eth0, 
ls -l /sys/class/net/eth0/device/driver
	=> /sys/class/net/eth0/device/driver -> ../../../../bus/platform/drivers/bcmgenet
basename $(readlink /sys/class/net/eth0/device/driver)
	=> bgmgenet
eth0 uses bgmgenet driver

for eth1 (waveshare board)
ls -l /sys/class/net/eth1/device/driver
	=> /sys/class/net/eth1/device/driver -> ../../../../../../../../../../../bus/usb/drivers/r8152
basename $(readlink /sys/class/net/eth0/device/driver)
	=> r8152
Not supported by EtherLab

cd /usr/local/src/
sudo git clone https://gitlab.com/etherlab.org/ethercat.git
cd ethercat
sudo ./bootstrap
#sudo ./configure --sysconfdir=/etc --enable-e100 --enable-e1000 --enable-e1000e --enable-generic --enable-ccat --enable-igb --enable-igb --enable-genet --enable-dwmac-intel --enable-stmmac-pci
sudo ./configure --sysconfdir=/etc --enable-generic
sudo make all modules
sudo make modules_install install
sudo depmod
sudo make doc

sudo modprobe ec_master main_devices=dc:a6:32:e5:86:6e

vi /etc/ethercat.conf
	=> MASTER0_DEVICE="dc:a6:32:e5:86:6e"  
	=> DEVICE_MODULES="generic"
	=> UPDOWN_INTERFACES="dc:a6:32:e5:86:6e"
 	=> MODPROBE_FLAGS="-b"

sudo ethercat master
sudo systemctl start ethercat
sudo systemctl status ethercat

Now that ethercat has started, use the provided API calls for OPCUA application to call

Testing
To test ethercat Master using the built-in tools, go to /ec/init.d, run sudo ethercat master


############################################
# Profibus
############################################
sudo apt-get install python3-dev
sudo sudo git clone https://github.com/mbuesch/pyprofibus
cd pyprofibus
sudo apt-get install cython3
./setup.py <cmd>

SERIAL:
For usb serial converter, go to Windows->Device Manager and look under Universal Serial Bus Controllers.
Make sure that the entry 'USB Serial Converter' is present and is working properly.


############################################
# Tensorflow integration - refer to https://www.tensorflow.org/install/lang_c for more info
# only works for Oracle VBOX on Intel chip notebook, see next section for Debian Arm64 chip
############################################
cd /usr/local/src
sudo wget -q --no-check-certificate https://storage.googleapis.com/tensorflow/versions/2.18.0/libtensorflow-cpu-linux-x86_64.tar.gz
sudo tar -xvf libtensorflow-cpu-linux-x86_64.tar.gz -C /usr/local
# cd libtensorflow-cpu-linux-86_64
sudo ldconfig /usr/local/lib

c_api.h is installed to /usr/local/include/tensorflow/c
libtensorflow.so is installed to /usr/local/lib

In Makefile, add -I/usr/local/include/
use #include <tensorflow/c/c_api.h> in c programs

############################################
https://shape.host/resources/how-to-install-tensorflow-on-debian-12-a-comprehensive-guide
# only works for Debian
############################################
Step 1: Updating the Package Repositories
sudo apt update
sudo apt upgrade

Step 2: Installing Python and Pip
sudo apt install python3-pip
python3 --version
pip --version

Step 3: Setting Up a Virtual Environment
sudo apt install python3-venv
cd /usr/local/src
sudo mkdir tensorflow
cd tensorflow
sudo python3 -m venv tensorflow
source tensorflow/bin/activate   
sudo pip install --upgrade pip --break-system-packages

Step 4: Installing TensorFlow on Debian 12
sudo pip install --upgrade tensorflow --break-system-packages
sudo pip install --upgrade pandas --break-system-packages
sudo pip install --upgrade matplotlib --break-system-packages

python -c "import tensorflow as tf; print(tf.__version__)"
sudo ./myNewServer

############################################
# Tensorflow integration - refer to : https://www.tensorflow.org/install/source#ubuntu
# only works for ubuntu
############################################
sudo apt install python3-dev python3-pip
sudo pip install -U --user pip
	error: externally-managed-environment

# install Bazel
cd /usr/local/src/
# sudo git clone https://github.com/bazelbuild/bazelisk  
# download bazelisk-arm64.deb from https://github.com/bazelbuild/bazelisk/releases
sudo dpkg -i bazelisk-arm64.deb

#install Clang
sudo apt-get install clang-19
sudo apt install clang-format-19
sudo apt install clang-tidy-19
sudo clang-19 --version

#the following has errors - replaced by above
#cd /usr/local/src
#sudo wget https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.2/clang+llvm-17.0.2-x86_64-linux-gnu-ubuntu-22.04.tar.xz
#sudo tar -xvf clang+llvm-17.0.2-x86_64-linux-gnu-ubuntu-22.04.tar.xz
#cd /usr
#sudo ln -s /usr/local/src/clang+llvm-17.0.2-x86_64-linux-gnu-ubuntu-22.04  clang+llvm-17.0.2-x86_64-linux-gnu-ubuntu-22.04
#clang --version
#sudo ./configure

#Download the TensorFlow source code
cd /usr/local/src/
sudo git clone https://github.com/tensorflow/tensorflow.git
cd /usr/local/src/tensorflow
sudo ./configure
	> Please specify the location of python. [Default is /usr/bin/python3]: <default? 
	> Please input the desired Python library path to use.  Default is [/usr/lib/python3.11/dist-packages]
	> Do you wish to build TensorFlow with ROCm support? [y/N]: N
	> Do you wish to build TensorFlow with CUDA support? [y/N]: N
	> Do you want to use Clang to build TensorFlow? [Y/n]: Y
	> Please specify the path to clang executable. [Default is /usr/bin/clang]: /usr/bin/clang-19
	> Please specify optimization flags to use during compilation when bazel option "--config=opt" is specified [Default is -Wno-sign-compare]: <default>
	> Would you like to interactively configure ./WORKSPACE for Android builds? [y/N]: <default>

export HERMETIC_PYTHON_VERSION=3.11.2 	(to match the currently installed python3 version : python3 --version)
<before running the next command, need to ensure there is at least 8~10GB free space>
<temporary files are installed to /home/pi/.cache/bazel/_bazel_pi/XXX>

bazel build //tensorflow/tools/pip_package:wheel --repo_env=USE_PYWRAP_RULES=1 --repo_env=WHEEL_NAME=tensorflow_cpu	--config=dbg --config=monolithic
	> generated whieel will be located in : bazel-bin/tensorflow/tools/pip_package/wheel_house/
pip install bazel-bin/tensorflow/tools/pip_package/wheel_house/tensorflow-version-tags.whl

OR:
Alternatively: https://shape.host/resources/how-to-install-tensorflow-on-debian-12-a-comprehensive-guide
sudo apt install python3-venv
cd /usr/local/src/
mkdir tensorflow
cd tensorflow
sudo python3 -m venv tensorflow
source tensorflow/bin/activate
pip install --upgrade pip
pip install --upgrade tensorflow
python -c "import tensorflow as tf; print(tf.__version__)"

To test : python -c "import tensorflow as tf; print(tf.add(1, 2).numpy())"

############################################
https://github.com/jpfr/cj5star
############################################
Copy cj5star.c and cj5star.h to /home/pi/OPCProject

############################################
https://github.com/Beckhoff/ADS
############################################
git clone https://github.com/Beckhoff/ADS.git
cd ADS
meson setup build
ninja -C build

############################################
https://opensource.lely.com/canopen/docs/installation/
############################################
Refer to canbus.txt


############################################
syslog-ng
############################################
>>Server : 192.168.1.112
sudo apt install syslog-ng
/etc/syslog-ng/syslog-ng.conf
<no customisation required>

>>Client : 192.168.1.109
sudo apt install syslog-ng-core
/etc/syslog-ng/syslog-ng.conf
[Sources]
source s_open62541 {
        file("/var/log/open62541.log" follow-freq(1));

destination d_remote_syslog { syslog(192.168.1.112, port(514) transport("udp")); };
logs messages are stored to /var/logs/messages


$$$$$$$$$$$$$$$$$$$$$$$$$$
How to resolve this error
$$$$$$$$$$$$$$$$$$$$$$$$$$
./myNewServer: /lib/aarch64-linux-gnu/libstdc++.so.6: version `GLIBCXX_3.4.32' not found (required by ./myNewServer)
./myNewServer: /lib/aarch64-linux-gnu/libstdc++.so.6: version `CXXABI_1.3.15' not found (required by ./myNewServer)

/etc/profile.d/chdir.sh
1. cd /lib/aarch64-linux-gnu/
2. sudo mv libstdc++.so.6 libstdc++.so.6.original
3. sudo ln -s /usr/local/gcc14.2.0/lib64/libstdc++.so.6 libstdc++.so.6

