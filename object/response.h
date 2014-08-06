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
char* Response_id(Response_T response);

/**
 * int Response_recordCount(Response_T)
 * @return Number of records associated with @param response
**/
int Response_recordCount(Response_T response);

/**
 * int Response_merge(Response_T, Response_T)
 * Add all records from @param src to @param dest, re-writing records if
 * there are conflicts that must be resolved via the Timestamp.
 * Note: If either response is authoritative, no merging takes place, and dest is overwritten
 * with the authoritative response.
 * @see MARP v1 Specification for tie-breaker rules.
 * @return the number of records modified or added to @param dest
 **/
int Response_merge(Response_T dest, Response_T src);

/**
 * const void* Response_getRecord(uint16_t, size_t)
 * @param protocol: identifies the record, in Host Byte Order
 * @param length: overwritten with the return buffer's length
 * @return a constant buffer for the record
 **/
const void* Response_getRecord(Response_T response, uint16_t protocol, size_t* length);

/**
 * int Response_addRecord(uint16_t, const void*, size_t)
 * @param protocol: Identifier for the record in Host Byte Order
 * @param record: Buffer to the record, a copy is made.
 * @param recordLen: Length of the record buffer.
 * @return: 0 on success, -1 on failure.
 **/
int Response_addRecord(Response_T response, uint16_t protocol, const void* record);

/**
 * int Response_buildRecord(Response_T, uint16_t, char*, size_t, int)
 * Uses all parameters to build a record and add it to the response.
 * @return: 0 on success, -1 on failure
 **/
int Response_buildRecord(Response_T response, uint16_t protocol, const char* encrypted, uint16_t encLen, uint16_t ttl);

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

/**
 * void Response_printDecrypted(Response_T, char[SHA256_SIZE])
 * Prints information on all records of this response to standard out
 * @param response: to print
 * @param key: Used to decrypt all records
 * @return None
 **/
void Response_printDecrypted(Response_T response, char key[SHA256_SIZE]);

#endif
