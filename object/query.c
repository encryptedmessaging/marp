/**
 * File: query.h
 * Author: Ethan Gordon
 * A MARP Standard Query data group.
 **/

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <assert.h>

#define SHA256_SIZE 32

#include "query.h"

/* Query Payload Object */
struct query {
  char hash[SHA256_SIZE];
  uint16_t *protocols;
  char* host;
  size_t size;
};

/**
 * Query_T Query_init(void*, size_t)
 * @param buf: Serialized information to parse into Query, cannot be NULL
 * @param bufLen: Length of buf
 * @return New Query, or NULL on failure
 **/
Query_T Query_init(void* buf, size_t bufLen) {
  Query_T ret;
  int count = 0;
  int i;
  uint16_t *readBuf;

  if (bufLen < SHA256_SIZE) {
    return NULL;
  }

  bufLen -= SHA256_SIZE;

  ret = calloc(1, sizeof(struct query));
  if (ret == NULL) {
    return NULL;
  }

  readBuf = (uint16_t*)buf;

  /* Fill in Query */
  (void)memcpy(&(ret->hash), buf, SHA256_SIZE);
  readBuf += SHA256_SIZE / sizeof(uint16_t);

  while (*readBuf != 0) {
    count++;
    bufLen -= sizeof(uint16_t);
    if (bufLen <= 0) {
      /* No 0 terminator! */
      free(ret);
      return NULL;
    }
    readBuf++;
  }
  /* readBuf is pointing at the 0, there are bufLen+2 bytes left in host */
  readBuf++;
  bufLen -= 2;

  ret->host = calloc(bufLen, sizeof(char));
  if (ret->host == NULL) {
    free(ret);
    return NULL;
  }

  memcpy(&(ret->host), (void*)readBuf, bufLen);

  count++; /* Add terminating 0 */
  ret->protocols = calloc(count, sizeof(uint16_t));
  if (ret->protocols == NULL) {
    free(ret);
    free(ret->host);
    return NULL;
  }

  i = 0;
  for (readBuf = (uint16_t*)buf; *readBuf != 0; readBuf++) {
    ret->host[i] = ntohs(*readBuf);
    i++;
  }

  /* Set size */
  ret->size = SHA256_SIZE + bufLen + (count * sizeof(uint16_t));

  return ret;

} /* End Query_init() */

/**
 * char[32] Query_id(Query_T)
 * @param query: to get the ID of
 * @return the 32-byte hash associated with this query.
 **/
char* Query_id(Query_T query) {
  assert(query != NULL);
  return query->hash;
}


/**
 * uint16_t Query_protocols(Query_T)
 * @param query: to get the protocol list
 * @return: 0-terminated list of protocol identifiers
 **/
const uint16_t* Query_protocols(Query_T query) {
  assert(query != NULL);
  return query->protocols;
}

/**
 * char* Query_host(Query_T)
 * The host-name associated with this query, or NULL if a reverse query.
 **/
const char* Query_host(Query_T query) {
  assert(query != NULL);
  return query->host;
}

/**
 * int Query_addProtocol(Query_T, uint16_t)
 * Add @param protocol to @param query.
 * WARNING: A reverse query should have EXACTLY ONE (1) protcol.
 * @return 0 on success, negative on failure.
 **/
int Query_addProtocol(Query_T query, uint16_t protocol) {
  uint16_t* proto;
  size_t count;

  assert(query != NULL);

  for (proto = query->protocols; *proto != 0; proto++) {
    if (*proto == protocol) return EXIT_SUCCESS;
  }

  /* Need to add to list */
  count = (proto - query->protocols) + 2;
  query->protocols = realloc(query->protocols, count * sizeof(uint16_t));
  if (query->protocols == NULL) {
    return -1;
  }
  query->protocols[count-2] = protocol;
  query->protocols[count-1] = 0;

  query->size += sizeof(uint16_t);

  return EXIT_SUCCESS;
}

/**
 * int Query_rmProtocol(Query_T, uint16_t)
 * Remove @param protocol from @param query
 * WARNING: A reverse query should have EXACTLY ONE (1) protocol.
 * @return 0 on success, negative on failure.
 **/
int Query_rmProtocol(Query_T query, uint16_t protocol) {
  uint16_t* proto;
  bool found = false;
  assert(query != NULL);

  for(proto = query->protocols; *proto != 0; proto++) {
    if (*proto == protocol) found = true;
    if (found) {
      proto[0] = proto[1];
    }
  }

  if (found) {
    query->size -= sizeof(uint16_t);
    return EXIT_SUCCESS;
  } else return -1;
}

/**
 * void Query_free(Query_T)
 * De-allocates all resources for this query.
 **/
void Query_free(Query_T query) {
  if (query == NULL) return;

  if (query->host) free(query->host);
  if (query->protocols) free(query->protocols);
  free(query);
}

/* Serialize Functions */

/**
 * size_t Query_size(Query_T)
 * @param query
 * @return the guarenteed maximum size of the serialized buffer from Query_serialize()
 **/
size_t Query_size(Query_T query) {
  if (query == NULL) return 0;
  return query->size;
}

/**
 * size_t Query_serialize(Query_T, void*)
 * @param query
 * @param buffer: Filled with the serialized query, must be at least Query_size() big
 * @return Actual number of bytes written
 **/
size_t Query_serialize(Query_T query, void* buffer) {
  uint16_t* proto;
  uint16_t* buf;
  size_t count = SHA256_SIZE;

  assert(buffer != NULL);
  assert(query != NULL);

  buf = (uint16_t*)buffer;

  memcpy(buffer, &(query->hash), SHA256_SIZE);
  buf += SHA256_SIZE / sizeof(uint16_t);

  for(proto = query->protocols; *proto != 0; proto++) {
    *buf = htons(*proto);
    buf++;
    count += sizeof(uint16_t);
  }
  *buf = 0;
  buf++;
  count += sizeof(uint16_t);
  
  strncpy((char*)buf, query->host, query->size - count);
  return query->size;
}
