/*
 * server_func.h
 *
 * This file defines all of my server related functions
 * and for the local database of skeletons
 */

 #include <list>
 #include <pthread.h>
 #include "binder.h"
 #include "rpc.h"
 
struct Info *server_info;		// info for server with port and sockets for listening
int binder_sock;				// socket for connection with binder

struct server_func {
	char 		*name;
	int 		*argTypes;
	skeleton 	func;
};

std::list<server_func*> server_db;		// local server database

struct thread_data {
	char *message;						// message containing what to be executed
	int messagelen;						// length of the message
	int sockfd;							// the socket of the client that wants to execute the function
};

void *ExecFunc(void *threadarg);

std::list<pthread_t *> threadList;