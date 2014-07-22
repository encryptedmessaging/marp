/**
 * File: signal.c
 * Author: Ethan Gordon
 * Simple stateles module to catch SIGINT to cleanup program.
 **/

#define _GNU_SOURCE

#include <stdbool.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

extern char* programName;
extern volatile bool isRunning;

/* Signal Handler Function */
static void handler(int signal) {
  isRunning = false;
}

#define FAILURE -1

/**
 * int Signal_init(void)
 * Specifically registers SIGINT to cleanup program false.
 * @return 0 on success, negative on error.
 **/
int Signal_init(void) {
  int error;
  void (*signalRet)(int);
  sigset_t sSet;

  /* Ensure signal is unblocked */
  sigemptyset(&sSet);
  sigaddset(&sSet, SIGINT);
  ret = sigprocmask(SIG_UNBLOCK, &sSet, NULL); 

  if (error) {
    perror(programName);
    return error;
  }

  /* Register signal handler. */
  signalRet = signal(SIGINT, handler);
  if (signalRet == SIG_ERR) {
    perror(programName);
    return -1;
  }

  return 0;
}
