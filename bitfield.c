#include "bitfield.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
extern int pieces_length;
extern char *filename;
Bitmap *bitmap = NULL;
int download_piece_num = 0;

int create_bitfield() {
  bitmap = (Bitmap *)malloc(sizeof(Bitmap));
  if (bitmap == NULL) {
    printf("allocate memory for bitmap failed\n");
    return -1;
  }

  bitmap->valid_legnth = pieces_length / 20;
  bitmap->bitfield_length = pieces_length / 20 / 8;

  if ((pieces_length / 20) % 8 != 0)
    bitmap->bitfield_length++;

  bitmap->bitfield = (unsigned char *)malloc(bitmap->bitfield_length);
  if (bitmap->bitfield == NULL) {
    printf("allocate memory for bitmap->bitfield failed\n");
    if (bitmap != NULL) {
      free(bitmap);
    }
    return -1;
  }
  char bitmapfile[64];
  sprintf(bitmapfile, "%dbitmap", pieces_length);
  int i;
  FILE *fp = fopen(bitmapfile, "rb");
  if (fp == NULL) {
    memset(bitmap->bitfield, 0, bitmap->bitfield_length);
  } else {
    fseek(fp, 0, SEEK_SET);
    for (i = 0; i < bitmap->bitfield_length; i++) {
      (bitmap->bitfield)[i] = fgetc(fp);
    }
    fclose(fp);
    download_piece_num = get_download_piece_num();
  }
  return 0;
}

int get_bit_value(Bitmap *bitmap, int index) {
  int ret;
  int byte_index;
  unsigned char byte_value;
  unsigned char inner_byte_index;
  if (bitmap == NULL || index > bitmap->valid_legnth)
    return -1;
  byte_index = index / 8;
  inner_byte_index = index % 8;
  byte_value = bitmap->bitfield[byte_index];
  byte_value = byte_value >> (7 - inner_byte_index);
  if (byte_value % 2 == 0)
    ret = 0;
  else
    ret = 1;
  return ret;
}

int set_bit_value(Bitmap *bitmap, int index, unsigned char value) {
  if (bitmap == NULL || bitmap->bitfield == NULL) {
    return -1;
  }
  if (index < 0 || index >= bitmap->valid_legnth) {
    return -1;
  }
  int byte_index, inner_byte_index;
  byte_index = index / 8;
  inner_byte_index = index % 8;
  unsigned char byte = bitmap->bitfield[byte_index];
  unsigned char mask = 1 << (7 - inner_byte_index);
  if (value == 0) {
    bitmap->bitfield[byte_index] = byte & ~mask;
  } else if (value == 1) {
    bitmap->bitfield[byte_index] = byte | mask;
  } else {
    printf("value only support 0 and 1\n");
    return -1;
  }

  return 0;
}
int all_zero(Bitmap *bitmap) {
  if (bitmap == NULL) {
    return -1;
  }
  memset(bitmap->bitfield, 0, bitmap->bitfield_length);
  return 0;
}

int all_set(Bitmap *bitmap) {
  if (bitmap == NULL) {
    return -1;
  }
  memset(bitmap->bitfield, 0b11111111, bitmap->bitfield_length);
  return 0;
}

void releasae_memory_in_bitfield() {
  if (bitmap != NULL) {
    if (bitmap->bitfield != NULL)
      free(bitmap->bitfield);
    free(bitmap);
  }
}

int print_bitfield(Bitmap *bitmap) {
  if (bitmap == NULL || bitmap->bitfield == NULL)
    return -1;

  int i, j;
  printf("bitmap: ");
  for (i = 0; i < bitmap->bitfield_length - 1; i++) {
    for (j = 0; j < 8; j++) {
      printf("%d", ((bitmap->bitfield)[i] >> (7 - j)) & 1);
    }
  }
  for (i = 0; i < bitmap->valid_legnth % 8; i++) {
    printf("%d",
           (bitmap->bitfield)[bitmap->bitfield_length - 1] >> (7 - i) & 1);
  }
  printf("\n");

  return 0;
}

int restore_bitmap() {
  if (bitmap == NULL || bitmap->bitfield == NULL) {
    return -1;
  }
  char bitfilename[64];
  int fd;
  sprintf(bitfilename, "%dbitmap", pieces_length);
  fd = open(bitfilename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (fd < 0) {
    return -1;
  }

  if (write(fd, bitmap->bitfield, bitmap->bitfield_length)) {
    close(fd);
    return -1;
  }
  close(fd);
  return 0;
}

int is_interested(Bitmap *dst, Bitmap *src) {
  if (dst == NULL || dst->bitfield == NULL || src == NULL ||
      src->bitfield == NULL)
    return -1;
  const unsigned char chars[] = {0x80, 0x40, 0x20, 0x10,
                                 0x08, 0x04, 0x02, 0x01};
  int i, j;
  unsigned char c1, c2;

  for (i = 0; i < dst->bitfield_length - 1; i++) {
    for (j = 0; j < 8; j++) {
      c1 = dst->bitfield[i] & chars[j];
      c2 = src->bitfield[i] & chars[j];
      if (c1 > 0 && c2 == 0) {
        return 1;
      }
    }
  }

  for (i = 0; i < (dst->valid_legnth % 8); i++) {
    c1 = dst->bitfield[dst->bitfield_length - 1] & chars[i];
    c2 = src->bitfield[src->bitfield_length - 1] & chars[i];
    if (c1 > 0 && c2 == 0) {
      return 1;
    }
  }

  return 0;
}

int get_download_piece_num() {
  if (bitmap == NULL || bitmap->bitfield == NULL)
    return -1;
  int i, j;
  download_piece_num = 0;
  for (i = 0; i < bitmap->bitfield_length - 1; i++) {
    for (j = 0; j < 8; j++) {
      if (((bitmap->bitfield)[i] >> (7 - j)) % 2 == 1) {
        download_piece_num++;
      }
    }
  }

  for (i = 0; i < bitmap->valid_legnth % 8; i++) {
    if (((bitmap->bitfield)[bitmap->bitfield_length - 1] >> (7 - i)) % 2 == 1) {
      download_piece_num++;
    }
  }
  return 0;
}
