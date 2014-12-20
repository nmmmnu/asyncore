#ifndef _H_ASYNCORE_METHOD_H
#define _H_ASYNCORE_METHOD_H

#include "asyncore.h"

const char *_async_system();

async_server_t *_async_create_server(async_server_t *server);

int _async_poll(async_server_t *server, int timeout);

int _async_client_status(async_server_t *server, uint16_t id, char operation);

void _async_client_close(async_server_t *server, uint16_t id);

#endif

