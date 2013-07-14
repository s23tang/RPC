//
//  message.cpp
//  RPC
//
//  Created by Haochen Ding & Shisong Tang on 2013-06-26.
//  Copyright (c) 2013 Haochen Ding & Shisong Tang. All rights reserved.
//

#include "message.h"
#include "rpc.h"
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
            msg->port = new char[PORT_SIZE];
            //memcpy(msg->port, buf, PORT_SIZE);
            strcpy(msg->port, buf);
            
            buf = buf + PORT_SIZE;
            
            // get the function name
            msg->name = new char[strlen(buf) + 1];
            strcpy(msg->name, buf);
            buf = buf + NAME_SIZE;
            
            // get the arguement types
            // get the argTypes number first and allocate the argTypes array
            num_argTypes = (length - SERVER_ID_SIZE - PORT_SIZE - NAME_SIZE) / sizeof(int);
            msg->argTypesSize = num_argTypes;
            msg->argTypes = new int[num_argTypes];
            
            // loop to add argTypes
            for (int i = 0; i < num_argTypes; i++) {
                memcpy(&msg->argTypes[i], buf, ARGTYPE_SIZE);
                buf = buf + ARGTYPE_SIZE;
            }
            
            break;
        case CACHE_REQUEST:
        case LOC_REQUEST:
            // get the function name
            msg->name = new char[strlen(buf) + 1];
            strcpy(msg->name, buf);
            buf = buf + NAME_SIZE;
            
            // get the arguement types
            // get the argTypes number first and allocate the argTypes array
            num_argTypes = (length - NAME_SIZE) / sizeof(int);
            msg->argTypesSize = num_argTypes;
            msg->argTypes = new int[num_argTypes];
            
            // loop to add argTypes
            for (int i = 0; i < num_argTypes; i++) {
                memcpy(&msg->argTypes[i], buf, ARGTYPE_SIZE);
                buf = buf + ARGTYPE_SIZE;
            }

            break;
        case LOC_SUCCESS:
            // get the server_identifier
            msg->server_identifier = new char[strlen(buf) + 1];
            strcpy(msg->server_identifier, buf);
            buf = buf + SERVER_ID_SIZE;
            
            // get the server's port number
            msg->port = new char[PORT_SIZE];
            strcpy(msg->port, buf);
            
            break;
        case EXECUTE:
        case EXECUTE_SUCCESS:
            // get the function name
            msg->name = new char[strlen(buf) + 1];
            strcpy(msg->name, buf);
            buf = buf + NAME_SIZE;
            
            // get the arguement types
            // loop to get the argTypes number first and allocate the argTypes array
            num_argTypes = 0;
            while (true) {
                int temp;
                memcpy(&temp, buf + ARGTYPE_SIZE*num_argTypes, ARGTYPE_SIZE);
                
                if (temp == 0) {
                    num_argTypes++;
                    break;
                }
                
                num_argTypes++;
            }
            
            msg->argTypesSize = num_argTypes;
            
            msg->argTypes = new int[num_argTypes];
            
            // add argTypes
            memcpy(msg->argTypes, buf, ARGTYPE_SIZE*num_argTypes);
            buf = buf + ARGTYPE_SIZE*num_argTypes;

            msg->args = new void*[num_argTypes-1];

            // used to update the total length of the args
            msg->argsLength = 0;
            // loop to add different type of args
            for (int j = 0; j < num_argTypes - 1; j++) {
                int arg_type = (msg->argTypes[j] >> (8*2)) & 0xff;
                int arg_length = msg->argTypes[j] & 0xffff;

                if (arg_length == 0) arg_length = 1;
                
                switch (arg_type) {
                    case ARG_CHAR: {
                        char *curArgs = new char[arg_length];
                        memcpy(curArgs, buf, arg_length);
                        msg->args[j] = (void *)curArgs;
                        buf = buf + arg_length;
                        msg->argsLength = msg->argsLength + arg_length;
                        break;
                    }
                    case ARG_SHORT: {
                        short *curArgs = new short[arg_length];
                        memcpy(curArgs, buf, arg_length * sizeof(short));
                        msg->args[j] = (void *)curArgs;
                        buf = buf + arg_length * sizeof(short);
                        msg->argsLength = msg->argsLength + arg_length*sizeof(short);
                        break;
                    }
                    case ARG_INT: {
                        int *curArgs = new int[arg_length];
                        memcpy(curArgs, buf, arg_length * sizeof(int));
                        msg->args[j] = (void *)curArgs;
                        buf = buf + arg_length * sizeof(int);
                        msg->argsLength = msg->argsLength + arg_length * sizeof(int);
                        break;
                    }
                    case ARG_LONG: {
                        long *curArgs = new long[arg_length];
                        memcpy(curArgs, buf, arg_length * sizeof(long));
                        msg->args[j] = (void *)curArgs;
                        buf = buf + arg_length * sizeof(long);
                        msg->argsLength = msg->argsLength + arg_length * sizeof(long);
                        break;
                    }
                    case ARG_DOUBLE: {
                        double *curArgs = new double[arg_length];
                        memcpy(curArgs, buf, arg_length * sizeof(double));
                        msg->args[j] = (void *)curArgs;
                        buf = buf + arg_length * sizeof(double);
                        msg->argsLength = msg->argsLength + arg_length * sizeof(double);
                        break;
                    }
                    case ARG_FLOAT: {
                        float *curArgs = new float[arg_length];
                        memcpy(curArgs, buf, arg_length * sizeof(float));
                        msg->args[j] = (void *)curArgs;
                        buf = buf + arg_length * sizeof(float);
                        msg->argsLength = msg->argsLength + arg_length * sizeof(float);
                        break;
                    }
                    default:
                        break;
                }
            }
            
            break;
        case TERMINATE:
            break;
        case CACHE_FAILURE:
        case REGISTER_SUCCESS:
        case REGISTER_FAILURE:
        case LOC_FAILURE:
        case EXECUTE_FAILURE:
            // get the warning/error code
            memcpy(&msg->reasonCode, buf, RCODE_SIZE);
            break;
        default:
            break;
    }
    return msg;
}



