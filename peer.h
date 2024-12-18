#ifndef PEER_H
#define PEER_H
#include "bitfield.h"
#include <time.h>
#define INITIAL -1 // 初始化状态
#define HALFSHAKED 0
#define HANDSHAKED 1
#define SENDBITFIELD 2
#define RECVBITFIELD 3
#define DATA 4
#define CLOSING 5
// 发送和接收缓冲区的大小， 16k 放一个 slice， 2k 用来存放其他消息
#define MSG_SIZE (2 * 1024 + 16 * 1024)
// slice for piece
typedef struct _Request_piece {
  int index;  // piece index
  int begin;  // piece offset
  int length; // request length, 16kb
  struct _Request_piece *next;
} Request_piece;

typedef struct _Peer {
  int socket; // socket fd
  char ip[16];
  unsigned short port;
  char id[21]; // peer_id

  int state;           // current status
  int am_choking;      // 是否将peer阻塞
  int am_interested;   // 是否对peer感兴趣
  int peer_choking;    // 是否被peer阻塞
  int peer_interested; // 是否被peer感兴趣

  Bitmap bitmap;

  unsigned char *in_buff; // 从peer处获取的消息
  int buf_len;
  unsigned char *out_msg; // 发送给peer的消息
  int msg_len;
  unsigned char *out_msg_copy; // out_msg副本 发送时使用该缓冲区
  int msg_copy_len;            // 长度
  int msg_copy_index;          // 偏移量

  Request_piece *Request_piece_head;   // 向 peer请求数据的队列
  Request_piece *Requested_piece_head; // 被 peer 请求数据的队列

  unsigned int down_total; // 从该peer下载的总字节数
  unsigned int up_total;   // 向该 peer上传的总字节数

  time_t start_timestamp; // 最近一次接收到 peer 消息的时间
  time_t recet_timestamp; // 最近一次发送消息给peer的时间
  time_t last_down_timestamp;
  time_t last_up_timestamp;
  long long down_count; // 本计时周期 从 peer 下载的字节总数
  long long up_count;   // 本计时周期 向peer 上传的字节总数
  float down_rate;      // 本计时周期 从 peer 下载的速度
  float up_rate;        // 本计时周期 向 peer 上传的速度

  struct _Peer *next;
} Peer;

int initialize_peer(Peer *peer);
Peer *add_peer_node();
int del_peer_node();
void free_peer_node(Peer *p);
int cancel_request_list(Peer *peer);
int cancel_requested_list(Peer *peer);
void release_memory_in_peer();
void print_peers_data();

#endif // !PEER_H
