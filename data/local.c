/**
 * File: local.h
 * Author: Ethan Gordon
 * Store and access data from the local .marp configuration file.
 **/
#define _GNU_SOURCE
#define SHA256_SIZE 32

/* ECC Config */
#define uECC_CURVE 3 /* P256 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <oaes_base64.h>
#include <oaes_lib.h>

#include "../uthash.h"
#include "inih/ini.h"
#include "../libsha2/sha256.h"
#include "local.h"
#include "../micro-ecc/uECC.h"


#define KEY_SIZE 32
#define PUB_SIZE 2*KEY_SIZE+1
#define PUB_BASE64 89
  

/* Individual Local Entry, Placed in Hash Table */
typedef struct entry {
  /* Concatonated SHA256('handle@host') + protocol */
  char* id;

  /* Encrypted Address */
  char* encrypted;
  size_t encLen;

  /* TTL either from section or host */
  int ttl;

  UT_hash_handle hh; 
} entry;

/* Local Structure, the basis for the AO */
struct local {
  /* Local HashTable Head, should be NULL */
  entry* localCache;
  /* Private Key */
  char* privkey;
};

/* Persistent Structure to Pass Along the Host Handlers */
struct sHost {
  /* Host name as a C-string */
  const char* host;
  /* TTL for entire hostname, used if section has no TTL entry */
  int hostTTL;
  /* Name of the current section (handle) */
  char* currentSection;
  /* Name of the Section TTL (good until the next TTL or next Section */
  int sectionTTL;
};

typedef struct local *Local_T;

/* AO Object */
static Local_T config;

/* Array of Protocols, Max 255 for now */
#define PROTO_MAX 255
static char** protocols;

/* argv[0] from main */
extern char* programName;

static void Local_loadKey(const char* file) {
  int error, fd, pubfd;
  size_t dummy;
  uint8_t privkey[KEY_SIZE];
  uint8_t pubkey[PUB_SIZE];
  char pubstr[PUB_BASE64 + 1];

  char* pubFile;
  if (config->privkey) { free(config->privkey); config->privkey = NULL; }

  config->privkey = calloc(KEY_SIZE, sizeof(char));
  if (config->privkey == NULL) {
    fprintf(stderr, "%s: Local_loadKey: Out of Memory\n", programName);
    exit(EXIT_FAILURE);
  }

  fd = open(file, O_RDONLY);
  if (fd < 0) {
    if (errno == ENOENT) {
      printf("%s: Local_loadKey: Private key file not found, generating new...\n", programName);
      /* Create new ECC keypair */
      fd = creat(file, 0600);
      if (fd < 0) {
        fprintf(stderr, "%s: Local_loadKey: Could not create private key file... %s\n", programName, strerror(errno));
        exit(EXIT_FAILURE);
      }

      pubkey[0] = 0x04;
      error = uECC_make_key(&(pubkey[1]), privkey);
      if (error == 0) {
        fprintf(stderr, "%s: Local_loadKey: Could not make new key\n", programName);
        exit(EXIT_FAILURE);
      }

      dummy = PUB_BASE64;
      error = oaes_base64_encode(pubkey, PUB_SIZE, pubstr, &dummy);
      pubstr[PUB_BASE64 + 1] = '\0';

      if (error) {
        fprintf(stderr, "%s: Local_loadKey: Could not base64 encode public key\n", programName);
        exit(EXIT_FAILURE);
      }

      pubFile = calloc(strlen(file) + strlen(".pub") + 1, sizeof(char));
      if (pubFile == NULL) {
        fprintf(stderr,"%s: Local_loadKey: Out of Memory\n", programName);
        exit(EXIT_FAILURE);
      }
      strcat(pubFile, file);
      strcat(pubFile, ".pub");

      pubfd = creat(pubFile, 0666);
      if (pubfd < 0) {
        fprintf(stderr, "%s: Local_loadKey: Could not create public key file %s... %s\n", programName, pubFile, strerror(errno));
        exit(EXIT_FAILURE);
      }

      error = write(pubfd, pubstr, PUB_BASE64);
      if (error < 0) {
        fprintf(stderr, "%s: Local_loadKey: Could not write to public key file... %s\n", programName, strerror(errno));
        exit(EXIT_FAILURE);
      }

      error = write(fd, privkey, KEY_SIZE);
      if (error < 0) {
        fprintf(stderr, "%s: Local_loadKey: Could not write to private key file... %s\n", programName, strerror(errno));
        exit(EXIT_FAILURE);
      }

      printf("%s: Local_loadKey: Private Key saved to %s.\n", programName, file);
      printf("%s: Local_loadKey: Public Key saved to %s.\n", programName, pubFile);
      printf("%s: Local_loadKey: Add the following dns entry to make your marp server authoritative:\n", programName);
      printf("%s: Local_loadKey: <host>\tIN\tTXT\tmarp:%s\n\n", programName, pubstr);
      memcpy(config->privkey, privkey, KEY_SIZE);
      return;
    } else {
      fprintf(stderr, "%s: Local_loadKey: Could not load private key... %s\n", programName, strerror(errno));
      exit(EXIT_FAILURE);
    }
  }
  /* Read Private Key */
  error = read(fd, config->privkey, KEY_SIZE);
  if (error < KEY_SIZE) {
    fprintf(stderr, "%s: Local_loadKey: Could not read private key... %s\n", programName, strerror(errno));
    exit(EXIT_FAILURE);
  }
  printf("%s: Local_loadKey: Private Key loaded from %s\n", programName, file);
}


