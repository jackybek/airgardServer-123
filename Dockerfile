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
sudo DEBIAN_FRONTEND="noninteractive" apt-get install lib32readline8 lib32readline-dev -y
sudo DEBIAN_FRONTEND="noninteractive" apt-get install gnome
sudo DEBIAN_FRONTEND="noninteractive" apt-get install libncurses5-dev libncursesw5-dev -y # to detect Ctrl+Key

####################################
# -- install ntp client
####################################
sudo apt-get install ntp -y
cd /etc/
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
cd /root
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
cd /root/gcc
sudo git checkout releases/gcc-14.2.0
sudo ./contrib/download_prerequisites
cd /root
sudo mkdir objdir
cd /root/objdir
sudo DEBIAN_FRONTEND="noninteractive" apt-get install g++
sudo ../gcc/configure --prefix=/usr/local/gcc14.2.0 --disable-multilib --with-system-zlib --enable-languages=c,c++ --program-suffix=14.2.0
sudo ulimit -m unlimited
sudo ulimit -v unlimited
sudo make -j4                     
sudo make install
cd /etc
su -
sudo echo "PATH=/usr/local/gcc14.2.0/bin:"$PATH > environment 
# original environment : PATH=/root/cmake-3.31.6/bin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin
sudo source /etc/environment
sudo echo $PATH
cd /usr/local/gcc14.2.0/bin
sudo ln -s gcc14.2.0 gcc
sudo gcc --version -a

cd /etc/profile.d
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
#cd /root
#sudo wget https://github.com/madler/zlib/releases/download/v1.2.13/zlib-1.2.13.tar.gz
#sudo tar -xvf zlib-1.2.13.tar.gz
#cd /root/zlib-2.2.13
#sudo ./configure --prefix=/usr/local/zlib
#sudo make
#sudo make install

####################################
# -- reinstall openssl libraries
####################################
cd /usr/local/src/
sudo wget https://www.openssl.org/source/openssl-3.5.1.tar.gz
sudo tar -xf openssl-3.5.1.tar.gz
sudo pwd
cd /usr/local/src/openssl-3.5.1/
sudo ./config --prefix=/usr/local/ssl --openssldir=/usr/local/ssl/ shared zlib
sudo make -j4
# sudo make test
sudo make install_sw
cd /etc/ld.so.conf.d/
sudo touch openssl-open62541.conf 
sudo echo "/usr/local/ssl/" | sudo tee -a /etc/ld.so.conf.d/openssl-open62541.conf
export LD_LIBRARY_PATH=/usr/local/ssl/lib/
cd /etc/profile.d
sudo echo "export LD_LIBRARY_PATH=/usr/local/ssl/lib; ldconfig" | sudo tee -a ssl_export_ld_library_path.sh
sudo ldconfig -v
cd /etc/
#sudo echo ":/usr/local/ssl/bin" | tee -a environment 
#sudo source /etc/environment
sudo echo $PATH
sudo /usr/local/ssl/bin/openssl version -a

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

############################################################################
# -- build CMAKE from source to get the latest version : https://cmake.org
# -- https://markusthill.github.io/blog/2024/installing-cmake/
############################################################################
cd /root
sudo DEBIAN_FRONTEND="noninteractive" apt-get remove --purge --autoremove cmake -y
cd /usr/local/src/
sudo wget https://cmake.org/files/v3.31/cmake-3.31.6.tar.gz
sudo tar -xvf cmake-3.31.6.tar.gz
cd cmake-3.31.6/
sudo ./configure
# if the above step fail do the following
  # export OPENSSL_INCLUDE=/usr/local/ssl/include/
  # export OPENSSL_LIBRARIES=/usr/local/ssl/lib/
  # export OPENSSL_INCLUDE=/usr/local/ssl/include/
  # in /etc/profile.d, add the above 3 lines
  # sudo apt-get install libssl-dev
