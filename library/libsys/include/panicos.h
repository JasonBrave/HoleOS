/*
 * System call header
 *
 * This file is part of PanicOS.
 *
 * PanicOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PanicOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PanicOS.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _LIBSYS_PANICOS_H
#define _LIBSYS_PANICOS_H

int fork(void);
_Noreturn int proc_exit(int);
int wait(void);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
// int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int dir_open(const char* dirname);
int dir_read(int handle, char* buffer);
int dir_close(int handle);
int file_get_size(const char* filename);
int lseek(int fd, int offset, int whence);
int file_get_mode(const char* filename);

enum OpenMode {
	O_READ = 1,
	O_WRITE = 2,
	O_CREATE = 4,
	O_APPEND = 8,
	O_TRUNC = 16,
};

enum FileSeekMode {
	FILE_SEEK_SET,
	FILE_SEEK_CUR,
	FILE_SEEK_END,
};

#endif
