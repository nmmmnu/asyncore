#ifndef _H_ASYNCORE_H
#define _H_ASYNCORE_H

//#include <stdbool.h>

typedef struct{
	uint32_t max_clients;		// 4
	uint32_t connected_clients;	// 4
	uint16_t port;			// 2

	void *user_data;		// system dependent
} async_server_t;


#ifndef INFTIM
#define INFTIM -1
#endif

#define ASYNC_OPREAD  'r'
#define ASYNC_OPWRITE 'w'
#define ASYNC_OPCONN  'c'

const char *async_system();

async_server_t *async_create_server(uint32_t max_clients, uint16_t port, uint16_t backlog);
int async_poll(async_server_t *server, int timeout);
int async_client_status(async_server_t *server, uint16_t id, char operation);
void async_client_close(async_server_t *server, uint16_t id);

#endif

