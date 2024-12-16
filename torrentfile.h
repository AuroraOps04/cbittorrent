#ifndef TORRENTFILE_H
#define TORRENTFILE_H
extern char *torrentfile_content;
extern long torrentfile_size;
extern char info_hash[20];

int read_torrentfile(const char *filename);

int find_keyword(const char *keyword, int *position);
int get_infohash();

void release_torrentfile_memory();

#endif // !TORRENTFILE_H
