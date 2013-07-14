//
//  rpc.cpp
//  RPC
//
//  Created by Haochen Ding & Shisong Tang on 2013-06-26.
//  Copyright (c) 2013 Haochen Ding & Shisong Tang. All rights reserved.
//
#include <iostream>
#include <list>
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
#include <errno.h>

#include "error.h"
#include "initialization.h"
#include "message.h"
#include "rpc.h"
#include "server_func.h"
#include "cache.h"

using namespace std;

extern int rpcInit()
{
	// set up socket for listening
	server_info = new Info();
    int result;                     // return value
    
    result = Init(&server_info);
	if (result < 0) return result;
    
	/* done creating socket for acceting connections from clients */
	
	// open connection to binder
	char *host_name;						// binder address
	char *port_num;							// binder port
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
	// open connection to binder
	char *host_name;						// binder address
	char *port_num;							// binder port
	struct addrinfo hints, *res, *counter;	// addrinfo for specifying and getting info and a counter
	int client_binder_sock;					// socket for connection with binder
	int result;								// check returns
	
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
		client_binder_sock = socket(counter->ai_family, counter->ai_socktype, counter->ai_protocol);
		if (client_binder_sock == -1) continue;	// has not found a socket descriptor
		
		result = connect(client_binder_sock, counter->ai_addr, counter->ai_addrlen);
		if (result == -1) continue; // cannot connect to socket
		
		break;						// found good socket descriptor
	}
	if (counter == NULL) return ESOCK;
    
	freeaddrinfo(res);
    
	// form the location request message
	char *locReq;					// request buffer to send the request message
	int requestLen;					// total size of the buffer
	int msglen;						// size of the "message" portion
	int argType_size=0;				// counter for argTypes size
    
	for (int i=0; argTypes[i] != 0; i++) argType_size++;
	argType_size++;
	
	requestLen = LENGTH_SIZE + TYPE_SIZE + NAME_SIZE + ARGTYPE_SIZE * argType_size;
	msglen = NAME_SIZE + ARGTYPE_SIZE * argType_size;
    
	locReq = new char [requestLen];
    
    int msgtype = LOC_REQUEST;
    
	// place the information into the request buffer
	memcpy(locReq, &msglen, LENGTH_SIZE);
	memcpy(locReq+LENGTH_SIZE, &msgtype, TYPE_SIZE);
	strcpy(locReq+LENGTH_SIZE+TYPE_SIZE, name);
	memcpy(locReq+LENGTH_SIZE+TYPE_SIZE+NAME_SIZE, argTypes, ARGTYPE_SIZE * argType_size);
    
	if (send(client_binder_sock, locReq, requestLen, 0) == -1) {
		cerr << "ERROR: could not send location request from function (" << name << ") to binder" << endl;
		return ESEND;
	}
    
	// sent location request to binder, time to parse received results
    
	int nbytes;						// track total bytes actually received
	int loc_replyLen;				// length of location request reply msg
	int loc_replyType;				// type of the location request reply
    
	if ((nbytes = (int) recv(client_binder_sock, &loc_replyLen, sizeof(int), 0)) <= 0) {
		close(client_binder_sock);
		return ERECV;
	}
    
    if ((nbytes = (int) recv(client_binder_sock, &loc_replyType, sizeof(int), 0)) <= 0) {
		close(client_binder_sock);
		return ERECV;
	}
    
    char *msgbuf = new char[loc_replyLen];
    
    if ((nbytes = (int) recv(client_binder_sock, msgbuf, loc_replyLen, 0)) <= 0) {
    	delete [] msgbuf;
		close(client_binder_sock);
		return ERECV;
	}
    
	Message *loc_reply;
    
    if (loc_replyType == LOC_FAILURE) {
    	loc_reply = parseMessage(msgbuf, LOC_FAILURE, loc_replyLen);
    	int errcode = loc_reply->reasonCode;
        
    	delete loc_reply;
    	delete [] msgbuf;
		close(client_binder_sock);
		
		return errcode;
    }
    else if (loc_replyType == LOC_SUCCESS) {
		close(client_binder_sock);
    	loc_reply = parseMessage(msgbuf, LOC_SUCCESS, loc_replyLen);
        
    	// open connection to requested server
		char *server_host_name;											// binder address
		char *server_port_num;											// binder port
		struct addrinfo server_hints, *server_res, *server_counter;		// addrinfo for specifying and getting info and a counter
		int server_sock;												// socket for server
		
		// get server address and port from environment variable
		server_host_name = loc_reply->server_identifier;
		server_port_num = loc_reply->port;
		
		// specify to use TCP instead of datagram
		memset(&server_hints, 0, sizeof server_hints);
		server_hints.ai_family = AF_INET;
		server_hints.ai_socktype = SOCK_STREAM;
		server_hints.ai_flags = AI_PASSIVE;
		
		if (getaddrinfo(server_host_name, server_port_num, &server_hints, &server_res) != 0) return EGADDR;
		
		// go along the linked list for a valid entry
		for(server_counter = server_res; server_counter != NULL; server_counter = server_counter->ai_next) {
			server_sock = socket(server_counter->ai_family, server_counter->ai_socktype, server_counter->ai_protocol);
			if (server_sock == -1) continue;	// has not found a socket descriptor
			
			result = connect(server_sock, server_counter->ai_addr, server_counter->ai_addrlen);
			if (result == -1) continue; // cannot connect to socket
			
			break;						// found good socket descriptor
		}
		if (server_counter == NULL) return ESOCK;
		
		freeaddrinfo(server_res);
        
		// form the execute message
		char *execReq;					// execute buffer to send the execute message
		int execLen;					// total size of the buffer
		int *argLen_arr;				// array to track each arg length
		
		argLen_arr = new int[argType_size-1];
        
		execLen = LENGTH_SIZE + TYPE_SIZE + NAME_SIZE + ARGTYPE_SIZE * argType_size;
        
		for (int i = 0; i < argType_size - 1; i++) {
            int arg_type = (argTypes[i] >> (8*2)) & 0xff;
            int arg_length = argTypes[i] & 0xffff;
            
            int size_of_type;
            if (arg_type == ARG_CHAR) {
                size_of_type = sizeof(char);
            }
            else if (arg_type == ARG_SHORT) {
                size_of_type = sizeof(short);
            }
            else if (arg_type == ARG_INT) {
                size_of_type = sizeof(int);
            }
            else if (arg_type == ARG_LONG) {
                size_of_type = sizeof(long);
            }
            else if (arg_type == ARG_DOUBLE) {
                size_of_type = sizeof(double);
            }
            else if (arg_type == ARG_FLOAT) {
                size_of_type = sizeof(float);
            }
            if (arg_length == 0) {
            	argLen_arr[i] = size_of_type;
            	execLen = execLen + size_of_type;
            }
            else {
            	argLen_arr[i] = size_of_type * arg_length;
            	execLen = execLen + size_of_type * arg_length;
            }
        }
        
        execReq = new char[execLen];
        
		msglen = execLen - LENGTH_SIZE - TYPE_SIZE;
        
        msgtype = EXECUTE;
        
		// place the information into the execute message buffer
		memcpy(execReq, &msglen, LENGTH_SIZE);
		memcpy(execReq+LENGTH_SIZE, &msgtype, TYPE_SIZE);
		strcpy(execReq+LENGTH_SIZE+TYPE_SIZE, name);
		memcpy(execReq+LENGTH_SIZE+TYPE_SIZE+NAME_SIZE, argTypes, ARGTYPE_SIZE * argType_size);
        
		char *temp = execReq + LENGTH_SIZE + TYPE_SIZE + NAME_SIZE + ARGTYPE_SIZE * argType_size;			// temp iterator
        
		for (int j=0; j < argType_size-1; j++) {
			memcpy(temp, args[j], argLen_arr[j]);
			temp = temp + argLen_arr[j];
		}
        
		// finished packing the execute message, time to send
		if (send(server_sock, execReq, execLen, 0) == -1) {
			cerr << "ERROR: could not send execute message from function (" << name << ") to server " << server_host_name << endl;
			return ESEND;
		}
        
		// sent execute message to server, time to parse received results
        
        // nbytes is the total of bytes received
        int exec_replyLen;                          // length of execute request reply msg
        int exec_replyType;                         // type of the execute request reply
        
        if ((nbytes = (int) recv(server_sock, &exec_replyLen, sizeof(int), 0)) <= 0) {
            close(server_sock);
            return ERECV;
        }
        
        if ((nbytes = (int) recv(server_sock, &exec_replyType, sizeof(int), 0)) <= 0) {
            close(server_sock);
            return ERECV;
        }
        
        char *msgbuf2 = new char[exec_replyLen];
        
        if ((nbytes = (int) recv(server_sock, msgbuf2, exec_replyLen, 0)) <= 0) {
            delete [] msgbuf2;
            close(server_sock);
            return ERECV;
        }
        
        Message *exec_reply;
        
        if (exec_replyType == EXECUTE_FAILURE) {
            exec_reply = parseMessage(msgbuf2, EXECUTE_FAILURE, exec_replyLen);
            int errcode = exec_reply->reasonCode;
            
            delete exec_reply;
            delete [] msgbuf2;
            close(server_sock);
            
            return errcode;
        }
        else if (exec_replyType == EXECUTE_SUCCESS) {
            exec_reply = parseMessage(msgbuf2, EXECUTE_SUCCESS, exec_replyLen);
            for (int argcounter=0; exec_reply->argTypes[argcounter]!=0; argcounter++) {
                argTypes[argcounter] = exec_reply->argTypes[argcounter];
                args[argcounter] = exec_reply->args[argcounter];
            }
            
            close(server_sock);
            
        }/*else if EXECUTE_SUCCESS*/
    }/*else if LOC_SUCCESS*/
    
    return 0;
}

