/**
 * File: query.h
 * Author: Ethan Gordon
 * A MARP Standard Query data group.
 **/

#ifndef QUERY_H
#define QUERY_H

#define SHA256_SIZE 32

/* Query Payload Object */
typedef struct query *Query_T;

/**
 * Query_T Query_init(void*, size_t)
 * @param buf: Serialized information to parse into Query, cannot be NULL
 * @param bufLen: Length of buf
 * @return New Query, or NULL on failure
 **/
Query_T Query_init(void* buf, size_t bufLen);

/**
 * char[32] Query_id(Query_T)
 * @param query: to get the ID of
 * @return the 32-byte hash associated with this query.
 **/
char *Query_id(Query_T query);

/**
 * uint16_t Query_protocols(Query_T)
 * @param query: to get the protocol list
 * @return: 0-terminated list of protocol identifiers
 **/
const uint16_t* Query_protocols(Query_T query);

/**
 * char* Query_host(Query_T)
 * The host-name associated with this query, or NULL if a reverse query.
 **/
const char* Query_host(Query_T query);

/**
 * int Query_addProtocol(Query_T, uint16_t)
 * Add @param protocol to @param query.
 * WARNING: A reverse query should have EXACTLY ONE (1) protcol.
 * @return 0 on success, negative on failure.
 **/
int Query_addProtocol(Query_T query, uint16_t protocol);

/**
 * int Query_rmProtocol(Query_T, uint16_t)
 * Remove @param protocol from @param query
 * WARNING: A reverse query should have EXACTLY ONE (1) protocol.
 * @return 0 on success, negative on failure.
 **/
int Query_rmProtocol(Query_T query, uint16_t protocol);

/**
 * void Query_free(Query_T)
 * De-allocates all resources for this query.
 **/
void Query_free(Query_T query);

/* Serialize Functions */

/**
 * size_t Query_size(Query_T)
 * @param query
 * @return the guarenteed maximum size of the serialized buffer from Query_serialize()
 **/
size_t Query_size(Query_T query);

/**
 * size_t Query_serialize(Query_T, void*)
 * @param query
 * @param buffer: Filled with the serialized query, must be at least Query_size() big
 * @return Actual number of bytes written
 **/
size_t Query_serialize(Query_T query, void* buffer);

#endif
