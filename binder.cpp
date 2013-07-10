//
//  binder.cpp
//  RPC
//
//  Created by Haochen Ding on 2013-06-26.
//  Copyright (c) 2013 Haochen Ding. All rights reserved.
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

#include "binder.h"
#include "initialization.h"
#include "message.h"

using namespace std;


int main(int argc, const char *argv[]) {
    
    struct Info *info = new Info();
    
    struct sockaddr_storage client; // client address
    int new_accept;                 // newly accepted fd
    int result;                     // return value
    socklen_t addrlen;              // size of ai_addr
    
    fd_set master;                  // master fd list
    fd_set temp;                    // temp fd list
    
    int terminate;                  // terminate flag
    
    // initialize the binder and set the listener
    result = Init(&info);
	if (result < 0) exit(result);
    
    int listener = info->sockfd;
    
    cout << "server listening sockfd " << listener << endl;
    
    // print the binder infomation
    cout << "BINDER_ADDRESS " << info->address << endl;
    cout << "BINDER_PORT    " << info->port << endl;
    
    FD_SET(listener, &master);
    
    char clientIP[INET_ADDRSTRLEN];
    int maxFdNum = listener;
    int clientNum = 0;
    int nbytes;       // nbytes recived by message
    int lenBuf;       // buf to recieve message length
    int typeBuf;      // buf to recieve message type
    char *strBuf;     // buf to recieve message string
    char *sendBuf;    // buf to send the whole message
    
    terminate = 0;
    
    // define the database iterator
    list<struct db_struct*>::iterator it;
    vector<int>::iterator server_it;
    
    int printOnce =0;
    while (true) {
        
        if (terminate == 1) break;
        
        temp = master;
        if (select(maxFdNum + 1, &temp, NULL, NULL, NULL) == -1) {
            cerr << "Server: Select error." << endl;
            exit(4);
        }
        
        // run through the existing connections to look for datas to read
        for (int i = 0; i <= maxFdNum; i++) {
            if (!binder_db.empty() && printOnce == 0) {
                cout << "here is what is stored" << endl;
                cout << "name " << binder_db.front()->name << endl;
                cout << "argTypes " << binder_db.front()->argTypes[0] << " " << binder_db.front()->argTypes[1] << " " << binder_db.front()->argTypes[2] << " " << binder_db.front()->argTypes[3] << endl;
                cout << "argTypes size " << binder_db.front()->argTypeSize << endl;
                cout << "address " << binder_db.front()->address << endl;
                cout << "port " << binder_db.front()->port << endl;
                printOnce = 1;
            }
            
            if (FD_ISSET(i, &temp)) {

                if (i == listener) {
                    if (clientNum < 5) {
                        // handle new connection
                        addrlen = sizeof client;
                        new_accept = accept(listener, (struct sockaddr *) &client, &addrlen);
                        
                        if (new_accept == -1) {
                            cerr << "Server: Accept error." << endl;
                        } else {
                            
                            FD_SET(new_accept, &master);
                            if (new_accept > maxFdNum) {
                                maxFdNum = new_accept;
                            }
                            clientNum++;
                            
                            cout << "Binder: new connection from " << inet_ntop(client.ss_family, get_in_addr((struct sockaddr*) &client), clientIP, INET6_ADDRSTRLEN) << " on socket " << new_accept << endl;
                        }
                    }
                } else {
                    // handle data from a client
                    //                    memset(&buf[0], 0, sizeof(buf));
                    
                    // recieve the length of the message
                    if ((nbytes = (int) recv(i, &lenBuf, sizeof(int), 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            cout << "Binder: socket " << i << " hung up." << endl;
                        } else {
                            cerr << "Binder: Recieve error." << endl;
                        }
                        close(i);
                        FD_CLR(i, &master);
                        clientNum--;
                    } else {
                        // recieve the type of the message
                        
                        if ((nbytes = (int) recv(i, &typeBuf, sizeof(int), 0)) <= 0) {
                            // got invalid message or connection closed by client
                            if (nbytes == 0) {
                                // get
                                cout << "Binder: socket " << i << " hung up." << endl;
                            } else {
                                cerr << "Binder: Recieve error." << endl;
                            }
                            close(i);
                            FD_CLR(i, &master);
                            clientNum--;
                        } else {
                            
                        if (typeBuf == TERMINATE) {
                            
                            if (servers.size() != 0) {
                                for (server_it = servers.begin(); server_it != servers.end(); server_it++) {
                                    int sendLength = createMessage(&sendBuf, TERMINATE, 0, NULL);
                                    result = (int) send(*server_it, sendBuf, sendLength, 0);
                                    if (result < 0) {
                                        cerr << "Binder: Send error, TERMINATE" << endl;
                                    }
                                    
                                    close(*server_it);
                                }
                            }
                            
                            terminate = 1;
                            
                            break;
                        } else {

                            strBuf = new char[lenBuf + 1];
                            
                            if ((nbytes = (int) recv(i, strBuf, lenBuf, 0)) <= 0) {
                                // got invalid message or connection closed by client
                                if (nbytes == 0) {
                                    // get
                                    cout << "Binder: socket " << i << " hung up." << endl;
                                } else {
                                    cerr << "Binder: Recieve error." << endl;
                                }
                                close(i);
                                FD_CLR(i, &master);
                                clientNum--;
                            } else {
                                Message *msg;
                                
                                switch (typeBuf) {
                                    case REGISTER: {
                                        cout << "checkign the length of message received " << lenBuf << endl;
                                        // parse the message we recieved
                                        msg = parseMessage(strBuf, typeBuf, lenBuf);
                                        
                                        // add the new connected server to the servers vector
                                        bool found = false;
                                        if (servers.size() != 0) {
                                            for (server_it = servers.begin(); server_it != servers.end(); server_it++) {
                                                if (*server_it == i) {
                                                    found = true;
                                                    break;
                                                }
                                            }
                                        }
                                        if (!found) {
                                            servers.push_back(i);
                                        }
                                        
                                        
                                        bool diff = true;
                                        
                                        if (binder_db.size() != 0){
                                            for (it = binder_db.begin(); it != binder_db.end(); it++) {
                                                if ((msg->name == (*it)->name) && (msg->port == (*it)->port) && (msg->argTypesSize == (*it)->argTypeSize)) {
                                                    int counter = msg->argTypesSize - 1;
                                                    while (counter >= 0) {
                                                        if (msg->argTypes[counter] != (*it)->argTypes[counter]) {
                                                            break;
                                                        }
                                                        counter--;
                                                    }
                                                    if (counter == -1) {
                                                        diff = false;
                                                        break;
                                                    }
                                                    
                                                }
                                            }
                                        }
                                        
                                        if (!diff) {
                                            // if same functions from same servers, create success message
                                            // with warning of overriding
                                            
                                            int sendLength = createMessage(&sendBuf, REGISTER_SUCCESS, REGISTER_WARNING, msg);
                                            result = (int) send(i, sendBuf, sendLength, 0);
                                            if (result < 0) {
                                                cerr << "Binder: Send error, REGISTER_SUCCESS." << endl;
                                            }
                                        } else {
                                            // if no same functions from same servers,
                                            // add the new function to the database
                                            struct db_struct* db_entry = new db_struct();
                                            
                                            db_entry->address = msg->server_identifier;
                                            db_entry->port = msg->port;
                                            db_entry->name = msg->name;
                                            db_entry->argTypes = new int[msg->argTypesSize];
                                            for (int count = 0; count < msg->argTypesSize; count++) {
                                                db_entry->argTypes[count] = msg->argTypes[count];
                                            }
                                            db_entry->argTypeSize = msg->argTypesSize;
                                            
                                            binder_db.push_back(db_entry);
                                            
                                            int sendLength = createMessage(&sendBuf, REGISTER_SUCCESS, 0, msg);
                                            result = (int) send(i, sendBuf, sendLength, 0);
                                            if (result < 0) {
                                                cerr << "Binder: Send error, REGISTER_SUCCESS." << endl;
                                            }
                                            
                                        }
                                        break;
                                    }
                                    case LOC_REQUEST: {
                                        
                                        // parse the message we recieved
                                        msg = parseMessage(strBuf, typeBuf, lenBuf);
                                        
                                        // bool value to indicate if we found the same function
                                        bool found = false;
                                        
                                        // loop to scan the database for the same function
                                        if (binder_db.size() != 0){
                                            for (it = binder_db.begin(); it != binder_db.end(); it++) {
                                                if ((msg->name == (*it)->name) && (msg->argTypesSize == (*it)->argTypeSize)) {
                                                    int counter = msg->argTypesSize - 1;
                                                    
                                                    // loop to scan the argtypes to see if the two functions are the same
                                                    while (counter >= 0) {
                                                        if (msg->argTypes[counter] != (*it)->argTypes[counter]) {
                                                            break;
                                                        }
                                                        counter--;
                                                    }
                                                    
                                                    // got the function here
                                                    if (counter == -1) {
                                                        found = true;
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                        
                                        // if we found the same function in the database
                                        // send LOC_SUCCESS message to the client with the server info
                                        // further more, we need to modify the database based on the round-robin algorithm
                                        if (found) {
                                            msg->server_identifier = (*it)->address;
                                            msg->port = (*it)->port;
                                            int sendLength = createMessage(&sendBuf, LOC_SUCCESS, 0, msg);
                                            result = (int) send(i, sendBuf, sendLength, 0);
                                            if (result < 0) {
                                                cerr << "Binder: Send error, LOC_SUCCESS." << endl;
                                            }
                                            
                                            for (it = binder_db.begin(); it != binder_db.end(); it++) {
                                                if ((*it)->address == msg->server_identifier) {
                                                    // put the functions within the same server to the end of the database
                                                    binder_db.splice(binder_db.end(), binder_db, it);
                                                }
                                            }
                                        } else {
                                            // if we cannot find the same function
                                            // just send the LOC_FAILURE and the reasnoCode
                                            int sendLength = createMessage(&sendBuf, LOC_FAILURE, -1, msg);
                                            result = (int) send(i, sendBuf, sendLength, 0);
                                            if (result < 0) {
                                                cerr << "Binder: Send error, LOC_FAILURE." << endl;
                                            }
                                        }
                                        
                                        break;
                                    }
                                    default:
                                        break;
                                }
                            }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
