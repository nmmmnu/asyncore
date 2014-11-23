#include <stdlib.h>
#include <unistd.h>		// close, getdtablesize

#include <poll.h>

#include <sys/socket.h>		// socket
#include <arpa/inet.h>		// AF_INET

#include "asyncore.h"
#include "asyncore_static.c"



typedef struct{
	uint32_t max_clients;		// 4
	uint32_t connected_clients;	// 4
	uint16_t port;			// 2

	// eo async_server

	uint32_t last_client;		// 4

	struct pollfd clients[];	// dynamic
} async_server_poll;


const char *async_system(){
	return "poll";
}


async_server *async_create_server(uint32_t max_clients, uint16_t port, uint16_t backlog){
	// check input data
	if (max_clients == 0)
		return NULL;

	if (max_clients > getdtablesize())
		max_clients = getdtablesize();

	if (backlog < 5)
		backlog = 5;


	// malloc
	async_server_poll *server = malloc(sizeof(async_server) + sizeof( struct pollfd ) * (max_clients + 1));

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

	server->last_client = 0;
	server->clients[0].fd = master_socket;
	server->clients[0].events = POLLRDNORM;

	uint32_t i;
	for (i = 1; i <= max_clients; i++){
		// -1 indicates available entry
		server->clients[i].fd = -1;
	}


	return (async_server *) server;
};


int async_poll(async_server *server2, int timeout){
	async_server_poll *server = (async_server_poll *) server2;

	// INFTIM = wait indefinitely
	int activity = poll(server->clients, server->max_clients + 1, timeout);


	if (activity <= 0)
		return activity;


	// check for incomming connections
	if (server->clients[0].revents & POLLRDNORM) {
		struct sockaddr_in address;
		size_t addrlen = sizeof(address);

		int new_socket = accept(server->clients[0].fd, (struct sockaddr *) & address, (socklen_t *) & addrlen);
		if (new_socket < 0)
			return -1;

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


int async_client_connect(async_server *server2){
	async_server_poll *server = (async_server_poll *) server2;

	if (server->last_client == 0)
		return -1;

	uint32_t id = server->last_client;
	int socket =  server->clients[id].fd;
	server->last_client = 0;

	return socket;
}


int async_client_status(async_server *server2, uint16_t id, char operation){
	async_server_poll *server = (async_server_poll *) server2;

	id++; // clients[0] is the server

	struct pollfd *client = & server->clients[id];

	if (client->fd < 0)
		return -1;

	if (operation == ASYNCOPREAD)
		if (client->revents & (POLLRDNORM | POLLERR))
			return client->fd;

	if (operation == ASYNCOPWRITE)
		if (client->revents & (POLLWRNORM | POLLERR))
			return client->fd;

	return -1;
}


void async_client_close(async_server *server2, uint16_t id){
	async_server_poll *server = (async_server_poll *) server2;

	id++; // clients[0] is the server

	struct pollfd *client = & server->clients[id];

	server->connected_clients--;


	{
		// info part
		struct sockaddr_in address;
		size_t addrlen = sizeof(address);

		getpeername(client->fd, (struct sockaddr *) & address , (socklen_t *) & addrlen);

		LOG("Disconnect",
			client->fd,
			inet_ntoa(address.sin_addr),
			ntohs(address.sin_port),
			server->connected_clients
		);
	}


	close(client->fd);
	client->fd = -1;
}


