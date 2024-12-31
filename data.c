#include "data.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <openssl/types.h>

#include "bitfield.h"
#include "torrentfile.h"
#include <sys/stat.h>


/**
 * 每个缓冲区节点的大小为 16KB，默认生成 1024个节点， 总大小为 16MB,
 * 缓冲区以一个piece（通常为 256 KB ） 为基本单位
 * 也就是临近的16个节点为一组， 这16个节点要么全部被占用，要么全部空闲， 1～16
 * 存放第一个 piece， 17～32存放第二个
 * 所有缓冲区在启动时统一申请，结束时统一释放
 */
#define btcache_len 1024
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
int last_slice_len = 0; // 最后一个piece的 最后一个slice的长度

int *fds = NULL; // 所有文件的文件描述符
int fds_len = 0;
int have_piece_index[64]; // 保存已经下载的piece的索引
int end_mode = 0; // 是否进入终端模式

Btcache *initialize_btcache_node() {
    Btcache *node = (Btcache *) malloc(sizeof(Btcache));
    if (!node) {
        return NULL;
    }
    node->index = -1;
    node->begin = -1;
    node->length = -1;
    node->in_use = 0;
    node->read_write = -1;
    node->access_count = 0;
    node->next = NULL;
    node->is_full = 0;
    node->is_writed = 0;
    node->buff = (unsigned char *) malloc(1 << 14);
    if (node->buff == NULL) {
        free(node);
        return NULL;
    }
    return node;
}

int create_btcache() {
    Btcache *node = NULL, *last = NULL;
    int i;
    for (i = 0; i < btcache_len; i++) {
        node = initialize_btcache_node();
        if (node == NULL) {
            release_memory_in_btcache();
            return -1;
        }
        if (last != NULL) {
            last->next = node;
            last = node;
        } else {
            last = node;
            btcache_head = last;
        }
    }
    int count = file_length % piece_length / (16 * 1024);
    if (file_length % piece_length % (16 * 1024) != 0) count++;
    last_piece_count = count;
    last_slice_len = file_length % piece_length % (16 * 1024);
    if (last_slice_len == 0) last_slice_len = 16 * 1024;
    last_piece_index = piece_length / 20 - 1;
    while (count > 0) {
        node = initialize_btcache_node();
        if (node == NULL) {
            release_memory_in_btcache();
            return -1;
        }
        if (last_piece == NULL) {
            last_piece = node;
            last = node;
        } else {
            last->next = node;
            last = node;
        }
    }
    for (i = 0; i < 64; i++) {
        have_piece_index[i] = -1;
    }
    return 0;
}

void release_memory_in_btcache() {
    Btcache *node;
    while (btcache_head != NULL) {
        node = btcache_head;
        btcache_head = btcache_head->next;
        if (node->buff != NULL)
            free(node->buff);
        free(node);
    }
    release_last_piece();
    if (fds != NULL) free(fds);
}

void release_last_piece() {
    Btcache *node;
    while (last_piece != NULL) {
        node = last_piece;
        last_piece = last_piece->next;
        if (node->buff != NULL)
            free(node->buff);
        free(node);
    }
}

int get_files_count() {
    int number = 0;
    Files *p = files;
    if (is_multi_file()) {
        while (p != NULL) {
            number++;
            p = p->next;
        }
        return number;
    }
    return 1;
}

int create_files() {
    int ret, i;
    Files *node = NULL;
    char buf[1] = {0x0};
    fds_len = get_files_count();
    fds = (int *) malloc(sizeof(int) * fds_len);
    if (fds == NULL) return -1;
    if (filename == NULL) return -1;
    if (!is_multi_file()) {
        *fds = open(filename, O_RDWR | O_CREAT, 0777);
        if (*fds < 0) return -1;
        ret = lseek(*fds, file_length - 1, SEEK_SET);
        if (ret < 0)return -1;
        ret = write(*fds, buf, 1);
        if (ret != 1) return -1;
    } else {
        ret = chdir(filename);
        if (ret < 0) {
            // create folder;
            ret = mkdir(filename, 0777);
            if (ret < 0) return -1;
            ret = chdir(filename);
            if (ret < 0) return -1;
        }
        node = files;
        i = 0;
        while (node != NULL) {
            // creat folder
            if (mkdir(filename, 0755) < 0) {
                return -1;
            }
            fds[i] = open(node->path, O_RDWR | O_CREAT, 0777);
            if (fds[i] < 0) return -1;
            ret = lseek(fds[i], node->length, SEEK_SET);
            if (ret < 0) return -1;
            ret = write(fds[i], buf, 1);
            if (ret != 1) return -1;
            i++;
            node = node->next;
        }
    }
    return 0;
}


