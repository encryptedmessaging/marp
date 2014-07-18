/**
 * File: cache.h
 * Author: Ethan Gordon
 * An in-memory cache of authoritative MARP records,
 * identified by the hash plus the 2-byte protocol.
 **/

#ifndef CACHE_H
#define CACHE_H

#include <stdint.h>

#define SHA256_SIZE 32

int Cache_init(void);

int Cache_dump(char* cacheFile);

int Cache_load(char* cacheFile);

int Cache_addUpdate(char hash[SHA256_SIZE], uint16_t protocol, void* record, size_t recordLen);

void* Cache_get(char hash[SHA256_SIZE], uint16_t protocol, size_t* recordLen);

void Cache_destroy(void);

#endif
