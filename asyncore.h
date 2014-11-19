#ifndef _H_ASYNCORE_H
#define _H_ASYNCORE_H



struct async_server{
	uint32_t max_clients;		// 4
	uint32_t connected_clients;	// 4
	uint16_t port;			// 2
};


const char *async_system();

struct async_server *async_create_server(uint32_t max_clients, uint16_t port, uint16_t backlog);
int async_poll(struct async_server *server);
int async_client_socket_new(struct async_server *server);
int async_client_socket(struct async_server *server, uint16_t id);
void async_client_close(struct async_server *server, uint16_t id);



#endif

