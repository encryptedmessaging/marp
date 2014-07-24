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
#include <assert.h>

#include "response.h"

/**
 * Convert uint64_t from host to network byte order and back
 **/

static uint64_t htonll(uint64_t value){
  uint64_t vv = *((uint64_t*) &value);
  int num = 42;
  if(*(char *)&num == 42) /* Locally Little Endian (Swap) */
    return (((uint64_t)htonl((uint32_t)vv)) << 32) + htonl((uint32_t)(vv >> 32));
/* Already Big Endian */
  return vv;
}

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

/* Free records in @param list of length @param length. */
static void freeList(struct record* list, int length) {
  int i;

  assert(list != NULL);

  for (i = 0; i < length; i++) {
    if (list[i].encrypted) free(list[i].encrypted);
  }
  free(list);
}

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
 * Note: If either response has a signature, no merging takes place ,and dest is overwritten with the
 * authoritative response.
 * @see MARP v1 Specification for tie-breaker rules.
 * @return the number of records modified or added to @param dest, or NULL
 **/
int Response_merge(Response_T dest, Response_T src) {
  Response_T tmp;
  int i, j, newIndex, ret;
  newIndex = 0;

  struct record* newList;
  if (dest == NULL || src == NULL) return EXIT_SUCCESS;

  /* Check for Signature */
  if (dest->signature != NULL) return EXIT_SUCCESS;
  if (src->signature != NULL) {
    tmp = malloc(sizeof(struct response));
    if (tmp == NULL) return -1;
    *tmp = *dest;
    *dest = *src;
    *src = *tmp;
    free(tmp);
    return dest->recordCount;
  }

  /* Two Non-Signature Responses. */
  newList = calloc(src->recordCount + dest->recordCount, sizeof(struct record));
  if (newList == NULL) return 0;

  for(i = 0; i < src->recordCount; i++) {
    int found = -1;
    for (j = 0; j < dest->recordCount; j++) {
      if (src->records[i].protocol == dest->records[j].protocol)
        found = j;
    }

    /* If not found, copy to new list */
    if (found < 0) {
      newList[newIndex] = src->records[i];
      newList[newIndex].encrypted = calloc(src->records[i].length, sizeof(char));
      if (newList[newIndex].encrypted == NULL) {
        freeList(newList, newIndex); return 0;
      }
      memcpy(newList[newIndex].encrypted, src->records[i].encrypted, src->records[i].length);
    } else { /* If found, compare timestamps */
      if (src->records[i].timestamp > dest->records[found].timestamp) {
        newList[newIndex] = src->records[i];
        newList[newIndex].encrypted = calloc(src->records[i].length, sizeof(char));
        if (newList[newIndex].encrypted == NULL) {
          freeList(newList, newIndex); return 0;
        }
        memcpy(newList[newIndex].encrypted, src->records[i].encrypted, src->records[i].length);
      } else {
        newList[newIndex] = dest->records[found];
        newList[newIndex].encrypted = calloc(dest->records[found].length, sizeof(char));
        if (newList[newIndex].encrypted == NULL) {
          freeList(newList, newIndex); return 0;
        }
        memcpy(newList[newIndex].encrypted, dest->records[found].encrypted, dest->records[found].length);
      }
    }
    newIndex++;
  }

  ret = newIndex;

  /* Loop through dest, if anything is not in newList, add it */
  for (i = 0; i < dest->recordCount; i++) {
    int found = 0;
    for (j = 0; j < newIndex; j++) {
      if (newList[j].protocol == dest->records[i].protocol) {
        found = 1;
        break;
      }
    }
    if (!found) {
      newList[newIndex] = dest->records[i];
      newList[newIndex].encrypted = calloc(dest->records[i].length, sizeof(char));
      if (newList[newIndex].encrypted == NULL) {
        freeList(newList, newIndex); return 0;
      }
      memcpy(newList[newIndex].encrypted, dest->records[i].encrypted, dest->records[i].length);
      newIndex++;
    }
  }

  /* Free Dest List, the replace with newList */
  freeList(dest->records, dest->recordCount);
  dest->records = newList;
  dest->recordCount = newIndex;

  return ret;
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
size_t Response_size(Response_T response) {
  int i;
  size_t count = SHA256_SIZE + SIGNATURE + sizeof(uint8_t);
  if (response == NULL) return 0;

  for (i = 0; i < response->recordCount; i++) {
    count += (3 * sizeof(uint16_t) + sizeof(int64_t) + response->records[i].length);
  }

  return count;
} /* End Response_size */

/**
 * size_t Response_serialize(Response_T, void*)
 * @param response
 * @param buffer: Filled with the serialized response, must be at least Response_size() big
 * @return Actual number of bytes written
 **/
size_t Response_serialize(Response_T response, void* buffer) {
  uint16_t* numPtr;
  uint8_t* byteBuf = buffer;
  int i;

  if (response == NULL || buffer == NULL) return 0;
  memcpy(byteBuf, response->hash, SHA256_SIZE);
  byteBuf += SHA256_SIZE;
  *byteBuf = response->recordCount;

  byteBuf++;
  for (i = 0; i < response->recordCount; i++) {
    numPtr = (uint16_t*)byteBuf;
    *numPtr = htons(response->records[i].protocol);
    numPtr++;
    *numPtr = htons(response->records[i].length);
    numPtr++;

    memcpy(numPtr, response->records[i].encrypted, response->records[i].length);
    byteBuf = (uint8_t*)numPtr;
    byteBuf += response->records[i].length;
    numPtr = (uint16_t*)byteBuf;
  
    *numPtr = htons(response->records[i].ttl);
    numPtr++;
    *((uint64_t*)numPtr) = htonll((uint64_t)response->records[i].timestamp);
    byteBuf = (uint8_t*)numPtr;
    byteBuf += sizeof(int64_t);
  }
  
  if (response->signature)
    memcpy(byteBuf, response->signature, SIGNATURE);

  return Response_size(response);
} /* End Response_serialize() */
