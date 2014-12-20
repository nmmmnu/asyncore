#include <stdio.h>
#include <string.h>
#include <stdlib.h>	// free
#include <unistd.h>	// write
#include <stdint.h>

#include "asyncore.h"


#define WELCOME_MSG	"Welcome to Test Echo Server\n"	\
			"Type 'bye' to exit...\n"
#define CMD_BUY  "bye"

#define MAX_BUFFER 1024

// is ok to be global since server is single thread
char buffer[MAX_BUFFER + 1];


typedef struct{
	uint32_t	time;
	char		state;

	uint16_t	datasent;
	uint16_t	datasize;
	char		data[MAX_BUFFER + 1];
} client_state;


void client_welcome(	int sock, async_server_t *server, int64_t id);
void client_read(	int sock, async_server_t *server, int64_t id);
void client_write(	int sock, async_server_t *server, int64_t id);


int main(){
	// max_clients can not be more than ulimit -n
	const uint32_t max_clients	= 2;
	const uint16_t port		= 8888;
	const uint16_t backlog		= 5;
	const uint32_t timeout		= 5 * 1000;
	const uint32_t conn_timeout	= 30;

	async_server_t server_static;
	async_server_t *server = async_create_server(& server_static, max_clients, port, backlog);


	if (server == NULL){
		printf("Can't create server\n");
		exit(1);
	}

	server->user_data = malloc( max_clients * sizeof(client_state) );

	if (server == NULL){
		printf("Can't create server (2)\n");
		exit(1);
	}


	printf("%s echo server started\n", async_system() );
	printf("Listening on port %u\n", port);

	for(;;){
		int count = async_poll(server, timeout);

		if (count < 0){
			printf("Can't poll server\n");
			exit(1);
		}

		if (count == 0){
			printf("Do something in a meanwhile...\n");
			continue;
		}

		int64_t i;
		for(i = 0; i < max_clients; i++){
			int sock;
			client_state *cs = (client_state *) server->user_data;

			sock = async_client_status(server, i, 'c');
			if (sock >= 0){
				client_welcome(sock, server, i);
				continue;
			}


			if (cs[i].state == 'r'){
				sock = async_client_status(server, i, 'r');
				if (sock >= 0){
					client_read(sock, server, i);
					continue;
				}
			}


			if (cs[i].state == 'w'){
				sock = async_client_status(server, i, 'w');
				if (sock >= 0){
					client_write(sock, server, i);
					continue;
				}
			}

			if (async_client_get_timeout(server, i) > conn_timeout){
				async_client_close(server, i);
			}
		}
	}

	// Will not come here...

	free(server->user_data);

	return 0;
}


static void reset_write(async_server_t *server, int64_t id, const char *buffer, uint16_t size){
	client_state *cs = (client_state *) server->user_data;

	cs[id].state = 'w';
	cs[id].datasent = 0;
	cs[id].datasize = size;
	strcpy(cs[id].data, buffer);
}


void client_welcome(int sock, async_server_t *server, int64_t id){
	// we have connected client

	async_client_reset_timeout(server, id);

	reset_write(server, id, WELCOME_MSG, strlen(WELCOME_MSG) );
}


void client_read(int sock, async_server_t *server, int64_t id){
	ssize_t len = read(sock, buffer, MAX_BUFFER);

	if (len < 0){
		printf("Error: can not read from fd: %8d\n", sock);
		async_client_close(server, id);
		return;
	}

	if (len == 0){
		async_client_close(server, id);
		return;
	}

	// business logic here

	if (memcmp(buffer, CMD_BUY, strlen(CMD_BUY)) == 0){
		async_client_close(server, id);
		return;
	}

	async_client_reset_timeout(server, id);

	reset_write(server, id, buffer, len);
}


void client_write(int sock, async_server_t *server, int64_t id){
	client_state *cs = (client_state *) server->user_data;

	uint16_t pos     = cs[id].datasent;
	uint16_t datalen = cs[id].datasize - pos;

	ssize_t len = write(sock, & cs[id].data[pos], datalen);

	if (len < 0){
		printf("Error: can not write to fd: %8d\n", sock);
		async_client_close(server, id);
		return;
	}

	if (len == 0){
		async_client_close(server, id);
		return;
	}

	if (len >= datalen){
		// write state finished
		cs[id].state = 'r';
	}
}