/** Handlers **/
static int hostHandler(void* hostPtr, const char* section, const char* name, const char* value) {
  uint16_t protocol;
  char* handleAtHost;
  char* id;
  entry* newEntry;
  entry* dummy;
  uint8_t key[SHA256_SIZE] = {0};
  OAES_CTX * ctx;
  struct sHost* host = (struct sHost*)hostPtr;

  /* Check protocols */
  if (protocols == NULL) return -1;

  /* Check for Global Section */
  if (strcmp(section, "global") == 0) {
    if (strcmp(name, "TTL") == 0)
      host->hostTTL = atoi(value); 
    return 0;
  }

  /* Clear TTL on Section Change */
  if (host->currentSection == NULL) {
    host->currentSection = calloc(strlen(section) + 1, sizeof(char));
    if (host->currentSection == NULL) return -1; 
    strcpy(host->currentSection, section);
    host->sectionTTL = 0;
  }

  if (strcmp(host->currentSection, section) != 0) {
    free(host->currentSection);
    host->currentSection = calloc(strlen(section) + 1, sizeof(char));
    if (host->currentSection == NULL) return -1;
    strcpy(host->currentSection, section);
    host->sectionTTL = 0;

  }

  /* Check for TTL Change */
  if (strcmp(name, "TTL") == 0) {
    host->sectionTTL = atoi(value);
    return 0;
  }
  
  /* Make ID */
  id = calloc(SHA256_SIZE + sizeof(uint16_t), sizeof(char));
  if (id == NULL) return -1;
  
  /* Find Protocol */
  for (protocol = 1; protocol < PROTO_MAX; protocol++) {
    if (protocols[protocol] == NULL) continue;
    if (strcmp(name, protocols[protocol]) == 0) break;
  }

  if (protocol == PROTO_MAX) {
    fprintf(stderr, "%s: Protocol %s not supported yet.\n", programName, name);
    free(id); return -1;
  }
  
  /* Create ID */
  handleAtHost = calloc(strlen(section) + strlen(name) + 1, sizeof(char));
  if (handleAtHost == NULL) {
    free(id); return -1;
  }

  strcat(handleAtHost, section);
  strcat(handleAtHost, "@");
  strcat(handleAtHost, host->host);

  sha256_simple((uint8_t*)handleAtHost, strlen(handleAtHost), (uint8_t*)key);
  sha256_simple(key, SHA256_SIZE, (uint8_t*)id);
  memcpy(&(id[SHA256_SIZE]), &protocol, sizeof(uint16_t));
  free(handleAtHost);

  /* Build Entry */
  newEntry = calloc(1, sizeof(entry));
  if (newEntry == NULL) {
    free(id); return -1;
  }

  newEntry->id = id;

  ctx = oaes_alloc();

  (void)oaes_key_import_data(ctx, key, SHA256_SIZE);
  (void)oaes_encrypt(ctx, (const uint8_t*)value, strlen(value), NULL, &(newEntry->encLen));
  newEntry->encrypted = calloc(newEntry->encLen, sizeof(char));
  if (newEntry->encrypted == NULL) {
    free(newEntry->id);
    free(newEntry);
    oaes_free(&ctx);
    return -1;
  }
  oaes_encrypt(ctx, (const uint8_t*)value, strlen(value), (uint8_t*)newEntry->encrypted, &(newEntry->encLen));

  if (host->sectionTTL)
    newEntry->ttl = host->sectionTTL;
  else newEntry->ttl = host->hostTTL;

  /* Store Entry in Hash Table */
  HASH_REPLACE(hh, config->localCache, id[0], SHA256_SIZE + sizeof(uint16_t), newEntry, dummy);
  if (dummy != NULL) {
    free(dummy->id);
    free(dummy);
  }

  oaes_free(&ctx);
  return 0;
}
 
static int nameHandler(void* nada, const char* section, const char* name, const char* value) {
  int protocol;
  /* Only "name" section allowed */
  if (strcmp(section, "name") != 0) return -1;
  
  protocol = atoi(value);
  if (protocol > PROTO_MAX) {
    fprintf(stderr, "%s: Protocol #%s not supported yet.\n", programName, value);
    return -1;
  }
  
  protocols[protocol] = calloc(strlen(name) + 1, sizeof(char));
  if (protocols[protocol] == NULL) return -1;
  strcpy(protocols[protocol], name);

  return 0;
}

