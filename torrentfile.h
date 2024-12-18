#ifndef TORRENTFILE_H
#define TORRENTFILE_H

typedef struct _Annouce_list {
  char announce[128];
  struct _Annouce_list *next;
} Announce_list;

typedef struct _Files {
  char path[256];
  long length;
  struct _Files *next;
} Files;

int read_torrentfile(const char *filename);

int find_keyword(const char *keyword, int *position);
int get_infohash();
int get_filename();
int get_announce_list();
int get_piece_lenght();
int get_pieces();
int is_multi_file();
int get_files_length_path();
int get_file_length();
int get_peerid();
int parse_torrentfile(const char *filename);

void release_torrentfile_memory();

#endif // !TORRENTFILE_H
