//
//  binder.cpp
//  RPC
//
//  Created by Haochen Ding & Shisong Tang on 2013-06-26.
//  Copyright (c) 2013 Haochen Ding & Shisong Tang. All rights reserved.
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
    int terminatephase;             // flag for going into terminate phase which is checking for whether all servers have closed
    
    // initialize the binder and set the listener
    result = Init(&info);
	if (result < 0) exit(result);
    
    int listener = info->sockfd;
    
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
    terminatephase = 0;
    
    // define the database iterator
    list<struct db_struct*>::iterator it;
    vector<server_info*>::iterator server_it;
    
    //int printOnce =1;
    while (true) {
        
        if (terminatephase == 1 && servers.empty()) break;
        
        temp = master;
        if (select(maxFdNum + 1, &temp, NULL, NULL, NULL) == -1) {
            cerr << "Server: Select error." << endl;
            exit(4);
        }
        
        // run through the existing connections to look for datas to read
        for (int i = 0; i <= maxFdNum; i++) {
            /*if (binder_db.size() == printOnce && printOnce < 11) {
                cout << "here is what is stored" << endl;
                cout << "name " << binder_db.back()->name << endl;
                cout << "argTypes ";
                for (int troll=0; binder_db.back()->argTypes[troll] !=0; troll++){
                    cout << binder_db.back()->argTypes[troll] << " ";
                }
                cout << endl;
                cout << "argTypes size " << binder_db.back()->argTypeSize << endl;
                cout << "address " << binder_db.back()->address << endl;
                cout << "port " << binder_db.back()->port << endl;
                printOnce++;
            }*/
            
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
                        if (nbytes == 0) {
                            cout << "Binder: socket " << i << " has closed." << endl;

                            // the server has closed, then remove all of this servers functions
                            if (!servers.empty()) {
                                for (server_it = servers.begin(); server_it != servers.end(); server_it++) {
                                    if ((*server_it)->sockfd == i) {
                                        // clear all the functions associated with this server from the database
                                        it = binder_db.begin();
                                        while (it != binder_db.end()) {
                                            if (!strcmp((*it)->address, (*server_it)->address) && !strcmp((*it)->port, (*server_it)->port)) {
                                                it = binder_db.erase(it);
                                            }
                                            else it++;
                                        }
                                        // remove the server from the server info vector
                                        server_it = servers.erase(server_it);
                                        break;
                                    }
                                }
                            }
                            
                        }
                        else {
                            // got error or connection closed by client
                            if (nbytes == 0) {
                                // connection closed
                                cout << "Binder: socket " << i << " hung up." << endl;
                            } else {
                                cerr << "Binder: Recieve error." << endl;
                            }
                        }
                        close(i);
                        FD_CLR(i, &master);
                        clientNum--;
                    } else if (terminatephase == 0) {   // will only continue when we are not in the terminate phase
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
                                    result = (int) send((*server_it)->sockfd, sendBuf, sendLength, 0);
                                    if (result < 0) {
                                        cerr << "Binder: Send error, TERMINATE" << endl;
                                    }
                                }
                            }
                            
                            terminatephase = 1;
                            
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
                                        // parse the message we recieved
                                        msg = parseMessage(strBuf, typeBuf, lenBuf);
                                        
                                        // add the new connected server to the servers vector
                                        bool found = false;
                                        if (servers.size() != 0) {
                                            for (server_it = servers.begin(); server_it != servers.end(); server_it++) {
                                                if ((*server_it)->sockfd == i) {
                                                    found = true;
                                                    break;
                                                }
                                            }
                                        }
                                        if (!found) {
                                            struct server_info * temp = new server_info();
                                            temp->address = new char [SERVER_ID_SIZE];
                                            strcpy(temp->address, msg->server_identifier);
                                            temp->port = new char [PORT_SIZE];
                                            strcpy(temp->port, msg->port);
                                            temp->sockfd = i;
                                            servers.push_back(temp);
                                        }
                                        
                                        
                                        bool diff = true;
                                        
                                        if (binder_db.size() != 0){
                                            for (it = binder_db.begin(); it != binder_db.end(); it++) {
                                                if (!strcmp(msg->server_identifier, (*it)->address) && !strcmp(msg->name, (*it)->name) && !strcmp(msg->port, (*it)->port) && (msg->argTypesSize == (*it)->argTypeSize)) {
                                                    int counter = msg->argTypesSize - 1;
                                                    while (counter >= 0) {
                                                        if (msg->argTypes[counter] != (*it)->argTypes[counter]) {
                                                            break;
                                                        }
                                                        //check that input output bits are the same, and same type for stored and current
                                                        int s_pre_arg = (int)(*it)->argTypes[counter] & 0xffff0000;
                                                        int c_pre_arg = (int)msg->argTypes[counter] & 0xffff0000;

                                                        if (s_pre_arg != c_pre_arg) break;

                                                        // check that they are both scalar or both arrays
                                                        int server_arg_len = (int)(*it)->argTypes[counter] & 0xffff;
                                                        int curr_arg_len = (int)msg->argTypes[counter] & 0xffff;

                                                        if (!(((server_arg_len > 0) && (curr_arg_len > 0)) || ((server_arg_len == 0) && (curr_arg_len == 0)))) break;

                                                        // both inout are same, type is the same, and they or both scalar or both arrays
                                                        // then check the next argument type
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
                                            cout << "I should be coming here once though son" << endl;
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
                                                if (!strcmp(msg->name, (*it)->name) && (msg->argTypesSize == (*it)->argTypeSize)) {
                                                    int counter = msg->argTypesSize - 1;
                                                    while (counter >= 0) {
                                                        if (msg->argTypes[counter] != (*it)->argTypes[counter]) {
                                                            break;
                                                        }
                                                        //check that input output bits are the same, and same type for stored and current
                                                        int s_pre_arg = (int)(*it)->argTypes[counter] & 0xffff0000;
                                                        int c_pre_arg = (int)msg->argTypes[counter] & 0xffff0000;

                                                        if (s_pre_arg != c_pre_arg) break;

                                                        // check that they are both scalar or both arrays
                                                        int server_arg_len = (int)(*it)->argTypes[counter] & 0xffff;
                                                        int curr_arg_len = (int)msg->argTypes[counter] & 0xffff;

                                                        if (!(((server_arg_len > 0) && (curr_arg_len > 0)) || ((server_arg_len == 0) && (curr_arg_len == 0)))) break;

                                                        // both inout are same, type is the same, and they or both scalar or both arrays
                                                        // then check the next argument type
                                                        counter--;
                                                    }
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
                                                if (!strcmp((*it)->address, msg->server_identifier) && !strcmp((*it)->port, msg->port) ) {
                                                    // put the functions within the same server to the end of the database
                                                    binder_db.splice(binder_db.end(), binder_db, it);
                                                }
                                            }
                                        } else {
                                            // if we cannot find the same function
                                            // just send the LOC_FAILURE and the reasnoCode
                                            int sendLength = createMessage(&sendBuf, LOC_FAILURE, -11, msg);
                                            result = (int) send(i, sendBuf, sendLength, 0);
                                            if (result < 0) {
                                                cerr << "Binder: Send error, LOC_FAILURE." << endl;
                                            }
                                        }
                                        
                                        break;
                                    }
                                    case CACHE_REQUEST: {
                                        // parse the message we recieved
                                        msg = parseMessage(strBuf, typeBuf, lenBuf);
                                        
                                        // bool value to indicate if we found the same function
                                        bool found = false;
                                        
                                        // loop to scan the database for the same function
                                        if (binder_db.size() != 0){
                                            for (it = binder_db.begin(); it != binder_db.end(); it++) {
                                                if (!strcmp(msg->name, (*it)->name) && (msg->argTypesSize == (*it)->argTypeSize)) {

                                                    int counter = msg->argTypesSize - 1;
                                                    while (counter >= 0) {
                                                        if (msg->argTypes[counter] != (*it)->argTypes[counter]) {
                                                            break;
                                                        }
                                                        //check that input output bits are the same, and same type for stored and current
                                                        int s_pre_arg = (int)(*it)->argTypes[counter] & 0xffff0000;
                                                        int c_pre_arg = (int)msg->argTypes[counter] & 0xffff0000;
                                                        
                                                        if (s_pre_arg != c_pre_arg) break;
                                                        
                                                        // check that they are both scalar or both arrays
                                                        int server_arg_len = (int)(*it)->argTypes[counter] & 0xffff;
                                                        int curr_arg_len = (int)msg->argTypes[counter] & 0xffff;
                                                        
                                                        if (!(((server_arg_len > 0) && (curr_arg_len > 0)) || ((server_arg_len == 0) && (curr_arg_len == 0)))) break;
                                                        
                                                        // both inout are same, type is the same, and they or both scalar or both arrays
                                                        // then check the next argument type
                                                        counter--;
                                                    }
                                                    if (counter == -1) {
                                                        found = true;
                                                        // if found same function, cache them for processing
                                                        cache_db.push_back(make_pair((*it)->address, (*it)->port));
                                                    }
                                                }
                                            }
                                        }
                                        
                                        if (found) {
                                            vector<pair<char*, char*> >::iterator cache_it;
                                            int sendBufLength = LENGTH_SIZE + TYPE_SIZE + cache_db.size() * SERVER_ID_SIZE + cache_db.size() * PORT_SIZE;
                                            int cacheLength = cache_db.size() * SERVER_ID_SIZE + cache_db.size() * PORT_SIZE;
                                            int msgType = CACHE_SUCCESS;
                                            
                                            // create the CACHE_SUCCESS message
                                            sendBuf = new char[sendBufLength];
                                            memcpy(sendBuf, &cacheLength, LENGTH_SIZE);
                                            memcpy(sendBuf + LENGTH_SIZE, &msgType, TYPE_SIZE);

                                            sendBuf = sendBuf + LENGTH_SIZE + TYPE_SIZE;
                                            // loop to add server address to the message with specific function name
                                            for (cache_it = cache_db.begin(); cache_it != cache_db.end(); cache_it++) {

                                                memcpy(sendBuf, cache_it->first, SERVER_ID_SIZE);
                                                sendBuf = sendBuf + SERVER_ID_SIZE;
                                                strcpy(sendBuf, cache_it->second);
                                                sendBuf = sendBuf + PORT_SIZE;
                                            }
                                            sendBuf = sendBuf - sendBufLength;
                                            // send the server list to the client
                                            result = (int) send(i, sendBuf, sendBufLength, 0);

                                            if (result < 0) {
                                                cerr << "Binder: Send error, CACHE_SUCCESS." << endl;
                                            }

                                        } else {
                                            // if we cannot find the same function
                                            // just send the CACHE_FAILURE and the reasnoCode
                                            int sendLength = createMessage(&sendBuf, CACHE_FAILURE, -11, msg);
                                            result = (int) send(i, sendBuf, sendLength, 0);
                                            if (result < 0) {
                                                cerr << "Binder: Send error, CACHE_FAILURE." << endl;
                                            }
                                            
                                        }
                                        
                                        cache_db.clear();
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
