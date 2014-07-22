/**
 * File: signal.h
 * Author: Ethan Gordon
 * Simple stateles module to catch SIGINT to cleanup program.
 **/

#ifndef SIGNAL_H
#define SIGNAL_H

#include <stdbool.h>

/**
 * int Signal_init(void)
 * Specifically registers SIGINT to cleanup program false.
 * @return 0 on success, -1 on error.
 **/
int Signal_init(void);

#endif

