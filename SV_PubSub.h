void pubSubInitialise(UA_Server *, char *, int, char *);
void pubSubAddConnection(UA_Server *, UA_String *, UA_NetworkAddressUrlDataType *);

void pubSendDataSet(UA_Server *);
void pubInitialiseField(UA_Server *);
void pubWriterGroup(UA_Server *);
void pubDataSetWriter(UA_Server *);

UA_StatusCode subReaderGroup(UA_Server *);
UA_StatusCode subDataSetReader(UA_Server *);
UA_StatusCode subInitialiseField(UA_Server *, UA_NodeId);
void subReceiveDataSet(UA_DataSetMetaDataType *);

int print_debug_UDP(UA_PubSubConnectionConfig *, UA_NetworkAddressUrlDataType *);
int print_debug_MQTT(UA_PubSubConnectionConfig *, UA_NetworkAddressUrlDataType *);

#define UA_ENABLE_PUBSUB_INFORMATIONMODEL

#define MAX_STRING_SIZE 64
#define APPLICATION_URI "urn:virtualskies.com.sg"

#define UA_ENABLE_PUBSUB                // activate open62541.h PUBSUB section
#define UA_EANBLE_PUBSUB_ETH_UDAP       // activate open62541.h PUBSUB section
#define UA_ENABLE_PUBSUB_MQTT           // activate open62541.h PUBSUB section
#define UA_ENABLE_JSON_ENCODING         // activate open62541.h JSON section => use : open62541.c.json & open62541.h.json

#define CONNECTION_CONFIGNAME           "Publisher Connection"
#define NETWORKADDRESSURL_PROTOCOL      "opc.udp://224.0.0.22:4840/"    // multicast address range 224.0.0.0 to 224.0.0.255 ; for verifying using wireshark

#define CONNECTION_NAME_UADP            "ttl"   // ""UADP Publisher Connection"
#define TRANSPORT_PROFILE_URI_UDP       "http://opcfoundation.org/UA-Profile/Transport/pubsub-udp-uadp"
#define TRANSPORT_PROFILE_URI_ETH       "http://opcfoundation.org/UA-Profile/Transport/pubsub-eth-uadp"
#define TRANSPORT_PROFILE_URI_UADP      "http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-uadp"
#define TRANSPORT_PROFILE_URI_JSON      "http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt-json"
#define PUBLISH_INTERVAL_UDP            9000

#define PUBLISHERID                     2234    // used in addDataSetReader() and addPubSubConnection()
#define WRITERGROUPID                   100     // used in addDataSetReader() and addWriterGroup()
#define DATASETWRITERID                 2234    // used in addDataSetReader() and addDataSetWriter()

