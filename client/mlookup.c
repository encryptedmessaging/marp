/**
 * File: mlookup.c
 * Author: Ethan Gordon
 * Simple CLI MARP Client
 * Usage: mlookup <handle@host> <protocol> [<server>]
 **/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* External Required Variables */
char* programName;
bool isRunning = true;

static void printUsage(void) {
  fprintf(stderr, "Usage: mlookup <handle@host> <protocol#> [<server>]\n");
}

int main(int argc, char** argv) {
  /* Query_T query;
     Frame_T frame; */
  programName = argv[0];

  if (argc < 3 || argc > 4) {
    printUsage();
    exit(EXIT_FAILURE);
  }

  

  return EXIT_SUCCESS;
}
