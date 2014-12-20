#ifndef _H_ASYNCORE_H
#define _H_ASYNCORE_H

#include <stdint.h>

typedef struct{
	uint32_t max_clients;		// 4
	uint32_t connected_clients;	// 4
	uint16_t port;			// 2

	uint32_t last_client;		// 4
	int master_socket;		// system dependent
	void *status_data;		// system dependent
	void *client_data;		// system dependent
	void *user_data;		// system dependent
} async_server_t;

#define LOG_FORMAT "%-12s | fd: %8d | ip: %-15s | port: %6d | clients: %8u\n"
#define LOG(op, fd, ip, port, clients) fprintf(stderr, LOG_FORMAT, op, fd, ip, port, clients)

#define ASYNC_OPREAD  'r'
#define ASYNC_OPWRITE 'w'
#define ASYNC_OPCONN  'c'

const char *async_system();

async_server_t *async_create_server(async_server_t *server, uint32_t max_clients, uint16_t port, uint16_t backlog);
void async_free_server(async_server_t *server);

int async_poll(async_server_t *server, int timeout);
int async_client_status(async_server_t *server, uint16_t id, char operation);
void async_client_close(async_server_t *server, uint16_t id);

void async_client_reset_timeout(async_server_t *server, uint16_t id);
int async_client_get_timeout(async_server_t *server, uint16_t id);

#endif

