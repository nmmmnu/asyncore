#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>		// close

#include <poll.h>

#include <sys/socket.h>		// socket
#include <arpa/inet.h>		// AF_INET

#include "asyncore.h"
#include "asyncore_static.c"



struct async_server_select{
	uint32_t max_clients;		// 4
	uint32_t connected_clients;	// 4
	uint16_t port;			// 2

	uint16_t last_client;		// 2

	fd_set readfds;			// system dependent, fixed size

	int master_socket;		// system dependent
	int client_socket[];		// dynamic
};



struct async_server *async_create_server(uint32_t max_clients, uint16_t port, uint16_t backlog){
	// check input data
	if (max_clients == 0)
		return NULL;

	if (max_clients > FD_SETSIZE)
		max_clients = FD_SETSIZE;

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
	server->last_client = 0;
	server->port = port;
	server->master_socket = master_socket;

	memset(server->client_socket, 0, sizeof(int) * max_clients);


	return (struct async_server *) server;
};


int async_poll(struct async_server *server2){
	struct async_server_poll *server = (struct async_server_select *) server2;

	// clear the socket set
	FD_ZERO( & server->readfds);

	// add master socket to set
	FD_SET(server->master_socket, & server->readfds);

	// add child sockets to set
	uint32_t i;
	for (i = 0 ; i < max_clients ; i++){
		// socket descriptor
		int sd = server->client_socket[i];

		//if valid socket descriptor then add to read list
		if(sd > 0)
			FD_SET(sd , & server->readfds);
	}

	// wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
	int activity = select(FD_SETSIZE, & server->readfds , NULL , NULL , NULL);

	if ((activity < 0) && (errno != EINTR))
		return -1;

	 // if something happened on the master socket,
	 // then its an incoming connection
	if (FD_ISSET(server->master_socket, & server->readfds)){
		struct sockaddr_in address;
		size_t addrlen = sizeof(address);

		int new_socket = accept(server->master_socket, (struct sockaddr *) & address, (socklen_t*) & addrlen);
		if (new_socket < 0)
			return -1;

		//add new socket to array of sockets
		int inside = 0;
		uint32_t i;
		for (i = 0; i < max_clients; i++)
			if( server->client_socket[i] == 0 ){
				server->client_socket[i] = new_socket;

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


int async_client_socket_new(struct async_server *server2){
	struct async_server_poll *server = (struct async_server_select *) server2;

	if (server->last_client == 0)
		return -1;

	uint32_t id = server->last_client;
	int socket =  server->client_socket[id];
	server->last_client = 0;

	return socket;
}



int main(){
	// max_clients can not be more than ulimit -n
	// but up to FD_SETSIZE, e.g.1024
	const unsigned int max_clients = 2;
	const unsigned int port = 8888;
	const unsigned int backlog = 1000;




	for(;;){
		//clear the socket set
		FD_ZERO(&readfds);

		//add master socket to set
		FD_SET(master_socket, &readfds);

		//add child sockets to set
		int i;
		for (i = 0 ; i < max_clients ; i++){
			// socket descriptor
			int sd = client_socket[i];

			//if valid socket descriptor then add to read list
			if(sd > 0)
				FD_SET(sd , &readfds);
		}

		//wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
		int activity = select(FD_SETSIZE, &readfds , NULL , NULL , NULL);

		if ((activity < 0) && (errno != EINTR)){
			printf("select() error\n");
		}

		 //If something happened on the master socket , then its an incoming connection
		if (FD_ISSET(master_socket, &readfds)){
			int new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen);
			if ( new_socket < 0){
				printf("Can't accept()");
				//exit(1);
			}

			printf("New connection , fd = %d, ip = %s, port = %d \n" ,
					new_socket,
					inet_ntoa(address.sin_addr),
					ntohs(address.sin_port)
			);


			int id = -1;

			//add new socket to array of sockets
			int i;
			for (i = 0; i < max_clients; i++){
				if( client_socket[i] == 0 ){
					id = i;
					break;
				}
			}

			if (id >= 0){
				client_socket[i] = new_socket;
			}else{
				//if we are here, max clients are reached
				//sorry :(
				close(new_socket);
			}
		}


		//else its some IO operation on some client
		for (i = 0; i < max_clients; i++){
			int sd = client_socket[i];

			if (FD_ISSET(sd , &readfds)){
				char *buffer[1024 + 1];
				int len = read(sd , buffer, 1024);
				if (len == 0){
					// Disconnect
					getpeername(sd, (struct sockaddr*)&address , (socklen_t*)&addrlen);
					printf("Disconnection , fd = %d, ip = %s, port = %d \n" ,
							sd,
							inet_ntoa(address.sin_addr),
							ntohs(address.sin_port)
					);

					// Close the socket and mark as 0 in list for reuse
					close(sd);
					client_socket[i] = 0;

					continue;
				}

				send(sd , buffer , len , 0 );
			}
		}
	} // for(;;)


	return 0;
}



