old function name		new filename			new function name	Verify	old 	New	Remarks
=================		============			=================	======	===	===	=======
addPubSubConnection()		SV_PubSubAddConnection.c	PubSubAddConnection()	ok	A1/B1	A1/B1	Section: 1 addPubSubConnection_UADP_ETH(), 2 addPubSubConnection_MQTT(), 3 addPubSubConnection_AMQP()

addDataSetWriter() *1		SV_PublishDataSetWriter.c 	PubDataSetWriter()	ok	A5	A5	Section: 0 JSON_ENCODING, 1 UADP_ETH(), 2 MQTT(), 3 AMQP()
addWriterGroup() *2		SV_PublishWriterGroup.c 	PubWriterGroup()	ok	A4	A4	Section: 0 JSON_ENCODING, 1 UADP_ETH(), 2 MQTT(), 3 AMQP()
addDataSetField() *3		SV_PublishInitialiseField.c 	PubInitialiseField()	ok	A3	A3	common for all 
addPublishedDataSet() *4	SV_PublishSendDataSet.c 	PubSendDataSet()	ok	A2	A2	common for all


fillDataSetMetaData() *4	SV_SubscribeReceiveDataSet.c	SubReceiveDataSet()	ok	B5	B2	common for all; no need to split up according to connectionType (UADP_ETH, MQTT, AMQP)
addSubscribedVariables() *3	SV_SubscribeInitialiseField.c	SubInitialiseField()	ok	B4	B3	common for all
addDataSetReader() *1		SV_SubscribeDataSetReader.c 	SubDataSetReader()	ok	B3	B5	Section: 0 JSON_ENCODING, 1 UADP_ETH(), 2 MQTT(), 3 AMQP()
addReaderGroup() *2		SV_SubscribeReaderGroup.c	SubReaderGroup()	ok	B2	B4	Section: 0 JSON_ENCODING, 1 UADP_ETH(), 2 MQTT(), 3 AMQP()


callback()			SV_PubSubCallback.c		archived (not used)

addSubscription() - inactive

CreateServerPubSub()		SV_PubSubInitialise.c		PubSubInitialise()	ok	0		Start here


***

***

OBJ= 
open62541.o 
SV_Misc.o  
SV_Historizing.o \
SV_Event.o 
SV_Monitor.o 
SV_Method.o  
SV_CreateNodes.o \
json_checker.o 
SV_Register.o 
SV_Configure.o 
SV_Encrypt.o 
SV_main.o

