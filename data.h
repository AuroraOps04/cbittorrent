#ifndef DATA_H
#define DATA_H
#include "peer.h"
typedef struct _Btcache {
  unsigned char *buff;      // 存放数据的缓冲区
  int index;                // 数据所在的piece的索引
  int begin;                // 数据所在piece块的偏移
  int length;               // 数据的长度
  unsigned char in_use;     // 该缓冲区是否正在使用
  unsigned char read_write; // 读 0 写 1
  int access_count;         // 访问计数
  struct _Btcache *next;
} Btcache;

Btcache *initialize_btcache_node();
int create_btcache();
void release_memory_in_btcache();
// 获取种子文件中 待下载文件的数目
int get_files_count();
// 根据种子文件中的信息创建保存下载数据的文件
int create_files();

int write_btcache_node_to_harddisk(Btcache *node);
// 从  硬盘中读取一个 slcie 到缓冲区,在peer需要时发送给peer
// 要读入的slice 的 indx begin length 已存入到 node 中
int read_slice_from_harddisk(Btcache *nodee);
// 检查一个piece的数据是否正确，正确就写入硬盘
int write_piece_to_harddisk(Btcache *node);
// 从硬盘中读取一个piece的数据到缓冲区
int read_piece_from_harddisk(Btcache *node, int index);

// 将整个缓冲区的数据写入硬盘
int write_btcache_to_harddisk(Peer *peer);
// 当缓冲区不够用时，释放那些从硬盘中读取的piece
int release_read_btcache_node(int base_count);
// 从btcache缓存区中清除那些下载未完成的piece
void clear_btcache_before_peer_cloe(Peer *peer);

// 将刚刚从peer处获取的一个slice存入缓冲区

int write_slice_to_btcache(int index, int begin, int length, unsigned char *buf,
                           int len, Peer *peer);
// 从缓冲区中读取一个slice, 放到peer的发送缓冲区
int read_slice_for_send(int index, int begin, int length, Peer *peer);
// 最后一个piece 因为他不是完整的
int write_last_piece_to_btcache(Peer *peer);
int write_slice_to_last_pice(int index, int begin, int length,
                             unsigned char *buf, int len, Peer *peer);
int read_last_piee_from_harddisk(Btcache *p, int index);
int read_slice_for_send_last_piece(int index, int begin, int length,
                                   Peer *peer);
void release_last_piece();

#endif // !DATA_H
