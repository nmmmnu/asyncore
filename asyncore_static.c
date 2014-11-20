#include <stdio.h>

#define LOG_FORMAT "%-12s | fd: %8d | ip: %-15s | port: %6d | clients: %8u\n"
#define LOG(op, fd, ip, port, clients) fprintf(stderr, LOG_FORMAT, op, fd, ip, port, clients)

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


