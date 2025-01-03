#include "peer.h"
#include "bitfield.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Bitmap *bitmap;

Peer *peer_head = NULL;

int initialize_peer(Peer *peer) {
  if (peer == NULL)
    return -1;

  peer->socket = -1;
  memset(peer->ip, 0, 16);
  peer->port = 0;
  memset(peer->id, 0, 21);
  peer->state = INITIAL;

  peer->in_buff = NULL;
  peer->out_msg = NULL;
  peer->out_msg_copy = NULL;

  peer->in_buff = (unsigned char *)malloc(MSG_SIZE);
  if (peer->in_buff == NULL)
    goto OUT;
  memset(peer->in_buff, 0, MSG_SIZE);
  peer->buf_len = 0;

  peer->out_msg = (unsigned char *)malloc(MSG_SIZE);
  if (peer->out_msg == NULL)
    goto OUT;
  memset(peer->out_msg, 0, MSG_SIZE);
  peer->msg_len = 0;

  peer->out_msg_copy = (unsigned char *)malloc(MSG_SIZE);
  if (peer->out_msg_copy == NULL)
    goto OUT;
  memset(peer->out_msg_copy, 0, MSG_SIZE);
  peer->msg_copy_len = 0;
  peer->msg_copy_index = 0;

  peer->Request_piece_head = NULL;
  peer->Requested_piece_head = NULL;

  peer->down_total = 0;
  peer->up_total = 0;

  peer->start_timestamp = 0;
  peer->recet_timestamp = 0;

  peer->last_down_timestamp = 0;
  peer->last_up_timestamp = 0;
  peer->down_count = 0;
  peer->up_count = 0;
  peer->down_rate = 0.0;
  peer->up_rate = 0.0;

  peer->next = (Peer *)0;
  return 0;
OUT:
  if (peer->in_buff != NULL)
    free(peer->in_buff);
  if (peer->out_msg != NULL)
    free(peer->out_msg);
  if (peer->out_msg_copy != NULL)
    free(peer->out_msg_copy);
  return -1;
}

Peer *add_peer_node() {
  int ret;
  Peer *node, *p;
  node = (Peer *)malloc(sizeof(Peer));
  if (node == NULL) {
    printf("%s:%d error\n", __FILE__, __LINE__);
    return NULL;
  }
  ret = initialize_peer(node);
  if (ret < 0) {
    printf("%s:%d error\n", __FILE__, __LINE__);
    free(node);
    return NULL;
  }
  if (peer_head == NULL)
    peer_head = node;
  else {
    p = peer_head;
    while (p->next != NULL)
      p = p->next;
    p->next = node;
  }
  return node;
}

int del_peeer_node(Peer *peer) {
  Peer *p = peer_head, *q;
  if (peer == NULL)
    return -1;

  while (p != NULL) {
    if (p == peer) {
      if (p == peer_head) {
        peer_head = p->next;
      } else {
        q->next = p->next;
      }
      free_peer_node(p);
      return 0;
    } else {
      q = p;
      p = p->next;
    }
  }
  return -1;
}

void free_peer_node(Peer *p) {
  if (p != NULL) {
    if (p->in_buff != NULL)
      free(p->in_buff);
    if (p->out_msg != NULL)
      free(p->out_msg);
    if (p->out_msg_copy != NULL)
      free(p->out_msg_copy);
    free(p);
  }
}

int cancel_request_list(Peer *peer) {
  if (peer == NULL)
    return -1;
  Request_piece *p = peer->Request_piece_head, *q;
  while (p != NULL) {
    q = p->next;
    free(p);
    p = q;
  }
  return 0;
}

int cancel_requested_list(Peer *peer) {
  if (peer == NULL)
    return -1;
  Request_piece *p = peer->Requested_piece_head, *q;
  while (p != NULL) {
    q = p->next;
    free(p);
    p = q;
  }
  return 0;
}

void release_memory_in_peer() {
  Peer *node;
  while (peer_head != NULL) {
    node = peer_head->next;
    free_peer_node(peer_head);
    peer_head = node;
  }
}