int directSendToServer(char* server_host_name, char* server_port_num, char* name, int* argTypes, int argTypesSize, void** args) {
    list<pair<char*, char*> >::iterator server_list_it;
    int result;
    int msglen;
    int msgtype;
    int nbytes;
    
    struct addrinfo server_hints, *server_res, *server_counter;		// addrinfo for specifying and getting info and a counter
    int server_sock;												// socket for server
    
    // specify to use TCP instead of datagram
    memset(&server_hints, 0, sizeof server_hints);
    server_hints.ai_family = AF_INET;
    server_hints.ai_socktype = SOCK_STREAM;
    server_hints.ai_flags = AI_PASSIVE;
    
    if (getaddrinfo(server_host_name, server_port_num, &server_hints, &server_res) != 0) return EGADDR;
    // go along the linked list for a valid entry
    for(server_counter = server_res; server_counter != NULL; server_counter = server_counter->ai_next) {
        server_sock = socket(server_counter->ai_family, server_counter->ai_socktype, server_counter->ai_protocol);
        if (server_sock == -1) continue;	// has not found a socket descriptor
                
        result = connect(server_sock, server_counter->ai_addr, server_counter->ai_addrlen);
        
        if (result == -1) {
            continue;
        }
            //continue; // cannot connect to socket
        
        break;						// found good socket descriptor
    }
    if (server_counter == NULL) return ESOCK;

    freeaddrinfo(server_res);
    
    // form the execute message
    char *execReq;					// execute buffer to send the execute message
    int execLen;					// total size of the buffer
    int *argLen_arr;				// array to track each arg length
    
    argLen_arr = new int[argTypesSize-1];
    
    execLen = LENGTH_SIZE + TYPE_SIZE + NAME_SIZE + ARGTYPE_SIZE * argTypesSize;
    for (int i = 0; i < argTypesSize - 1; i++) {
        int arg_type = (argTypes[i] >> (8*2)) & 0xff;
        int arg_length = argTypes[i] & 0xffff;
        
        int size_of_type;
        if (arg_type == ARG_CHAR) {
            size_of_type = sizeof(char);
        }
        else if (arg_type == ARG_SHORT) {
            size_of_type = sizeof(short);
        }
        else if (arg_type == ARG_INT) {
            size_of_type = sizeof(int);
        }
        else if (arg_type == ARG_LONG) {
            size_of_type = sizeof(long);
        }
        else if (arg_type == ARG_DOUBLE) {
            size_of_type = sizeof(double);
        }
        else if (arg_type == ARG_FLOAT) {
            size_of_type = sizeof(float);
        }
        if (arg_length == 0) {
            argLen_arr[i] = size_of_type;
            execLen = execLen + size_of_type;
        }
        else {
            argLen_arr[i] = size_of_type * arg_length;
            execLen = execLen + size_of_type * arg_length;
        }
    }
    
    execReq = new char[execLen];
    
    msglen = execLen - LENGTH_SIZE - TYPE_SIZE;
    
    msgtype = EXECUTE;
    
    // place the information into the execute message buffer
    memcpy(execReq, &msglen, LENGTH_SIZE);
    memcpy(execReq+LENGTH_SIZE, &msgtype, TYPE_SIZE);
    strcpy(execReq+LENGTH_SIZE+TYPE_SIZE, name);
    memcpy(execReq+LENGTH_SIZE+TYPE_SIZE+NAME_SIZE, argTypes, ARGTYPE_SIZE * argTypesSize);
    
    char *temp = execReq + LENGTH_SIZE + TYPE_SIZE + NAME_SIZE + ARGTYPE_SIZE * argTypesSize;			// temp iterator
    
    
    for (int j=0; j < argTypesSize-1; j++) {
        memcpy(temp, args[j], argLen_arr[j]);
        temp = temp + argLen_arr[j];
    }
    
    // finished packing the execute message, time to send
    if (send(server_sock, execReq, execLen, 0) == -1) {
        cerr << "ERROR: could not send execute message from function (" << name << ") to server " << server_host_name << endl;
        return ESEND;
    }
    
    // sent execute message to server, time to parse received results
    
    // nbytes is the total of bytes received
    int exec_replyLen;                          // length of execute request reply msg
    int exec_replyType;                         // type of the execute request reply
    
    if ((nbytes = (int) recv(server_sock, &exec_replyLen, sizeof(int), 0)) <= 0) {
        close(server_sock);
        return ERECV;
    }
    
    if ((nbytes = (int) recv(server_sock, &exec_replyType, sizeof(int), 0)) <= 0) {
        close(server_sock);
        return ERECV;
    }
    
    char *msgbuf2 = new char[exec_replyLen];
    
    if ((nbytes = (int) recv(server_sock, msgbuf2, exec_replyLen, 0)) <= 0) {
        delete [] msgbuf2;
        close(server_sock);
        return ERECV;
    }
    
    Message *exec_reply;
    
    if (exec_replyType == EXECUTE_FAILURE) {
        exec_reply = parseMessage(msgbuf2, EXECUTE_FAILURE, exec_replyLen);
        int errcode = exec_reply->reasonCode;
        
        delete exec_reply;
        delete [] msgbuf2;
        close(server_sock);
        
        return errcode;
    }
    else if (exec_replyType == EXECUTE_SUCCESS) {
        exec_reply = parseMessage(msgbuf2, EXECUTE_SUCCESS, exec_replyLen);
        for (int argcounter=0; exec_reply->argTypes[argcounter]!=0; argcounter++) {
            argTypes[argcounter] = exec_reply->argTypes[argcounter];
            args[argcounter] = exec_reply->args[argcounter];
        }
        
        close(server_sock);
        
    }
    return 0;
}


