#include <stdlib.h>
#include <unistd.h>		// close, getdtablesize
#include <string.h>		// memset
#include <errno.h>		// errno

#include <sys/select.h>

#include <sys/socket.h>		// socket
#include <arpa/inet.h>		// AF_INET

#include "asyncore.h"
#include "asyncore_static.c"



struct async_server_select{
	uint32_t max_clients;		// 4
	uint32_t connected_clients;	// 4
	uint16_t port;			// 2

	int16_t last_client;		// 2

	fd_set readfds;			// system dependent, fixed size

	int master_socket;		// system dependent
	int client_socket[];		// dynamic
};



const char *async_system(){
	return "select";
}


struct async_server *async_create_server(uint32_t max_clients, uint16_t port, uint16_t backlog){
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
	struct async_server_select *server = malloc(sizeof(struct async_server_select) + sizeof( int ) * max_clients);

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
	server->last_client = -1;
	server->port = port;
	server->master_socket = master_socket;

	memset(server->client_socket, 0, sizeof(int) * max_clients);


	return (struct async_server *) server;
};


int async_poll(struct async_server *server2, int timeout){
	struct async_server_select *server = (struct async_server_select *) server2;

	// clear the socket set
	FD_ZERO(& server->readfds);

	// add master socket to set
	FD_SET(server->master_socket, & server->readfds);

	// add child sockets to set
	uint32_t i;
	for (i = 0 ; i < server->max_clients ; i++){
		// socket descriptor
		int sd = server->client_socket[i];

		//if valid socket descriptor then add to read list
		if(sd > 0)
			FD_SET(sd , & server->readfds);
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
	int activity = select(FD_SETSIZE, & server->readfds , NULL , NULL , timep);
	// server->readfds and struct timeval time are destroyed now.

	if ((activity < 0) && (errno != EINTR))
		return -1;

	 // if something happened on the master socket,
	 // then its an incoming connection
	if (FD_ISSET(server->master_socket, & server->readfds)){
		struct sockaddr_in address;
		size_t addrlen = sizeof(address);

		int new_socket = accept(server->master_socket, (struct sockaddr *) & address, (socklen_t *) & addrlen);
		if (new_socket < 0)
			return -1;

		//add new socket to array of sockets
		int inside = 0;
		uint32_t i;
		for (i = 0; i < server->max_clients; i++)
			if( server->client_socket[i] == 0 ){
				server->client_socket[i] = new_socket;

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

		activity--;
	}


	return activity;
}


int async_client_socket_new(struct async_server *server2){
	struct async_server_select *server = (struct async_server_select *) server2;

	if (server->last_client < 0)
		return -1;

	uint32_t id = server->last_client;
	int socket =  server->client_socket[id];
	server->last_client = -1;

	return socket;
}


int async_client_socket(struct async_server *server2, uint16_t id){
	struct async_server_select *server = (struct async_server_select *) server2;

	int sd = server->client_socket[id];

	if (FD_ISSET(sd, & server->readfds))
		return sd;

	return -1;
}


void async_client_close(struct async_server *server2, uint16_t id){
	struct async_server_select *server = (struct async_server_select *) server2;

	int sd = server->client_socket[id];

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
	server->client_socket[id] = 0;
}

