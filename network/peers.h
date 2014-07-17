/**
 * File: peers.h
 * Author: Ethan Gordon
 * A local cache of MARP peers. (Abstract Object)
 **/

#ifndef PEERS_H
#define PEERS_H

/** Peer Object **/
struct peer {
  /* Remote IP Address */
  char* ip;
  /* Remote Port */
  uint16_t port;
};

typedef *struct peer Peer_T;

int Peers_init(char* peerFile);

int Peers_dump(char* peerFile);

Peer_T Peers_random();

int Peers_add(char* ip, uint16_t port);

void Peers_destroy();

#endif
