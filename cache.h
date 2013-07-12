/*
 * cache.h
 *
 * This file defines the clients' cache database
 *
 */


#ifndef _cache_h
#define _cache_h

#include <vector>
#include <pair>
#include "rpc.h"

using namespace std;

struct cache_entry {
    char                        *name;
    int                         *argTypes;
    int                         argTypeSize;
    vector<pair<char*, char*>>  server_list;
};

vector<cache_entry*> cache;

#endif
