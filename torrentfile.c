#include "torrentfile.h"
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
long torrentfile_size = 0;
char *torrentfile_content = NULL;
char *filename = NULL;
unsigned char info_hash[20];

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

  SHA_CTX ctx;
  SHA1_Init(&ctx);
  SHA1_Update(&ctx, &torrentfile_content[begin], end - begin + 1);
  SHA1_Final(info_hash, &ctx);

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

void release_torrentfile_memory() {
  torrentfile_size = 0;
  if (torrentfile_content != NULL)
    free(torrentfile_content);

  if (filename != NULL)
    free(filename);
}