int createMessage(char **buf, int msgType, int retCode, struct Message *oldMsg) {
    
    int msgLength = 0;
    
    switch (msgType) {
        case REGISTER_SUCCESS: {
            // allocate enough memory for the buffer and update the length of the buffer
            msgLength = LENGTH_SIZE + TYPE_SIZE + sizeof(int);
            *buf = new char[msgLength];
            
            // create the REGISTER_SUCCESS message
            int length = sizeof(int);
            memcpy(*buf, &length, LENGTH_SIZE);
            memcpy((*buf) + LENGTH_SIZE, &msgType, TYPE_SIZE);
            memcpy((*buf) + LENGTH_SIZE + TYPE_SIZE, &retCode, sizeof(int));
            
            break;
        }
        case REGISTER_FAILURE: {
            // allocate enough memory for the buffer and update the length of the buffer
            msgLength = LENGTH_SIZE + TYPE_SIZE + sizeof(int);
            (*buf) = new char[msgLength];
            
            // create the REGISTER_FAILURE message
            int length = sizeof(int);
            memcpy((*buf), &length, LENGTH_SIZE);
            memcpy((*buf) + LENGTH_SIZE, &msgType, TYPE_SIZE);
            memcpy((*buf) + LENGTH_SIZE + TYPE_SIZE, &retCode, sizeof(int));
            
            break;
        }
        case LOC_SUCCESS: {
            // allocate enough memory for the buffer and update the length of the buffer
            msgLength = LENGTH_SIZE + TYPE_SIZE + SERVER_ID_SIZE + PORT_SIZE;
            (*buf) = new char[msgLength];
            
            // create the LOC_SUCCESS message
            int length = SERVER_ID_SIZE + PORT_SIZE;
            memcpy((*buf), &length, LENGTH_SIZE);
            memcpy((*buf) + LENGTH_SIZE, &msgType, TYPE_SIZE);
            strcpy((*buf) + LENGTH_SIZE + TYPE_SIZE, oldMsg->server_identifier);
            strcpy((*buf) + LENGTH_SIZE + TYPE_SIZE + SERVER_ID_SIZE, oldMsg->port);
            
            break;
        }
        case CACHE_FAILURE:
        case LOC_FAILURE: {
            // allocate enough memory for the buffer and update the length of the buffer
            msgLength = LENGTH_SIZE + TYPE_SIZE + sizeof(int);
            (*buf) = new char[msgLength];
            
            // create the LOC_FAILURE message
            int length = sizeof(int);
            memcpy((*buf), &length, LENGTH_SIZE);
            memcpy((*buf) + LENGTH_SIZE, &msgType, TYPE_SIZE);
            memcpy((*buf) + LENGTH_SIZE + TYPE_SIZE, &retCode, sizeof(int));
            
            break;
        }
        case EXECUTE_SUCCESS: {
            // allocate enough memory for the buffer and update the length of the buffer
            msgLength = LENGTH_SIZE + TYPE_SIZE + NAME_SIZE + ARGTYPE_SIZE * oldMsg->argTypesSize + oldMsg->argsLength;
            (*buf) = new char[msgLength];

            // create the EXECUTE_SUCCESS message
            int length = NAME_SIZE + ARGTYPE_SIZE * oldMsg->argTypesSize + oldMsg->argsLength;
            memcpy((*buf), &length, LENGTH_SIZE);

            memcpy((*buf) + LENGTH_SIZE, &msgType, TYPE_SIZE);
            
            (*buf) = (*buf) + LENGTH_SIZE + TYPE_SIZE;

            strcpy((*buf), oldMsg->name);
            (*buf) = (*buf) + NAME_SIZE;

            memcpy((*buf), oldMsg->argTypes, ARGTYPE_SIZE*oldMsg->argTypesSize);
            (*buf) = (*buf) + ARGTYPE_SIZE*oldMsg->argTypesSize;
            
            int bytes_read = 0;                         //track how many bytes gone
            // loop to add different type of args
            for (int j = 0; j < oldMsg->argTypesSize - 1; j++) {
                int arg_type = (oldMsg->argTypes[j] >> (8*2)) & 0xff;
                int arg_length = oldMsg->argTypes[j] & 0xffff;
                
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

                // check if scalar or array
                if (arg_length == 0) {
                    memcpy((*buf), oldMsg->args[j], size_of_type);
                    (*buf) = (*buf) + size_of_type;
                    bytes_read = bytes_read + size_of_type;
                }
                else {
                    memcpy((*buf), oldMsg->args[j], arg_length * size_of_type);
                    (*buf) = (*buf) + arg_length * size_of_type;
                    bytes_read = bytes_read + arg_length * size_of_type;
                }  
            }

            (*buf) = (*buf)- LENGTH_SIZE - TYPE_SIZE - NAME_SIZE - ARGTYPE_SIZE*oldMsg->argTypesSize - bytes_read;
            
            break;
        }
        case EXECUTE_FAILURE: {
            // allocate enough memory for the buffer and update the length of the buffer
            msgLength = LENGTH_SIZE + TYPE_SIZE + sizeof(int);
            (*buf) = new char[msgLength];
            
            // create the EXECUTE_FAILURE message
            int length = sizeof(int);
            memcpy((*buf), &length, LENGTH_SIZE);
            memcpy((*buf) + LENGTH_SIZE, &msgType, TYPE_SIZE);
            memcpy((*buf) + LENGTH_SIZE + TYPE_SIZE, &retCode, sizeof(int));
            
            break;
        }
        case TERMINATE: {
            // allocate enough memory for the buffer and update the length of the buffer
            msgLength = LENGTH_SIZE + TYPE_SIZE;
            (*buf) = new char[msgLength];
            
            // create the TERMINATE message
            int length = sizeof(int);
            memcpy((*buf), &length, LENGTH_SIZE);
            memcpy((*buf) + LENGTH_SIZE, &msgType, TYPE_SIZE);
            
            break;
        }
        default:
            break;
    }
    return msgLength;
}
