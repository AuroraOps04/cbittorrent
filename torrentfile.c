#include "torrentfile.h"
#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
long torrentfile_size = 0;
char *torrentfile_content = NULL;
char *filename = NULL;
unsigned char info_hash[20];
int piece_length = 0; //
unsigned char *pieces = NULL;
int pieces_length = 0;
long long file_length = 0;
char peer_id[20];

Files *files = NULL;
Announce_list *announce_list_head = NULL;

int read_torrentfile(const char *filename) {
  if (filename == NULL) {
    return -1;
  }

  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    return -1;
  }
  fseek(fp, 0, SEEK_END);
  torrentfile_size = ftell(fp);
  printf("torrentfile_size: %ld\n", torrentfile_size);

  torrentfile_content = (char *)malloc(sizeof(char) * torrentfile_size);
  char buf[1024] = {0};
  int current = 0;
  unsigned long long length = 0;
  fseek(fp, 0, SEEK_SET);
  while ((length = fread(&buf, 1, 1024, fp)) > 0) {
    memcpy(&torrentfile_content[current], buf, length);
    current += length;
  }
  return 0;
}

int find_keyword(const char *keyword, int *position) {
  if (torrentfile_content == NULL) {
    return -1;
  }
  for (int i = 0; i < torrentfile_size; ++i) {
    if (torrentfile_content[i] == keyword[0] &&
        memcmp(&torrentfile_content[i], keyword, strlen(keyword)) == 0) {
      *position = i;
      return 0;
    }
  }
  return -1;
}
int get_infohash() {
  if (torrentfile_content == NULL) {
    return -1;
  }
  int push_pop = 0;
  int i, begin, end = 0;
  if (find_keyword("4:info", &i) < 0) {
    return -1;
  }
  begin = i += 6;

  while (i < torrentfile_size) {
    if (torrentfile_content[i] == 'd') {
      ++push_pop;
      ++i;
    } else if (torrentfile_content[i] == 'i') {
      ++i;
      if (i == torrentfile_size)
        return -1;
      while (torrentfile_content[i] != 'e') {
        if (++i == torrentfile_size)
          return -1;
      }
      ++i;
      if (i == torrentfile_size)
        return -1;
    } else if (torrentfile_content[i] == 'l') {
      ++push_pop;
      ++i;
    } else if (torrentfile_content[i] >= '0' && torrentfile_content[i] <= '9') {
      int number = 0;
      while (torrentfile_content[i] >= '0' && torrentfile_content[i] <= '9') {
        number = number * 10 + torrentfile_content[i] - '0';
        ++i;
      }
      i += number + 1;
    } else if (torrentfile_content[i] == 'e') {
      --push_pop;
      if (push_pop == 0) {
        end = i;
        break;
      }
      ++i;
    } else {
      return -1;
    }
  }
  if (i == torrentfile_size)
    return -1;

  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  EVP_DigestInit_ex(ctx, EVP_sha1(), NULL);
  EVP_DigestUpdate(ctx, torrentfile_content + begin, end - begin + 1);
  EVP_DigestFinal_ex(ctx, info_hash, NULL);
  EVP_MD_CTX_free(ctx);

  printf("info_hash: ");
  for (int i = 0; i < 20; i++) {
    printf("%02x", info_hash[i]);
  }
  printf("\n");

  return 0;
}

int get_filename() {
  if (torrentfile_content == NULL) {
    return -1;
  }
  int p;
  if (find_keyword("4:name", &p) < 0) {
    return -1;
  }
  p += 6;
  int number = 0;
  while (torrentfile_content[p] != ':') {
    number = number * 10 + torrentfile_content[p] - '0';
    p++;
  }
  p++;
  filename = (char *)malloc(number + 1);
  memcpy(filename, &torrentfile_content[p], number);
  filename[number] = 0;
  return 0;
}
int get_announce_list() {
  int p;
  if (torrentfile_content == NULL) {
    return -1;
  }
  if (find_keyword("13:announce-list", &p) >= 0) {
    p += 16;
    p++; // skip 'l'
    while (torrentfile_content[p] != 'e') {
      p++; // skip 'l'
      int number = 0;
      while (torrentfile_content[p] != ':') {
        number = number * 10 + torrentfile_content[p] - '0';
        p++;
      }
      p++; // skip ':'
      Announce_list *node = (Announce_list *)malloc(sizeof(Announce_list));
      node->next = NULL;
      memcpy(node->announce, &torrentfile_content[p], number);
      node->announce[number] = 0;

      p += number;
      p++; // skip 'e'

      if (announce_list_head == NULL) {
        announce_list_head = node;
      } else {
        Announce_list *q = announce_list_head;
        while (q->next != NULL) {
          q = q->next;
        }
        q->next = node;
      }
    }

  } else {
    if (find_keyword("8:announce", &p) < 0) {
      return -1;
    }

    p += 10;
    int number = 0;
    while (torrentfile_content[p] != ':') {
      number = number * 10 + torrentfile_content[p] - '0';
      p++;
    }
    p++;

    Announce_list *node = (Announce_list *)malloc(sizeof(Announce_list));
    node->next = NULL;
    memcpy(node->announce, &torrentfile_content[p], number);
    node->announce[number] = 0;
    announce_list_head = node;
  }

  Announce_list *q = announce_list_head;
  while (q != NULL) {
    printf("%s\n", q->announce);
    q = q->next;
  }

  return 0;
}

