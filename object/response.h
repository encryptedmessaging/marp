/**
 * File: response.h
 * Author: Ethan Gordon
 * A MARP Standard Response data group.
 **/

#ifndef RESPONSE_H
#define RESPONSE_H

#define SHA256_SIZE 32

/* Query Payload Object */
typedef struct response *Response_T;

/**
 * Response_T Response_init(void*, size_t)
 * @param buf: Response to de-serialized. An empty standard response is created if NULL.
 * @param bufLen: Length of buf, should be 0 if buf is NULL.
 * @return Newly Allocated Response
 **/
Response_T Response_init(void* buf, size_t bufLen);

/**
 * char[32] Response_id(Response_T)
 * @return Hash associated with @param response
 **/
char (*Response_id(Response_T response))[SHA256_SIZE];

/**
 * uint16_t* Response_protocols(Response_T)
 * @return 0-terminated protocol array associated wtih @param response
 **/
uint16_t* Response_protocols(Response_T response);

/**
 * int Response_merge(Response_T, Response_T)
 * Add all records from @param src to @param dest, re-writing records if
 * there are conflicts that must be resolved via the Timestamp.
 * @see MARP v1 Specification for tie-breaker rules.
 * @return the number of records modified or added to @param dest
 **/
int Response_merge(Response_T dest, Response_T src);

/**
 * void Response_free(Response_T)
 * De-allocated all resources associated with @param response
 **/
void Response_free(Response_T response);

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

#endif
