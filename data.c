#include "data.h"
#include "bitfield.h"
#include "torrentfile.h"

/**
 * 每个缓冲区节点的大小为 16KB，默认生成 1024个节点， 总大小为 16MB,
 * 缓冲区以一个piece（通常为 256 KB ） 为基本单位
 * 也就是临近的16个节点为一组， 这16个节点要么全部被占用，要么全部空闲， 1～16
 * 存放第一个 piece， 17～32存放第二个
 * 所有缓冲区在启动时统一申请，结束时统一释放
 */
#define btcache_len = 1024
extern char *filename;
extern Files *files;
extern int file_length;
extern int piece_length;
extern char *pieces; // 存放所有piece的hash值
extern int pieces_length;

extern Bitmap *bitmap;
extern int download_piece_num;
extern Peer *peer_head;

Btcache *btcache_head = NULL;
Btcache *last_piece = NULL;
int last_piece_index = 0; // 最后一个piece的索引 为 总piece数-1
int last_piece_count = 0; // 最后一个piece下载了 多少 slice
int last_slice_len = 0;   // 最后一个piece的 最后一个slice的长度

int *fds = NULL; // 所有文件的文件描述符
int fds_len = 0;
int have_piece_index[64]; // 保存已经下载的piece的索引
int end_mode = 0; // 是否进入终端模式
//
//
