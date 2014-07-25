/**
 * File: cache.h
 * Author: Ethan Gordon
 * An in-memory cache of authoritative MARP records,
 * identified by the hash plus the 2-byte protocol.
 **/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "cache.h"
#include "../uthash.h"
#define SHA256_SIZE 32

extern char* programName;

typedef struct cache {
  char* id;
  void* buf;
  size_t bufLen;
  UT_hash_handle hh;
} cache;

static cache* memCache = NULL;

/**
 * int Cache_dump(char*)
 * @param cacheFile: Serialize and dump the in-memory cache to file (2GB max)
 * @return number of cache entries written on success, negative on failure
 **/
int Cache_dump(const char* cacheFile) {
  cache *current, *tmp;
  int fd, error, count;
  
  /* Open File */
  fd = creat(cacheFile, O_WRONLY);
  if (fd < 0) {
    perror(programName);
    return fd;
  }

  /* Dump All Contents */
  count = 0;
  HASH_ITER(hh, memCache, current, tmp) {
    uint8_t *copy = calloc(SHA256_SIZE + sizeof(uint16_t) + sizeof(size_t) + current->bufLen, sizeof(uint8_t));
    if (copy == NULL) {
      close(fd);
      return -1;
    }
    memcpy(copy, current->id, SHA256_SIZE + sizeof(uint16_t));
    memcpy(&(copy[SHA256_SIZE + sizeof(uint16_t)]), &(current->bufLen), sizeof(size_t));
    memcpy(&(copy[SHA256_SIZE + sizeof(uint16_t) + sizeof(size_t)]), current->buf, current->bufLen);

    error = write(fd, copy, SHA256_SIZE + sizeof(uint16_t) + sizeof(size_t) + current->bufLen);
    free(copy);
    if (error < 0) {
      perror(programName);
      close(fd);
      return error;
    }
    count++;
  }

  return count;
}
/**
 * int Cache_load(char*)
 * @param cacheFile: De-serialize and write file contents to in-memory cache.
 * Note: cacheFile must have been written by Cache_dump()!
 * @return number of cache entries read on success, or negative on failure
 **/
int Cache_load(const char* cacheFile) {
  int fd, error, count;
  cache *newCache, *replaced;

  /* Open File */
  fd = open(cacheFile, O_RDONLY);
  if (fd < 0) {
    perror(programName);
    return fd;
  }

  count = 0;

  while (1) {
    /* Make buffer for reading. */
    newCache = calloc(1, sizeof(cache));
    if (newCache == NULL) break;
    newCache->id = calloc(SHA256_SIZE + sizeof(uint16_t), sizeof(char));
    if (newCache->id == NULL) { free(newCache); break; }

    /* Read id and bufLen */
    error = read(fd, newCache->id, SHA256_SIZE + sizeof(uint16_t));
    if (error < SHA256_SIZE + sizeof(uint16_t)) {
      if (error < 0) perror(programName);
      free(newCache->id); free(newCache); break;
    }
    error = read(fd, &(newCache->bufLen), sizeof(size_t));
    if (error < sizeof(size_t)) {
      if (error < 0) perror(programName);
      free(newCache->id); free(newCache); break;
    }

    /* Create and fill buffer */
    newCache->buf = calloc(newCache->bufLen, sizeof(char));
    if (newCache->buf == NULL) {
      free(newCache->id); free(newCache); break;
    }

    error = read(fd, newCache->buf, newCache->bufLen);
    if (error < newCache->bufLen) {
      if (error < 0) perror(programName);
      free(newCache->buf); free(newCache->id); free(newCache); break;
    }

    /* Add to Hash Table */
    HASH_REPLACE(hh, memCache, id[0], sizeof(cache), newCache, replaced);
    if (replaced != NULL) {
      free(replaced->buf); free(replaced->id); free(replaced);
    }
  }

  close(fd);
  return count;
}

/**
 * int Cache_addUpdate(char[32], uint16_t, void*, size_t)
 * Note: A defensive copy is made of the entry buffer.
 * @param hash, protocol: used to identify the cache entry
 * @param record: Entry data buffer
 * @param recordLen: Size of entry data buffer
 * @return 0 on success, negative on failure
 **/
int Cache_addUpdate(char hash[SHA256_SIZE], uint16_t protocol, void* record, size_t recordLen) {
  cache *add, *replaced;
  replaced = NULL;

  add = calloc(1, sizeof(cache));
  if (add == NULL) return -1;

  add->id = calloc(SHA256_SIZE + sizeof(uint16_t), sizeof(char));
  if (add->id == NULL) {
    free(add); return -1;
  }

  memcpy(add->id, hash, SHA256_SIZE);
  memcpy(&(add->id[SHA256_SIZE]), &protocol, sizeof(uint16_t));

  add->buf = calloc(recordLen, sizeof(char));
  if (add->buf == NULL) {
    free(add->id); free(add); return -1;
  }

  memcpy(add->buf, record, recordLen);

  HASH_REPLACE(hh, memCache, id[0], sizeof(cache), add, replaced);
  if (replaced != NULL) {
    free(replaced->buf); free(replaced->id); free(replaced);
  }
  
  return EXIT_SUCCESS;
} /* End Cache_addUpdate() */

/**
 * const void* Cache_get(char[32], uint16_t, size_t*)
 * Note: The returned buffer is a freshly allocated copy and must be freed by the caller.
 * @param hash, protocol: used to identify the cache entry
 * @param recordLen: value-parameter, is filled with the size of the return value if not NULL
 * @return pointer to the cache entry buffer, DO NOT FREE!
 **/
const void* Cache_get(char hash[SHA256_SIZE], uint16_t protocol, size_t* recordLen) {
  char* getID;
  cache *getCache;

  getID = calloc(SHA256_SIZE + sizeof(uint16_t), sizeof(char));
  if (getID == NULL) return NULL;

  memcpy(getID, hash, SHA256_SIZE);
  memcpy(&(getID[SHA256_SIZE]), &protocol, sizeof(uint16_t));

  HASH_FIND(hh, memCache, getID, SHA256_SIZE + sizeof(uint16_t), getCache);

  free(getID);

  if (getCache == NULL) {
    return NULL;
  }

  *recordLen = getCache->bufLen;
  return getCache->buf;
}

/**
 * void Cache_destroy(void)
 * De-allocates all resources associated with the in-memory cache.
 **/
void Cache_destroy(void) {
  cache *current, *tmp;

  HASH_ITER(hh, memCache, current, tmp) {
    HASH_DEL(memCache, current);
    free(current->buf);
    free(current->id);
    free(current);
  }
}

