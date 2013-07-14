//
//  message.h
//  RPC
//
//  Created by Haochen Ding & Shisong Tang on 2013-06-26.
//  Copyright (c) 2013 Haochen Ding & Shisong Tang. All rights reserved.
//

#ifndef RPC_message_h
#define RPC_message_h

#define REGISTER            0
#define REGISTER_SUCCESS    1
#define REGISTER_FAILURE    2
#define LOC_REQUEST         3
#define LOC_SUCCESS         4
#define LOC_FAILURE         5
#define EXECUTE             6
#define EXECUTE_SUCCESS     7
#define EXECUTE_FAILURE     8
#define TERMINATE           9
#define CACHE_REQUEST       11
#define CACHE_FAILURE       12
#define CACHE_SUCCESS       13

#define REGISTER_WARNING    10

#define LENGTH_SIZE    		4
#define TYPE_SIZE			4
#define SERVER_ID_SIZE		128
#define NAME_SIZE			100
#define PORT_SIZE			6
#define ARGTYPE_SIZE        4
#define RCODE_SIZE          4



struct Message {
    char *server_identifier;
    int *argTypes;
    int argTypesSize;
    char *port;
    char *name;
    int reasonCode;
    void **args;
    int argsLength;
};

struct Message *parseMessage(char *buf, int msgType, int length);

int createMessage(char **buf, int msgType, int retCode, struct Message *oldMsg);

#endif
