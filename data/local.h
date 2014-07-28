/**
 * File: local.h
 * Author: Ethan Gordon
 * Store and access data from the local .marp configuration file.
 **/

#ifndef LOCAL_H
#define LOCAL_H

#define SHA256_SIZE 32

/**
 * int Local_init(const char*)
 * Loads the contents of a config file to memory.
 * @param configFile: absolute or relative path to MARP configuration file
 * @return 0 on success, negative on failure
 **/
int Local_init(const char* configFile);

/**
 * char* Local-get(char[32], uint16_t)
 * @param hash, protocol: Used as record identification.
 * @return Case 1: If the hash is a <handle>@<host> combination, return the plaintext address
 * @return Case 2: If the hash is an address, return the plaintext <handle>@<host>
 * @return NULL on miss
 **/
const char* Local_get(char hash[SHA256_SIZE], uint16_t protocol);

/**
 * char* Local_getPubkey(void)
 * @return Plaintext, base64-encoded ECC public key for this server.
 **/
const char* Local_getPubkey(void);

/**
 * char* Local_getPrivkey(void)
 * @return Plaintext, base64-encoded ECC private key for this server.
 **/
const char* Local_getPrivkey(void);

/**
 * int Local_getTTL(char[32])
 * @param hash: The <handle>@<host> entry associated with the TTL
 * @return The TTL of all records at this hash in seconds.
 **/
int Local_getTTL(char hash[SHA256_SIZE], uint16_t protocol);

/**
 * void Local_destory(void)
 * De-allocte all resources associated with the local config.
 **/
void Local_destroy(void);

#endif
