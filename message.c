#include "message.h"
#include <string.h>

#define HANDSHAKE -2
#define KEEP_ALIVE -1
#define CHOKE 0
#define UNCHOKE 1
#define INTERESTED 2
#define UNINTERESTED 3
#define HAVE 4
#define BITFIELD 5
#define REQUEST 6
#define PIECE 7
#define CANCEL 8
#define PORT 9
// unit: seconds
#define KEEP_ALIVE_TIME 45

extern Bitmap *bitmap;
extern unsigned char info_hash[20];
extern char peer_id[21];
extern int have_piece_index[64]; // data.c中定义 存放下载到的piece的index
extern Peer *peer_head;
// 字节序： Endianness
// 大端字节序 高位放到低地址 也就是 c[0]; 大 也就是大头（高位）
// 低地址放大头（高位）就是大端字节序（BE)
// 低地址放小头（低位） 就是小端字节序（SE)
int int_to_char(int i, unsigned char *c) {
  c[3] = i % 256;
  c[2] = (i - c[3]) / 256 % 256;
  c[1] = (i - c[3] - c[2] * 256) / 256 / 256 % 256;
  c[0] = (i - c[3] - c[2] * 256 - c[1] * 256 * 256) / 256 / 256 / 256 % 256;
  return 0;
}
int char_to_int(unsigned char c[4]) {
  return c[3] + (c[2] << 8) + (c[1] << 16) + (c[0] << 24);
}

int create_keep_alive_msg(Peer *peer) {
  unsigned char *buf = peer->out_msg + peer->msg_len;
  int len = MSG_SIZE - peer->msg_len;
  if (len < 4)
    return -1;
  memset(buf, 0, 4);
  peer->msg_len += 4;
  return 0;
}

int create_chock_interested_msg(int type, Peer *peer) {
  unsigned char *buf = peer->out_msg + peer->msg_len;
  int len = MSG_SIZE - peer->msg_len;
  if (len < 5)
    return -1;
  memset(buf, 0, 5);
  buf[3] = 1;
  buf[4] = type;
  peer->msg_len += 5;
  return 0;
}

int create_have_msg(int index, Peer *peer) {

  unsigned char *buf = peer->out_msg + peer->msg_len;
  int len = MSG_SIZE - peer->msg_len;
  unsigned char c[4];
  if (len < 9)
    return -1;
  memset(buf, 0, 9);
  buf[3] = 5;
  buf[4] = 4;
  int_to_char(index, c);
  buf[5] = c[0];
  buf[6] = c[1];
  buf[7] = c[2];
  buf[8] = c[3];
  peer->msg_len += 9;
  return 0;
}
int is_complete_message(unsigned char *buff, unsigned int len, int *ok_len){

}

