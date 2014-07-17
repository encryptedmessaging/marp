/**
 * File: ecdsa.h
 * Author: Ethan Gordon
 * Wrapper for the micro-ecc Library.
 **/

#ifndef MARP_ECDSA_H
#define MARP_ECDSA_H

#define ECC_SIZE 32
#define SHA256_SIZE 32

char[1 + 2*ECC_SIZE] ecdsa_sign(char[SHA256_SIZE] dataHash, char[ECC_SIZE] privkey);

int ecdsa_verify(char[SHA256_SIZE] dataHash, char[1 + 2*ECC_SIZE] pubkey);

#endif
