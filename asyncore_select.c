#include "asyncore_method.h"

#include <stdlib.h>		// malloc
#include <string.h>		// memset
#include <errno.h>		// errno
#include <unistd.h>		// close
#include <stdio.h>

#include <sys/select.h>
#include <arpa/inet.h>		// AF_INET


typedef struct{
	fd_set readfds;			// system dependent, fixed size
					// usually 1024 bits / 128 bytes
	fd_set writefds;		// system dependent, fixed size
					// usually 1024 bits / 128 bytes

	int client_sockets[];		// dynamic
}select_status_data_t;


const char *_async_system(){
	return "select";
}

async_server_t *_async_create_server(async_server_t *server){
	if (server->max_clients > FD_SETSIZE)
		server->max_clients = FD_SETSIZE;

	select_status_data_t *status_data = malloc(sizeof(select_status_data_t) + sizeof( int ) * (server->max_clients + 1));

	if (status_data == NULL)
		return NULL;

	memset(status_data->client_sockets, 0, sizeof(int) * (server->max_clients + 1));

	status_data->client_sockets[0] = server->master_socket;

	server->status_data = status_data;

	return server;
};

int _async_poll(async_server_t *server, int timeout){
	select_status_data_t *status_data = server->status_data;

	// clear the socket set
	FD_ZERO(& status_data->readfds);
	FD_ZERO(& status_data->writefds);

	// add master socket to set
	FD_SET(status_data->client_sockets[0], & status_data->readfds);
	// no need to put it in writefds

	// add child sockets to set
	uint32_t i;
	for (i = 1 ; i <= server->max_clients ; i++){
		// socket descriptor
		int sd = status_data->client_sockets[i];

		//if valid socket descriptor then add to read list
		if(sd > 0){
			FD_SET(sd, & status_data->readfds);
			FD_SET(sd, & status_data->writefds);
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
	int activity = select(FD_SETSIZE, & status_data->readfds, & status_data->writefds, NULL, timep);
	// server->readfds, server->writefds and struct timeval time are destroyed now.

	if ((activity < 0) && (errno != EINTR))
		return -1;

	 // if something happened on the master socket,
	 // then its an incoming connection
	if (FD_ISSET(status_data->client_sockets[0], & status_data->readfds)){
		struct sockaddr_in address;
		size_t addrlen = sizeof(address);

		int new_socket = accept(status_data->client_sockets[0], (struct sockaddr *) & address, (socklen_t *) & addrlen);
		if (new_socket < 0)
			return -1;

		//add new socket to array of sockets
		int inside = 0;
		uint32_t i;
		for (i = 1; i <= server->max_clients; i++)
			if( status_data->client_sockets[i] == 0 ){
				status_data->client_sockets[i] = new_socket;

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

/*
int ____async_client_connect(async_server_t *server){
	select_status_data_t *status_data = server->status_data;

	if (server->last_client < 0)
		return -1;

	uint32_t id = server->last_client;
	int socket =  status_data->client_sockets[id];
	server->last_client = -1;

	return socket;
}
*/

int _async_client_status(async_server_t *server, uint16_t id, char operation){
	select_status_data_t *status_data = server->status_data;

	id++; // clients[0] is the server

	int sd = status_data->client_sockets[id];

	if (sd < 0)
		return -1;

	switch(operation){
	case ASYNC_OPCONN:
		if (id != server->last_client)
			break;

		server->last_client = -1;
		return sd;

	case ASYNC_OPREAD:
		return FD_ISSET(sd, & status_data->readfds) ? sd : -1;

	case ASYNC_OPWRITE:
		return FD_ISSET(sd, & status_data->writefds) ? sd : -1;
	}

	return -1;
}

void _async_client_close(async_server_t *server, uint16_t id){
	select_status_data_t *status_data = server->status_data;

	id++; // clients[0] is the server

	int sd = status_data->client_sockets[id];

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
	status_data->client_sockets[id] = 0;
}

