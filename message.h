//
//  message.h
//  RPC
//
//  Created by Haochen Ding on 2013-06-26.
//  Copyright (c) 2013 Haochen Ding. All rights reserved.
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

struct message {
    char server_identifier[128];
    int *argTypes;
    short port;
    char *name;
    int reasonCode;
    void **args;
};


#endif
