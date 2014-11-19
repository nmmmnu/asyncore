#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>		// close

#include <sys/socket.h>		// socket
#include <arpa/inet.h>		// AF_INET

#include "asyncore.h"

#ifndef INFTIM
#define INFTIM -1
#endif

#define LOG_FORMAT "%-25s , fd = %8d, ip = %-15s, port = %6d, clients = %8u\n"
#define log(op, fd, ip, port, clients) printf(LOG_FORMAT, op, fd, ip, port, clients)





static int _async_create_socket(uint16_t port, uint16_t backlog){
	int master_socket = socket(AF_INET , SOCK_STREAM , 0);

	if(! master_socket )
		return -1;

	const int opt = 1;
	if ( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, & opt, sizeof(opt)) < 0 )
		return -2;

	struct sockaddr_in address;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if ( bind(master_socket, (struct sockaddr *) & address, sizeof(address)) < 0 )
		return -3;

	if (listen(master_socket, backlog) < 0)
		return -4;

	return master_socket;
}


struct async_server *async_create_server(uint32_t max_clients, uint16_t port, uint16_t backlog){
	// check input data
	if (max_clients == 0)
		return NULL;

	if (backlog < 5)
		backlog = 5;


	// malloc
	struct async_server *server = malloc(sizeof(struct async_server) + sizeof( struct pollfd ) * max_clients);

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
	server->last_client = 0;
	server->port = port;
	server->clients[0].fd = master_socket;
	server->clients[0].events = POLLRDNORM;

	uint32_t i;
	for (i = 1; i <= max_clients; i++){
		// -1 indicates available entry
		server->clients[i].fd = -1;
	}


	return server;
};


int async_poll(struct async_server *server){
	// INFTIM = wait indefinitely
	int activity = poll(server->clients, server->max_clients + 1, INFTIM);


	if (activity <= 0)
		return activity;


	// check for incomming connections
	if (server->clients[0].revents & POLLRDNORM) {
		struct sockaddr_in address;
		size_t addrlen = sizeof(address);

		int new_socket = accept(server->clients[0].fd, (struct sockaddr *) & address, (socklen_t *) & addrlen);
		if ( new_socket < 0){
		//	printf("Can't accept()");
			return -1;
		}

		// put it into the array
		int inside = 0;
		uint32_t i;
		for (i = 1; i <= server->max_clients; i++)
			if (server->clients[i].fd < 0) {
				server->clients[i].fd = new_socket;
				server->clients[i].events = POLLRDNORM;

				server->connected_clients++;
				server->last_client = i;

				inside++;

				break;
			}


		if (inside){
			log("New connection",
				new_socket,
				inet_ntoa(address.sin_addr),
				ntohs(address.sin_port),
				server->connected_clients
			);
		}else{
			// max_clients are reached, drop the client...
			// sorry :(
			close(new_socket);

			log("New connection dropped",
				new_socket,
				inet_ntoa(address.sin_addr),
				ntohs(address.sin_port),
				server->connected_clients
			);
		}

		activity--;
	}


	return activity;
}


int async_client_socket_new(struct async_server *server){
	if (server->last_client == 0)
		return -1;

	uint32_t id = server->last_client;
	int socket =  server->clients[id].fd;
	server->last_client = 0;

	return socket;
}


int async_client_socket(struct async_server *server, uint16_t id){
	struct pollfd *client = & server->clients[id];

	if (client->fd < 0)
		return -1;

	if (client->revents & (POLLRDNORM | POLLERR))
		return client->fd;

	return -1;
}


void async_client_close(struct async_server *server, uint16_t id){
	struct pollfd *client = & server->clients[id];

	close(client->fd);
	client->fd = -1;

	server->connected_clients--;

	// info part
	struct sockaddr_in address;
	size_t addrlen = sizeof(address);

	getpeername(client->fd, (struct sockaddr *) & address , (socklen_t *) & addrlen);

	log("Disconnection",
		client->fd,
		inet_ntoa(address.sin_addr),
		ntohs(address.sin_port),
		server->connected_clients
	);

	return;
}