int cache_request(char* name, int* argTypes, int argType_size) {
    list<pair<char*, char*> >::iterator server_list_it;
    int msgType;
    int sendLength;
    char *sendBuf;
    char *msgBuf;
    
    int result;					// for error checking
    int ret_type;				// whether REGISTER_SUCCESS or REGISTER_FAILURE
    int ret_len;                // length of receieved message
    
    struct Message *reply;      // the reply message
    
    // open connection to binder
	char *host_name;						// binder address
	char *port_num;							// binder port
	struct addrinfo hints, *res, *counter;	// addrinfo for specifying and getting info and a counter
	int binder_sock;					// socket for connection with binder
	
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
    
    
    
    // if we cannot find the same function
    // just send the CACHE_REQUEST to the binder to update the server list
    
    sendLength = NAME_SIZE + argType_size * 4;
    sendBuf = new char[LENGTH_SIZE + TYPE_SIZE + sendLength];
    msgType = CACHE_REQUEST;
    
    memcpy(sendBuf, &sendLength, LENGTH_SIZE);
    memcpy(sendBuf+LENGTH_SIZE, &msgType, TYPE_SIZE);
    strcpy(sendBuf+LENGTH_SIZE+TYPE_SIZE, name);
    
    memcpy(sendBuf+LENGTH_SIZE+TYPE_SIZE+NAME_SIZE, argTypes, argType_size * 4);
    
    // send CACHE_REQUEST to binder
    result = (int) send(binder_sock, sendBuf, LENGTH_SIZE + TYPE_SIZE + sendLength, 0);
    if (result < 0) {
        cerr << "rpcCacheCall: Send error, CACHE_FAILURE." << endl;
    }
    
    // receive the reply from binder
    result = (int) recv(binder_sock, &ret_len, 4, 0);
    if (result < 0) return ERECV;
    result = (int) recv(binder_sock, &ret_type, 4, 0);
    if (result < 0) return ERECV;
    
    
    //    msgBuf = new char[ret_len];
    
    if (ret_type == CACHE_SUCCESS) {
        char *ret_val;
        ret_val = new char[ret_len];
        result = (int) recv(binder_sock, ret_val, ret_len, 0);
        if (result < 0) return ERECV;
        
        cache_entry cacheEntry;
        cacheEntry.name = name;
        cacheEntry.argTypes = argTypes;
        cacheEntry.argTypeSize = argType_size;
        
        // loop to get each servers info from the reply message
        int numServers = ret_len/(SERVER_ID_SIZE + PORT_SIZE);
        for (int i = 0; i < numServers; i++) {
            pair<char*, char*> server_list_entry;
            
            char *server_id = new char[strlen(ret_val) + 1];
            strcpy(server_id, ret_val);
            ret_val = ret_val + SERVER_ID_SIZE;
            
            char *server_port = new char[PORT_SIZE];
            strcpy(server_port, ret_val);
            ret_val = ret_val + PORT_SIZE;
            
            server_list_entry = make_pair(server_id, server_port);
            cacheEntry.server_list.push_back(server_list_entry);
        }
        
        // add this cache entry to the list
        cache.push_back(cacheEntry);
        close(binder_sock);

        return 0;
    }
    else if (ret_type == CACHE_FAILURE) {
        int ret_val;
        result = (int) recv(binder_sock, &ret_val, 4, 0);
        if (result < 0) return ERECV;
        
        //        reply = parseMessage(ret_val, CACHE_FAILURE, ret_len);
        
    	int errcode = ret_val;
        
		close(binder_sock);
        
		return errcode;
        
    }
}


