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

/**
 * int Cache_dump(char*)
 * @param cacheFile: Serialize and dump the in-memory cache to file (2GB max)
 * @return number of cache entries written on success, negative on failure
 **/
int Cache_dump(const char* cacheFile);

/**
 * int Cache_load(char*)
 * @param cacheFile: De-serialize and write file contents to in-memory cache.
 * Note: cacheFile must have been written by Cache_dump()!
 * @return number of cache entries read on success, or negative on failure
 **/
int Cache_load(const char* cacheFile);

/**
 * int Cache_addUpdate(char[32], uint16_t, void*, size_t)
 * Note: A defensive copy is made of the entry buffer.
 * @param hash, protocol: used to identify the cache entry
 * @param record: Entry data buffer
 * @param recordLen: Size of entry data buffer
 * @return 0 on success, negative on failure
 **/
int Cache_addUpdate(char hash[SHA256_SIZE], uint16_t protocol, void* record, size_t recordLen);

/**
 * void* Cache_get(char[32], uint16_t, size_t*)
 * Note: The returned buffer is a freshly allocated copy and must be freed by the caller.
 * @param hash, protocol: used to identify the cache entry
 * @param recordLen: value-parameter, is filled with the size of the return value if not NULL
 * @return pointer to the cache entry buffer, DO NOT FREE!
 **/
const void* Cache_get(char hash[SHA256_SIZE], uint16_t protocol, size_t* recordLen);

/**
 * void Cache_destroy(void)
 * De-allocates all resources associated with the in-memory cache.
 **/
void Cache_destroy(void);

#endif
