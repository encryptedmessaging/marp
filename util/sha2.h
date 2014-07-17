/**
 * File: sha2.h
 * Author: Ethan Gordon
 * Wrapper for the <sha2> Library.
 **/

#ifndef MARP_SHA_H
#define MARP_SHA_H

#define SHA256_SIZE 32

char[SHA256_SIZE] sha2_hash256(void* buf, size_t bufLen);

#endif
