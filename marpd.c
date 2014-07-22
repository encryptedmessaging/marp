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
#include "frame.h"
#include "signal.h"
#include "network/socket.h"
#include "network/peers.h"
#include "data/cache.h"
#include "data/local.h"

/* Running Tracker (Used for Signal Handling) */
volatile bool isRunning;

/* Program name, for printing errors */
char* programName;

/* File Constants */
#define PORT 5001

int main(int argc, char** argv) {
  Frame_T frame = NULL;
  Socket_T socket = NULL;
  int error = 0;

  isRunning = true;

  /* Globalize program name. */
  programName = argv[0];

  /* Initialize Signals */
  error = Signal_init();
  if (error < 0) {
    fprintf(stderr, "%s: main: Could not initialize Signal Handler.", programName);
    return EXIT_FAILURE;
  }

  /* Initialize Local Configuration File */

  /* Initialize In-Memory Cache */

  /* Initialize Server UDP Socket */
  socket = Socket_init(PORT);
  if (socket == NULL) {
    fprintf(stderr, "%s: main: Could not initialize socket.", programName);
    return EXIT_FAILURE;
  }

  /* Main Loop */
  while (isRunning) {
    /* Create new frame */
    frame = Frame_init();
    
    if (frame == NULL) {
      break;
    }

    /* Listen for incoming query on socket */
    error = Frame_listen(frame, socket, 1);

    if (error || !isRunning) {
      Frame_free(frame);
    } else {
      error = Frame_respond(frame, socket);
      if (error) {
        Frame_free(frame);
      }
    }
  }

  printf("Exiting...\n");

  /* Destroy Server UDP Socket */
  Socket_free(socket);

  /* Destroy In-Memory Cache */

  /* Destroy Local Config File Data */

  return EXIT_SUCCESS;
}
