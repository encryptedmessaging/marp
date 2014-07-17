/**
 * File: cache.h
 * Author: Ethan Gordon
 * An in-memory cache of authoritative MARP records,
 * identified by the hash plus the 2-byte protocol.
 **/

#ifndef CACHE_H
#define CACHE_H

#define SHA256_SIZE 32

int Cache_init();

int Cache_dump(char* cacheFile);

int Cache_load(char* cacheFile);

int Cache_addUpdate(char[SHA256_SIZE] hash, uint16_t protocol, void* record, size_t recordLen);

void* Cache_get(char[SHA256_SIZE] hash, uint16_t protocol, size_t* recordLen);

void Cache_destroy();

#endif
