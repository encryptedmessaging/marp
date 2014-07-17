/**
 * File: response.h
 * Author: Ethan Gordon
 * A MARP Standard Response data group.
 **/

#ifndef RESPONSE_H
#define RESPONSE_H

#define SHA256_SIZE 32

/* Query Payload Object */
typedef *struct response Response_T;

Response_T Response_init(void* buf, size_t bufLen);

char[SHA56_SIZE] Response_id(Response_T response);

uint16_t* Response_protocols(Response_T response);

int Response_merge(Response_T dest, Response_T src);

/* Serialize Functions */
size_t Response_size(Response_T response);

int Response_serialize(Response_T response, void* buffer);

void Response_free(Response_T response);

#endif