static int handler(void* nada, const char* section, const char* name, const char* value) {
  struct sHost host;
  /* Global Configuration */
  if (strcmp(section, "global") == 0) {
    if (strcmp(name, "privkey") == 0) {
      if (config->privkey) {
        return -1;
      } else {
        Local_loadKey(value);
      }
    }
    /* Names Configuration */
    if (strcmp(name, "names") == 0) {
      protocols = calloc(PROTO_MAX, sizeof(char*));
      if (protocols == NULL) {
        return -1;
      }
      if (ini_parse(value, nameHandler, NULL) < 0) {
        fprintf(stderr, "%s: Error parsing names file %s: %s\n", programName, value, strerror(errno));
        /* Free Protocols */
        int i;
        for (i = 0; i < PROTO_MAX; i++) {
          if (protocols[i]) free(protocols[i]);
        }
        free(protocols);
        protocols = NULL;
        return -1;
      }
    }
    return 0;
  }

  /* Host Configuration */
  host.host = section;
  host.currentSection = NULL;
  host.hostTTL = 0;
  host.sectionTTL = 0;
  
  /* Not Fatal */
  if (strcmp(name, "include") != 0) return 0;
 
  /* Not Fatal if Falure */
  if (ini_parse(value, hostHandler, &host) < 0) {
    fprintf(stderr, "%s: Error parsing host file %s: %s", programName, value, strerror(errno));
    fprintf(stderr, "Already-Parsed entries are still in local cache.");
  }

  /* Cleanup Host Configuration */
  free(host.currentSection);

  return 0;
}

/**
 * int Local_init(const char*)
 * Loads the contents of a config file to memory.
 * @param configFile: absolute or relative path to MARP configuration file
 * @return 0 on success, negative on failure
 **/
int Local_init(const char* configFile) {
  /* Make a new AO */
  config = calloc(1, sizeof(struct local));
  if (config == NULL) return -1;
  
  config->localCache = NULL;
  
  /* Parse File */
  if (ini_parse(configFile, handler, NULL) < 0) {
    /* Failure, destroy evrything! */
    Local_destroy();
    return -1;
  }

  return EXIT_SUCCESS;
} /* End Local_init() */

/**
 * char* Local-get(char[32], uint16_t)
 * @param hash, protocol: Used as record identification.
 * @return Case 1: If the hash is a <handle>@<host> combination, return the plaintext address
 * @return Case 2: If the hash is an address, return the plaintext <handle>@<host>
 * @return NULL on miss
 **/
const char* Local_get(char hash[SHA256_SIZE], uint16_t protocol, size_t* encLen) {
  entry* record;
  char* getID;

  getID = calloc(SHA256_SIZE + sizeof(uint16_t), sizeof(char));
  if (getID == NULL) return NULL;

  memcpy(getID, hash, SHA256_SIZE);
  memcpy(&(getID[SHA256_SIZE]), &protocol, sizeof(uint16_t));

  HASH_FIND(hh, config->localCache, getID, SHA256_SIZE + sizeof(uint16_t), record);

  free(getID);

  if (record == NULL) {
    return NULL;
  }

  *encLen = record->encLen;
  return record->encrypted;
}

/**
 * char* Local_getPrivkey(void)
 * @return Plaintext, base64-encoded ECC private key for this server.
 **/
const char* Local_getPrivkey(void) {
  return config->privkey;
}

/**
 * int Local_getTTL(char[32])
 * @param hash: The <handle>@<host> entry associated with the TTL
 * @return The TTL of all records at this hash in seconds.
 **/
int Local_getTTL(char hash[SHA256_SIZE], uint16_t protocol) {
  entry* record; 
  char* getID;

  getID = calloc(SHA256_SIZE + sizeof(uint16_t), sizeof(char));
  if (getID == NULL) return 0;

  memcpy(getID, hash, SHA256_SIZE);
  memcpy(&(getID[SHA256_SIZE]), &protocol, sizeof(uint16_t));

  HASH_FIND(hh, config->localCache, getID, SHA256_SIZE + sizeof(uint16_t), record);

  free(getID);

  if (record == NULL) {
    return 0;
  }

  return record->ttl;
}

/**
 * void Local_destory(void)
 * De-allocte all resources associated with the local config.
 **/
void Local_destroy(void) {
  int i;
  entry *current, *tmp;
  /* Free Protocols */
  if (protocols) {
    for(i = 0; i < PROTO_MAX; i++) {
      if (protocols[i]) free(protocols[i]);
    }
    free(protocols);
  }

  /* Free Config */
  if (config) {
    if (config->privkey) free(config->privkey);

    HASH_ITER(hh, config->localCache, current, tmp) {
      HASH_DEL(config->localCache, current);
      if (current->id) free(current->id);
      if (current->encrypted) free(current->encrypted);
      free(current);
    }

    free(config);
  }
}
