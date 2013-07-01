//
//  message.cpp
//  RPC
//
//  Created by Haochen Ding on 2013-06-28.
//  Copyright (c) 2013 Haochen Ding. All rights reserved.
//

#include "message.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

struct Message *parseMessage(char *buf, int msgType, int length) {
    
    int num_argTypes;
    Message *msg = new Message();
    
    switch (msgType) {
        case REGISTER:
            // get the server_identifier
            msg->server_identifier = new char[strlen(buf) + 1];
            strcpy(msg->server_identifier, buf);
            buf = buf + SERVER_ID_SIZE;
            
            // get the server's port number
            memcpy(&msg->port, buf, PORT_SIZE);
            buf = buf + PORT_SIZE;
            
            // get the function name
            msg->name = new char[strlen(buf) + 1];
            strcpy(msg->name, buf);
            buf = buf + NAME_SIZE;
            
            // get the arguement types
            // get the argTypes number first and allocate the argTypes array
            num_argTypes = (length - SERVER_ID_SIZE - PORT_SIZE - NAME_SIZE) / sizeof(int);
            msg->argTypes = new int[num_argTypes - 1];
            
            // loop to add argTypes
            for (int i = 0; i < num_argTypes; i++) {
                memcpy(&msg->argTypes[i], buf, ARGTYPE_SIZE);
                buf = buf + ARGTYPE_SIZE;
            }
            
            break;
        case LOC_REQUEST:
            // get the function name
            msg->name = new char[strlen(buf) + 1];
            strcpy(msg->name, buf);
            buf = buf + NAME_SIZE;
            
            // get the arguement types
            // get the argTypes number first and allocate the argTypes array
            num_argTypes = (length - NAME_SIZE) / sizeof(int);
            msg->argTypes = new int[num_argTypes - 1];
            
            // loop to add argTypes
            for (int i = 0; i < num_argTypes; i++) {
                memcpy(&msg->argTypes[i], buf, ARGTYPE_SIZE);
                buf = buf + ARGTYPE_SIZE;
            }

            break;
        case EXECUTE:
            // get the function name
            msg->name = new char[strlen(buf) + 1];
            strcpy(msg->name, buf);
            buf = buf + NAME_SIZE;
            
            // get the arguement types
            // loop to get the argTypes number first and allocate the argTypes array
            num_argTypes = 0;
            while (true) {
                int temp;
                memcpy(&temp, buf, ARGTYPE_SIZE);
                
                if (temp == 0) {
                    num_argTypes++;
                    break;
                }
                
                num_argTypes++;
            }
            
            msg->argTypes = new int[num_argTypes - 1];
            
            // loop to add argTypes
            for (int i = 0; i < num_argTypes; i++) {
                memcpy(&msg->argTypes[i], buf, ARGTYPE_SIZE);
                buf = buf + ARGTYPE_SIZE;
            }
            
            
            break;
        case TERMINATE:
            break;
        default:
            break;
    }
}
