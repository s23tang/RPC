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

struct Message *parseMessage(char *buf, int msgType) {
    
    Message *msg = new Message();
    
    switch (msgType) {
        case REGISTER:
            
            msg->server_identifier = new char[strlen(buf) + 1];
            strcpy(msg->server_identifier, buf);
            msg = msg + strlen(buf) + 1;
            
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
}
