/**
 * File: mlookup.c
 * Author: Ethan Gordon
 * Simple CLI MARP Client
 * Usage: mlookup <handle@host> <protocol> [<server>]
 **/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Local Files */
#include "../network/socket.h"
#include "../frame.h"
#include "../object/query.h"
#include "../object/response.h"
#include "../libsha2/sha256.h"

/* External Required Variables */
char* programName;
bool isRunning = true;

/* Network Information */
#define DEFAULT_PORT 5001
#define LOCALHOST "127.0.0.1"
#define DEFAULT_TIMEOUT 1

static void printUsage(void) {
  fprintf(stderr, "Usage: mlookup <handle@host> <protocol#> [<server>]\n");
}

int main(int argc, char** argv) {
  Query_T query;
  Frame_T frame;
  Socket_T socket;
  Response_T response;
  uint16_t protocol;
  int error, querySize;
  uint16_t respLen;
  void* byteBuf;
  char key[SHA256_SIZE];
  const char *handleAtHost, *server;

  /* Parse Command Line Arguments */
  programName = argv[0];

  if (argc < 3 || argc > 4) {
    printUsage();
    exit(EXIT_FAILURE);
  }

  handleAtHost = argv[1];
  protocol = (uint16_t)atoi(argv[2]);
  if (argc == 4) server = argv[3];
  else server = LOCALHOST;

  sha256_simple((const uint8_t*)handleAtHost, strlen(handleAtHost), (uint8_t*)key);

  /* Construct Request Frame */
  socket = Socket_init(0); /* Unbound Socket */
  if (socket == NULL) return EXIT_FAILURE;

  query = Query_build(handleAtHost);
  if (query == NULL) { 
    fprintf(stderr, "%s: Could not build query.\n", programName);
    Socket_free(socket);
    return EXIT_FAILURE; 
  }

  error = Query_addProtocol(query, protocol);
  if (error < 0) {
    fprintf(stderr, "%s: Invalid protocol %d\n", programName, protocol);
    Socket_free(socket);
    Query_free(query);
    return EXIT_FAILURE;
  }

  querySize = Query_size(query);
  byteBuf = calloc(querySize, sizeof(char));
  if (byteBuf == NULL) {
    fprintf(stderr, "%s: Out of Memory", programName);
    Socket_free(socket);
    Query_free(query);
    return EXIT_FAILURE;
  }
  Query_serialize(query, byteBuf);

  frame = Frame_buildQuery(1, 0, byteBuf, querySize);
  if (frame == NULL) {
    fprintf(stderr, "%s: Could not build query frame.\n", programName);
    Socket_free(socket);
    Query_free(query);
    free(byteBuf);
    return EXIT_FAILURE;
  }

  error = Frame_send(frame, socket, server, DEFAULT_PORT);
  if (error < 0) {
    fprintf(stderr, "%s: Frame_send: Error sending frame\n", programName);
    Socket_free(socket);
    Frame_free(frame);
    Query_free(query);
    free(byteBuf);
    return EXIT_FAILURE;
  }

  /* Clear Frame For Response */
  Frame_free(frame);
  frame = Frame_init();
  if (frame == NULL) {
    Query_free(query);
    free(byteBuf);
    Socket_free(socket);
    return EXIT_FAILURE;
  }

  /* Wait for Response */
  error = Frame_listen(frame, socket, DEFAULT_TIMEOUT);
  if (error < 0) {
    fprintf(stderr, "%s: Frame_listen: Error receiving response, timeout reached.\n", programName);
    Socket_free(socket);
    Frame_free(frame);
    Query_free(query);
    free(byteBuf);
    return EXIT_FAILURE;
  }

  free(byteBuf);
  byteBuf = NULL;
  
  /* Print Response */
  Frame_printInfo(frame);
  byteBuf = (uint8_t*)Frame_getPayload(frame, &respLen);

  if (respLen > 0) {
    response = Response_init(byteBuf, respLen);
    if (response == NULL) {
      Socket_free(socket);
      Frame_free(frame);
      Query_free(query);
      return EXIT_FAILURE;
    }
    Response_printDecrypted(response, key);
  }

  Socket_free(socket);
  Frame_free(frame);
  Query_free(query);
  return EXIT_SUCCESS;
}
