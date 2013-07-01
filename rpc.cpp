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
	server_info = new Info();
    struct sockaddr_storage client; // client address
    int new_accept;                 // newly accepted fd
    int result;                     // return value
    socklen_t addrlen;              // size of ai_addr
    
    fd_set master;                  // master fd list
    fd_set temp;                    // temp fd list
    
    result = Init(server_info);
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
	
	memcpy(buf, &length, LENGTH_SIZE);
	memcpy(buf+4, REGISTER, TYPE_SIZE);
	memcpy(buf+8, server_info->address, SERVER_ID_SIZE);
	memcpy(buf+136, &server_info->port, PORT_SIZE);
	strcpy(buf+138, name);				
	memcpy(buf+238, argTypes, argType_size);
	
	// buf which is the message to be sent is now filled
	
	result = (int) send(binder_sock, (const void *) buf, LENGTH_SIZE + TYPE_SIZE + length, 0);
	if (result != 0) return ESEND;
	
	// free buf since we are no longer using it
	delete [] buf;
	
	result = (int) recv(binder_sock, &ret_type, 4, 0);
	if (result != 0) return ERECV;
	result = (int) recv(binder_sock, &ret_val, 4, 0);
	if (result != 0) return ERECV;
	
	// determine if register success
	
	if (ret_type == REGISTER_SUCCESS || ret_type == REGISTER_FAILURE) {
		if (ret_val < 0) return ret_val;
	}

	// create the entry to be added into local database
	
	server_func *server_entry = new server_func();
	server_entry->name = new char[strlen(name)+1];
	server_entry->argTypes = new int[argType_size/4];
	
	strcpy(server_entry->name, name);						// copy the name
	memcpy(server_entry->argTypes, argTypes, argType_size);		// copy the argument types
	server_entry->func = f;									// copy the skeleton (address of function)
	
	// add the entry to the local database;
	
	// must do a check here, that does: check if a function with the exact same name, and same argTypes exists
	//   if that exists, free that one and updated with the new one.
	
	int replaced = 0;
	list<server_func*>::iterator it;
	for (it=server_db.begin(); it!=server_db.end(); it++) {
		if (server_entry->name == (*it)->name) {
			int i;
			for (i=0; (*it)->argTypes[i] != 0; i++);
			if (argType_size/4 == ++i) {	// if same size of argument types then replace with latest
				delete [] (*it)->name;
				delete [] (*it)->argTypes;
				delete [] (*it);
				*it = server_entry;
				replaced = 1;
				break;
			}
		}
	}
	
	if (replaced == 0) server_db.push_back(server_entry);
	
	return ret_val;				// return either the warning or a 0 for success
}

extern int rpcExecute() {	// for this function remember to use threads!!!
    fd_set master;                  // master fd list
    fd_set temp;                    // temp fd list
	int new_accept;                 // newly accepted fd
	struct sockaddr_storage client; // client address
	
    
	int listener = server_info->sockfd;
    char clientIP[INET_ADDRSTRLEN];
    int maxFdNum = listener;
    int clientNum = 0;
	
    FD_SET(listener, &master);
    
	/* just starting to edit this! copied over from binder.cpp */
	while (true) {
        temp = master;
        if (select(maxFdNum + 1, &temp, NULL, NULL, NULL) == -1) {
            cerr << "Server: Select error." << endl;
            return ESELECT;
        }
		
		// run through the existing connections to look for datas to read
        for (int i = 0; i <= maxFdNum; i++) {
			if (FD_ISSET(i, &temp)) {
                if (i == listener) {
                    // handle new connection
                    addrlen = sizeof client;
                    new_accept = accept(listener, (struct sockaddr *) &client, &addrlen);
                    
                    if (new_accept == -1) {
                        cerr << "Server: Accept error." << endl;
                    } 
					else {
                        FD_SET(new_accept, &master);
                        if (new_accept > maxFdNum) maxFdNum = new_accept;
                        clientNum++;
                        
                        cout << "Server: new connection from " << inet_ntop(client.ss_family, get_in_addr((struct sockaddr*) &client), clientIP, INET6_ADDRSTRLEN) << " on socket " << new_accept << endl;
                    }
                } 
				else {// recieve the length of the message
                    if ((nbytes = (int) recv(i, &lenBuf, sizeof(int), 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            cout << "Server: socket " << i << " hung up." << endl;
                        } 
						else cerr << "Server: Recieve error." << endl;
						
                        close(i);
                        FD_CLR(i, &master);
                        clientNum--;
                    } else {
                        // recieve the type of the message
                        
                        if ((nbytes = (int) recv(i, &typeBuf, sizeof(int), 0)) <= 0) {
                            // got invalid message or connection closed by client
                            if (nbytes == 0) {
                                // get
                                cout << "Server: socket " << i << " hung up." << endl;
                            } else {
                                cerr << "Server: Recieve error." << endl;
                            }
                            close(i);
                            FD_CLR(i, &master);
                            clientNum--;
                        } else {
                            strBuf = new char[lenBuf + 1];
                            
                            if ((nbytes = (int) recv(i, &strBuf, sizeof(strBuf), 0)) <= 0) {
                                // got invalid message or connection closed by client
                                if (nbytes == 0) {
                                    // get
                                    cout << "Server: socket " << i << " hung up." << endl;
                                } else {
                                    cerr << "Server: Recieve error." << endl;
                                }
                                close(i);
                                FD_CLR(i, &master);
                                clientNum--;
                            } else {
                            cout << strBuf << endl;
                                
                                Message *msg = new Message();
                                
                                switch (typeBuf) {
                                    case REGISTER:
                                        
                                        msg = parseMessage(strBuf, typeBuf);
                                        
                                        
                                        break;
                                    case LOC_REQUEST:
                                        break;
                                    case EXECUTE:
                                        break;
                                    case TERMINATE:
                                        break;
                                    default:
                                        break;
                                }
                            
                            // we got some data from client
                            for (int j = 0; j <=maxFdNum ; j++) {
                                // send to everyone
                                if (FD_ISSET(j, &master)) {
                                    // except the listener and ourselves
                                    if (j == i) {
                                        
//                                        if (send(j , modifed_str, nbytes, 0) == -1) {
//                                            cerr << "Server: Send error." << endl;
//                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
		}/*for*/
	}/*while*/
}

extern int rpcTerminate() {
	// remember to free local database and server_info
}