extern int rpcCacheCall(char* name, int* argTypes, void** args) {
    
    list<cache_entry>::iterator it;
    list<pair<char*, char*> >::iterator server_list_it;
    
    int msgType;
    int sendLength;
    char *sendBuf;
    char *msgBuf;
    
    int counter;
    int result;
    bool sent;
    
    // get the number of argument types
    int argTypesSize = 0;
    for (int i = 0; argTypes[i] != 0; i++) argTypesSize++;
    argTypesSize++;
    
    // bool value to indicate if we found the same function
    bool found = false;
    
    // loop to scan the database for the same function
    for (it = cache.begin(); it != cache.end(); it++) {

        if (!strcmp(name, it->name) && (argTypesSize == it->argTypeSize)) {
            int counter = argTypesSize - 1;
            while (counter >= 0) {
                if (argTypes[counter] != it->argTypes[counter]) {
                    break;
                }
                //check that input output bits are the same, and same type for stored and current
                int s_pre_arg = (int)it->argTypes[counter] & 0xffff0000;
                int c_pre_arg = (int)argTypes[counter] & 0xffff0000;
                
                if (s_pre_arg != c_pre_arg) break;
                
                // check that they are both scalar or both arrays
                int server_arg_len = (int)it->argTypes[counter] & 0xffff;
                int curr_arg_len = (int)argTypes[counter] & 0xffff;
                
                if (!(((server_arg_len > 0) && (curr_arg_len > 0)) || ((server_arg_len == 0) && (curr_arg_len == 0)))) break;
                
                // both inout are same, type is the same, and they or both scalar or both arrays
                // then check the next argument type
                counter--;
            }
            if (counter == -1) {
                found = true;
                // if found same function, break out of the loop
                break;
            }
        }
    }
    
    if (found) {
        sent = false;
        
        for (server_list_it = it->server_list.begin(); server_list_it != it->server_list.end(); server_list_it++) {
            
            // open connection to requested server
            char *server_host_name;
            char *server_port_num;
            
            // get server address and port
            server_host_name = server_list_it->first;
            server_port_num = server_list_it->second;
                        
            // call the server directly
            result = directSendToServer(server_host_name, server_port_num, name, argTypes, argTypesSize, args);
            if (result == 0) {
                // call the server successfully
                sent = true;
                
                // modify the cache according to round robin
                for (it = cache.begin(); it != cache.end(); it++) {
                    for (server_list_it = it->server_list.begin(); server_list_it != it->server_list.end(); server_list_it++) {
                        if (server_list_it->first == server_host_name) {
                            it->server_list.splice(it->server_list.end(), it->server_list, server_list_it);
                        }
                    }
                }
                break;
            }
        }
        
        // judge if the call successful
        if (sent) {
        } else {
            // if none server could be called, request cache from binder
            // remove the old server list
            cache.erase(it);
            result = cache_request(name, argTypes, argTypesSize);
            sent = false;
            if (result == 0) {
                it = cache.begin();
                counter = cache.size() - 1;
                while (counter != 0) {
                    it++;
                    counter--;
                };

                // call the server directly
                for (server_list_it = it->server_list.begin(); server_list_it != it->server_list.end(); server_list_it++) {
                    
                    // open connection to requested server
                    char *server_host_name;
                    char *server_port_num;
                    
                    // get server address and port from environment variable
                    server_host_name = server_list_it->first;
                    server_port_num = server_list_it->second;
                    
                    // call the server directly
                    result = directSendToServer(server_host_name, server_port_num, name, argTypes, argTypesSize, args);
                    
                    if (result == 0) {
                        sent = true;
                        
                        // modify the cache according to round robin
                        for (it = cache.begin(); it != cache.end(); it++) {
                            for (server_list_it = it->server_list.begin(); server_list_it != it->server_list.end(); server_list_it++) {
                                if (server_list_it->first == server_host_name) {
                                    it->server_list.splice(it->server_list.end(), it->server_list, server_list_it);
                                }
                            }
                        }
                        break;
                    }
                }
                
                // judge if the call successful
                if (sent) {
                } else {
                    return -1;
                }
            } else {
                return result;
            }
        }
        
    } else {
        // request cache from binder
        result = cache_request(name, argTypes, argTypesSize);
        sent = false;
        if (result == 0) {
            it = cache.begin();
            counter = cache.size() - 1;
            while (counter != 0) {
                it++;
                counter--;
            };
            // call the server directly
            for (server_list_it = it->server_list.begin(); server_list_it != it->server_list.end(); server_list_it++) {
                // open connection to requested server
                char *server_host_name;
                char *server_port_num;
                
                // get server address and port from environment variable
                server_host_name = server_list_it->first;
                server_port_num = server_list_it->second;
                // call the server directly
                result = directSendToServer(server_host_name, server_port_num, name, argTypes, argTypesSize, args);
                if (result == 0) {
                    // call the server successfully
                    sent = true;
                    
                    // modify the cache according to round robin
                    for (it = cache.begin(); it != cache.end(); it++) {
                        for (server_list_it = it->server_list.begin(); server_list_it != it->server_list.end(); server_list_it++) {
                            if (server_list_it->first == server_host_name) {
                                it->server_list.splice(it->server_list.end(), it->server_list, server_list_it);
                            }
                        }
                    }
                    break;
                }
            }
            
            // judge if the call successful
            if (sent) {
            } else {
                return -1;
            }
            
        } else {
            return result;
        }
        
        
        // modify the cache according to round robin
        
    }
    return 0;
    
}



