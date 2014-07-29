/**
 * File: peers.h
 * Author: Ethan Gordon
 * A local cache of MARP peers. (Abstract Object)
 **/

#include <stdint.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "peers.h"


/** Peer Object **/
struct peer {
  /* Used for array access */
  int index;
  /* Remote IP Address */
  char* ip;
  /* Remote Port */
  uint16_t port;
  /* Sockaddr Struct for socket.h calls */
  struct sockaddr_in socket_address;
};

struct peerAO {
  size_t size;
  size_t cap;
  Peer_T* peers;
  bool* exists;
};

static struct peerAO *peerList;
extern char* programName;

#define INITIAL_SIZE 4
#define MAX_STR_BUF 23
#define PORT_LEN 5

/**
 * int Peers_init(char*)
 * Initialize the AO list of peers.
 * @param peerFile: Optional file with newline-delimited <host>:<port> combinations.
 * @return number of peers read on success, negative on failure.
 **/
int Peers_init(char* peerFile) {
  int count = 0;
  FILE* peers;
  char* hostport;

  srand(time(NULL));

  peerList = calloc(1, sizeof(struct peerAO));
  if (peerList == NULL) return -1;

  peerList->peers = calloc(INITIAL_SIZE, sizeof(Peer_T));
  if (peerList->peers == NULL) { free(peerList); return -1; }
  peerList->exists = calloc(INITIAL_SIZE, sizeof(bool));
  if (peerList->exists == NULL) { free(peerList->peers); free(peerList); return -1; }

  peerList->size = 0;
  peerList->cap = INITIAL_SIZE;

  if (peerFile != NULL) {
    peers = fopen(peerFile, "r");
    if (peers == NULL) {
      perror(programName);
      return 0;
    }
    
    hostport = calloc(MAX_STR_BUF, sizeof(char));
    if (hostport == NULL) {
      fclose(peers);
      return 0;
    }

    while(!feof(peers)) {
      char *ip, *portStr;
      uint16_t port;
      char* nullCheck = fgets(hostport, MAX_STR_BUF, peers);
      if (nullCheck == NULL) break;
      
      ip = hostport;
      portStr = strrchr(hostport, ':');
      if (portStr == NULL) continue;
      *portStr = '\0';
      portStr++;

      port = (uint16_t)atoi(portStr);
      if (Peers_add(ip, port) == 0) count++;
    }
    free(hostport);
    fclose(peers);
  }

  return count;
}

/**
 * int Peers_dump(char*)
 * Dump all known peers to a file.
 * @param peerFile: Overwritten with newline-delimited <host>:<port> combinations.
 * @return number of peers written on success, negative on failure
 **/
int Peers_dump(char* peerFile) {
  FILE* peers;
  int i, count;
  if (peerFile == NULL) return -1;

  peers = fopen(peerFile, "w+");
  if (peers == NULL) {
    perror(programName);
    return -1;
  }

  for (i = 0; i < peerList->cap; i++) {
    if (peerList->exists[i]) {
      char port[PORT_LEN];
      fputs(peerList->peers[i]->ip, peers);
      fputc(':', peers);
      snprintf(port, PORT_LEN, "%d", peerList->peers[i]->port);
      fputs(port, peers);
      fputc('\n', peers);
      count++;
    }
  }

  return count;
}
  

/**
 * Peer_T Peers_random(void)
 * @return A random peer from known peers.
 **/
Peer_T Peers_random(void) {
  int i;
  do {
    /* Don't care too much about uniformity for this... */
    i = rand() % peerList->cap;
  } while (!peerList->exists[i]);
  return peerList->peers[i];
}

/**
 * struct sockaddr Peers_socket(Peer_T);
 * @return Socket associated with @param peer
 **/
struct sockaddr* Peers_socket(Peer_T peer) {
  assert(peer != NULL);
  return (struct sockaddr*)(&peer->socket_address);
}

/**
 * int Peers_add(char*, uint16_t)
 * Add peer to known peers.
 * @param ip: A valid ipv4 or ipv6 address
 * @param port: Valid port number
 * @return: 0 on success, negative on failure
 **/
int Peers_add(const char* ip, uint16_t port) {
  Peer_T newPeer = calloc(1, sizeof(struct peer));
  if (newPeer == NULL) return -1;

  assert(ip != NULL);
  
  newPeer->ip = calloc(strlen(ip) + 1, sizeof(char));
  if (newPeer->ip == NULL) {
    free(newPeer); return -1;
  }

  strcpy(newPeer->ip, ip);
  newPeer->port = port;

  newPeer->socket_address.sin_family = AF_INET;
  newPeer->socket_address.sin_addr.s_addr=inet_addr(ip);
  newPeer->socket_address.sin_port=htons(port);

  if (peerList->size + 1 >= peerList->cap) {
    int i;
    /* Grow Peer List */
    peerList->cap *= 2;
    peerList->peers = realloc(peerList->peers, peerList->cap * sizeof(Peer_T));
    if (peerList->peers == NULL) { free(newPeer->ip); free(newPeer); peerList->cap /= 2; return -1; }
    peerList->exists = realloc(peerList->exists, peerList->cap * sizeof(bool));
    if (peerList->exists == NULL) { free(newPeer->ip); free(newPeer); peerList->cap /= 2; return -1; }
    for (i = peerList->cap / 2; i < peerList->cap; i++) peerList->exists[i] = false;
  }
  
  /* Add Peer to List */
  newPeer->index = peerList->size;
  peerList->peers[peerList->size] = newPeer;
  peerList->exists[peerList->size] = true;
  peerList->size++;

  return 0;
}

/**
 * int Peers_drop(Peer_T peer)
 * Removes and frees peer from known peers (usually used after failed Ping)
 * @param peer: Must be a peer returned by Peers_random()
 * @return: 0 on success, negative on failure or not found
 **/
int Peers_drop(Peer_T peer) {
  int i = peer->index;
  assert(peer != NULL);

  if (peerList->exists[i]) {
    peerList->exists[i] = false;
    free(peerList->peers[i]->ip);
    free(peerList->peers[i]);
    peerList->peers[i] = NULL;
    return 0;
  }

  return -1;
}

/**
 * void Peers_destroy(void)
 * Cleans up all resources associated with this AO.
 * @return None
 **/
void Peers_destroy(void) {
  int i;
  if (peerList) {
    if (peerList->peers) {
      for (i = 0; i < peerList->cap; i++) {
        if (peerList->peers[i]) {
          if (peerList->peers[i]->ip) free(peerList->peers[i]->ip);
          free(peerList->peers[i]);
        }
      }
      free(peerList->peers);
    }
    if (peerList->exists) free(peerList->exists);
    free(peerList);
    peerList = NULL;
  }
}

