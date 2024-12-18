#ifndef TORRENTFILE_H
#define TORRENTFILE_H

typedef struct _Annouce_list {
  char announce[128];
  struct _Annouce_list *next;
} Announce_list;

int read_torrentfile(const char *filename);

int find_keyword(const char *keyword, int *position);
int get_infohash();
int get_filename();
int get_announce_list();

void release_torrentfile_memory();

#endif // !TORRENTFILE_H
