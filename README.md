# Message Address Resolution Protocol Daemon #

This is the reference implementation of MARP v1, written for any POSIX-compliant operating system.

### What is MARP? ###

Marp is a means to translate email-style addresses (represented as <handle>@<host>) to cryptographic addresses (usually a base58- or base64- encoded random-looking string) in a distributed manner that relies on the same trust network as DNS.
The trust network relies on each server generating its own Public/Private ECC Keypair, and publishing the public key as a TXT record for the <host> domain.