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

using namespace std;


int main(int argc, const char *argv[]) {
    
    struct BinderInfo *binderInfo = new BinderInfo();
    
    struct sockaddr_storage client; // client address
    int new_accept;                 // newly accepted fd
    int result;                     // return value
    socklen_t addrlen;              // size of ai_addr
    
    fd_set master;                  // master fd list
    fd_set temp;                    // temp fd list
    
    // initialize the binder and set the listener
    binderInit(binderInfo);
    int listener = binderInfo->sockfd;
    
    // print the binder infomation
    cout << "BINDER_ADDRESS " << binderInfo->address << endl;
    cout << "BINDER_PORT    " << binderInfo->port << endl;
    
    FD_SET(listener, &master);
    
    char clientIP[INET_ADDRSTRLEN];
    int maxFdNum = listener;
    int clientNum = 0;
    int nbytes;       // nbytes recived by message
    int intBuf;       // buf to recieve message length
    char *strBuf;     // buf to recieve message string
    
    while (true) {
        temp = master;
        if (select(maxFdNum + 1, &temp, NULL, NULL, NULL) == -1) {
            cerr << "Server: Select error." << endl;
            exit(4);
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
                    } else {
                        
                        FD_SET(new_accept, &master);
                        if (new_accept > maxFdNum) {
                            maxFdNum = new_accept;
                        }
                        clientNum++;
                        
                        cout << "Server: new connection from " << inet_ntop(client.ss_family, get_in_addr((struct sockaddr*) &client), clientIP, INET6_ADDRSTRLEN) << " on socket " << new_accept << endl;
                    }
                } else {
                    // handle data from a client
                    //                    memset(&buf[0], 0, sizeof(buf));
                    
                    // recieve the length of the message
                    if ((nbytes = (int) recv(i, &intBuf, sizeof(int), 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            cout << "Server: socket " << i << " hung up." << endl;
                        } else {
                            cerr << "Server: Recieve error." << endl;
                        }
                        close(i);
                        FD_CLR(i, &master);
                        clientNum--;
                    } else {
                        // recieve the type of the message
                        
                        if ((nbytes = (int) recv(i, &intBuf, sizeof(int), 0)) <= 0) {
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
                            strBuf = new char[intBuf + 1];
                            
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
                            
                            // we got some data from client
                            for (int j = 0; j <=maxFdNum ; j++) {
                                // send to everyone
                                if (FD_ISSET(j, &master)) {
                                    // except the listener and ourselves
                                    if (j == i) {
                                        
                                        if (send(j , modifed_str, nbytes, 0) == -1) {
                                            cerr << "Server: Send error." << endl;
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
    }
    }