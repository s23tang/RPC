/*
 * cache.h
 *
 * This file defines the clients' cache database
 *
 */


#ifndef _cache_h
#define _cache_h

#include <utility>
#include <list>
#include "rpc.h"

using namespace std;

struct cache_entry {
    char                        *name;
    int                         *argTypes;
    int                         argTypeSize;
    list<pair<char*, char*> >   server_list;
};

list<cache_entry> cache;

#endif
