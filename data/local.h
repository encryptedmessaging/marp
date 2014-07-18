/**
 * File: local.h
 * Author: Ethan Gordon
 * Store and access data from the local .marp configuration file.
 **/

#ifndef LOCAL_H
#define LOCAL_H

#define SHA256_SIZE 32

int Local_init(char* configFile);

char* Local_get(char hash[SHA256_SIZE], uint16_t protocol);

char* Local_getPubkey(void);

char* Local_getPrivkey(void);

int Local_getTTL(char hash[SHA256_SIZE]);

void Local_destroy(void);

#endif
