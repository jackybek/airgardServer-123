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
# -- library is installed  to /usr/local/include
cd /usr/local/src/
sudo DEBIAN_FRONTEND="noninteractive" apt-get install git -y
sudo git clone https://libwebsockets.org/repo/libwebsockets
cd /usr/local/src/libwebsockets
sudo mkdir build
cd /usr/local/src/libwebsockets/build
sudo cmake ..
sudo make -j4
sudo make install
sudo ldconfig
sudo pkg-config --modversion libwebsockets
#sudo DEBIAN_FRONTEND="noninteractive" apt-get remove git -y
# -- alternative - use apt-get
#sudo echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections
#sudo DEBIAN_FRONTEND="noninteractive" apt-get install libwebsockets-dev -y

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
################################################
cd /root 
#sudo DEBIAN_FRONTEND="noninteractive" apt-get install git -y
sudo git clone https://github.com/open62541/open62541.git --branch v1.4.10 -c advice.detachedHead=FALSE
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
sudo cmake -S .. -DCMAKE_C_COMPILER=/usr/local/gcc14.2.0/bin/gcc14.2.0 -DCMAKE_C_COMPILER_AR=/usr/local/gcc14.2.0/bin/gcc-ar14.2.0 -DCMAKE_C_COMPILER_RANLIB=/usr/local/gcc14.2.0/bin/gcc-ranlib14.2.0 -DOPENSSL_CRYPTO_LIBRARY=/usr/local/ssl/lib64/libcrypto.so -DOPENSSL_INCLUDE_DIR=/usr/local/ssl/include -DOPENSSL_SSL_LIBRARY=/usr/local/ssl/lib64/libssl.so -DUA_ARCHITECTURE=posix -DUA_DEBUG_FILE_LINE_INFO=ON -DUA_ENABLE_AMALGAMATION=OFF -DBUILD_SHARED_LIBS=OFF -DUA_ENABLE_DA=ON -DUA_ENABLE_DATATYPES_ALL=ON -DUA_ENABLE_DEBUG_SANITIZER=ON -DUA_ENABLE_DIAGNOSTICS=ON -DUA_ENABLE_DISCOVERY=ON -DUA_ENABLE_DISCOVERY_MULTICAST=ON -DUA_ENABLE_DISCOVERY_SEMAPHORE=ON -DUA_ENABLE_ENCRYPTION=OPENSSL -DUA_ENABLE_ENCRYPTION_OPENSSL=ON -DUA_ENABLE_HISTORIZING=ON -DUA_ENABLE_JSON_ENCODING=ON -DUA_ENABLE_METHODCALLS=ON -DUA_ENABLE_MQTT=ON -DUA_ENABLE_NODEMANAGEMENT=ON -DUA_ENABLE_NODESETLOADER=OFF -DUA_ENABLE_NODESET_COMPILER_DESCRIPTIONS=ON -DUA_ENABLE_PARSING=ON -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_PUBSUB_ENCRYPTION=ON -DUA_ENABLE_PUBSUB_FILE_CONFIG=ON -DUA_ENABLE_PUBSUB_INFORMATIONMODEL=ON -DUA_ENABLE_SUBSCRIPTIONS=ON -DUA_ENABLE_SUBSCRIPTIONS_EVENTS=ON -DUA_ENABLE_TYPEDESCRIPTION=ON -DUA_ENABLE_XML_ENCODING=ON -DUA_FORCE_WERROR=ON -DUA_LOGLEVEL=500 -DUA_NAMESPACE_ZERO=FULL

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
sudo export open62541_NODESET_DIR='/root/open62541/deps/ua-nodeset/Schema/'

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
