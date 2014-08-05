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
#include <pthread.h>
#include <stdbool.h>

/* Local Files */
#include "frame.h"
#include "network/socket.h"

#define FRAME_MAX 512
#define LOCAL_VERSION 1
#define PEER_MAX 10

extern char* programName;

/* Holds Frame header and payload. */
struct header {
  uint32_t qid;
  uint8_t version;
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
} /* Frame_init() */

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
    free(buf);
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

static void* Frame_thread(void* arg);

struct threadarg {
  Socket_T socket;
  Frame_T frame;
};

/**
 * int Frame_respond(Frame_T, Socket_T)
 * Creates a new pthread in which to respond to the given Frame.
 * Note: The provided frame is automatically de-allocated.
 * @param frame: Must be an alread-filled frame.
 * @param socket: Must be an already connected or bound socket.
 * @return: A pointer to the pthread, or NULL on failure.
 **/
pthread_t* Frame_respond(Frame_T frame, Socket_T socket) {
  pthread_t *thread;
  int error;
  struct threadarg *arg;

  /* Sanity Check */
  if (frame == NULL) return NULL;
  if (socket == NULL) return NULL;
  if (frame->payload == NULL) return NULL;

  /* Version Check */
  if (frame->sHeader.version != LOCAL_VERSION) {
    fprintf(stderr, "%s: Version %d not supported by this server!", programName, frame->sHeader.version);
    return NULL;
  }
  
  /* Allocate and Start Thread */
  arg = calloc(1, sizeof(struct threadarg));
  if (arg == NULL) return NULL;
  arg->socket = socket;
  arg->frame = frame;

  thread = calloc(1, sizeof(pthread_t));
  if (thread == NULL) { free(arg); return NULL; }

  error = pthread_create(thread, NULL, Frame_thread, arg);
  if (error < 0) { free(thread); free(arg); return NULL; }
  
  return thread;
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
} /* End Frame_free() */

/******************************************************
 **************** The Meat of the Program *************
 ******************************************************/

/* Operation Codes */
enum {
  kSTD,
  kREV,
  kPER,
  kMAL,
  kNTF,
  kPNG
};

/**
 * void Frame_responseSTD(Frame_T, Frame_T)
 * @param frame: A standard query to parse
 * @param response: filled with the standard response
 * @return None
 **/