int get_piece_length() {
  if (torrentfile_content == NULL) {
    return -1;
  }
  int i = 0;
  const char *k = "12:piece length";
  if (find_keyword(k, &i) < 0) {
    return -1;
  }
  i += strlen(k);
  i++; // skip 'i'
  piece_length = 0;
  while (torrentfile_content[i] != 'e') {
    piece_length = piece_length * 10 + torrentfile_content[i] - '0';
    i++;
  }
  printf("piece length: %d\n", piece_length);
  return 0;
}

int get_pieces() {
  if (torrentfile_content == NULL) {
    return -1;
  }
  int i = 0;
  const char *k = "6:pieces";
  if (find_keyword(k, &i) < 0) {
    return -1;
  }
  i += strlen(k);

  pieces_length = 0;
  while (torrentfile_content[i] >= '0' && torrentfile_content[i] <= '9') {
    pieces_length = pieces_length * 10 + torrentfile_content[i] - '0';
    i++;
  }
  i++;
  pieces = (unsigned char *)malloc(pieces_length);
  memcpy(pieces, &torrentfile_content[i], pieces_length);

  return 0;
}
int is_multi_file() {
  if (torrentfile_content == NULL) {
    return -1;
  }
  int i;
  if (find_keyword("5:files", &i) < 0) {
    return 0;
  }
  return 1;
}

int get_file_length() {
  if (torrentfile_content == NULL)
    return -1;

  int i;
  const char *k = "6:length";
  if (is_multi_file() == 1) {
    if (get_files_length_path() < 0)
      return -1;
    file_length = 0;
    Files *p = files;
    while (p != NULL) {
      file_length += p->length;
      p = p->next;
    }
  } else {
    if (find_keyword(k, &i) < 0) {
      return -1;
    }
    i += strlen(k);
    i++; // skip 'i'
    file_length = 0;
    while (torrentfile_content[i] != 'e') {
      file_length = file_length * 10 + torrentfile_content[i] - '0';
      i++;
    }
  }

  printf("file_length %lld\n", file_length);
  return 0;
}
int get_files_length_path() {
  if (torrentfile_content == NULL) {
    return -1;
  }
  if (is_multi_file() == 0)
    return 0;

  for (int i = 0; i < torrentfile_size; i++) {
    if (memcmp(&torrentfile_content[i], "6:length", 8) == 0) {
      i += 8;
      i++; // skip 'i'
      Files *n = (Files *)malloc(sizeof(Files));
      n->next = NULL;
      n->length = 0;
      while (torrentfile_content[i] != 'e') {
        n->length = n->length * 10 + torrentfile_content[i] - '0';
        i++;
      }
      if (files == NULL)
        files = n;
      else {
        Files *p = files;
        while (p->next != NULL) {
          p = p->next;
        }
        p->next = n;
      }

    } else if (memcmp(&torrentfile_content[i], "4:path", 6) == 0) {
      i += 6;
      i++; // skip 'l'
      int count = 0;
      while (torrentfile_content[i] >= '0' && torrentfile_content[i] <= '9') {
        count = count * 10 + torrentfile_content[i] - '0';
        i++;
      }
      i++; // skip ':'
      Files *p = files;
      if (p == NULL)
        return -1;
      while (p->next != NULL) {
        p = p->next;
      }
      memcpy(p->path, &torrentfile_content[i], count);
      *(p->path + count) = 0;
    }
  }

  if (files != NULL) {
    Files *p = files;
    while (p != NULL) {
      printf("path: %s length:%ld\n", p->path, p->length);
      p = p->next;
    }
  }

  return 0;
}
int get_peerid() {
  srand(time(NULL));
  sprintf(peer_id, "-TT1000-%11d", rand());
  printf("peer_id %s\n", peer_id);
  return 0;
}

int parse_torrentfile(const char *filename) {
  int ret;
  ret = read_torrentfile(filename);
  if (ret < 0) {
    printf("%s %d wrong\n", __FILE__, __LINE__);
    return -1;
  }

  ret = get_announce_list();
  if (ret < 0) {
    printf("%s %d wrong\n", __FILE__, __LINE__);
    return -1;
  }

  ret = is_multi_file();
  if (ret < 0) {
    printf("%s %d wrong\n", __FILE__, __LINE__);
    return -1;
  }

  ret = get_piece_length();
  if (ret < 0) {
    printf("%s %d wrong\n", __FILE__, __LINE__);
    return -1;
  }

  ret = get_pieces();
  if (ret < 0) {
    printf("%s %d wrong\n", __FILE__, __LINE__);
    return -1;
  }

  ret = get_filename();
  if (ret < 0) {
    printf("%s %d wrong\n", __FILE__, __LINE__);
    return -1;
  }

  ret = get_file_length();
  if (ret < 0) {
    printf("%s %d wrong\n", __FILE__, __LINE__);
    return -1;
  }

  ret = get_infohash();
  if (ret < 0) {
    printf("%s %d wrong\n", __FILE__, __LINE__);
    return -1;
  }
  ret = get_peerid();
  if (ret < 0) {
    printf("%s %d wrong\n", __FILE__, __LINE__);
    return -1;
  }

  return 0;
}

void release_torrentfile_memory() {
  torrentfile_size = 0;
  if (torrentfile_content != NULL)
    free(torrentfile_content);

  if (filename != NULL)
    free(filename);

  if (pieces != NULL)
    free(pieces);

  if (announce_list_head != NULL) {
    Announce_list *node = announce_list_head;
    announce_list_head = announce_list_head->next;
    free(node);
  }
  if (files != NULL) {
    Files *node = files;
    files = files->next;
    free(node);
  }
}
