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

/**
 * Socket_T Socket_init(uint16_t)
 * Creates a new socket bound to a port.
 * @param port: Oh which to bind the socket, if 0, an unbound socket is created.
 * @return New Socket
 **/
Socket_T Socket_init(uint16_t port);

/**
 * int Socket_read(Socket_T, void*, size_t)
 * Reads a given datagram from the socket.
 * Sender address is associated with the first 4 bytes written to buffer, the QID.
 * @param socket: from which to read
 * @param buf: Data is read into this buffer.
 * @param len: The maximum size of the datagram to store.
 * @param timeout: How long to wait for data (seconds)
 * @return number of bytes read on success, -1 on failure
 **/
int Socket_read(Socket_T socket, void* buf, size_t len, int timeout);

/**
 * int Socket_respond(Socket_T, void*, size_t)
 * Writes a given datagram to socket. The recipient is determined by the
 * first 4 bytes of the buffer, the QID.
 * @param socket: to which data is written
 * @param buf: Data to write, minimum length 4 bytes
 * @param len: Length of the buffer
 * @return number of bytes written on success, -1 on failure
 **/
int Socket_respond(Socket_T socket, const void* buf, size_t len);

/**
 * int Socket_write(Socket_T, char*, uint16_t, void*, size_t)
 * Write data to arbitrary recipient.
 * The next Socket_read() buffer with the same QID (4 bytes of buf) will not have
 * their QID stored. Subsequent calls with the same QID will expect that many
 * responses before the QID is stored again.
 * @param socket: to which data is written
 * @param ip, port: Standard ipv4 or ipv6 address and standard port number
 * @param buf: data to write, minimum length 4 bytes
 * @param len: Length of the buffer
 **/
int Socket_write(Socket_T socket, char* ip, uint16_t port, void* buf, size_t len);

/**
 * void Socket_clearQID(Socket_T, uint32_t)
 * @param qid: Removes all associations with this qid.
 * @return 0 on success, -1 if qid does not exist
 **/
int Socket_clearQID(Socket_T socket, uint32_t qid);

/**
 * void Socket_free(Socket_T)
 * @param socket: Deallocates all resources associated with this socket.
 * @return None
 **/
void Socket_free(Socket_T socket);

#endif