static void Frame_responseSTD(Frame_T frame, Frame_T response) {
  Query_T query;
  Response_T resp;
  const uint16_t* protocols;
  uint16_t* protocolCopy;
  const uint16_t* proto;
  bool found = false;
  uint8_t respHead[SHA256_SIZE + sizeof(uint16_t)];
  int error, count;

  count = error = 0;

  /* Parse Query */
  query = Query_init(frame->payload, frame->sHeader.length);
  if (query == NULL) {
    response->sHeader.op = kMAL;
    return;
  }
  protocols = Query_protocols(query);

  /* Create Response */
  memset(respHead, 0x00, SHA256_SIZE + sizeof(uint16_t));
  memcpy(respHead, Query_id(query), SHA256_SIZE);
  
  resp = Response_init(respHead, SHA256_SIZE + sizeof(uint16_t));
  if (resp == NULL) {
    response->sHeader.op = kMAL;
    Query_free(query);
    return;
  }

  /* First, Check Local Database for Results */
  for (proto = protocols; *proto != 0; proto++) {
    size_t len;
    const char* encrypted;
    
    encrypted = Local_get((char*)respHead, *proto, &len);
    if (encrypted != NULL) {
      error = Response_buildRecord(resp, *proto, encrypted, (uint16_t)len, Local_getTTL((char*)respHead, *proto));
      if (error < 0) {
        Response_free(resp);
        Query_free(query);
        response->sHeader.op = kNTF;
        return;
      }
      found = true;
    }
    count++;
  }

  if (found) {
    /* Found in Local Database! Sign and Return */
    response->sHeader.length = Response_size(resp);
    /*error = Response_sign(resp, Local_getPrivkey()); */
    response->payload = calloc(response->sHeader.length, sizeof(uint8_t));
    if (response->payload == NULL || error < 0) {
      if (response->payload) free(response->payload);
      response->payload = NULL;
      response->sHeader.length = 0;
      response->sHeader.op = kNTF;
    } else {
      response->sHeader.aa = 1;
      Response_serialize(resp, response->payload);
    }

    Response_free(resp);
    Query_free(query);
    return;
  }

  /* Only do cache check if client is okay with non-authoritative responses. */

  if (!response->sHeader.aa) {
    
    /* Make a Defensive Copy of Protocol List */
    protocolCopy = calloc(count, sizeof(uint16_t));
    memcpy(protocolCopy, protocols, count * sizeof(uint16_t));
    
    /* Check Cache */
    for (proto = protocolCopy; *proto != 0; proto++) {
      size_t len;
      const void* record = NULL;
      record = Cache_get((char*)respHead, *proto, &len);
      if (record != NULL) {
        Response_addRecord(resp, *proto, record);
        Query_rmProtocol(query, *proto);
      }
    }

    /* If we covered all the protocols, return! */
    protocols = Query_protocols(query);
    if (*protocols == 0) {
      response->sHeader.length = Response_size(resp);
      response->payload = calloc(response->sHeader.length, sizeof(uint8_t));
      if (response->payload == NULL || error < 0) {
        if (response->payload) free(response->payload);
        response->payload = NULL;
        response->sHeader.length = 0;
        response->sHeader.op = kNTF;
      } else {
        Response_serialize(resp, response->payload);
      }

      Response_free(resp);
      Query_free(query);
      return;
    }
  }

  /* If not in the Local File or the Cache, recurse the request if requested */
  if (frame->sHeader.rd && frame->sHeader.recurse) {
    uint8_t* recBuf;
    size_t newRespLen;
    Recursor_T recursor;

    frame->sHeader.recurse--;
    frame->sHeader.length = Query_size(query);
    
    
    /* Serialize and Recurse */
    recBuf = calloc(sizeof(struct header) + frame->sHeader.length, sizeof(uint8_t));
    if (recBuf == NULL) {
      Response_free(resp);
      Query_free(query);
      response->sHeader.op = kNTF;
      return;
    }
    memcpy(recBuf, &(frame->sHeader), sizeof(struct header));
    Query_serialize(query, recBuf + sizeof(struct header));

    recursor = Recursor_init(recBuf, sizeof(struct header) + frame->sHeader.length, PEER_MAX, frame->sHeader.recurse + 1);
    free(recBuf);
    if (recursor == NULL) {
      Response_free(resp);
      Query_free(query);
      response->sHeader.op = kNTF;
      return;
    }
    while((recBuf = (uint8_t*)Recursor_poll(recursor, &newRespLen)) != NULL) {
      Frame_T f;
      Response_T src;
      f = calloc(1, sizeof(struct frame));
      if (f == NULL) continue;
      memcpy(recBuf, &(f->sHeader), sizeof(struct header));
      f->payload = recBuf + sizeof(struct header);
      if (f->sHeader.op != kSTD || f->sHeader.z || f->sHeader.qid != frame->sHeader.qid) {
        free(f); continue;
      }
      
      src = Response_init(f->payload, f->sHeader.length);
      if (src == NULL) {
        free(f); continue;
      }

      Response_merge(resp, src);
    }
  }

  /* Serialize Response, Cleanup, and Return */
  response->sHeader.length = Response_size(resp);
  response->payload = calloc(response->sHeader.length, sizeof(uint8_t));
  if (response->payload == NULL || error < 0) {
    if (response->payload) free(response->payload);
    response->payload = NULL;
    response->sHeader.length = 0;
    response->sHeader.op = kNTF;
  } else {
    Response_serialize(resp, response->payload);
  }

  Response_free(resp);
  Query_free(query);
  return;
}

/**
 * void* Frame_thread(void*)
 * @param arg: Socket and Frame to respond to
 * @return a pointer to the integer return value. MUST BE FREED!
 **/
static void* Frame_thread(void* arg) {
  Socket_T socket;
  Frame_T frame;
  Frame_T response;
  int *ret;
  int bad = 0;
  void* resBuf;
  struct threadarg *args = arg;

  /* Fill Socket and Frame */
  assert(arg != NULL);
  socket = args->socket;
  frame = args->frame;
  ret = (int*)args;
  *ret = -1;

  /* Make Response Frame */
  response = malloc(sizeof(Frame_T));
  if (response == NULL) return ret;
  *response = *frame;
  response->sHeader.qr = 0; /* Response */
  response->sHeader.length = 0;
  response->payload = NULL;

  /* Validate Frame Header */
  if (frame->sHeader.z) bad = 1;
  if (!frame->sHeader.qr) bad = 1;
  if (bad) {
    response->sHeader.op = kMAL;
    *ret = Socket_respond(socket, &response->sHeader, sizeof(struct header));
    free(response);
    Frame_free(frame);
    Frame_free(response);
    return ret;
  }

  /* Op Code Mux */
  switch (frame->sHeader.op) {
  case kSTD: /* Standard Query */
    Frame_responseSTD(frame, response);
  case kREV: /* TODO: Currently Unsupported */
    response->sHeader.op = kNTF;
  case kPER:
    break;
  case kPNG:
    break;
  default:
    response->sHeader.op = kMAL;
  }

  /* Serialize and Send Response */
  resBuf = calloc(sizeof(struct header) + response->sHeader.length, sizeof(uint8_t));
  if (resBuf == NULL) {
    response->sHeader.op = kNTF;
    *ret = Socket_respond(socket, &response->sHeader, sizeof(struct header));
  } else {
    *ret = Socket_respond(socket, resBuf, sizeof(struct header) + response->sHeader.length);
  }

  Frame_free(frame);
  Frame_free(response);
  return ret;
} /* End Frame_thread() */
