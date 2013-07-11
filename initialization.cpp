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


using std::cout;
using std::cin;
using std::cerr;
using std::endl;
using std::string;
using std::string;

int Init(struct Info **info) {
    char hostName[128];

    struct addrinfo hints, *res, *counter;  // info for recursing
    struct sockaddr_in saddr, taddr;		// for packing server and a placeholder
	int sockfd;                             // socket file descriptor for listening, and placeholder
	int result, status;						// for checking return values
    
    /* get the host name of the binder machine */
    
    result = gethostname(hostName, sizeof hostName);
    if (result == -1) {
        cout << "Initialization: get host error." << endl;
        return(EHOST);
    }
    
    char *actName = new char[128];
    strcpy(actName, hostName);
    
    (*info)->address = actName;
    
    /* get the socket fd and the free port */
	
    memset(&hints, 0 , sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	status = getaddrinfo(INADDR_ANY, "0", &hints, &res);
	if (status != 0) return -6;
	
	// go along the linked list for a valid entry
	for(counter = res; counter != NULL; counter = counter->ai_next) {
		sockfd = socket(counter->ai_family, counter->ai_socktype, counter->ai_protocol);
		if (sockfd == -1) continue;	// has not found a socket descriptor
		
		status = bind(sockfd, counter->ai_addr, counter->ai_addrlen);
		if (status == -1) continue; // cannot connect to socket
		
		break;						// found good socket descriptor
	}
	if (counter == NULL) return -2; // end here
    
    /*
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
    }*/
	
	result = listen(sockfd, 10);
	if (result == -1) {
        cout << "Initialization: listen socket error." << endl;
        return(ELISTEN);
    }
    
    (*info)->sockfd = sockfd;
	
	socklen_t len = sizeof(taddr);
	result = getsockname(sockfd, (struct sockaddr *)&taddr, &len);
	if (result == -1) {
        cout << "Initialization: get socket name error." << endl;
        return(ESNAME);
    };
    
    char portstring[2];
    
    sprintf(portstring, "%hu", ntohs(taddr.sin_port));
	
    char *actPort = new char[2];
    strcpy(actPort, portstring);
    
	(*info)->port = actPort;
	
	return 0;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &((struct sockaddr_in*) sa)->sin_addr;
    }
    return &((struct sockaddr_in6*) sa)->sin6_addr;
}