#ifdef UA_ENABLE_PUBSUB_MQTT
 #define TRANSPORT_PROFILE_URI_MQTT     "http://opcfoundation.org/UA-Profile/Transport/pubsub-mqtt"
 #define TRANSPORT_PROFILE_URI_AMQP     "http://opcfoundation.org/UA-Profile/Transport/pubsub-amqp"
 #define PUBLISH_INTERVAL_MQTT          9000
 #define PUBLISH_INTERVAL_AMQP          9000
 #define MESSAGE_TIMEOUT_MQTT           12000
 #define MESSAGE_TIMEOUT_AMQP           12000
 #define CONNECTIONOPTION_NAME_MQTT     "mqttClientId"
 #define CONNECTIONOPTION_NAME_AMQP     "amqpClientId"
 #define CLIENT_ID_MQTT                 "OPCServer-33-Mqtt" // during execution, mqtt-11 will show : New client connected from 192.168.1.33 as OPCServer-33-Mqtt (c0, k400, u'jackybek')
 #define CLIENT_ID_AMQP                 "TESTCLIENTPUBUSBAMQP"
 #define PUBLISHER_METADATAUPDATETIME_MQTT      0
 #define PUBLISHER_METADATAUPDATETIME_AMQP      0
 #define PUBLISHER_METADATAQUEUENAME_MQTT       "MetaDataTopic"
 #define PUBLISHER_METADATAQUEUENAME_AMQP       "MetaDataTopic"
 #define PUBLISHER_TOPIC_EDGE2CLOUD_MQTT        "AirgardTopicEdgeToCloud"
 #define PUBLISHER_TOPIC_CLOUD2EDGE_MQTT        "AirgardTopicCloudToEdge"
 #define PUBLISHER_QUEUE_EDGE2CLOUD_AMQP        "AirgardQueueEdgeToCloud"
 #define PUBLISHER_QUEUE_CLOUD2EDGE_AMQP        "AirgardQueueCloudToEdge"
 //#define BROKER_ADDRESS_URL_MQTT              "opc.mqtt://192.168.1.119:1883/"
 //#define BROKER_ADDRESS_URL_AMQP              "opc.amqp://192.168.1.119:5672/"
 #define USERNAME_OPTION_NAME_MQTT              "mqttUsername"
 #define USERNAME_OPTION_NAME_AMQP              "amqpUsername"
 #define PASSWORD_OPTION_NAME_MQTT              "mqttPassword"
 #define PASSWORD_OPTION_NAME_AMQP              "amqpPassword"
 #define USERNAME_MQTT                          "jackybek" // during execution, mqtt-11 will show : New client connected from 192.168.1.33 as OPCServer-33-Mqtt (c0, k400, u'jackybek')
 #define USERNAME_AMQP                          "jackybek"
 #define PASSWORD_MQTT                          "molekhaven24"
 #define PASSWORD_AMQP                          "molekhaven24"

 #define CA_FILE_PATH_MQTT                      "/etc/ssl/certs/mosq-ca.crt"            // symbolic link => /home/pi/Downloads/openest.io/mosq-ca.crt
 #define CA_PATH_MQTT                           "/etc/ssl/certs"
 #define CLIENT_CERT_PATH_MQTT                  "/etc/ssl/certs/mosq-client-33.crt"     // symbolic link => /home/pi/Downloads/openest.io/mosq-client-33.crt
 #define CLIENT_KEY_PATH_MQTT                   "/etc/ssl/certs/mosq-client-33.key"     // symbolic link => /home/pi/Downloads/openest.io/mosq-client-33.key
 // tested successfully on command line
 // mosquitto_sub -u jackybek -P molekhaven24 -h 192.168.1.11 -p 8883 -t AirgardTopicEdgeToCloud
 // --capath /etc/ssl/certs --cafile /etc/ssl/certs/mosq-ca.crt --cert /etc/ssl/certs/mosq-client-33.crt
 // --key /etc/ssl/certs/mosq-client-33.key --tls-version tlsv1.2 -d -i OPCServer-33

 #define SUBSCRIBER_METADATAUPDATETIME_MQTT      0
 #define SUBSCRIBER_METADATAQUEUENAME_MQTT       "MetaDataTopic"
 #define SUBSCRIBER_TOPIC_MQTT                   "AirgardTopic"
 #define MILLI_AS_NANO_SECONDS                  (1000 * 1000)
 #define SECONDS_AS_NANO_SECONDS                (1000 * 1000 * 1000)

 #define MAJOR_SOFTWARE_VERSION                 2
 #define MINOR_SOFTWARE_VERSION                 1
#endif

#ifdef UA_ENABLE_WEBSOCKET_SERVER
 #define TRANSPORT_PROFILE_URI_WSSBIN   "http://opcfoundation.org/UA_Profile/Transport/wss-uasc-uabinary"
 #define TRANSPORT_PROFILE_URI_WSSJSON  "http://opcfoundation.org/UA-Profile/Transport/wss-uajson"
 #define BROKER_ADDRESS_URL_WSS         "opc.wss://192.168.1.165:7681/"
#endif

#define UA_AES128CTR_SIGNING_KEY_LENGTH 16
#define UA_AES128CTR_KEY_LENGTH 16
#define UA_AES128CTR_KEYNONCE_LENGTH 4
