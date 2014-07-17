/**
 * File: local.h
 * Author: Ethan Gordon
 * Store and access data from the local .marp configuration file.
 **/

#ifndef LOCAL_H
#define LOCAL_H

#define SHA256_SIZE 32
#define ECC_SIZE 32

int Local_init(char* configFile);

char* Local_get(char[32] hash, uint16_t protocol);

char[1 + 2*ECC_SIZE] Local_getPubkey();

char[ECC_SIZE] Local_getPrivkey();

int Local_getTTL(char[32] hash);

void Local_destroy();

#endif
