#include "message.h"
#include "bitfield.h"
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
  buf[4] = HAVE;
  int_to_char(index, c);
  buf[5] = c[0];
  buf[6] = c[1];
  buf[7] = c[2];
  buf[8] = c[3];
  peer->msg_len += 9;
  return 0;
}

int crate_bitfield_msg(char *bitfield, int bitfield_len, Peer *peer) {
  if (bitfield == NULL)
    return -1;
  unsigned char *buf = peer->out_msg + peer->msg_len;
  int len = MSG_SIZE - peer->msg_len;
  int bit_msg_len = 4 + 1 + bitfield_len;
  if (len < bit_msg_len)
    return -1;
  memset(buf, 0, bit_msg_len);
  int_to_char(bit_msg_len - 4, buf);
  // id
  buf[4] = BITFIELD;
  memcpy(&buf[5], bitfield, bitfield_len);
  peer->msg_len += bit_msg_len;
  return 0;
}

int create_request_msg(int index, int begin, int length, Peer *peer) {
  unsigned char *buf = peer->out_msg + peer->msg_len;
  int len = MSG_SIZE - peer->msg_len;
  if (len < 17)
    return -1;
  memset(buf, 0, 17);
  buf[3] = 13;
  buf[4] = REQUEST;
  int_to_char(index, &buf[5]);
  int_to_char(begin, &buf[9]);
  int_to_char(length, &buf[13]);
  peer->msg_len += 17;
  return 0;
}
int create_piece_msg(int index, int begin, unsigned char *block, int b_len,
                     Peer *peer) {
  unsigned char *buf = peer->out_msg + peer->msg_len;
  int len = MSG_SIZE - peer->msg_len;
  int piece_msg_len = 4 + 9 + b_len;
  if (len < piece_msg_len)
    return -1;
  int_to_char(piece_msg_len - 4, buf);
  buf[4] = PIECE;
  int_to_char(index, &buf[5]);
  int_to_char(index, &buf[9]);
  memcpy(&buf[13], block, b_len);
  peer->msg_len += piece_msg_len;
  return 0;
}
int create_cancel_msg(int index, int begin, int length, Peer *peer) {
  unsigned char *buf = peer->out_msg + peer->msg_len;
  int len = MSG_SIZE - peer->msg_len;
  if (len < 17)
    return -1;
  memset(buf, 0, 17);
  buf[3] = 13;
  buf[4] = CANCEL;
  int_to_char(index, &buf[5]);
  int_to_char(begin, &buf[9]);
  int_to_char(length, &buf[13]);
  peer->msg_len += 17;

  return 0;
}
int create_port_msg(int port, Peer *peer) {
  unsigned char *buf = peer->out_msg + peer->msg_len;
  int len = MSG_SIZE - peer->msg_len;
  if (len < 7)
    return -1;
  memset(buf, 0, 7);
  buf[3] = 3;
  buf[4] = PORT;
  buf[5] = (port >> 8) % 256;
  buf[6] = port % 256;
  return 0;
}

int is_complete_message(unsigned char *buff, unsigned int len, int *ok_len) {
  if (buff == NULL)
    return -1;

  if (len < 4)
    return -1;

  unsigned char c[4];
  memcpy(buff, c, 4);

  // 4 是 length prefix 的长度
  *ok_len = char_to_int(c) + 4;
  if (ok_len < 0)
    return -1;

  if (len < *ok_len) {
    return -1;
  }

  return 0;
}
// info_hash 不一致就关闭 ，否则保存 peerid 如果init状态 就发送握手消息， 如果
// HALFSHAKED 就设置成握手状态
int process_handshake_msg(Peer *peer, unsigned char *buf, int len) { return 0; }

int parse_response(Peer *peer) { return 0; }
