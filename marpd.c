/**
 * File: marp.c
 * Author: Ethan Gordon
 * Entry point for the entire application.
 **/


/* Standard Libraries */
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

/* Local Files */
#include "uthash.h"
#include "frame.h"
#include "signal.h"
#include "network/socket.h"
#include "network/peers.h"
#include "data/cache.h"
#include "data/local.h"

struct thread_container {
  int id;
  pthread_t *thread;

  UT_hash_handle hh;
};

/* Running Tracker (Used for Signal Handling) */
volatile bool isRunning;

/* Program name, for printing errors */
char* programName;

/* File Constants */
#define PORT 5001
#define MAX_THREAD 10

int main(int argc, char** argv) {
  Frame_T frame = NULL;
  Socket_T socket = NULL;
  int error = 0;
  int count = 0;
  struct thread_container *head, *current, *tmp;
  head = NULL;

  isRunning = true;

  /* Globalize program name. */
  programName = argv[0];
  

  /* Initialize Signals */
  error = Signal_init();
  if (error < 0) {
    fprintf(stderr, "%s: main: Could not initialize Signal Handler.\n", programName);
    return EXIT_FAILURE;
  }
  printf("%s: main: Initialized Signals...\n", programName);

  /* Initialize Local Configuration File */
  error = Local_init("config/marp.conf");
  if (error < 0) {
    fprintf(stderr, "%s: main: Could not initialize local config.\n", programName);
    return EXIT_FAILURE;
  }
  printf("%s: main: Config File Parsed...\n", programName);

  /* Initialize In-Memory Cache */
  error = Cache_load("config/cache.dat");
  if (error < 0) {
    fprintf(stderr, "%s: main: Could not initialize local cache.\n", programName);
    return EXIT_FAILURE;
  }
  printf("%s: main: Loaded %d cache entries from config/cache.dat...\n", programName, error);

  /* Initialize Server UDP Socket */
  socket = Socket_init(PORT);
  if (socket == NULL) {
    fprintf(stderr, "%s: main: Could not initialize socket.\n", programName);
    return EXIT_FAILURE;
  }
  printf("%s: main: Server started on port %d...\n\n", programName, PORT);

  fflush(stdout);

  /* Main Loop */
  while (isRunning) {
    /* Create new frame */
    frame = Frame_init();
    
    if (frame == NULL) {
      break;
    }

    /* Listen for incoming query on socket */
    error = Frame_listen(frame, socket, 1);

    if (error < 0 || !isRunning) {
      Frame_free(frame);
    } else {
      /* Prepare Thread Pool */
      printf("%s: main: Received new query...\n", programName);
      current = calloc(1, sizeof(struct thread_container));
      if (current == NULL || !isRunning) {
        fprintf(stderr, "%s: Out of Memory\n", programName);
        Frame_free(frame);
        break;
      }
      
      /* Block if Max Threads Reached */
      HASH_FIND_INT(head, &count, tmp);
      if (tmp != NULL) {
        int *response;
        pthread_join(*(tmp->thread), (void**)&response);
        free(response);
        HASH_DEL(head, tmp);
        free(tmp->thread);
        free(tmp);
      } 
      
      /* Launch Response Thread */
      current->thread = Frame_respond(frame, socket);

      if (current->thread == NULL) {
        fprintf(stderr,"%s: main: Error starting new thread for frame.\n", programName);
        free(current);
        Frame_free(frame);
      } else {
        /* Add Thread to Thread Pool */
        current->id = count;
        HASH_ADD_INT(head, id, current);
      }
      putchar('\n');
    }
    count++;
    count %= MAX_THREAD;
  }

  /* Destroy Thread Pool */
  printf("%s: main: Waiting for threads to exit...\n", programName);
  HASH_ITER(hh, head, current, tmp) {
    int* response;
    HASH_DEL(head, current);
    pthread_join(*(current->thread), (void**)&response);
    free(current->thread);
    free(current);
  }

  /* Destroy Server UDP Socket */
  Socket_free(socket);

  /* Destroy In-Memory Cache */
  error = Cache_dump("config/cache.dat");
  if (error < 0)
    fprintf(stderr, "%s: main: Cache dump to file config/cache.dat failed!", programName);
  else printf("%s: main: Dumped %d records to cache file config/cache.dat...\n", programName, error);

  Cache_destroy();

  /* Destroy Local Config File Data */
  Local_destroy();

  printf("%s: Exiting...\n", programName);
  return EXIT_SUCCESS;
}
