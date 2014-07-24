/**
 * File: socket.c
 * Author: Ethan Gordon
 * A read-write UDP interface that associates QIDs with addresses for
 * automatic responses.
 **/

#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <assert.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>

extern char* programName;

#include "../uthash.h"
#include "socket.h"

/* Socket struct, holds QID-Address table and UDP information */
typedef struct kvQid {
  int qid;
  struct sockaddr* address;
  UT_hash_handle hh;
} kvQid;

struct socket {
  kvQid* hashmap;
  struct sockaddr_in localaddr;
  int socketfd;
};  

/**
 * Socket_T Socket_init(uint16_t)
 * Creates a new socket bound to a port.
 * @param port: Oh which to bind the socket, if 0, an unbound socket is created.
 * @return New Socket
 **/
Socket_T Socket_init(uint16_t port) {
  Socket_T ret;
  int error;

  ret = malloc(sizeof(struct socket));
  if (ret == NULL) {
    return NULL;
  }

  ret->hashmap = NULL;
  ret->socketfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (ret->socketfd < 0) {
    perror(programName);
    free(ret);
    return NULL;
  }

  bzero(&(ret->localaddr),sizeof(ret->localaddr));
  ret->localaddr.sin_family = AF_INET;
  ret->localaddr.sin_addr.s_addr=htonl(INADDR_ANY);
  ret->localaddr.sin_port=htons(port);
  error = bind(ret->socketfd,(struct sockaddr *)&(ret->localaddr),sizeof(ret->localaddr));
  if (error < 0) {
    perror(programName);
    free(ret);
    return NULL;
  }

  return ret;
} /* End Socket_init() */

/**
 * int Socket_read(Socket_T, void*, size_t)
 * Reads a given datagram from the socket.
 * Sender address is associated with the first 4 bytes written to buffer, the QID.
 * @param socket: from which to read
 * @param buf: Data is read into this buffer.
 * @param len: The maximum size of the datagram to store.
 * @param timeout: How long to wait for data (seconds)
 * @return number of bytes read on success, negative on failure
 **/
int Socket_read(Socket_T socket, void* buf, size_t len, int timeout) {
  struct timeval tv;
  int error;
  socklen_t socklen;
  uint32_t qid;
  kvQid *kvqid, *k;

  assert(socket != NULL);
  assert(buf != NULL);

  /* Allocate all new memory. */
  kvqid = malloc(sizeof(kvQid));
  if (kvqid == NULL) {
    fprintf(stderr, "%s: No Memory", programName);
    return -1;
  }

  kvqid->address = malloc(sizeof(struct sockaddr));
  if (kvqid->address == NULL) {
    fprintf(stderr, "%s: No Memory", programName);
    return -1;
  }


  /* Set Timeout Value */
  tv.tv_sec = timeout;
  tv.tv_usec = 0;

  error = setsockopt(socket->socketfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));
  if (error < 0) {
    perror(programName);
    return error;
  }

  socklen = sizeof(*(kvqid->address));

  error = recvfrom(socket->socketfd, buf, len, 0, kvqid->address, &socklen);
  if (error < sizeof(qid)) {
    if (error < 0) perror(programName);
    free(kvqid->address);
    free(kvqid);
  } else {
    /* Load QID */
    memcpy(&qid, buf, sizeof(qid));
    qid = ntohl(qid);

    /* Check if QID exists already. */
    HASH_FIND_INT(socket->hashmap, &qid, k);  /* qid already in the hash? If so, ignore datagram. */
    if (k != NULL) {
      /* Already exists! Don't add to hash table. */
      free(kvqid->address);
      free(kvqid);
      if (k->address == NULL) {
        /* Was used in Socket_write(), so no error */
        return error;
      } else return -1;
    } else {
      HASH_ADD_INT(socket->hashmap, qid, kvqid);
    }
  }

  return error;

} /* End Socket_read() */

/**
 * int Socket_respond(Socket_T, void*, size_t)
 * Writes a given datagram to socket. The recipient is determined by the
 * first 4 bytes of the buffer, the QID.
 * @param socket: to which data is written
 * @param buf: Data to write, minimum length 4 bytes
 * @param len: Length of the buffer
 * @return number of bytes written on success, -1 on failure
 **/