int write_btcache_node_to_harddisk(Btcache *node) {
    int pos = node->index * piece_length + node->begin;
    int ret;
    Files *p = files;
    int i = 0;
    if (!is_multi_file()) {
        ret = lseek(fds[i], pos, SEEK_SET);
        if (ret < 0) return -1;
        ret = write(fds[i], node->buff, node->length);
        if (ret != node->length) return -1;
        return 0;
    }
    while (p != NULL) {
        if (p->length >= pos) {
            if (pos + node->length < p->length) {
                ret = lseek(fds[i], pos, SEEK_SET);
                if (ret < 0) return -1;
                ret = write(fds[i], node->buff, node->length);
                if (ret != node->length) return -1;
            } else {
                ret = lseek(fds[i], pos, SEEK_SET);
                if (ret < 0) return -1;
                ret = write(fds[i], node->buff, p->length - pos);
                if (ret != p->length - pos) return -1;
                int left = node->length - (p->length - pos);
                p = p->next;
                i++;
                while (left > 0) {
                    if (p->length >= left) {
                        ret = lseek(fds[i], 0, SEEK_SET);
                        if (ret < 0) return -1;
                        ret = write(fds[i], node->buff + node->length - left, left);
                        if (ret != left) return -1;
                        break;
                    }
                    ret = lseek(fds[i], 0, SEEK_SET);
                    if (ret < 0) return -1;
                    ret = write(fds[i], node->buff + node->length - left, p->length);
                    if (ret != p->length) return -1;
                    left -= p->length;
                    p = p->next;
                    i++;
                }
            }
            return 0;
        }
        pos -= p->length;
        p = p->next;
        i++;
    }

    return -1;
}

int read_slice_from_harddisk(Btcache *node) {
    if (node == NULL || node->buff == NULL) return -1;
    int pos = node->index * piece_length + node->begin;
    int ret;
    int i = 0;
    if (!is_multi_file()) {
        ret = lseek(fds[i], pos, SEEK_SET);
        if (ret < 0) return -1;
        
    }


    return 0;
}

/**
 *
 * @param sequence  第几个 btcache,某个piece开头的slice
 * @param peer
 * @return
 */
int write_piece_to_harddisk(const int sequence, Peer *peer) {
    Btcache *node_ptr = btcache_head, *p = NULL;
    const int slice_count = piece_length / (16 * 1024);
    unsigned char piece_hash1[20], piece_hash2[20];
    int index, index_copy;
    int i = 0;
    if (peer == NULL || btcache_head == NULL) return -1;

    while (i < sequence) {
        node_ptr = node_ptr->next;
        if (node_ptr == NULL) return -1;
        i++;
    }
    p = node_ptr;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha1(), NULL);

    i = slice_count;
    while (i > 0 && node_ptr != NULL) {
        EVP_DigestUpdate(ctx, node_ptr->buff, 16 * 1024);
        node_ptr = node_ptr->next;
        i--;
    }
    EVP_DigestFinal_ex(ctx, piece_hash1, NULL);
    EVP_MD_CTX_free(ctx);

    // 从 piece 获取 hash 值
    index = p->index * 20;
    index_copy = p->index;
    for (i = 0; i < 20; i++) piece_hash2[i] = pieces[index + i];
    if (memcpy(piece_hash2, piece_hash1, 20) != 0) {
        return -1;
    }
    node_ptr = p;
    i = slice_count;
    while (i > 0) {
        write_btcache_node_to_harddisk(node_ptr);
        // 从peer的请求队列中删除piece请求
        Request_piece *req_p = peer->Request_piece_head, *prev = peer->Request_piece_head;
        while (req_p != NULL) {
            if (req_p->begin == node_ptr->begin && req_p->index == node_ptr->index && req_p->length == node_ptr->
                length) {
                if (req_p == peer->Request_piece_head) {
                    peer->Request_piece_head = req_p->next;
                } else {
                    prev->next = req_p->next;
                }
                free(req_p);
                req_p = prev = NULL;
                break;
            }
            prev = req_p;
            req_p = req_p->next;
        }
        // reset node_ptr
        node_ptr->index = -1;
        node_ptr->begin = -1;
        node_ptr->length = -1;
        node_ptr->in_use = 0;
        node_ptr->read_write = -1;
        node_ptr->is_full = 0;
        node_ptr->access_count = 0;
        node_ptr = node_ptr->next;
        i--;
    }
    // if(end_mode == 1) delete_request_end_mode(index_copy);
    set_bit_value(bitmap, index_copy, 1);
    for (i = 0; i < 64; i++) {
        if (have_piece_index[i] != -1) {
            have_piece_index[i] = index_copy;
            break;
        }
    }
    download_piece_num++;
    if (download_piece_num % 10 == 0) restore_bitmap();
    printf("%%%%%% Total piece download:%d %%%%%%\n", download_piece_num);
    printf("written piece index:%d\n", index_copy);

    return 0;
}