extern int rpcRegister(char* name, int* argTypes, skeleton f) {
    // count size of argTypes
    int argType_size;			// size of argTypes
    int length;					// length of the message portion
    int result;					// for error checking
    int ret_type;				// whether REGISTER_SUCCESS or REGISTER_FAILURE
    int ret_val;				// warnings or errors returned
    int ret_len;                // length of receieved message
    
    for (argType_size=0; argTypes[argType_size]!=0; argType_size++);
    argType_size++;
    argType_size = 4*(argType_size);			// 4 bytes per element counting 0 at the end
    
    length = SERVER_ID_SIZE + PORT_SIZE + NAME_SIZE + argType_size;
    
    char *buf = new char[LENGTH_SIZE + TYPE_SIZE + length];
    
    int msgtype = REGISTER;
    
    memcpy(buf, &length, LENGTH_SIZE);
    memcpy(buf+LENGTH_SIZE, &msgtype, TYPE_SIZE);
    memcpy(buf+LENGTH_SIZE+TYPE_SIZE, server_info->address, SERVER_ID_SIZE);
    strcpy(buf+LENGTH_SIZE+TYPE_SIZE+SERVER_ID_SIZE, server_info->port);
    strcpy(buf+LENGTH_SIZE+TYPE_SIZE+SERVER_ID_SIZE+PORT_SIZE, name);
    memcpy(buf+LENGTH_SIZE+TYPE_SIZE+SERVER_ID_SIZE+PORT_SIZE+NAME_SIZE, argTypes, argType_size);
    
    // buf which is the message to be sent is now filled
    
    result = (int) send(binder_sock, buf, LENGTH_SIZE + TYPE_SIZE + length, 0);
    if (result == -1) return result;
    
    // free buf since we are no longer using it
    delete [] buf;
    
    result = (int) recv(binder_sock, &ret_len, 4, 0);
    if (result < 0) return ERECV;
    result = (int) recv(binder_sock, &ret_type, 4, 0);
    if (result < 0) return ERECV;
    result = (int) recv(binder_sock, &ret_val, 4, 0);
    if (result < 0) return ERECV;
    
    // determine if register success
    
    if (ret_type == REGISTER_SUCCESS || ret_type == REGISTER_FAILURE) {
        if (ret_val < 0) return ret_val;
    }
    
    // create the entry to be added into local database
    
    server_func *server_entry = new server_func();
    server_entry->name = new char[strlen(name)+1];
    server_entry->argTypes = new int[argType_size/4];
    
    strcpy(server_entry->name, name);							// copy the name
    memcpy(server_entry->argTypes, argTypes, argType_size);		// copy the argument types
    server_entry->func = f;										// copy the skeleton (address of function)
    
    // add the entry to the local database;
    
    // must do a check here, that does: check if a function with the exact same name, and same argTypes exists
    //   if that exists, free that one and updated with the new one.
    
    int replaced = 0;
    list<server_func*>::iterator it;
    for (it=server_db.begin(); it!=server_db.end(); it++) {
        if (!strcmp(server_entry->name, (*it)->name)) {
            int i;
            for (i=0; (*it)->argTypes[i] != 0; i++);
            if (argType_size/4 == ++i) {	// if same size of argument types then check if arguments same
                int j;
                for (j=0; (*it)->argTypes[j] != 0; j++) {
                    // check that the input output bits are the same, and same type
                    int s_pre_arg = (int)(*it)->argTypes[j] & 0xffff0000;
                    int c_pre_arg = (int)server_entry->argTypes[j] & 0xffff0000;
                    
                    if (s_pre_arg != c_pre_arg) break;
                    
                    // check that they are both scalar or both arrays
                    int server_arg_len = (int)(*it)->argTypes[j] & 0xffff;
                    int curr_arg_len = (int)server_entry->argTypes[j] & 0xffff;
                    
                    if (!(((server_arg_len > 0) && (curr_arg_len > 0)) || ((server_arg_len == 0) && (curr_arg_len == 0)))) break;
                    
                    // both inout are same, type is the same, and they or both scalar or both arrays
                    // then check the next argument type
                }
                if (++j == i) {
                    delete [] (*it)->name;
                    delete [] (*it)->argTypes;
                    delete [] (*it);
                    *it = server_entry;
                    replaced = 1;
                    break;
                }
            }
        }
    }
    
    if (replaced == 0) server_db.push_back(server_entry);
    
    return ret_val;				// return either the warning or a 0 for success
}

