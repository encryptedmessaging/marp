/**
 * File: response.h
 * Author: Ethan Gordon
 * A MARP Standard Response data group.
 **/
#define SHA256_SIZE 32
#define SIGNATURE 65

#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

#include "response.h"

/**
 * Convert uint64_t from host to network byte order and back
 **/
/*
static uint64_t htonll(uint64_t value){
  uint64_t vv = *((uint64_t*) &value);
  int num = 42;
  if(*(char *)&num == 42) Locally Little Endian (Swap)
    return (((uint64)htonl((uint32_t)vv)) << 32) + htonl((uint32_t)(vv >> 32)));
  Already Big Endian 
  return vv;
}
*/

static uint64_t ntohll(uint64_t value){
  uint64_t vv = *((uint64_t*) &value);
  int num = 42;
  if(*(char *)&num == 42) /* Locally Little Endian (Swap)*/
    return (((uint64_t)ntohl((uint32_t)vv)) << 32) + ntohl((uint32_t)(vv >> 32));
  /* Already Big Endian */
  return vv;
}

/* Response Payload Object */

struct record {
  uint16_t protocol;
  uint16_t length;
  char* encrypted;
  uint16_t ttl;
  int64_t timestamp;
};

struct response {
  char hash[SHA256_SIZE];
  uint8_t recordCount;
  struct record* records;
  char* signature;
};

/**
 * Response_T Response_init(void*, size_t)
 * @param buf: Response to de-serialized. An empty standard response is created if NULL.
 * @param bufLen: Length of buf, should be 0 if buf is NULL.
 * @return Newly Allocated Response
 **/
Response_T Response_init(void* buf, size_t bufLen) {
  Response_T ret;
  int i;
  uint8_t *byteBuf = (uint8_t*)buf;

  ret = calloc(1, sizeof(struct response));
  if (ret == NULL) return NULL;

  if (buf == NULL) return ret;

  /* Fill Hash */
  if (bufLen < SHA256_SIZE) return NULL;
  memcpy(&(ret->hash), byteBuf, SHA256_SIZE);
  bufLen -= SHA256_SIZE;
  byteBuf += SHA256_SIZE;

  /* Fill Record Count */
  if (bufLen == 0) {
    free(ret);
    return NULL;
  }
  ret->recordCount = *byteBuf;
  byteBuf++;
  bufLen--;

  /* Fill Records */
  ret->records = calloc(ret->recordCount, sizeof(struct record));
  if (ret->records == NULL) {
    free(ret);
    return NULL;
  }
  for(i = 0; i < ret->recordCount; i++) {
    if (bufLen < 2*sizeof(uint16_t)) {
      Response_free(ret);
      return NULL;
    }
    /* Protocol and Length */
    ret->records[i].protocol = ntohs(*(uint16_t*)byteBuf);
    byteBuf += sizeof(uint16_t);
    bufLen -= sizeof(uint16_t);
    ret->records[i].length = ntohs(*(uint16_t*)byteBuf);
    byteBuf += sizeof(uint16_t);
    bufLen -= sizeof(uint16_t);

    /* Encrypted Data */
    if (bufLen < ret->records[i].length) {
      Response_free(ret);
      return NULL;
    }
    memcpy(ret->records[i].encrypted, byteBuf, ret->records[i].length);
    byteBuf += ret->records[i].length;
    bufLen -= ret->records[i].length;
    
    /* TTL and Timestamp */
    if (bufLen < sizeof(uint16_t) + sizeof(uint64_t)) {
      Response_free(ret);
      return NULL;
    }

    ret->records[i].ttl = ntohs(*(uint16_t*)byteBuf);
    byteBuf += sizeof(uint16_t);
    bufLen -= sizeof(uint16_t);
    ret->records[i].timestamp = (int64_t)ntohll(*(uint64_t*)byteBuf);
    byteBuf += sizeof(uint64_t);
    bufLen -= sizeof(uint64_t);
  } /* End for */

  if (bufLen > SIGNATURE) {
    ret->signature = calloc(SIGNATURE, sizeof(char));
    if (ret->signature == NULL) {
      Response_free(ret);
      return NULL;
    }
    memcpy(ret->signature, byteBuf, SIGNATURE);
  }

  return ret;

} /* End Response_init() */

/**
 * char[32] Response_id(Response_T)
 * @return Hash associated with @param response
 **/
char* Response_id(Response_T response) {
  if (response == NULL) return NULL;
  return response->hash;
} /* End Response_id() */

/**
 * int Response_recordCount(Response_T)
 * @return Number of records associated with @param response
 **/
int Response_recordCount(Response_T response) {
  if (response == NULL) return 0;
  return response->recordCount;
} /* End Response_recordCount() */

/**
 * int Response_merge(Response_T, Response_T)
 * Add all records from @param src to @param dest, re-writing records if
 * there are conflicts that must be resolved via the Timestamp.
 * @see MARP v1 Specification for tie-breaker rules.
 * @return the number of records modified or added to @param dest
 **/
int Response_merge(Response_T dest, Response_T src) {
  return EXIT_SUCCESS;
} /* End Response_merge() */

/**
 * void Response_free(Response_T)
 * De-allocated all resources associated with @param response
 **/
void Response_free(Response_T response) {
  int i;
  if (response == NULL) return;

  if (response->signature) free(response->signature);
  for (i = 0; i < response->recordCount; i++) {
    if (response->records[i].encrypted) 
      free(response->records[i].encrypted);
  }
  if (response->records) free(response->records);
  free(response);
} /* End Response_free() */

/* Serialize Functions */

/**
 * size_t Response_size(Response_T)
 * @param response
 * @return the guarenteed maximum size of the serialized buffer from Response_serialize()
 **/
size_t Response_size(Response_T response);

/**
 * size_t Response_serialize(Response_T, void*)
 * @param response
 * @param buffer: Filled with the serialized response, must be at least Response_size() big
 * @return Actual number of bytes written
 **/
size_t Response_serialize(Response_T response, void* buffer);

