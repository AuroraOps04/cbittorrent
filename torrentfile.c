#include "torrentfile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
long torrentfile_size = 0;
char *torrentfile_content = NULL;

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
  int i, begin, end;
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
        ++i;
        number = number * 10 + torrentfile_content[i] - '0';
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

  return 0;
}

void release_torrentfile_memory() {
  torrentfile_size = 0;
  if (torrentfile_content != NULL)
    free(torrentfile_content);
}
