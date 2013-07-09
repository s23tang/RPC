//
//  binder.h
//  RPC
//
//  Created by Haochen Ding on 2013-06-26.
//  Copyright (c) 2013 Haochen Ding. All rights reserved.
//

#ifndef __RPC__binder__
#define __RPC__binder__

#include <list>
#include <vector>

struct db_struct {
	char *name;
	int  *argTypes;
    int  argTypeSize;
	char *address;
	char *port;
};

// a list used as a function database
std::list<db_struct*> binder_db;

// a vector to record connected servers
std::vector<int> servers;

#endif /* defined(__RPC__binder__) */
