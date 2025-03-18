
int main(int argc, char** argv);
//void* StartOPCUAServer(void* x_void_ptr, char*, char*);
void* StartOPCUAServer(void* x_void_ptr, char* argv[]);
UA_NodeId CreateOPCUANodes(void* x_void_ptr);
void PopulateOPCUANodes(char* g_argv_ip);
