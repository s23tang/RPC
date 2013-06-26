#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>
#include <pthread.h>

#include "initialization.h"
#include "error.h"


using namespace std;

void binderInit(struct BinderInfo *binderinfo) {
    char hostName[128];
    
    int result = gethostname(hostName, sizeof hostName);
    if (result == -1) {
        cout << "Initialization: get host error." << endl;
        exit(EGHOST);
    }
    
    
}