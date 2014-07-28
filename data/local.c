/**
 * File: local.h
 * Author: Ethan Gordon
 * Store and access data from the local .marp configuration file.
 **/
#define SHA256_SIZE 32

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "../uthash.h"
#include "../util/libsha2/sha256.h"
#include "local.h"

/* Individual Local Entry, Placed in Hash Table */
typedef struct entry {
  /* Concatonated SHA256('handle@host') + protocol */
  char* id;

  /* Plaintext Address */
  const char* plaintext;

  /* TTL either from section or host */
  int ttl;

  UT_hash_handle hh; 
} entry;

/* Local Structure, the basis for the AO */
struct local {
  /* Local HashTable Head, should be NULL */
  entry* localCache;
  /* Base64-Encoded Public Key */
  char* pubkey;
  /* Base64-Encoded Private Key */
  char* privkey;
};

/* Persistent Structure to Pass Along the Host Handlers */
struct sHost {
  /* Host name as a C-string */
  const char* host;
  /* TTL for entire hostname, used if section has no TTL entry */
  int hostTTL;
  /* Name of the current section (handle) */
  const char* currentSection;
  /* Name of the Section TTL (good until the next TTL or next Section */
  int sectionTTL;
};

typedef struct local *Local_T;

/* AO Object */
static Local_T config = NULL;

/* Array of Protocols, Max 255 for now */
#define PROTO_MAX 255
static char** protocols;

/* argv[0] from main */
extern char* programName;

/** Handlers **/
static int hostHandler(void* hostPtr, const char* section, const char* name, const char* value) {
  uint16_t protocol;
  char* handleAtHost;
  char* id;
  entry* newEntry;
  entry* dummy;
  struct sHost* host = (struct sHost*)hostPtr;

  /* Check for Global Section */
  if (strcmp(section, "global") == 0) {
    if (strcmp(name, "TTL") == 0)
      host->hostTTL = atoi(value); 
    return 0;
  }

  /* Clear TTL on Section Change */
  if (strcmp(host->currentSection, section) != 0) {
    host->currentSection = section;
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
  for (protocol = 0; protocol < sizeof(uint8_t); protocol++) {
    if (strcmp(name, protocols[protocol]) == 0) break;
  }

  if (protocol == sizeof(uint8_t)) {
    fprintf(stderr, "%s: Protocols %s not supported yet.", programName, name);
    free(id); return -1;
  }
  
  /* Create ID */
  handleAtHost = calloc(strlen(section) + strlen(name) + 1, sizeof(char));
  if (handleAtHost == NULL) {
    free(id); return -1;
  }

  (void)strcat(handleAtHost, section);
  (void)strcat(handleAtHost, "@");
  (void)strcat(handleAtHost, host->host);

  sha256_simple((uint8_t*)handleAtHost, strlen(handleAtHost), id);
  memcpy(&(id[SHA256_SIZE]), &protocol, sizeof(uint16_t));
  free(handleAtHost);

  /* Build Entry */
  newEntry = calloc(1, sizeof(entry));
  if (newEntry == NULL) {
    free(id); return -1;
  }

  newEntry->id = id;
  newEntry->plaintext = value;
  if (host->sectionTTL)
    newEntry->ttl = host->sectionTTL;
  else newEntry->ttl = host->hostTTL;

  /* Store Entry in Hash Table */
  HASH_REPLACE(hh, config->localCache, id[0], sizeof(entry), newEntry, dummy);
  if (dummy != NULL) {
    free(dummy->id);
    free(dummy);
  }

  return 0;
}
 
static int nameHandler(void* nada, const char* section, const char* name, const char* value) {
  int protocol;
  /* Only "name" section allowed */
  if (strcmp(section, "name") != 0) return -1;
  
  protocol = atoi(value);
  if (protocol > sizeof(uint8_t)) {
    fprintf(stderr, "%s: Protocol #%s not supported yet.", programName, value);
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
  if (strcmp(section, "global")) {
    if (strcmp(name, "pubkey"))
      if (config->pubkey) {
        return -1;
      } else {
        config->pubkey = calloc(strlen(value) + 1, sizeof(char));
        if (config->pubkey == NULL) return -1;
        strcpy(config->pubkey, value);
      }
    if (strcmp(name, "privkey"))
      if (config->privkey) {
        return -1;
      } else {
        config->privkey = calloc(strlen(value) +1, sizeof(char));
        if (config->privkey == NULL) return -1;
        strcpy(config->privkey, value);
      }
    /* Names Configuration */
    if (strcmp(name, "names") == 0)
      if (ini_parse(value, nameHandler, NULL) < 0) {
        fprintf(stderr, "%s: Error parsing names file %s: %s", programName, value, strerror(errno));
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

  /* Host Configuration */
  host.host = section;
  host.currentSection = "";
  host.hostTTL = 0;
  host.sectionTTL = 0;
  
  /* Not Fatal */
  if (strcmp(name, "include") != 0) return 0;
 
  /* Not Fatal if Falure */
  if (ini_parse(value, hostHandler, &host) < 0) {
    fprintf(stderr, "%s: Error parsing host file %s: %s", programName, value, strerror(errno));
    fprintf(stderr, "Already-Parsed entries are still in local cache.");
  }

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
  protocols = calloc(PROTO_MAX, sizeof(char*));
  if (protocols == NULL) {
    free(config); return -1;
  }

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
const char* Local_get(char hash[SHA256_SIZE], uint16_t protocol) {
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

  return record->plaintext;
}

/**
 * char* Local_getPubkey(void)
 * @return Plaintext, base64-encoded ECC public key for this server.
 **/
const char* Local_getPubkey(void) {
  return config->pubkey;
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
    if (config->pubkey) free(config->pubkey);
    if (config->privkey) free(config->privkey);

    HASH_ITER(hh, config->localCache, current, tmp) {
      HASH_DEL(config->localCache, current);
      if (current->id) free(current->id);
      free(current);
    }

    free(config);
  }
}
