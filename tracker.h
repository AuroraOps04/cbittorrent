#ifndef TRACKER_H
#define TRACKER_H
typedef struct _Peer_list {
  int port;
  char* ip;
  struct _Peer_list *next;
} Peer_list;
int connect_tracker();
void release_tracker_memory();

#endif // !TRACKER_H
