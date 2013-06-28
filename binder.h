//
//  binder.h
//  RPC
//
//  Created by Haochen Ding on 2013-06-26.
//  Copyright (c) 2013 Haochen Ding. All rights reserved.
//

#ifndef __RPC__binder__
#define __RPC__binder__

#include <iostream>
#include <list>

struct Info {
    char *address;
    unsigned short port;
    int sockfd;
};

struct db_struct {
	char *name;
	int  *argTypes;
	char *address;
	char *port;
};

extern std::list<db_struct> binder_db;

#endif /* defined(__RPC__binder__) */
