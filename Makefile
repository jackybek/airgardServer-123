HOME=/home/pi/
CC=/usr/local/gcc14.2.0/bin/gcc
CFLAGS1=-g -std=c99 -Wall -Werror -Wno-implicit -Wfatal-errors
#CC=g++
#CFLAGS1=-x c++ -std=c++98 -v -fpermissive -Wno-literal-suffix -Wno-write-strings -Wno-long-long -Wno-return-type
#CFLAGS2=-I$(HOME) -I$(HOME)include/ -I$(HOME)plugins/ -I$(HOME)src/ -I$(HOME)open62541/build/src_generated/ \
#-I$(HOME)arch/ -I$(HOME)deps/ -I$(HOME)plugins/include/ -I/usr/local/include/ -I$(HOME)open62541/plugins/ -I$(HOME)open62541/include/ \
#-I$(HOME)open62541/plugins/include/
CFLAGS=2=-I$(HOME)open62541 -I/usr/include/libxml2 -I/usr/include/
CFLAGS3=-g -pass-exit-codes
DEPS=
LIBS=-lm -lrt -lpthread  -lcrypto -lssl -lmbedcrypto -lmbedtls -lmbedx509 -lwebsockets -lmariadbclient -lxml2 -ljson-c
OBJ= open62541.o SV_Misc.o \
SV_PubSubInitialise.o SV_PubSubAddConnection.o  \
SV_PublishDataSetWriter.o SV_PublishInitialiseField.o SV_PublishSendDataSet.o SV_PublishWriterGroup.o \
SV_SubscribeDataSetReader.o SV_SubscribeInitialiseField.o SV_SubscribeReceiveDataSet.o SV_SubscribeReaderGroup.o \
SV_Historizing.o SV_Event.o SV_Monitor.o SV_Method.o  SV_CreateNodes.o \
json_checker.o SV_Register.o SV_Configure.o SV_Encrypt.o SV_main.o

#KIV
# SV_Alarm.o
# SV_WebSockets.o
# SV_RegisterOPCServerToLDS.o \
# SV_PubSub.o
# SV_StartOPCUAServer.o => replaced by SV_main, SV_Encrypt, SV_Configure, SV_Register
# SV_ExtractXMLElementNames.o => implemented in Client
# SV_PopulateOPCUANodes.o => implemented in Client

ODIR=obj
LDIR1=-L/usr/local/lib/
LDIR2=-L/usr/lib/
LDIR3=-L/usr/local/gcc14.2.0/lib/
#LDFLAGS=-L$(HOME)open62541/build/bin/ -l:libopen62541.a -l:liblibmdnsd.a

all: myNewServer

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS1) $(CFLAGS2) $(CFLAGS3) -c  $< -o $@

myNewServer: $(OBJ)
	$(CC) $(LDIR1) $(LDIR2) $(LDIR3) -o $@ $^ $(LIBS)

# $(HOME)/open62541/build/src_generated => open62541/config.h is located here
# $(HOME)/open62541/plugins/include/ => open62541/client_config_default.h is located here
