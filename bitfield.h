#ifndef BITFIELD_H
#define BITFIELD_H
typedef struct _Bitmap {
  unsigned char *bitfield;
  int bitfield_length; // bytes length
  int valid_legnth;    // bit length
} Bitmap;

int create_bitfield();
int get_bit_value(Bitmap *bitmap, int index);
int set_bit_value(Bitmap *bitmap, int index, unsigned char value);

int all_zero(Bitmap *bitmap); // all set 0
int all_set(Bitmap *bitmap);  // all set 1
void releasae_memory_in_bitfield();

int print_bitfield(Bitmap *bitmap);
int restore_bitmap();
// 判断是否感兴趣
int is_interested(Bitmap *dst, Bitmap *src);

int get_download_piece_num();

#endif // ! BITFIELD_H
