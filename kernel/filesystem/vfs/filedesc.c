/*
 * Virtual filesystem file support
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

#include <common/errorcode.h>
#include <defs.h>
#include <filesystem/fat32/fat32.h>
#include <filesystem/initramfs/initramfs.h>

#include "vfs.h"

int vfs_fd_open(struct FileDesc* fd, const char* filename, int mode) {
	memset(fd, 0, sizeof(struct FileDesc));
	struct VfsPath filepath;
	filepath.pathbuf = kalloc();
	filepath.parts = vfs_path_split(filename, filepath.pathbuf);
	if (filename[0] != '/') {
		vfs_get_absolute_path(&filepath);
	}
	struct VfsPath path;
	int fs_id = vfs_path_to_fs(filepath, &path);

	if (vfs_mount_table[fs_id].fs_type == VFS_FS_INITRAMFS) {
		// initramfs
		if (mode & O_WRITE || mode & O_APPEND || mode & O_CREATE) {
			kfree(filepath.pathbuf);
			return ERROR_INVAILD; // initramfs is read-only
		}
		int blk = initramfs_open(path.pathbuf);
		if (blk < 0) {
			kfree(filepath.pathbuf);
			return blk;
		}
		fd->block = blk;
		fd->size = initramfs_file_get_size(path.pathbuf);
		fd->read = 1;
	} else if (vfs_mount_table[fs_id].fs_type == VFS_FS_FAT32) {
		// FAT32
		int fblock = fat32_open(vfs_mount_table[fs_id].partition_id, path);
		if (fblock < 0) {
			if (mode & O_CREATE) {
				// create and retry
				fat32_file_create(vfs_mount_table[fs_id].partition_id, path);
				fblock = fat32_open(vfs_mount_table[fs_id].partition_id, path);
			} else {
				kfree(filepath.pathbuf);
				return ERROR_NOT_EXIST;
			}
		}
		fd->block = fblock;
		fd->size = fat32_file_size(vfs_mount_table[fs_id].partition_id, path);
		if (mode & O_READ) {
			fd->read = 1;
		}
		if (mode & O_WRITE) {
			fd->path.parts = path.parts;
			fd->path.pathbuf = kalloc();
			memmove(fd->path.pathbuf, path.pathbuf, path.parts * 128);
			fd->write = 1;
			if (mode & O_APPEND) {
				fd->append = 1;
			}
			if (mode & O_TRUNC) {
				fd->size = 0;
			}
		}
	} else {
		kfree(filepath.pathbuf);
		return ERROR_INVAILD;
	}

	fd->offset = 0;
	fd->fs_id = fs_id;
	fd->used = 1;
	kfree(filepath.pathbuf);
	return 0;
}

int vfs_fd_read(struct FileDesc* fd, void* buf, unsigned int size) {
	if (!fd->used) {
		return ERROR_INVAILD;
	}
	if (!fd->read) {
		return ERROR_INVAILD;
	}
	if (fd->dir) {
		return ERROR_INVAILD;
	}

	int status;
	if (vfs_mount_table[fd->fs_id].fs_type == VFS_FS_INITRAMFS) {
		status = initramfs_read(fd->block, buf, fd->offset, size);
		if (status > 0) {
			fd->offset += status;
		}
	} else if (vfs_mount_table[fd->fs_id].fs_type == VFS_FS_FAT32) {
		if (fd->offset >= fd->size) { // EOF
			return 0;
		} else if (fd->offset + size > fd->size) {
			status = fd->size - fd->offset;
		} else {
			status = size;
		}
		int ret =
			fat32_read(vfs_mount_table[fd->fs_id].partition_id, fd->block, buf, fd->offset, size);
		if (ret < 0) {
			return ret;
		}
		fd->offset += status;
	} else {
		status = ERROR_INVAILD;
	}
	return status;
}

int vfs_fd_write(struct FileDesc* fd, const char* buf, unsigned int size) {
	if (!fd->used) {
		return ERROR_INVAILD;
	}
	if (!fd->write) {
		return ERROR_INVAILD;
	}
	if (fd->dir) {
		return ERROR_INVAILD;
	}
	if (vfs_mount_table[fd->fs_id].fs_type == VFS_FS_FAT32) {
		if (fd->append) {
			fd->offset = fd->size;
		}
		int ret =
			fat32_write(vfs_mount_table[fd->fs_id].partition_id, fd->block, buf, fd->offset, size);
		if (ret < 0) {
			return ret;
		}
		fd->offset += ret;
		if (fd->offset > fd->size) {
			fd->size += fd->offset - fd->size;
		}
		return ret;
	} else {
		return ERROR_INVAILD;
	}
}

int vfs_fd_close(struct FileDesc* fd) {
	if (!fd->used) {
		return ERROR_INVAILD;
	}
	if (fd->dir) {
		return ERROR_INVAILD;
	}

	if (fd->write) {
		if (vfs_mount_table[fd->fs_id].fs_type == VFS_FS_FAT32) {
			fat32_update_size(vfs_mount_table[fd->fs_id].partition_id, fd->path, fd->size);
		}
		kfree(fd->path.pathbuf);
	}

	fd->used = 0;
	return 0;
}

int vfs_fd_seek(struct FileDesc* fd, unsigned int off, enum FileSeekMode mode) {
	if (!fd->used) {
		return ERROR_INVAILD;
	}
	if (fd->dir) {
		return ERROR_INVAILD;
	}
	if (off >= fd->size) {
		return ERROR_INVAILD;
	}
	switch (mode) {
	case SEEK_SET:
		fd->offset = off;
		break;
	case SEEK_CUR:
		fd->offset += off;
		break;
	case SEEK_END:
		fd->offset = fd->size + off;
		break;
	}
	return fd->offset;
}
