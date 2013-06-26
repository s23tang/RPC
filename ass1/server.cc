/*
 *--server.cc
 * 		
 *	implementation for server side
 */

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <ctype.h>
#include "error.h"

using namespace std;

void usage(int errno) 
{
	switch(errno) {
		case 0:
			cerr << "ERROR: failed to getaddrinfo" << endl;
			exit(-1);
		case 1:
			cerr << "ERROR: could not find socket" << endl;
			exit(-1);
		case 2:
			cerr << "ERROR: could not send request" << endl;
		case 3:
			cerr << "ERROR: thread create failed" << endl;
		case 4:
			cerr << "ERROR: could not receive from server" << endl;
		case 5:
			cerr << "ERROR: could not bind to port" << endl;
			exit(-1);
		case 6:
			cerr << "ERROR: failed during listen()" << endl;
			exit(-1);
		case 7:
			cerr << "ERROR: failed during getsockname()" << endl;
			exit(-1);
		case 8:
			cerr << "ERROR: could not get host name" << endl;
			exit(-1);
		case 9:
			cerr << "ERROR: select failed" << endl;
		case 10:
			cerr << "ERROR: failed during accept()" << endl;
			exit(-1);
	}
}

int main()
{	
	/*
	char *host_name;						// server address 
	int port_num;							// server port
	struct addrinfo hints, *res, *counter;	// addrinfo for specifying and getting info and a counter
	int status;								// for checking return values
	int sockfd;								// file descriptor for the socket
	
	
	memset(&hints, 0 , sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	status = getaddrinfo(NULL, "0", &hints, &res);
	if (status != 0) usage(0);
	
	// go along the linked list for a valid entry
	for(counter = res; counter != NULL; counter = counter->ai_next) {
		sockfd = socket(counter->ai_family, counter->ai_socktype, counter->ai_protocol);
		if (sockfd == -1) continue;	// has not found a socket descriptor
		
		status = bind(sockfd, counter->ai_addr, counter->ai_addrlen);
		if (status == -1) continue; // cannot connect to socket
		
		break;						// found good socket descriptor
	}
	if (counter == NULL) usage(1);*/
	
	struct sockaddr_in saddr, taddr;			// for packing server and a placeholder
	struct sockaddr_storage client;			// client address
	int sockfd, tfd;						// socket file descriptor for listening, and placeholder
	int status;								// for checking return values
	char hostname[128];						// for the name of the server machine
	fd_set master_list, fds;				// master list of file descriptors, and another list
	socklen_t addrlen;						// size of ai_addr
	
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(0);				// next available port
	saddr.sin_addr.s_addr = INADDR_ANY;		// set ip for any
	
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) usage(1);
	status = bind(sockfd, (struct sockaddr *)&saddr, sizeof saddr);
	if (status == -1) usage(5);
	
	status = listen(sockfd, 10);
	if (status == -1) usage(6);
	
	socklen_t len = sizeof(taddr);
	status = getsockname(sockfd, (struct sockaddr *)&taddr, &len);
	if (status == -1) usage(7);
	
	status = gethostname(hostname, sizeof hostname);
	if (status == -1) usage(8);
	
	cout << "SERVER_ADDRESS " << hostname << endl; 
	cout << "SERVER_PORT " << ntohs(taddr.sin_port) << endl;
	
	FD_SET(sockfd, &master_list);			// add listening socket
	
	int maxfd = sockfd;
	int num_clients = 0;
	while(1) {
		fds = master_list;
		status = select(maxfd+1, &fds, NULL, NULL, NULL);
		if (status == -1) usage(9);
		
		for (int i=0; i<= maxfd; i++) {
			if (FD_ISSET(i, &fds)) {
				if (i == sockfd) {
					if (num_clients < 5) {
						addrlen = sizeof client;
						tfd = accept(sockfd, (struct sockaddr *)&client, &addrlen);
						if (tfd == -1) usage(10);
						
						// add new client
						FD_SET(tfd, &master_list);
						if (tfd > maxfd) maxfd = tfd;
						
						// increment number of clients connected
						num_clients++;
					}
				}
				else {
					// grab the request message and send back
					int string_length=0;
					int *str_len=&string_length;
					if (recv(i, str_len, 4, 0) <= 0) {
						close(i);
						FD_CLR(i, &master_list);
						num_clients--;
					}
					char *text = new char[string_length];
					if (recv(i, text, string_length, 0) <= 0) {
						close(i);
						FD_CLR(i, &master_list);
						num_clients--;
					}
					else {
						cout << text << endl;
						for(int j=0; j<string_length; j++) {
							if (islower(text[j])) text[j] = toupper(text[j]);
							for(j=j+1; j<string_length; j++) {
									if(isupper(text[j])) text[j] = tolower(text[j]);
									if(isspace(text[j])) break;
							}
						}
						int *str_len;
						*str_len = string_length;
						if (send(i, str_len, 4, 0) <= 0) usage(2);
						if (send(i, text, string_length, 0) <= 0) usage(2);
					}
					delete [] text;
				}
			}
		}
	}
	
	return 0;
}