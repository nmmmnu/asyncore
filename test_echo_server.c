#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include <sys/time.h>

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
} client_state;


void welcome_client(int sock, async_server *server, int64_t id);
void serve_client(  int sock, async_server *server, int64_t id);


int main(){
	// max_clients can not be more than ulimit -n
	const uint32_t max_clients	= 2;
	const uint16_t port		= 8888;
	const uint16_t backlog		= 5;
	const uint32_t timeout		= 5 * 1000;
	const uint32_t conn_timeout	= 30;


	async_server *server = async_create_server(max_clients, port, backlog);


	if (server == NULL){
		printf("Can't create server\n");
		exit(1);
	}

	server->client_states = malloc( max_clients * sizeof(client_state) );

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

			sock = async_client_status(server, i, 'c');
			if (sock >= 0){
				welcome_client(sock, server, i);
				continue;
			}

			sock = async_client_status(server, i, 'r');
			if (sock >= 0){
				serve_client(sock, server, i);
				continue;
			}


			struct timeval tv;
			gettimeofday(&tv, NULL);

			if (tv.tv_sec - ((client_state *)server->client_states)[i].time > conn_timeout){
				async_client_close(server, i);
			}

		}
	}

	// Will not come here...

	free(server->client_states);
	free(server);

	return 0;
}


static void reset_timeout(async_server *server, int64_t id){
	struct timeval tv;
	gettimeofday(&tv, NULL);

	//printf("%d\n", tv.tv_sec);

	((client_state *)server->client_states)[id].time = tv.tv_sec;
}


void welcome_client(int sock, async_server *server, int64_t id){
	// we have connected client
	write(sock, WELCOME_MSG, strlen(WELCOME_MSG));

	reset_timeout(server, id);
}


void serve_client(int sock, async_server *server, int64_t id){
	ssize_t len = read(sock, buffer, MAX_BUFFER);

	if (len < 0){
		printf("Can't read\n");
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

	write(sock, buffer, len);

	reset_timeout(server, id);
}