sudo gmake
sudo make install
cd /etc/profile.d
sudo echo "export CMAKE_ROOT=/usr/local/src/cmake-3.31.6/bin/; sudo ldconfig" | tee -a cmake_export_CMAKE_root_path.sh
sudo ldconfig -v
cd /etc
#sudo echo "export PATH=export PATH=/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin:/usr/X11/bin:/root/cmake-3.31.6/bin:$PATH" | tee -a environment
sudo echo "PATH=/usr/local/src/cmake-3.31.6/bin:"$PATH > environment
sudo source /etc/environment
sudo echo $PATH
sudo cmake --version -a
# -- alternative - use apt-get
# DEBIAN_FRONTEND="noninteractive" apt-get install cmake

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
a. endpoint to "\"connectivitycheck\gstatic.com\","
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
sudo passwd lwsws (mo..24)
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

cd /usr/local/src/
sudo wget "https://github.com/stephane/libmodbus/releases/download/v3.1.11/libmodbus-3.1.11.tar.gz"
sudo tar -xvf libmodbus-3.1.11.tar.gz
cd /usr/local/src/libmodbus-3.1.11/
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
uncomment bind_id : servers->setValue('login','bind_id','cn=admin,dc=bookworm,dc=virtualskies,dc=com');

To login via browser: http://192.168.1.112/phpldapadmin/		# ------------- change this URL if there is a change in IP address
login phpldapadmin : cn=admin,dc=bookworm,dc=virtualskies,dc=com
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
# add PTPD IEEE 1588 capability - defunct, refer to the next section
############################################
refer to : https://raw.githubusercontent.com/ptpd/ptpd/master/INSTALL
sudo apt-get install libpcap-dev -y
sudo apt-get install snmpd libsnmp-dev -y
sudo apt-get install autoconf automake libtool -y
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
# add TwinCAT ADS capability
############################################
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
cd /root
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
2. source : git clone https://github.com/open62541/open62541.git --branch v1.4.13 -c advice.detachedHead=FALSE
2.1 cd open62541
2.2 sudo git submodule update --init --recursive
3. During make, errors related to MQTT (almagamation=ON), but open62541.h is produced (along with open62541.c)
4. Next, set: almagamation=OFF, make = passed
5. libopen62541.a is produced
6. use libopen62541.a and open62541.h in application Makefile and application source codes (but don't use open62541.c in Makefile)
7. Successfully build application!!

################################################
cd /root 
#sudo DEBIAN_FRONTEND="noninteractive" apt-get install git -y
sudo git clone https://github.com/open62541/open62541.git --branch v1.4.13 -c advice.detachedHead=FALSE
cd /root/open62541
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
cd /root/open62541
sudo mkdir build
cd /root/open62541/build
# -- clears cache
sudo rm CMakeCache.txt
sudo rm *.cmake
sudo rm -r CMakeFiles
sudo rm -r doc

# once gcc is updated to v14.2.0, need to modify the following
# -DCMAKE_C_COMPILER=/usr/local/gcc14.2.0/bin/gcc14.2.0 
# -DCMAKE_C_COMPILER_AR=/usr/local/gcc14.2.0/bin/gcc-ar14.2.0
# -DCMAKE_C_COMPILER_RANLIB=/usr/local/gcc14.2.0/bin/gcc-ranlib14.2.0
sudo cmake -S .. -DCMAKE_C_COMPILER=/usr/local/gcc14.2.0/bin/gcc14.2.0 -DCMAKE_C_COMPILER_AR=/usr/local/gcc14.2.0/bin/gcc-ar14.2.0 -DCMAKE_C_COMPILER_RANLIB=/usr/local/gcc14.2.0/bin/gcc-ranlib14.2.0 -DOPENSSL_CRYPTO_LIBRARY=/usr/local/ssl/lib64/libcrypto.so -DOPENSSL_INCLUDE_DIR=/usr/local/ssl/include -DOPENSSL_SSL_LIBRARY=/usr/local/ssl/lib64/libssl.so -DUA_ARCHITECTURE=posix -DUA_DEBUG_FILE_LINE_INFO=ON -DUA_ENABLE_AMALGAMATION=OFF -DBUILD_SHARED_LIBS=OFF -DUA_ENABLE_DA=ON -DUA_ENABLE_DATATYPES_ALL=ON -DUA_ENABLE_DEBUG_SANITIZER=ON -DUA_ENABLE_DIAGNOSTICS=ON -DUA_ENABLE_DISCOVERY=ON -DUA_ENABLE_DISCOVERY_MULTICAST=ON -DUA_ENABLE_DISCOVERY_SEMAPHORE=ON -DUA_ENABLE_ENCRYPTION=OPENSSL -DUA_ENABLE_ENCRYPTION_OPENSSL=ON -DUA_ENABLE_HISTORIZING=ON -DUA_ENABLE_JSON_ENCODING=ON -DUA_ENABLE_METHODCALLS=ON -DUA_ENABLE_MQTT=ON -DUA_ENABLE_NODEMANAGEMENT=ON -DUA_ENABLE_NODESETLOADER=OFF -DUA_ENABLE_NODESET_COMPILER_DESCRIPTIONS=ON -DUA_ENABLE_PARSING=ON -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_ENCRYPTION=ON -DUA_ENABLE_PUBSUB_FILE_CONFIG=ON -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON -UA_ENABLE_PUBSUB_MONITORING=ON -UA_ENABLE_PUBSUB_SKS=ON -DUA_ENABLE_STATUSCODE_DESCRIPTIONS=ON -DUA_ENABLE_SUBSCRIPTIONS=ON -DUA_ENABLE_SUBSCRIPTIONS_ALARMS=OFF -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON -DUA_ENABLE_TYPEDESCRIPTION=ON -DUA_ENABLE_XML_ENCODING=ON -DUA_FORCE_WERROR=ON -DUA_LOGLEVEL=100 -DUA_MULTITHREADING=100 -DUA_NAMESPACE_ZERO=FULL -DUA_NODESET_DIR=/home/pi/open62541/deps/ua-nodeset

go to ~/open62541/plugins/ua_config_default.c and change the following
#define MANUFACTURER_NAME "Virtual Skies"
#define PRODUCT_NAME "Virtual Skies OPC UA Server"
#define PRODUCT_URI "http://www.virtualskies.com"
#define APPLICATION_NAME "OPC UA Server based on open62541"
#define APPLICATION_URI "urn:opc123.virtualskies.com.sg"
#define APPLICATION_URI_SERVER "urn:opc123.virtualskies.com.sg"

cd /root/open62541/build/
sudo make -j4
sudo make doc
sudo make doc_pdf
sudo make latexpdf
export open62541_NODESET_DIR='/root/open62541/deps/ua-nodeset/Schema/'

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
# add the hostname of the LDS server (OPCLds-44) to /etc/hosts
##########################################
192.168.1.44    OPCLds-44

##########################################
# -- build the user-defined application
# -- saved to /etc/profile.d/open62541-export.sh
# list in order : env -0 | sort -z | tr '\0' '\n'
##########################################
sudo export SVR_PRODUCT_URI="http://svr.virtualskies.com.sg"
sudo export SVR_MANUFACTURER_NAME="Virtual Skies"
sudo export SVR_PRODUCT_NAME="Virtual Skies OPC UA Server"
sudo export SVR_APPLICATION_URI_SERVER="urn:svr.virtualskies.com.sg"
sudo export SVR_APPLICATION_NAME="OPC UA Server based on open62541"
sudo export SVR_PRIVATEKEYLOC="/usr/local/ssl/private/Svrprivate-key33.pem"
sudo export SVR_SSLCERTIFICATELOC="/usr/local/ssl/certs/Svrcert33.pem"
sudo export SVR_TRUSTLISTLOC="/usr/local/ssl/trustlist/"
sudo export SVR_PORT="4840"
sudo export SVR_REVERSE_CONNECT_PORT="4839"
sudo export SVR_SQL_CONNECTION_IP="192.168.1.127"
sudo export SVR_SQL_USERNAME="debian"
sudo export SVR_SQL_PASSWORD="molekhaven24"
sudo export SVR_SQL_DATABASE="HistoryAirgard"
sudo export SVR_SQL_PORT="3306"
sudo export SVR_SQL_HISTORY_OPTION="ValueSet"
sudo export SVR_SQL_HISTORY_OPTION="Poll"
sudo export SVR_SQL_HISTORY_OPTION="UserDefine"
sudo export SVR_LDS_SSLCERTIFICATELOC="/usr/local/ssl/certs/ldscert44.pem"
sudo export SVR_LDS_APPLICATION_URI=urn:lds.virtualskies.com.sg
sudo export SVR_LDS_USERNAME="admin"
sudo export SVR_LDS_PASSWORD="defaultadminpassword24"
sudo export SVR_LDS_PORT="4841"
