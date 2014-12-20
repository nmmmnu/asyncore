#include "asyncore.h"
#include "asyncore_functions.h"

#include <stdlib.h>
#include <unistd.h>		// close, getdtablesize
#include <string.h>		// memset
#include <errno.h>		// errno
#include <stdio.h>


#include <sys/select.h>
#include <arpa/inet.h>		// AF_INET


typedef struct{
	uint32_t max_clients;		// 4
	uint32_t connected_clients;	// 4
	uint16_t port;			// 2

	void *user_data;		// system dependent

	// eo async_server_t

	uint32_t last_client;		// 4

	fd_set readfds;			// system dependent, fixed size
					// usually 1024 bits / 128 bytes
	fd_set writefds;		// system dependent, fixed size
					// usually 1024 bits / 128 bytes

	int clients[];			// dynamic
}async_server_select_t;



const char *async_system(){
	return "select";
}


async_server_t *async_create_server(uint32_t max_clients, uint16_t port, uint16_t backlog){
	// check input data
	if (max_clients == 0)
		return NULL;

	if (max_clients > FD_SETSIZE)
		max_clients = FD_SETSIZE;

	if (max_clients > getdtablesize())
		max_clients = getdtablesize();

	if (backlog < 5)
		backlog = 5;


	// malloc
	async_server_select_t *server = malloc(sizeof(async_server_select_t) + sizeof( int ) * (max_clients + 1));

	if (server == NULL)
		return NULL;


	// bind and listen
	int master_socket = _async_create_socket(port, backlog);

	if (master_socket < 0){
		free(server);
		return NULL;
	}


	// set server up
	server->max_clients = max_clients;
	server->connected_clients = 0;
	server->port = port;

	server->last_client = -1;

	memset(server->clients, 0, sizeof(int) * (max_clients + 1));

	server->clients[0] = master_socket;

	return (async_server_t *) server;
};


int async_poll(async_server_t *server2, int timeout){
	async_server_select_t *server = (async_server_select_t *) server2;

	// clear the socket set
	FD_ZERO(& server->readfds);
	FD_ZERO(& server->writefds);

	// add master socket to set
	FD_SET(server->clients[0], & server->readfds);
	// no need to put it in writefds

	// add child sockets to set
	uint32_t i;
	for (i = 1 ; i <= server->max_clients ; i++){
		// socket descriptor
		int sd = server->clients[i];

		//if valid socket descriptor then add to read list
		if(sd > 0){
			FD_SET(sd, & server->readfds);
			FD_SET(sd, & server->writefds);
		}
	}

	struct timeval time;
	time.tv_sec  = timeout / 1000;

	// version 1 - we did it lazy, but this fails on MacOS
	//time.tv_usec = timeout * 1000

	// version 2
	//time.tv_usec = ( timeout - time.tv_sec * 1000 ) * 1000;

	// version 3, stolen from poll() source code
	time.tv_usec = (timeout % 1000) * 1000;


	struct timeval *timep = timeout < 0 ? NULL : & time;

	// wait for an activity on one of the sockets
	int activity = select(FD_SETSIZE, & server->readfds, & server->writefds, NULL, timep);
	// server->readfds, server->writefds and struct timeval time are destroyed now.

	if ((activity < 0) && (errno != EINTR))
		return -1;

	 // if something happened on the master socket,
	 // then its an incoming connection
	if (FD_ISSET(server->clients[0], & server->readfds)){
		struct sockaddr_in address;
		size_t addrlen = sizeof(address);

		int new_socket = accept(server->clients[0], (struct sockaddr *) & address, (socklen_t *) & addrlen);
		if (new_socket < 0)
			return -1;

		//add new socket to array of sockets
		int inside = 0;
		uint32_t i;
		for (i = 1; i <= server->max_clients; i++)
			if( server->clients[i] == 0 ){
				server->clients[i] = new_socket;

				server->connected_clients++;
				server->last_client = i;

				inside++;

				break;
			}


		if (inside){
			LOG("Connect",
				new_socket,
				inet_ntoa(address.sin_addr),
				ntohs(address.sin_port),
				server->connected_clients
			);
		}else{
			// max_clients are reached, drop the client...
			// sorry :(

			LOG("Drop connect",
				new_socket,
				inet_ntoa(address.sin_addr),
				ntohs(address.sin_port),
				server->connected_clients
			);

			close(new_socket);
		}

		//activity--;
	}


	return activity;
}


int async_client_connect(async_server_t *server2){
	async_server_select_t *server = (async_server_select_t *) server2;

	if (server->last_client < 0)
		return -1;

	uint32_t id = server->last_client;
	int socket =  server->clients[id];
	server->last_client = -1;

	return socket;
}


int async_client_status(async_server_t *server2, uint16_t id, char operation){
	async_server_select_t *server = (async_server_select_t *) server2;

	id++; // clients[0] is the server

	int sd = server->clients[id];

	if (sd < 0)
		return -1;

	switch(operation){
	case ASYNC_OPCONN:
		if (id != server->last_client)
			break;

		server->last_client = -1;
		return sd;

	case ASYNC_OPREAD:
		return FD_ISSET(sd, & server->readfds) ? sd : -1;

	case ASYNC_OPWRITE:
		return FD_ISSET(sd, & server->writefds) ? sd : -1;
	}

	return -1;
}


void async_client_close(async_server_t *server2, uint16_t id){
	async_server_select_t *server = (async_server_select_t *) server2;

	id++; // clients[0] is the server

	int sd = server->clients[id];

	if (sd == 0)
		return;

	server->connected_clients--;


	{
		// info part
		struct sockaddr_in address;
		size_t addrlen = sizeof(address);

		getpeername(sd, (struct sockaddr *) & address , (socklen_t *) & addrlen);

		LOG("Disconnect",
			sd,
			inet_ntoa(address.sin_addr),
			ntohs(address.sin_port),
			server->connected_clients
		);
	}


	// Close the socket and mark as 0 in list for reuse
	close(sd);
	server->clients[id] = 0;
}

