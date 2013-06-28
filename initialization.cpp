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

#include "binder.h"
#include "initialization.h"
#include "error.h"


using std::cout;
using std::cin;
using std::cerr;
using std::endl;
using std::string;
using std::string;

int Init(struct Info *info) {
    char hostName[128];

    struct sockaddr_in saddr, taddr;			// for packing server and a placeholder
	int sockfd;						// socket file descriptor for listening, and placeholder
	int result;								// for checking return values
    
    /* get the host name of the binder machine */
    
    result = gethostname(hostName, sizeof hostName);
    if (result == -1) {
        cout << "Initialization: get host error." << endl;
        return(EHOST);
    }
    
    info->address = hostName;
    
    /* get the socket fd and the free port */
	
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(0);				// next available port
	saddr.sin_addr.s_addr = INADDR_ANY;		// set ip for any
	
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
        cout << "Initialization: create socket error." << endl;
        return(ESOCK);
    }
	result = bind(sockfd, (struct sockaddr *)&saddr, sizeof saddr);
	if (result == -1) {
        cout << "Initialization: binding socket error." << endl;
        return(EBIND);
    }
	
	result = listen(sockfd, 10);
	if (result == -1) {
        cout << "Initialization: listen socket error." << endl;
        return(ELISTEN);
    }
    
    info->sockfd = sockfd;
	
	socklen_t len = sizeof(taddr);
	result = getsockname(sockfd, (struct sockaddr *)&taddr, &len);
	if (result == -1) {
        cout << "Initialization: get socket name error." << endl;
        return(ESNAME);
    };
	
	info->port = ntohs(taddr.sin_port);
	
	return 0;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &((struct sockaddr_in*) sa)->sin_addr;
    }
    return &((struct sockaddr_in6*) sa)->sin6_addr;
}
