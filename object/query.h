/**
 * File: query.h
 * Author: Ethan Gordon
 * A MARP Standard Query data group.
 **/

#ifndef QUERY_H
#define QUERY_H

#define SHA256_SIZE 32

/* Query Payload Object */
typedef *struct query Query_T;

Query_T Query_init(void* buf, size_t bufLen);

char[SHA56_SIZE] Query_id(Query_T query);

uint16_t* Query_protocols(Query_T query);

char* Query_host(Query_T query);

int Query_addProtocol(Query_T query, uint16_t protocol);

int Query_rmProtocol(Query_T query, uint16_t protocol);

/* Serialize Functions */
size_t Query_size(Query_T query);

int Query_serialize(Query_T query, void* buffer);

void Query_free(Query_T query);

#endif
