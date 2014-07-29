/**
 * File: recursor.h
 * Author: Ethan Gordon
 * A read-write UDP interface that broadcasts a datagram, then launches a
 * thread to wait on responses for a set period of time.
 **/

#ifndef RECURSOR_H
#define RECURSOR_H

/* Socket struct, holds QID-Address table and UDP information */
typedef struct recursor *Recursor_T;

/**
 * Recursor_T Recursor_init(void* size_t, int, int)
 * @param data: to be broadcasted to all peers
 * @param len: Length of @param data
 * @param peers: Maximum number of peers to broadcast to
 * @param timeout: Amount of wall time before we stop listening for responses
 * @return New Recursor, or NULL if error or no peers to broadcast to.
 **/
Recursor_T Recursor_init(void* data, size_t len, int peers, int timeout);

/**
 * void* Recursor_poll(Recursor_T, size_t*)
 * @param recursor: from which to receive data
 * @param retLen (value): Writes with length of returned buffer.
 * @return Data received from network, or NULL on timeout / all responses received
 **/
const void* Recursor_poll(Recursor_T recursor, size_t* retLen);

/**
 * void Recursor_Timeout(Recursor_T)
 * Automatically forces timeout and kills the poll thread.
 * All un-polled responses are discarded, and the next call of Recursor_poll() will return NULL
 * @param recursor: on which to set timeout
 * @return None
 **/
void Recursor_Timeout(Recursor_T recursor);

/**
 * void Recursor_free(Recursor_T)
 * @param recursor: de-allocate all resources for this recursor
 * @return None
 **/
void Recursor_free(Recursor_T recursor);

#endif
