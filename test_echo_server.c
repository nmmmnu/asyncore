#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#include <sys/socket.h>

#include "asyncore.h"


int main(){
	const uint64_t max_clients = 2;
	struct async_server *server = async_create_server(max_clients, 8888, 5);

	const char *welcome = "welcome\n";
	
	const uint16_t max_buffer = 1024;
	char buffer[max_buffer + 1];

	if (server == NULL){
		printf("Can not create server\n");
		exit(1);
	}
	
	printf("%s echo server started\n", async_system() );
	
	for(;;){
		int count = async_poll(server, 1000);
				
		if (count < 0){
			printf("Can not poll server\n");
			exit(1);
		}
		
		int newsocket = async_client_socket_new(server);
		if (newsocket >= 0){
			send(newsocket, welcome,strlen(welcome), 0);
		}
		
		if (count == 0){
			printf("Do something is meanwhile...\n");
			continue;
		}
			
		int64_t i;
		for(i = 0; i < max_clients; i++){
			int sock = async_client_socket(server, i);
			if (sock < 0)
				continue;
			
			ssize_t len = read(sock, buffer, max_buffer);
			
			if (len < 0){
				printf("Can not read\n");
				exit(1);
			}
			
			if (len == 0){
				async_client_close(server, i);
				continue;
			}
			
			// business logic here
			
			send(sock, buffer, len, 0);
		}
	}
}


