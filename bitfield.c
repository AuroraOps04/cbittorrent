#include "bitfield.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int set_bit_value(Bitmap *bitmap, int index, unsigned char value) { return 0; }
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
