/**
 * File: socket.h
 * Author: Ethan Gordon
 * A read-write UDP interface that associates QIDs with addresses for
 * automatic responses.
 **/

#ifndef SOCKET_H
#define SOCKET_H

#include <stdint.h>

/* Socket struct, holds QID-Address table and UDP information */
typedef struct socket *Socket_T;

Socket_T Socket_init(void);

int Socket_read(Socket_T socket, void* buf, size_t len);

int Socket_respond(Socket_T socket, void* buf, size_t len);

int Socket_write(Socket_T socket, char* ip, uint16_t port, void* buf, size_t len);

void Socket_free(Socket_T socket);

#endif