// the argument contains thread_data structure
// this function will parse msg and send results to client
void *ExecFunc(void *threadarg)
{
    cout << "Executing..." << endl;
    thread_data *execArg = (thread_data *)threadarg;	// get the structure containing message and socket id
    char *unparsedMsg = execArg->message;				// get the raw message
    
    // use this to get parsed message;
    Message *parsedMsg;
    parsedMsg = parseMessage(unparsedMsg, EXECUTE, execArg->messagelen);
    
    // search and find the function on the server database
    skeleton to_run;
    for (list<server_func*>::iterator it=server_db.begin(); it != server_db.end(); it++){
        if (!strcmp(parsedMsg->name, (*it)->name)) {
            int i;
            for (i=0; (*it)->argTypes[i] != 0; i++);
            if (parsedMsg->argTypesSize == ++i) {	// if same size of argument types then check if arguments same
                int j;
                for (j=0; (*it)->argTypes[j] != 0; j++) {
                    // check that the input output bits are the same, and same type
                    int s_pre_arg = (int)(*it)->argTypes[j] & 0xffff0000;
                    int c_pre_arg = (int)parsedMsg->argTypes[j] & 0xffff0000;
                    
                    if (s_pre_arg != c_pre_arg) break;
                    
                    // check that they are both scalar or both arrays
                    int server_arg_len = (int)(*it)->argTypes[j] & 0xffff;
                    int curr_arg_len = (int)parsedMsg->argTypes[j] & 0xffff;
                    
                    if (!(((server_arg_len > 0) && (curr_arg_len > 0)) || ((server_arg_len == 0) && (curr_arg_len == 0)))) break;
                    
                    // both inout are same, type is the same, and they or both scalar or both arrays
                    // then check the next argument type
                }
                // if function is found, time to execute it
                if (++j == i) {
                    to_run = (*it)->func;
                    goto run;
                }
            }
        }
    }
    
run:
    
    int result = (*to_run)(parsedMsg->argTypes, parsedMsg->args);
    
    char *sendMsg;				// msg to send to client
    int sendMsg_len;			// length of message to send in bytes
    if (result < 0) {
        // error send EXECUTE_FAILURE to client
        sendMsg_len = createMessage(&sendMsg, EXECUTE_FAILURE, result, NULL);
    }
    else {
        // success send results back to client
        sendMsg_len = createMessage(&sendMsg, EXECUTE_SUCCESS, result, parsedMsg);
    }
    
    if (send(execArg->sockfd, sendMsg, sendMsg_len, 0) == -1) {
        cerr << "ERROR: could not send results from function (" << parsedMsg->name << ") back to client" << endl;
    }
    
    // delete parsedMsg and sendmsg
    /*delete [] sendMsg;
     delete [] parsedMsg->server_identifier;
     delete [] parsedMsg->argTypes;
     delete [] parsedMsg->name;
     for (int k=0; k < parsedMsg->argTypesSize-1; k++) {
     delete [] parsedMsg->args[k];
     }
     delete [] parsedMsg->args;*/
    
    pthread_exit((void *)threadarg);
}

