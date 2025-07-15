#ifdef no_almagamation
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#else
#include "open62541.h"
#endif

#include <stdio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>
#include <libxml2/libxml/xmlreader.h>
#include "SV_Misc.h"

//UA_ByteString loadFile(const char *const);

// sample found in /open62541/examples/common.h
// parses the certificate file - used in StartOPCUAServer.c

UA_ByteString loadFile(const char *const path)
{
    UA_ByteString fileContents = UA_STRING_NULL;

    // Open the file
    FILE *fp = fopen(path, "rb");
    if(!fp) {
        errno = 0; // We read errno also from the tcp layer...
        return fileContents;
    }

    // Get the file length, allocate the data and read
    fseek(fp, 0, SEEK_END);
    fileContents.length = (size_t)ftell(fp);
    fileContents.data = (UA_Byte *)UA_malloc(fileContents.length * sizeof(UA_Byte));
    if(fileContents.data) {
        fseek(fp, 0, SEEK_SET);
        size_t read = fread(fileContents.data, sizeof(UA_Byte), fileContents.length, fp);
        if(read != fileContents.length)
            UA_ByteString_clear(&fileContents);
    } else {
        fileContents.length = 0;
    }
    fclose(fp);

    return fileContents;
}

int stringEndsWith(
        char const * const name,
        char const * const extension_to_find)
{
        #ifdef DEBUG
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : in stringEndsWith() function : name = %s, extension = %s", name, extension_to_find);
        #endif

    int is_found = 0;
    size_t length = 0;
    char* ldot = NULL;
    if (name == NULL) return -1;
    if (extension_to_find == NULL) return -1;
    length = strlen(extension_to_find);
    if (length == 0) return -1;
    ldot = strrchr((char*)name, extension_to_find[0]);
    if (ldot != NULL)
    {
        #ifdef DEBUG
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : ldot = %s, extension = %s, length = %d", ldot, extension_to_find, length);
        #endif
        is_found = strncmp(ldot, extension_to_find, length);
        if (is_found == 0) // found
        {
                //UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,"--------SV_Encrypt.c : found file <%s>", name);
                return 0;       // success return 0
        }
        else
                return -1;      // fail to find : return -1
    }
    else
       return -1;       // all other situation return -1
}


xmlNodePtr srSeekChildNodeNamed(xmlNode* p, char* name, int len)
{
	xmlNodePtr curr_node;

	if (p == NULL || name == NULL)
		return NULL;
	for (curr_node = p; curr_node; curr_node = curr_node->next)
	{
		printf("In for loop : curr_node content : %s\n", (char*)xmlNodeGetContent(curr_node));

		printf("In the loop : (Len=%d) Comparing <curr_node->name> with <name> | %s : %s \n", len, curr_node->name, name);
		if (strncmp((const char*)curr_node->name, name, len) == 0)	// look for xmlNode with tag = <name>
		{
			// found seatch string <name>
			printf("----------------Found In srSeekChildNodeNamed() [line 195] : searching for %s, found %s; returning the xmlNodePtr\n", name, curr_node->name);
			return curr_node;
		}
	}
	return NULL;
}


/*
UA_ByteString loadFile(char* filename)
{
	int filesize = 0;
	UA_Byte* filecontent;
	FILE* fp;

	fp = fopen(filename, "r");
	fseek(fp, 0L, SEEK_END);
	filesize = ftell(fp) + 1;
	rewind(fp);

	filecontent = (UA_Byte*)calloc(filesize, sizeof(char));
	fread(filecontent, sizeof(char), filesize, fp);

	UA_ByteString output;
	output.length = filesize;
	output.data = filecontent;

	return output;

}
*/

//#ifdef HOLD
// Implementation of itoa()

char* itoa(int num, char* buffer, int base) {
    int curr = 0;

    if (num == 0) {
        // Base case
        buffer[curr++] = '0';
        buffer[curr] = '\0';
        return buffer;
    }

    int num_digits = 0;

    if (num < 0) {
        if (base == 10) {
            num_digits ++;
            buffer[curr] = '-';
            curr ++;
            // Make it positive and finally add the minus sign
            num *= -1;
        }
        else
            // Unsupported base. Return NULL
            return NULL;
    }

    num_digits += (int)floor(log(num) / log(base)) + 1;

    // Go through the digits one by one
    // from left to right
    while (curr < num_digits) {
        // Get the base value. For example, 10^2 = 1000, for the third digit
        int base_val = (int) pow(base, num_digits-1-curr);

        // Get the numerical value
        int num_val = num / base_val;

        char value = num_val + '0';
        buffer[curr] = value;

        curr ++;
        num -= base_val * num_val;
    }
    buffer[curr] = '\0';
    return buffer;
}
//#endif

// Reverses a string 'str' of length 'len'
void reverse(char* str, int len)
{
    int i = 0, j = len - 1, temp;
    while (i < j) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++;
        j--;
    }
}

int intToStr(int x, char str[], int d)
{
    int i = 0;
    while (x) {
        str[i++] = (x % 10) + '0';
        x = x / 10;
    }

    // If number of digits required is more, then
    // add 0s at the beginning
    while (i < d)
        str[i++] = '0';

    reverse(str, i);
    str[i] = '\0';
    return i;
}

void ftoa(float num, char* buf, int decimalpoint)
{
	// extaact integer part
	int ipart = (int) num;

	// extract decimal part
	float fpart = num - (float)ipart;

	// convert integer part to string
	int i = intToStr(ipart, buf, 0);

	// check for display option after decimal point
	if (decimalpoint != 0)
	{
		buf[i] = '.'; // add decimal point

		// Get the value of fraction part upto given no.
        	// of points after dot. The third parameter
        	// is needed to handle cases like 233.007
        	fpart = fpart * pow(10, decimalpoint);
 
        	intToStr((int)fpart, buf + i + 1, decimalpoint);
	}
}

int check_port_active(char* hostname, int portno)
{

        int sockfd;
        struct sockaddr_in serv_addr;
        struct hostent *server;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            printf("ERROR opening socket");
	    exit(0);
        }

        server = gethostbyname(hostname);

        if (server == NULL) {
            printf("ERROR, no such host\n");
            exit(0);
        }

        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr,
             (char *)&serv_addr.sin_addr.s_addr,
             server->h_length);

        serv_addr.sin_port = htons(portno);
        if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
            printf("Port is closed \n");
                close(sockfd);
                return -1;
        } else {
            printf("Port is active \n");
                close(sockfd);
                return 0;
        }
}
