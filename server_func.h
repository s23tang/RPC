/*
 * server_func.h
 *
 * This file defines all of my server related functions
 * and for the local database of skeletons
 */

 #include <list>
 #include "rpc.h"
 
Info *server_info;				// info for server with port and sockets for listening
int binder_sock;				// socket for connection with binder

struct server_func {
	char 		*name;
	int 		*argTypes;
	skeleton 	func;
};

std::list<server_func*> server_db;		// local server database