int Socket_respond(Socket_T socket, const void* buf, size_t len) {
  uint32_t qid;
  socklen_t socklen;
  kvQid *k;
  int error;

  assert(socket != NULL);
  assert(buf != NULL);
  if (len < sizeof(qid)) {
    fprintf(stderr, "%s: Socket_respnd(): buf too short, minimum is length of QID.", programName);
    return -1;
  }

  /* Pull QID */
  memcpy(&qid, buf, sizeof(qid));
  qid = ntohl(qid);
  HASH_FIND_INT(socket->hashmap, &qid, k);
  if (k == NULL) {
    fprintf(stderr, "%s: Socket_respond(): QID not found, can't respond.", programName);
    return -1;
  }

  HASH_DEL(socket->hashmap, k);

  socklen = sizeof(*(k->address));

  error = sendto(socket->socketfd, buf, len, 0, k->address, socklen);
  if (error < 0) {
    perror(programName);
    return -1;
  }

  free(k->address);
  free(k);

  return error; 
} /* End Socket_respond() */

/**
 * int Socket_write(Socket_T, char*, uint16_t, void*, size_t)
 * Write data to arbitrary recipient.
 * If the QId is unique, then the next Socket_read() buffer with the same QID (4 bytes of buf) will not have
 * their QID stored.
 * @param socket: to which data is written
 * @param ip, port: Standard ipv4 or ipv6 address and standard port number
 * @param buf: data to write, minimum length 4 bytes
 * @param len: Length of the buffer
 **/
int Socket_write(Socket_T socket, char* ip, uint16_t port, void* buf, size_t len) {
  uint32_t qid;
  kvQid *k;
  int error;
  struct sockaddr_in nodeaddr;

  assert(socket != NULL);
  assert(buf != NULL);
  assert(ip != NULL);

  if (len < sizeof(qid)) {
    fprintf(stderr, "%s: Socket_respnd(): buf too short, minimum is length of QID.", programName);
    return -1;
  }

  /* Write Address */
  bzero(&nodeaddr,sizeof(nodeaddr));
  nodeaddr.sin_family = AF_INET;
  nodeaddr.sin_addr.s_addr=inet_addr(ip);
  nodeaddr.sin_port=htons(port);

  /* Send Buffer */
  error = sendto(socket->socketfd, buf, len, 0, (struct sockaddr *)&nodeaddr, sizeof(nodeaddr));

  if (error < 0) {
    perror(programName);
    return -1;
  }

  /* Pull QID */
  memcpy(&qid, buf, sizeof(qid));
  qid = ntohl(qid);
  HASH_FIND_INT(socket->hashmap, &qid, k);
  if (k == NULL) {
    k = malloc(sizeof(kvQid));
    if (k == NULL) {
      fprintf(stderr, "%s: No Memory", programName);
      return -1;
    }
    k->address = NULL;
    k->qid = (int)qid;
    HASH_ADD_INT(socket->hashmap, qid, k);
  }

  return error;
} /* End Socket_write() */

/**
 * void Socket_clearQID(Socket_T, uint32_t)
 * @param qid: Removes all associations with this qid.
 * @return 0 on success, -1 if qid does not exist
 **/
int Socket_clearQID(Socket_T socket, uint32_t qid) {
  kvQid* k;

  assert(socket != NULL);

  HASH_FIND_INT(socket->hashmap, &qid, k);  /* qid already in the hash? */
  if (k == NULL) {
    return -1;
  }
  HASH_DEL(socket->hashmap, k);
  if (k->address) free(k->address);
  free(k);
  return EXIT_SUCCESS;
} /* End Socket_clearQID() */

/**
 * void Socket_free(Socket_T)
 * @param socket: Deallocates all resources associated with this socket.
 * @return None
 **/
void Socket_free(Socket_T socket) {
  kvQid *current_kvqid, *tmp;

  if (socket == NULL) {
    return;
  }

  /* Clear Hash Table */
  HASH_ITER(hh, socket->hashmap, current_kvqid, tmp) {
    HASH_DEL(socket->hashmap, current_kvqid);
    free(current_kvqid->address);
    free(current_kvqid);
  }

  close(socket->socketfd);

  /* Clear Socket */
  free(socket);

} /* End Socket_free() */
