/**
 * File: frame.c
 * Author: Ethan Gordon
 * Owns the header of all MARP packets.
 **/

#include <stdint.h>
#include <stdlib.h>

/* Local Files */
#include "frame.h"


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
  void* payload;
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
 * Block until a new frame is received via the socket over the network.
 * @param dest: Frame to fill, all contents will be overwritten
 * @param socket: Where to listen for data. Must not be NULL.
 * @param timeout: How long to wait, set to 0 to disable.
 * @return: 0 on Success, -1 on Failure or Timeout
 **/
int Frame_listen(Frame_T dest, Socket_T socket, int timeout);

/**
 * int Frame_respond(Frame_T, Socket_T)
 * Creates a new pthread in which to respond to the given Frame.
 * Note: The provided frame is automatically de-allocated.
 * @param frame: Must be an alread-filled frame.
 * @param socket: Must be an already connected or bound socket.
 * @return: The pthread identification number, or -1 on failure.
 **/
int Frame_respond(Frame_T frame, Socket_T socket);

/**
 * void Frame_free(Frame_T)
 * @param frame: Frame to completely clean up and de-allocate.
 * @return: None
 **/
void Frame_free(Frame_T frame);

