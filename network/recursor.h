/**
 * File: recursor.h
 * Author: Ethan Gordon
 * A read-write UDP interface that broadcasts a datagram, then launches a
 * thread to wait on responses for a set period of time.
 **/

#ifndef RECURSOR_H
#define RECURSOR_H

/* Socket struct, holds QID-Address table and UDP information */
typedef *struct recursor Recursor_T

Recursor_T Recursor_init(void* data, size_t len, int peers, int timeout);

void* Recursor_poll(Recursor_T recursor, size_t* retLen);

void Recursor_Timeout(Recursor_T recursor);

void Recursor_free(Recursor_T recursor);

#endif
