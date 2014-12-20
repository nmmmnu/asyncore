#ifndef _ASYNC_FUNCTIONS_H
#define _ASYNC_FUNCTIONS_H

#include <stdint.h>

#define LOG_FORMAT "%-12s | fd: %8d | ip: %-15s | port: %6d | clients: %8u\n"
#define LOG(op, fd, ip, port, clients) fprintf(stderr, LOG_FORMAT, op, fd, ip, port, clients)

int _async_create_socket(uint16_t port, uint16_t backlog);

#endif
