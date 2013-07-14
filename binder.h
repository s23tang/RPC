//
//  binder.h
//  RPC
//
//  Created by Haochen Ding & Shisong Tang on 2013-06-26.
//  Copyright (c) 2013 Haochen Ding & Shisong Tang. All rights reserved.
//

#ifndef __RPC__binder__
#define __RPC__binder__

#include <list>
#include <vector>
#include <utility>

struct db_struct {
	char *name;
	int  *argTypes;
    int  argTypeSize;
	char *address;
	char *port;
};

struct server_info {
	char *address;
	char *port;
	int sockfd;
};

// a list used as a function database
std::list<db_struct*> binder_db;

// a vector to record connected servers
std::vector<server_info*> servers;

// a vector for caching
std::vector<std::pair<char*, char*> > cache_db;


#endif /* defined(__RPC__binder__) */