extern int rpcExecute() {	// for this function remember to use threads!!!
    fd_set master;                  // master fd list
    fd_set temp;                    // temp fd list
    int new_accept;                 // newly accepted fd
    struct sockaddr_storage client; // client address
    socklen_t addrlen;              // size of ai_addr
    int result;						// checking valid returns on functions
    
    int listener = server_info->sockfd;
    char clientIP[INET_ADDRSTRLEN];
    int maxFdNum = listener;
    int clientNum = 0;
    
    // thread information
    pthread_attr_t threadInfo;
    pthread_attr_init(&threadInfo);
    pthread_attr_setdetachstate(&threadInfo, PTHREAD_CREATE_JOINABLE);
    void *status;
    
    // received message information
    int nbytes;						// track total bytes actually received
    int msglen;						// length of the message received
    int msgtype;					// type of the message received
    char *msgbuf;					// for receiving the message
    
    FD_SET(listener, &master);
    
    // add the binder to the master list, to keep it permanently open
    FD_SET(binder_sock, &master);
    if (binder_sock > maxFdNum) maxFdNum = binder_sock;
    
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
                    if (clientNum < 5) {
                        // handle new connection
                        addrlen = sizeof client;
                        new_accept = accept(listener, (struct sockaddr *) &client, &addrlen);
                        
                        if (new_accept == -1) {
                            cerr << "Server: (" << server_info->address << ") Accept error." << endl;
                        }
                        else {
                            FD_SET(new_accept, &master);
                            if (new_accept > maxFdNum) maxFdNum = new_accept;
                            clientNum++;
                            
                            cout << "Server: (" << server_info->address << ") new connection from " << inet_ntop(client.ss_family, get_in_addr((struct sockaddr*) &client), clientIP, INET6_ADDRSTRLEN) << " on socket " << new_accept << endl;
                        }
                    }
                }
                else {// receive the length of the message
                    
                    if ((nbytes = (int) recv(i, &msglen, sizeof(int), 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            cout << "Server: (" << server_info->address << ") socket " << i << " has closed." << endl;
                        } else {
                            cerr << "Server: (" << server_info->address << ") Recieve error." << endl;
                        }
                        close(i);
                        FD_CLR(i, &master);
                        clientNum--;
                    }
                    else {// got a message
                        
                        if ((nbytes = (int) recv(i, &msgtype, sizeof(int), 0)) <= 0) {
                            // got invalid message or connection closed by client
                            if (nbytes == 0) {
                                // connection closed
                                cout << "Server: (" << server_info->address << ") socket " << i << " has closed." << endl;
                            } else {
                                cerr << "Server: (" << server_info->address << ") Recieve error." << endl;
                            }
                            close(i);
                            FD_CLR(i, &master);
                            clientNum--;
                        }
                        else {
                            if (msgtype == TERMINATE && i==binder_sock) {
                                goto terminated;
                            }
                            
                            msgbuf = new char[msglen]; // there is a +1 in binder.cpp figure that out after
                            
                            if ((nbytes = (int) recv(i, msgbuf, msglen, 0)) <= 0) {
                                // got invalid message or connection closed by client
                                if (nbytes == 0) {
                                    // connection closed
                                    cout << "Server: (" << server_info->address << ") socket " << i << " has closed." << endl;
                                } else {
                                    cerr << "Server: (" << server_info->address << ") Recieve error." << endl;
                                }
                                close(i);
                                FD_CLR(i, &master);
                                clientNum--;
                            }
                            else {
                                if (msgtype != EXECUTE) {
                                    cerr << "Server: (" << server_info->address << ") only receives type EXECUTE messages" << endl;
                                    delete [] msgbuf;
                                    continue;
                                }
                                // if too many threads leads to corruption, can set check on vec_count < N
                                // where N is the max number of concurrent threads running
                                threadList.push_back(new pthread_t);
                                struct thread_data *execData = new thread_data;
                                execData->message = msgbuf;
                                execData->messagelen = msglen;
                                execData->sockfd = i;
                                
                                result = pthread_create(threadList.back(), &threadInfo, ExecFunc, (void *)execData);
                                if (result) {
                                    cerr << "Server: (" << server_info->address << ") ERROR in thread creation" << endl;
                                    return ETHREAD;
                                }
                                
                            }/* send to parse */
                        }/* process msg */
                    }/* else message */
                }/* else receive */
            }/* if FD_ISSET */
        }/* for */
    }/* while */
    
