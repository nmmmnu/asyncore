#include "asyncore.h"
#include "asyncore_functions.h"

#include <stdlib.h>
#include <unistd.h>		// close, getdtablesize
#include <stdio.h>

#include <poll.h>
#include <arpa/inet.h>		// AF_INET


//typecast server->clients to struct pollfd clients[];
//#define cl(server)	((struct pollfd *)server->clients)


const char *async_system(){
	return "poll";
}


async_server_t *async_create_server(async_server_t *server, uint32_t max_clients, uint16_t port, uint16_t backlog){
	// check input data
	if (max_clients == 0)
		return NULL;

	// check maximum number of files a process can have open
	if (max_clients > getdtablesize())
		max_clients = getdtablesize();

	if (backlog < 5)
		backlog = 5;

	// malloc
	struct pollfd *status_data = malloc(sizeof( struct pollfd ) * (max_clients + 1));

	if (status_data == NULL)
		return NULL;

	// bind and listen
	int master_socket = _async_create_socket(port, backlog);

	if (master_socket < 0){
		free(status_data);
		return NULL;
	}

	// set server up
	server->max_clients = max_clients;
	server->connected_clients = 0;
	server->port = port;

	server->last_client = 0;

	status_data[0].fd = master_socket;
	status_data[0].events = POLLRDNORM;

	uint32_t i;
	for (i = 1; i <= max_clients; i++){
		// -1 indicates available entry
		status_data[i].fd = -1;
	}

	server->status_data = status_data;

	return server;
};


void async_free_server(async_server_t *server){
	free(server->status_data);
}


int async_poll(async_server_t *server, int timeout){
	struct pollfd *status_data = server->status_data;

	// INFTIM = wait indefinitely
	int activity = poll( status_data, server->max_clients + 1, timeout);


	if (activity <= 0)
		return activity;


	// check for incomming connections
	if ( status_data[0].revents & POLLRDNORM) {
		struct sockaddr_in address;
		size_t addrlen = sizeof(address);

		int new_socket = accept( status_data[0].fd, (struct sockaddr *) & address, (socklen_t *) & addrlen);
		if (new_socket < 0)
			return -1;

		// put it into the array
		int inside = 0;
		uint32_t i;
		for (i = 1; i <= server->max_clients; i++)
			if (status_data[i].fd < 0) {
				status_data[i].fd = new_socket;
				status_data[i].events = POLLRDNORM | POLLWRNORM;

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


int async_client_status(async_server_t *server, uint16_t id, char operation){
	struct pollfd *status_data = server->status_data;

	id++; // clients[0] is the server

	struct pollfd *client = & status_data[id];

	if (client->fd < 0)
		return -1;

	switch(operation){
	case ASYNC_OPCONN:
		if (id == server->last_client){
			server->last_client = 0;
			return client->fd;
		}
		break;

	case ASYNC_OPREAD:
		if (client->revents & (POLLRDNORM | POLLERR))
			return client->fd;

		break;
	case ASYNC_OPWRITE:
		if (client->revents & (POLLWRNORM | POLLERR))
			return client->fd;

		break;
	}

	return -1;
}


void async_client_close(async_server_t *server, uint16_t id){
	struct pollfd *status_data = server->status_data;

	id++; // clients[0] is the server

	struct pollfd *client = & status_data[id];

	if (client->fd == -1)
		return;

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


