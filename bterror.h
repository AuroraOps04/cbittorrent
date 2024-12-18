#ifndef BTERROR_H
#define BTERROR_H

#define FILE_FD_ERR -1 // 无效的文件描述符
#define FILE_READ_ERR -2
#define FILE_WRITE_ERR -3
#define INVALID_TORRENFILE_ERR -4

void btexit(int errno, char *file, int line);

#endif // !BTERROR_H
