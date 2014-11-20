#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include <sys/socket.h>

#include "asyncore.h"


int main(){
	// max_clients can not be more than ulimit -n
	const uint32_t max_clients	= 2;
	const uint16_t port		= 8888;
	const uint16_t backlog		= 5;
	const uint32_t timeout		= 999;

	struct async_server *server = async_create_server(max_clients, port, backlog);

	const char *bye = "bye";

	const char *welcome =
			"Welcome to Test Echo Server\n"
			"Type 'bye' to exit...\n";

	const uint16_t max_buffer = 1024;
	char buffer[max_buffer + 1];

	if (server == NULL){
		printf("Can't create server\n");
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

		int newsocket = async_client_socket_new(server);
		if (newsocket >= 0){
			send(newsocket, welcome,strlen(welcome), 0);
		}

		if (count == 0){
			printf("Do something in a meanwhile...\n");
			continue;
		}

		int64_t i;
		for(i = 0; i < max_clients; i++){
			int sock = async_client_socket(server, i);
			if (sock < 0)
				continue;

			ssize_t len = read(sock, buffer, max_buffer);

			if (len < 0){
			//	async_client_close(server, i);
				printf("Can't read\n");
				continue;
			}

			if (len == 0){
				async_client_close(server, i);
				continue;
			}

			// business logic here

			if (memcmp(buffer, bye, strlen(bye)) == 0){
				async_client_close(server, i);
				continue;
			}

			send(sock, buffer, len, 0);
		}
	}

	// Will not come here...

	free(server);

	return 0;
}


