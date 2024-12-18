#ifndef MESSAGE_H
#define MESSAGE_H

#include "peer.h"
int int_to_char(int i, unsigned char c[4]);
int char_to_int(unsigned char c[4]);

int create_handshake_msg(char *info_hash, char *peer_id, Peer *peer);
int create_keep_alive_msg(Peer *peer);
int create_chock_interested_msg(int type, Peer *peer);
int create_have_msg(int index, Peer *peer);
int crate_bitfield_msg(char *bitfield, int bitfield_len, Peer *peer);
int create_request_msg(int index, int begin, int length, Peer *peer);
int create_piece_msg(int index, int begin, unsigned char *block, int b_len,
                     Peer *peer);
int create_cancel_msg(int index, int begin, int length, Peer *peer);
int create_port_msg(int port, Peer *peer);

/**
 * @brief  判断接收缓冲区内是否存放了一条完整的消息
 *
 * @param buff
 * @param ok_len 需要设置的第一条完整消息长度
 *
 * @return 有完整消息返回 0 ，否则返回 -1
 */
int is_complete_message(unsigned char *buff, unsigned int len, int *ok_len);
// 处理收到的消息， 接收缓冲区内存放着一条完整的消息
int parse_response(Peer *peer);
// 处理收到的消息， 接收缓冲区内存放着一条完整的消息,还有其他不完整的消息
int parse_response_uncomplete_msg(Peer *p, int ok_len);
// 根据当前的状态创建响应消息
int create_response_message(Peer *peer);
// 为发送have消息作准备， 需要发送给所有peer
int prepare_send_have_msg();
// 即将与peer断开时，丢弃套接字发送缓冲区中的所有为发送消息
void discard_send_buffer(Peer *peer);

#endif // !MESSAGE_H
