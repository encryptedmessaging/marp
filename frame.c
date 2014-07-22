/**
 * File: frame.c
 * Author: Ethan Gordon
 * Owns the header of all MARP packets.
 **/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

/* Local Files */
#include "frame.h"
#include "network/socket.h"

#define FRAME_MAX 512

extern char* programName;

/* Holds Frame header and payload. */
struct header {
  uint8_t version;
  uint32_t qid;
  unsigned int qr:1;
  unsigned int op:3;
  unsigned int aa:1;
  unsigned int rd:1;
  unsigned int  z:2;
  uint8_t recurse;
  uint16_t length;
};

struct frame {
  struct header sHeader;
  uint8_t* payload;
};

/**
 * Frame_T Frame_init(void)
 * Allocates a new Frame in the heap. 
 * @param: None
 * @return: An empty Frame or NULL on error, with errno set.
 **/
Frame_T Frame_init(void) {
  Frame_T ret;
  ret = calloc(1, sizeof(struct frame));
  return ret;
}

/**
 * int Frame_listen(Frame_T, Socket_T)
 * Block until a new frame is received via the socket over the network (or timeout is reached).
 * @param dest: Frame to fill, all contents will be overwritten
 * @param socket: Where to listen for data. Must not be NULL.
 * @param timeout: How long to wait, set to 0 to disable.
 * @return: bytes written on Success, negative on Failure or Timeout
 **/
int Frame_listen(Frame_T dest, Socket_T socket, int timeout) {
  int error;
  uint8_t *buf, *payload;

  assert(dest != NULL);
  assert(socket != NULL);

  if (timeout < 0) return timeout;

  if (dest->payload) free(dest->payload);
  buf = calloc(FRAME_MAX, sizeof(uint8_t));
  if (buf == NULL) {
    return -1;
}

  error = Socket_read(socket, (void*)buf, FRAME_MAX, timeout);
  if (error < 0) {
    return error;
  } else if (error < sizeof(dest->sHeader)) {
    fprintf(stderr, "%s: Received too small frame from socket.", programName);
  }

  dest->payload = calloc(error - sizeof(dest->sHeader), sizeof(uint8_t));
  if (dest->payload == NULL) {
    free(buf);
    return -1;
  }
  
  /* Fill Frame */
  memcpy(buf, &(dest->sHeader), sizeof(dest->sHeader));
  payload = buf + sizeof(dest->sHeader);
  memcpy(payload, &(dest->payload), error - sizeof(dest->sHeader));

  free(buf);

  return error;
} /* End Frame_listen() */

/**
 * int Frame_respond(Frame_T, Socket_T)
 * Creates a new pthread in which to respond to the given Frame.
 * Note: The provided frame is automatically de-allocated.
 * @param frame: Must be an alread-filled frame.
 * @param socket: Must be an already connected or bound socket.
 * @return: The pthread identification number, or -1 on failure.
 **/
int Frame_respond(Frame_T frame, Socket_T socket) {
  return EXIT_SUCCESS;
} /* End Frame_respond() */

/**
 * void Frame_free(Frame_T)
 * @param frame: Frame to completely clean up and de-allocate.
 * @return: None
 **/
void Frame_free(Frame_T frame) {
  if (frame == NULL) return;

  if (frame->payload) free(frame->payload);
  free(frame);
}

