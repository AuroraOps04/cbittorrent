#include "message.h"
#include "bitfield.h"
#include "peer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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

int crate_bitfield_msg(unsigned char *bitfield, int bitfield_len, Peer *peer) {
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

/**
 * ox19 "BitTorrent protocol" + 8 + hash_info(20) + peer_id(20)
 */
// info_hash 不一致就关闭 ，否则保存 peerid 如果init状态 就发送握手消息， 如果
// HALFSHAKED 就设置成握手状态
int process_handshake_msg(Peer *peer, unsigned char *buf, int len) {
  if (peer == NULL || buf == NULL || len <= 0)
    return -1;

  // check info hash
  if (memcpy(info_hash, buf + 28, 20) != 0) {
    discard_send_buffer(peer);
    close(peer->socket);
    peer->state = CLOSING;
    return -1;
  }

  memcpy(peer->id, buf + 48, 20);
  peer->id[20] = '\0';
  if (peer->state == INITIAL) {
    // create handshake message
    create_handshake_msg(info_hash, peer_id, peer);
    peer->state = HANDSHAKED;
  } else if (peer->state == HALFSHAKED) {
    peer->state = HANDSHAKED;
  }

  peer->start_timestamp = time(NULL);
  return 0;
}
int process_keep_alive_msg(Peer *peer, unsigned char *buf, int len) {
  if (peer == NULL || buf == NULL)
    return -1;
  peer->start_timestamp = time(NULL);
  return 0;
}
int process_choke_msg(Peer *peer, unsigned char *buf, int len) {
  if (peer == NULL || buf == NULL)
    return -1;
  if (peer->state != CLOSING && peer->peer_choking == 0) {
    peer->peer_choking = 1;
    peer->last_down_timestamp = 0;
    peer->down_count = 0;
    peer->down_rate = 0;
  }
  peer->start_timestamp = time(NULL);
  return 0;
}
int process_unchoke_msg(Peer *peer, unsigned char *buf, int len) {
  if (peer == NULL || buf == NULL)
    return -1;
  if (peer->state != CLOSING && peer->peer_choking == 1) {
    peer->peer_choking = 0;
    if (peer->am_interested) {
      // TODO: create request slice msg
    } else {
      peer->am_interested = is_interested(&peer->bitmap, bitmap);
      if (peer->am_interested == 1) {
        // TODO:
      } else {
        printf("Recevied unchoke but Not Interested to IP:%s \n", peer->ip);
      }
    }
  }
  peer->start_timestamp = time(NULL);
  return 0;
}

int process_interested_msg(Peer *peer, unsigned char *buf, int len) {
  if (peer == NULL || buf == NULL) {
    return -1;
  }
  if (peer->state == DATA) {
    peer->peer_interested = is_interested(bitmap, &peer->bitmap);
    if (peer->peer_interested == 0) {

      return -1;
    }
    if (peer->am_choking == 0) {
      create_chock_interested_msg(UNCHOKE, peer);
    }
  }

  peer->start_timestamp = time(NULL);
  return 0;
}
int process_uninterested_msg(Peer *peer, unsigned char *buf, int len) {
  if (peer == NULL || buf == NULL)
    return -1;
  if (peer->state == DATA) {
    peer->peer_interested = 0;
    // 取消发送请求
    cancel_requested_list(peer);
  }
  peer->start_timestamp = time(NULL);
  return 0;
}

int process_have_msg(Peer *peer, unsigned char *buf, int len) {
  if (peer == NULL || buf == NULL)
    return -1;
  unsigned char c[4];
  if (peer->state == DATA) {
    c[0] = buf[5];
    c[1] = buf[6];
    c[2] = buf[7];
    c[3] = buf[8];
    if (peer->bitmap.bitfield != NULL) {
      set_bit_value(&peer->bitmap, char_to_int(c), 1);
      if (peer->am_interested == 0) {
        peer->am_interested = is_interested(&peer->bitmap, bitmap);
        if (peer->am_interested == 1)
          create_chock_interested_msg(INTERESTED, peer);
      }
    }
  }

  peer->start_timestamp = time(NULL);
  return 0;
}

int process_cancel_msg(Peer *peer, unsigned char *buf, int len) {
  if (buf == NULL || peer == NULL)
    return -1;
  unsigned char c[4];
  int index, begin, length;
  c[0] = buf[5];
  c[1] = buf[6];
  c[2] = buf[7];
  c[3] = buf[8];
  index = char_to_int(c);
  // TODO: 没做完

  peer->start_timestamp = time(NULL);
  return 0;
}
int process_bitfield_msg(Peer *peer, unsigned char *buf, int len) {
  if (peer == NULL || buf == NULL) {
    return -1;
  }
  unsigned char c[4];
  if (peer->state == HANDSHAKE || peer->state == SENDBITFIELD) {
    c[0] = buf[0];
    c[1] = buf[1];
    c[2] = buf[2];
    c[3] = buf[3];
    if (peer->bitmap.bitfield != NULL) {
      free(peer->bitmap.bitfield);
      peer->bitmap.bitfield = NULL;
    }
    peer->bitmap.valid_legnth = bitmap->valid_legnth;
    // -1 是减掉的 message id的大小 剩下的就是 bitfield 的长度
    if (bitmap->bitfield_length != char_to_int(c) - 1) {
      peer->state = CLOSING;
      discard_send_buffer(peer);
      // clear bt cache
      close(peer->socket);
      return -1;
    }
    peer->bitmap.bitfield_length = char_to_int(c) - 1;
    peer->bitmap.bitfield =
        (unsigned char *)malloc(peer->bitmap.bitfield_length);
    memcpy(peer->bitmap.bitfield, buf + 5, peer->bitmap.bitfield_length);

    if (peer->state == HANDSHAKE) {
      crate_bitfield_msg(bitmap->bitfield, bitmap->bitfield_length, peer);
      peer->state = DATA;
    } else if (peer->state == SENDBITFIELD) {
      peer->state = DATA;
    }
    peer->peer_interested = is_interested(bitmap, &peer->bitmap);
    peer->am_interested = is_interested(&peer->bitmap, bitmap);
    if (peer->am_interested == 1)
      create_chock_interested_msg(INTERESTED, peer);
  }

  peer->start_timestamp = time(NULL);
  return 0;
}
int process_request_msg(Peer *peer, unsigned char *buf, int len) {
  unsigned char c[4];
  int index, begin, length;
  Request_piece *request_piece, *p;
  if (peer == NULL || buf == NULL)
    return -1;

  if (peer->am_choking == 0 && peer->peer_interested == 1) {
    memcpy(c, buf + 5, 4);
    index = char_to_int(c);
    memcpy(c, buf + 9, 4);
    begin = char_to_int(c);
    memcpy(c, buf + 13, 4);
    length = char_to_int(c);
    p = peer->Requested_piece_head;
    while (p != NULL) {
      if (p->index == index && p->begin == begin && p->length == length) {
        break;
      }
      p = p->next;
    }
    if (p != NULL)
      return 0;
    request_piece = (Request_piece *)malloc(sizeof(Request_piece));
    if (request_piece == NULL) {
      printf("%s:%d error", __FILE__, __LINE__);
      return 0;
    }
    request_piece->index = index;
    request_piece->begin = begin;
    request_piece->length = length;
    request_piece->next = NULL;
    if (peer->Requested_piece_head == NULL) {
      peer->Requested_piece_head = request_piece;
    } else {
      p = peer->Requested_piece_head;
      while (p->next != NULL) {
        p = p->next;
      }
      p->next = request_piece;
    }
    printf("***add a request FROM IP:%s index:%-6d begin:%-6x ***\n", peer->ip,
           index, begin);
  }

  peer->start_timestamp = time(NULL);
  return 0;
}

int process_piece_msg(Peer *peer, unsigned char *buf, int len) {
  unsigned char c[4];
  int index, begin, length;
  Request_piece *p;
  if (peer == NULL || buf == NULL) {
    return -1;
  }
  memcpy(c, buf, 4);
  // 9 = id + index + begin
  length = char_to_int(c) - 9;
  memcpy(c, buf + 5, 4);
  index = char_to_int(c);
  memcpy(c, buf + 9, 4);
  begin = char_to_int(c);
  p = peer->Request_piece_head;
  while (p != NULL) {
    if (p->index == index && p->begin == begin && p->length == length) {
      break;
    }
    p = p->next;
  }
  // 不是我们请求过的数据
  if (p == NULL) {
    printf("did not found matched request\n");
    return -1;
  }
  // 开始计时，并累计收到的数据的字节数
  if (peer->last_down_timestamp == 0) {
    peer->last_down_timestamp = time(NULL);
  }
  peer->down_count += length;
  peer->down_total += length;
  // 将数据写入到缓冲区 TODO;
  // 继续请求下一个数据块
  // create_req_slice_msg(peer);
  peer->start_timestamp = time(NULL);
  return 0;
}

int parse_response(Peer *peer) {
  unsigned char btkeyword[20];
  unsigned char keep_alive[4] = {0x0, 0x0, 0x0, 0x0};
  int index;
  unsigned char *buf = peer->in_buf;
  int len = peer->buf_len;
  if (buf == NULL || peer == NULL)
    return -1;
  btkeyword[0] = 19;
  memcpy(btkeyword + 1, "BitTorrent protocol", 19);
  for (index = 0; index < len;) {
    if ((len - index >= 68) && (memcmp(btkeyword, buf + index, 20) == 0)) {
      process_handshake_msg(peer, buf + index, 68);
      index += 68;
    } else if ((len - index >= 4) &&
               (mempcpy(buf + index, keep_alive, 4) == 0)) {
      process_keep_alive_msg(peer, buf + index, 4);
      index += 4;
    } else if ((len - index >= 5) && (buf[index + 5] == CHOKE)) {
      process_choke_msg(peer, buf + index, 5);
      index += 5;
    } else if ((len - index >= 5) && (buf[index + 5] == UNCHOKE)) {
      process_unchoke_msg(peer, buf + index, 5);
      index += 5;
    } else if ((len - index >= 5) && (buf[index + 5] == INTERESTED)) {
      process_interested_msg(peer, buf + index, 5);
      index += 5;
    } else if ((len - index >= 5) && (buf[index + 5] == UNINTERESTED)) {
      process_uninterested_msg(peer, buf + index, 5);
      index += 5;
    } else if ((len - index >= 9) && (buf[index + 5] == HAVE)) {
      process_have_msg(peer, buf + index, 9);
      index += 9;
    } else if ((len - index >= 5) && (buf[index + 5] == BITFIELD)) {
      process_bitfield_msg(peer, buf + index, 5 + bitmap->bitfield_length);
      index += bitmap->bitfield_length + 5;
    } else if ((len - index >= 17) && (buf[index + 5] == REQUEST)) {
      process_request_msg(peer, buf + index, 17);
      index += 17;
    } else if ((len - index >= 9) && (buf[index + 5] == PIECE)) {
      unsigned char c[4];
      int length;
      memcpy(c, buf + index, 4);
      length = char_to_int(c) - 9;
      process_piece_msg(peer, buf + index, 13 + length);
      index += length + 13;
    } else if ((len - index >= 17) && (buf[index + 5] == CANCEL)) {
      process_cancel_msg(peer, buf + index, 17);
      index += 17;
    } else if ((len - index >= 7) && (buf[index + 5] == PORT)) {
      process_port_msg(peer, buf + index, 7);
      index += 7;
    } else {
      unsigned char c[4];
      int length;
      if (index + 4 <= len) {
        // 未知消息跳过
        memcpy(c, buf + index, 4);
        length = char_to_int(c);
        if (index + 4 + length <= len) {
          index += 4 + length;
          continue;
        }
      }
      // 错误消息 清空缓存区
      peer->buf_len = 0;
      return -1;

      break;
    }
  }

  return 0;
}

// 处理完整消息， 然后将剩余的消息拷贝到缓冲区的开始
int parse_response_uncomplete_msg(Peer *p, int ok_len) {
  char *tmp_buf;
  int tmp_buf_len;
  tmp_buf_len = p->buf_len - ok_len;
  tmp_buf = (char *)malloc(tmp_buf_len);
  if (tmp_buf == NULL) {
    printf("%s:%d error\n", __FILE__, __LINE__);
    return -1;
  }
  memcpy(tmp_buf, p->in_buf + ok_len, tmp_buf_len);
  p->buf_len = ok_len;
  // 处理完整消息
  parse_response(p);
  memcpy(p->in_buf, tmp_buf, tmp_buf_len);
  p->buf_len = tmp_buf_len;
  if (tmp_buf != NULL) {
    free(tmp_buf);
  }
  return 0;
}
/**
 *
 * @brief 当下载到一个完整的slice时，向所有peer发送have消息
 */
int prepare_send_have_msg() {
  Peer *p = peer_head;
  int i;
  if (peer_head == NULL)
    return -1;
  if (have_piece_index[0] == -1)
    return -1;
  while (p != NULL) {
    for (i = 0; i < 64; i++) {
      if (have_piece_index[i] == -1)
        break;
      create_have_msg(have_piece_index[i], p);
    }
    p = p->next;
  }
  for (i = 0; i < 64; i++) {
    if (have_piece_index[i] == -1)
      break;
    else
      have_piece_index[i] = -1;
  }

  return 0;
}

int create_response_message(Peer *peer) {
  if (peer == NULL)
    return -1;
  if (peer->state == INITIAL) {
    create_handshake_msg(info_hash, peer_id, peer);
    peer->state = HALFSHAKED;
    return 0;
  }
  if (peer->state == HANDSHAKED) {
    crate_bitfield_msg(bitmap->bitfield, bitmap->bitfield_length, peer);
    peer->state = SENDBITFIELD;
    return 0;
  }
  if (peer->am_choking == 0 && peer->Requested_piece_head != NULL) {
    Request_piece *p = peer->Requested_piece_head;
    // TODO:
    return 0;
  }
  time_t now = time(NULL);
  long interval = now - peer->start_timestamp;
  if (interval > 180) {
    peer->state = CLOSING;
    discard_send_buffer(peer);
    // TODO: 将从该peer处下载到的不足一个piece的数据删除
    close(peer->socket);
  }
  long interval2 = now - peer->recet_timestamp;
  // 如果超过45s没有发送消息和接收消息，并且发送缓存区为空，就发送一个keep-alive消息
  if (interval > 45 && interval2 > 45 && peer->msg_len == 0)
    create_keep_alive_msg(peer);
  return 0;
}
/**
 * @brief 丢弃socket发送缓冲区中的所有未发送消息
 */
void discard_send_buffer(Peer *peer) {
  struct linger lin;
  int lin_len;
  lin.l_onoff = 1;
  lin.l_linger = 0;
  lin_len = sizeof(lin);
  if (peer->socket > 0) {
    // 通过设置套接字选项，丢弃发送缓冲区中的数据
    setsockopt(peer->socket, SOL_SOCKET, SO_LINGER, &lin, lin_len);
  }
}
