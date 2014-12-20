#include "asyncore.h"
#include "asyncore_method.h"

#include <stdlib.h>		// free
#include <string.h>		// memset
#include <unistd.h>		// getdtablesize

#include <sys/socket.h>		// socket
#include <arpa/inet.h>		// AF_INET

#include <sys/time.h>		// gettimeofday



typedef struct{
	uint32_t	time;
}client_data_t;



static int _async_create_socket(uint16_t port, uint16_t backlog);



const char *async_system(){
	return _async_system();
}

async_server_t *async_create_server(async_server_t *server, uint32_t max_clients, uint16_t port, uint16_t backlog){
	memset(server, 0, sizeof(async_server_t));

	// check input data
	if (max_clients == 0)
		return NULL;

	// check maximum number of files a process can have open
	if (max_clients > getdtablesize())
		max_clients = getdtablesize();

	if (backlog < 5)
		backlog = 5;

	// set server up
	server->max_clients = max_clients;
	server->connected_clients = 0;
	server->port = port;

	server->last_client = -1;

	// socket set up, bind and listen
	int master_socket = _async_create_socket(port, backlog);

	if (master_socket < 0)
		return NULL;

	server->master_socket = master_socket;

	// set up client infrastructure
	client_data_t *client_data = malloc(sizeof(client_data_t)  * (server->max_clients) );

	if (client_data == NULL)
		return NULL;

	//memset(client_data, 0, sizeof(client_data_t)  * (server->max_clients));

	server->client_data = client_data;

	if (_async_create_server(server))
		return server;

	free(server->client_data);
	// do clean up
	return 0;
}

void async_free_server(async_server_t *server){
	if (server->status_data)
		free(server->status_data);

	if (server->client_data)
		free(server->client_data);
}



int async_poll(async_server_t *server, int timeout){
	return _async_poll(server, timeout);
}

int async_client_status(async_server_t *server, uint16_t id, char operation){
	return _async_client_status(server, id, operation);
}

void async_client_close(async_server_t *server, uint16_t id){
	/* return */ _async_client_close(server, id);
}



void async_client_reset_timeout(async_server_t *server, uint16_t id){
	struct timeval tv;
	gettimeofday(&tv, NULL);

	client_data_t *client_data = server->client_data;

	client_data[id].time = tv.tv_sec;
}

int async_client_get_timeout(async_server_t *server, uint16_t id){
	struct timeval tv;
	gettimeofday(&tv, NULL);

	client_data_t *client_data = server->client_data;

	return tv.tv_sec - client_data[id].time;
}



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

