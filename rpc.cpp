//
//  rpc.cpp
//  RPC
//
//  Created by Haochen Ding on 2014-06-26.
//  Copyright (c) 2013 Haochen Ding and the mastermind Shisong Tang. All rights reserved.
//
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
#include <pthread.h>

#include "error.h"
#include "binder.h"
#include "initialization.h"
#include "message.h"
#include "rpc.h"
#include "server_func.h"

using namespace std;

extern int rpcInit() 
{
	// set up socket for listening
	info = new Info();
    struct sockaddr_storage client; // client address
    int new_accept;                 // newly accepted fd
    int result;                     // return value
    socklen_t addrlen;              // size of ai_addr
    
    fd_set master;                  // master fd list
    fd_set temp;                    // temp fd list
    
    result = Init(info);
	if (result < 0) return result;
	
	/* done creating socket for acceting connections from clients */
	
	// open connection to binder
	char *host_name;						// server address 
	char *port_num;							// server port
	struct addrinfo hints, *res, *counter;	// addrinfo for specifying and getting info and a counter
	
	// get server address and port from environment variable
	host_name = getenv("BINDER_ADDRESS");
	port_num = getenv("BINDER_PORT");
	
	// specify to use TCP instead of datagram
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	if (getaddrinfo(host_name, port_num, &hints, &res) != 0) return EGADDR;
	
	// go along the linked list for a valid entry
	for(counter = res; counter != NULL; counter = counter->ai_next) {
		binder_sock = socket(counter->ai_family, counter->ai_socktype, counter->ai_protocol);
		if (binder_sock == -1) continue;	// has not found a socket descriptor
		
		result = connect(binder_sock, counter->ai_addr, counter->ai_addrlen);
		if (result == -1) continue; // cannot connect to socket
		
		break;						// found good socket descriptor
	}
	if (counter == NULL) return ESOCK;
	
	freeaddrinfo(res);
	
	delete info;
	
	return 0;
}

extern int rpcCall(char* name, int* argTypes, void** args) {

}

extern int rpcCacheCall(char* name, int* argTypes, void** args) {
    
}

extern int rpcRegister(char* name, int* argTypes, skeleton f) {
    // count size of argTypes
	int argType_size;			// size of argTypes
	int length;					// length of the message portion
	int result;					// for error checking
	int ret_type;				// whether REGISTER_SUCCESS or REGISTER_FAILURE
	int ret_val;				// warnings or errors returned
	
    for (argType_size=0; argTypes[argType_size]!=0; argType_size++);
	argType_size = 4*(argType_size+1);			// 4 bytes per element counting 0 at the end
	
	length = SERVER_ID_SIZE + PORT_SIZE + NAME_SIZE + argType_size;
	
	char *buf = new char[LENGTH_SIZE + TYPE_SIZE + length];
	
	memcpy(buf, length, LENGTH_SIZE);
	memcpy(buf+4, REGISTER, TYPE_SIZE);
	memcpy(buf+8, info->address, SERVER_ID_SIZE);
	memcpy(buf+136, info->port, PORT_SIZE);
	strncpy(buf+138, name, NAME_SIZE);
	memcpy(buf+238, argTypes, argType_size);
	
	// buf which is the message to be sent is now filled
	
	result = send(binder_sock, (const void *) buf, LENGTH_SIZE + TYPE_SIZE + length, 0);
	if (result != 0) return ESEND;
	
	result = recv(binder_sock, &ret_type, 4, 0);
	if (result != 0) return ERECV;
	
	if (ret_type == REGISTER_SUCCESS) {
		result = recv(binder_sock, &ret_val, 4, 0);
		
	}
}

extern int rpcExecute() {
    
}

extern int rpcTerminate() {
    
}