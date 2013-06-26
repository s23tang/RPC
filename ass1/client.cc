/*
 *--client.cc
 * 		
 *	implementation for client side
 */

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <list>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
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

list <int> lens;			// list for length
list <char *> texts;		// list for texts
int fin = 1;
int done_send = 1;
int total_sent = 0;
int in_buffer = 0;

void *SendInput(void *sock)
{
	int *sfd = (int*)sock;
	string input;				// input in string
	int status;					// for checking return values
	char *text;					// for casting to c string
	int string_length;			// length of string inputed
	
	while(1) {
		if (!lens.empty() && !texts.empty()) {
			string_length = lens.front();
			int *str_len = &string_length;
			status = send(*sfd, str_len, 4, 0);
			if (status == -1) usage(2);
			status = send(*sfd, texts.front(), string_length, 0);
			if (status == -1) usage(2);
			
			delete [] texts.front();
			lens.pop_front();
			texts.pop_front();
			total_sent++;
			sleep(2);
		}
		if(!fin && total_sent == in_buffer) break;
	}
	
	done_send = 0;
	pthread_exit(NULL);
}

void *ReadInput(void *dummy) 
{
	string input;				// input in string
	char *text;					// for casting to c string
	int string_length;			// length of string inputed
	
	while(1) {
		if(!getline(cin, input)) {
			fin = 0;
			break;
		}
		
		string_length = input.length()+1;
		text = new char[string_length];
		strcpy(text, input.c_str());
		
		lens.push_back(string_length);
		texts.push_back(text);
		in_buffer++;
	}
	
	pthread_exit(NULL);
}

int main() 
{
	char *host_name;						// server address 
	char *port_num;							// server port
	struct addrinfo hints, *res, *counter;	// addrinfo for specifying and getting info and a counter
	int status;								// for checking return values
	int sockfd;								// file descriptor for the socket
	
	// get server address and port from environment variable
	host_name = getenv("SERVER_ADDRESS");
	port_num = getenv("SERVER_PORT");
	
	// specify to use TCP instead of datagram
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	status = getaddrinfo(host_name, port_num, &hints, &res);
	if (status != 0) usage(0);
	
	// go along the linked list for a valid entry
	for(counter = res; counter != NULL; counter = counter->ai_next) {
		sockfd = socket(counter->ai_family, counter->ai_socktype, counter->ai_protocol);
		if (sockfd == -1) continue;	// has not found a socket descriptor
		
		status = connect(sockfd, counter->ai_addr, counter->ai_addrlen);
		if (status == -1) continue; // cannot connect to socket
		
		break;						// found good socket descriptor
	}
	if (counter == NULL) usage(1);
	
	freeaddrinfo(res);
	
	pthread_t threads[2];
	
	// gonna start the real life here
	
	status = pthread_create(&threads[0], NULL, ReadInput, NULL);
	if (status) usage(3);
	
	status = pthread_create(&threads[1], NULL, SendInput, (void *)&sockfd);
	if (status) usage(3);
	
	int try_to_recv=0;
	while(1) {
		if (try_to_recv < total_sent) {
			int string_length=0;
			int *str_len = &string_length;
			status = recv(sockfd, str_len, 4, 0);
			if (status == -1) usage(4);
			
			char *text = new char[string_length];
			
			status = recv(sockfd, text, string_length, 0);
			if (status == -1) usage(4);
			cout << "Server: " << text << endl;
			try_to_recv++;
		}
		if(!done_send) break;
	}
	
	close(sockfd);
	
	pthread_exit(NULL);
}