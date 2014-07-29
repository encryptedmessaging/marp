/**
 * File: recursor.h
 * Author: Ethan Gordon
 * A read-write UDP interface that broadcasts a datagram, then launches a
 * thread to wait on responses for a set period of time.
 **/
#include "peers.h"
#include "recursor.h"

#define _GNU_SOURCE
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/select.h>

/* Socket struct, holds QID-Address table and UDP information */
struct recursor {
  int nfds;
  int* fds;
  size_t numfd;
  fd_set readSet;
  struct timeval timeout;

  void* buf;
  size_t bufLen;
};
  
extern char* programName;
#define MAX_BUF 512

/**
 * Recursor_T Recursor_init(void* size_t, int, int)
 * @param data: to be broadcasted to all peers
 * @param len: Length of @param data
 * @param peers: Maximum number of peers to broadcast to
 * @param timeout: Amount of wall time before we stop listening for responses
 * @return New Recursor, or NULL if error or no peers to broadcast to.
 **/
Recursor_T Recursor_init(void* data, size_t len, int peers, int timeout) {
  int i, error, offset;
  Recursor_T ret;
  Peer_T randPeer;

  /* Create Data */
  ret = calloc(1, sizeof(struct recursor));
  if (ret == NULL) return NULL;

  ret->fds = calloc(peers, sizeof(int));
  if (ret->fds == NULL) { free(ret); return NULL; }

  ret->buf = calloc(MAX_BUF, sizeof(char));
  if (ret->buf == NULL) {
    free(ret->fds); free(ret);
    return NULL;
  }
  ret->bufLen = MAX_BUF;

  ret->timeout.tv_sec = timeout;
  ret->timeout.tv_usec = 0;
  ret->nfds = 0;
  
  FD_ZERO(&ret->readSet);

  /* Send Data to Random Peers, then add sockets to fd set */
  offset = 0;
  for (i = 0; i < peers; i++) {
    randPeer = Peers_random();
    ret->fds[i - offset] = socket(AF_INET,SOCK_DGRAM,0);
    error = sendto(ret->fds[i-offset], data, len, 0, Peers_socket(randPeer), sizeof(struct sockaddr));
    if (error < 0) {
      perror(programName);
      close(ret->fds[i-offset]);
      offset++;
    } else {
      FD_SET(ret->fds[i-offset], &ret->readSet);
      if (ret->fds[i-offset] >= ret->nfds) ret->nfds = ret->fds[i-offset] + 1;
    }
  }

  ret->numfd = peers-offset;
  return ret;
}

/**
 * void* Recursor_poll(Recursor_T, size_t*)
 * @param recursor: from which to receive data
 * @param retLen (value): Writes with length of returned buffer.
 * @return Data received from network, or NULL on timeout / all responses received; Stored in a constant buffer, overwritten on next poll.
 **/
const void* Recursor_poll(Recursor_T recursor, size_t* retLen) {
  int i;
  assert(retLen != NULL);
  assert(recursor != NULL);
  /* All Receives Have Happened! */
  if (recursor->nfds == 0) return NULL;

  /* TODO (Compatability Note): Currently relies on Linux's implementation with shrinking timeout. */
  i = select(recursor->nfds, &recursor->readSet, NULL, NULL, &recursor->timeout);
  if (i <= 0) return NULL; /* Timeout Reached */
  
  for (i = 0; i < recursor->numfd; i++) {
    if (recursor->fds[i] < 0) continue;
    if (FD_ISSET(recursor->fds[i], &recursor->readSet)) {
      recursor->bufLen = recv(recursor->fds[i], recursor->buf, recursor->bufLen, 0);
      recursor->fds[i] = -1;
      break;
    }
  }

  FD_ZERO(&recursor->readSet);

  /* Rebuild fd set */
  recursor->nfds = 0;
  for (i = 0; i < recursor->numfd; i++) {
    if (recursor->fds[i] == -1) continue;
    if (recursor->nfds <= recursor->fds[i])
      recursor->nfds = recursor->fds[i] + 1;
    FD_SET(recursor->fds[i], &recursor->readSet);
  }
 
  *retLen = recursor->bufLen;
  return recursor->buf;
}
/**
 * void Recursor_Timeout(Recursor_T)
 * Automatically forces timeout and kills the poll thread.
 * All un-polled responses are discarded, and the next call of Recursor_poll() will return NULL
 * @param recursor: on which to set timeout
 * @return None
 **/
void Recursor_Timeout(Recursor_T recursor) {
  assert(recursor != NULL);
  recursor->nfds = 0;
}

/**
 * void Recursor_free(Recursor_T)
 * @param recursor: de-allocate all resources for this recursor
 * @return None
 **/
void Recursor_free(Recursor_T recursor) {
  if(recursor) {
    if (recursor->fds) free(recursor->fds);
    if (recursor->buf) free(recursor->buf);
    free(recursor);
  }
}

