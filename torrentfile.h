#ifndef TORRENTFILE_H
#define TORRENTFILE_H
extern char *torrentfile_content;
extern long torrentfile_size;

int read_torrentfile(const char *filename);

int find_keyword(const char *keyword, int *position);

void cleanup();

#endif // !TORRENTFILE_H
