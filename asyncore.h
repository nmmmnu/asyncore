#ifndef _H_ASYNCORE_H
#define _H_ASYNCORE_H


typedef struct{
	uint32_t max_clients;		// 4
	uint32_t connected_clients;	// 4
	uint16_t port;			// 2
} async_server;


#ifndef INFTIM
#define INFTIM -1
#endif

#define ASYNCOPREAD  'r'
#define ASYNCOPWRITE 'w'

const char *async_system();

async_server *async_create_server(uint32_t max_clients, uint16_t port, uint16_t backlog);
int async_poll(async_server *server, int timeout);
int async_client_connect(async_server *server);
int async_client_socket(async_server *server, uint16_t id, char operation);
void async_client_close(async_server *server, uint16_t id);

#endif