terminated:
    // wait till all threads join
    pthread_attr_destroy(&threadInfo);
    for (list<pthread_t*>::iterator it=threadList.begin(); it!=threadList.end(); it++) {
        pthread_join(**it, &status);
    }
    
    //close all the sockets in use
    // free up unecessary shit, free up dat database
    // when done thread exit
    
    pthread_exit(NULL);
    
}/* rpcExecute */

extern int rpcTerminate() {
    // open connection to binder
    char *host_name;						// binder address
    char *port_num;							// binder port
    struct addrinfo hints, *res, *counter;	// addrinfo for specifying and getting info and a counter
    int client_binder_sock;					// socket for connection with binder
    int result;								// check returns
    
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
        client_binder_sock = socket(counter->ai_family, counter->ai_socktype, counter->ai_protocol);
        if (client_binder_sock == -1) continue;	// has not found a socket descriptor
        
        result = connect(client_binder_sock, counter->ai_addr, counter->ai_addrlen);
        if (result == -1) continue; // cannot connect to socket
        
        break;						// found good socket descriptor
    }
    if (counter == NULL) return ESOCK;
    
    freeaddrinfo(res);
    
    //form the terminate message
    char *msgbuf;
    int msgbufLen;
    
    msgbufLen = createMessage(&msgbuf, TERMINATE, 0, NULL);
    
    if (send(client_binder_sock, msgbuf, msgbufLen, 0) == -1) {
        cerr << "ERROR: could not send terminate message to binder" << endl;
        delete [] msgbuf;
        return ESEND;
    }
    
    delete [] msgbuf;
    return 0;
}