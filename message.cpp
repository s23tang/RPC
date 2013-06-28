//
//  message.cpp
//  RPC
//
//  Created by Haochen Ding on 2013-06-28.
//  Copyright (c) 2013 Haochen Ding. All rights reserved.
//

#include "message.h"

struct Message *parseMessage(char *buf, int msgType) {
    switch (msgType) {
        case REGISTER:
            
            
            
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
