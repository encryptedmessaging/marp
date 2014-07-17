/**
 * File: aes.h
 * Author: Ethan Gordon
 * Wrapper for the OpenAES Library.
 **/

#ifndef MARP_AES_H
#define MARP_AES_H

#define SHA256_SIZE 32

void* aes_encrypt(void* buf, size_t bufLen, char[SHA256_SIZE] key, size_t* retLen);

void* aes_decrypt(void* buf, size_t bufLen, char[SHA256_SIZE] key, size_t* retLen);

#endif
