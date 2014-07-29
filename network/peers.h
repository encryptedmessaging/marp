/**
 * File: peers.h
 * Author: Ethan Gordon
 * A local cache of MARP peers. (Abstract Object)
 **/

#ifndef PEERS_H
#define PEERS_H

#include <stdint.h>
#include <sys/socket.h>

typedef struct peer *Peer_T;

/**
 * int Peers_init(char*)
 * Initialize the AO list of peers.
 * @param peerFile: Optional file with newline-delimited <host>:<port> combinations.
 * @return number of peers read on success, negative on failure.
 **/
int Peers_init(char* peerFile);

/**
 * int Peers_dump(char*)
 * Dump all known peers to a file.
 * @param peerFile: Overwritten with newline-delimited <host>:<port> combinations.
 * @return number of peers written on success, negative on failure
 **/
int Peers_dump(char* peerFile);

/**
 * Peer_T Peers_random(void)
 * @return A random peer from known peers.
 **/
Peer_T Peers_random(void);

/**
 * struct sockaddr Peers_socket(Peer_T);
 * @return Socket associated with @param peer
 **/
struct sockaddr* Peers_socket(Peer_T peer);

/**
 * int Peers_add(char*, uint16_t)
 * Add peer to known peers.
 * @param ip: A valid ipv4 or ipv6 address
 * @param port: Valid port number
 * @return: 0 on success, negative on failure
 **/
int Peers_add(const char* ip, uint16_t port);

/**
 * int Peers_drop(Peer_T peer)
 * Removes peer from known peers (usually used after failed Ping)
 * @param peer: Must be a peer returned by Peers_random()
 * @return: 0 on success, negative on failur or not found
 **/
int Peers_drop(Peer_T peer);

/**
 * void Peers_destroy(void)
 * Cleans up all resources associated with this AO.
 * @return None
 **/
void Peers_destroy(void);

#endif
