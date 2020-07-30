/*
 * FAT32 filesystem driver
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
#include <filesystem/vfs/vfs.h>
#include <hal/hal.h>

#include "fat32-struct.h"
#include "fat32.h"

static char* fat32_get_full_name(const struct FAT32DirEntry* dir, char* name) {
	// copy name, change to lower-case
	memmove(name, dir->name, 11);
	for (int i = 0; i < 11; i++) {
		if ((name[i] >= 'A') && (name[i] <= 'Z')) {
			name[i] += 'a' - 'A';
		}
	}
	// move extension to the right, reserve space for dot
	name[11] = name[10];
	name[10] = name[9];
	name[9] = name[8];
	// remove space at end of name
	unsigned int off = 7;
	while (name[off] == ' ') {
		off--;
	}
	// check if extension is empty, if so, do not put dot
	if (name[9] == ' ') {
		name[off + 1] = '\0';
		return name;
	}
	name[off + 1] = '.';
	for (int i = 2; i < 5; i++) {
		// copy extension,remove space at the end
		if (name[i + 7] == ' ') {
			name[off + i] = '\0';
			return name;
		}
		name[off + i] = name[i + 7];
	}
	name[off + 5] = '\0';
	return name;
}

static int fat32_dir_search(int partition_id, unsigned int cluster, const char* name,
							struct FAT32DirEntry* dir_dest) {
	for (int i = 0;; i += 32) {
		unsigned int clus = fat32_offset_cluster(partition_id, cluster, i);
		if (clus == 0) {
			break;
		}
		struct FAT32DirEntry dir;
		int errc = fat32_read_cluster(
			partition_id, &dir, clus,
			i % (fat32_superblock->sector_per_cluster * SECTORSIZE), sizeof(dir));
		if (errc < 0) {
			return errc;
		}
		if (dir.name[0] == 0xe5) {
			continue;
		} else if (dir.name[0] == 0) {
			break;
		} else if (dir.attr == ATTR_LONG_NAME) {
			continue;
		} else if (dir.attr == ATTR_VOLUME_ID) {
			continue;
		}
		char fullname[256];
		fat32_get_full_name(&dir, fullname);
		if (strncmp(name, fullname, 256) == 0) {
			memmove(dir_dest, &dir, sizeof(struct FAT32DirEntry));
			return i / 32;
		}
	}
	return ERROR_NOT_EXIST;
}

static int fat32_path_search(int partition_id, struct VfsPath path,
							 struct FAT32DirEntry* dir_dest) {
	unsigned int cluster = fat32_superblock->root_cluster;
	for (int i = 0; i < path.parts; i++) {
		int errc;
		if ((errc = fat32_dir_search(partition_id, cluster, path.pathbuf + i * 128,
									 dir_dest)) < 0) {
			return errc;
		}
		cluster = (dir_dest->cluster_hi << 16) | dir_dest->cluster_lo;
	}
	return 0;
}

int fat32_open(int partition_id, struct VfsPath path) {
	if (path.parts == 0) {
		return fat32_superblock->root_cluster;
	}
	struct FAT32DirEntry dir;
	int errc;
	dir.cluster_hi = dir.cluster_lo = 0;
	if ((errc = fat32_path_search(partition_id, path, &dir)) < 0) {
		return errc;
	}
	return (dir.cluster_hi << 16) | dir.cluster_lo;
}

int fat32_dir_first_file(int partition_id, unsigned int cluster) {
	for (int i = 0;; i += 32) {
		unsigned int clus = fat32_offset_cluster(partition_id, cluster, i);
		if (clus == 0) {
			break;
		}
		struct FAT32DirEntry dir;
		int errc = fat32_read_cluster(
			partition_id, &dir, clus,
			i % (fat32_superblock->sector_per_cluster * SECTORSIZE), sizeof(dir));
		if (errc < 0) {
			return errc;
		}
		if (dir.name[0] == 0xe5) {
			continue;
		} else if (dir.name[0] == 0) {
			break;
		} else if (dir.attr == ATTR_LONG_NAME) {
			continue;
		} else if (dir.attr == ATTR_VOLUME_ID) {
			continue;
		}
		return i / 32;
	}
	return -1;
}

int fat32_dir_read(int partition_id, char* buf, unsigned int cluster,
				   unsigned int entry) {
	for (int i = entry * 32;; i += 32) {
		unsigned int clus = fat32_offset_cluster(partition_id, cluster, i);
		if (clus == 0) {
			break;
		}
		struct FAT32DirEntry dir;
		int errc = fat32_read_cluster(
			partition_id, &dir, clus,
			i % (fat32_superblock->sector_per_cluster * SECTORSIZE), sizeof(dir));
		if (errc < 0) {
			return errc;
		}
		if (dir.name[0] == 0xe5) {
			continue;
		} else if (dir.name[0] == 0) {
			return 0;
		} else if (dir.attr == ATTR_LONG_NAME) {
			continue;
		} else if (dir.attr == ATTR_VOLUME_ID) {
			continue;
		}
		fat32_get_full_name(&dir, buf);
		return i / 32 + 1;
	}
	return 0;
}

int fat32_file_size(int partition_id, struct VfsPath path) {
	struct FAT32DirEntry dir;
	int errc = fat32_path_search(partition_id, path, &dir);
	if (errc < 0) {
		return errc;
	}
	return dir.size;
}

int fat32_file_mode(int partition_id, struct VfsPath path) {
	struct FAT32DirEntry dir;
	int errc = fat32_path_search(partition_id, path, &dir);
	if (errc < 0) {
		return errc;
	}
	int mode = 0777;
	if (dir.attr & ATTR_DIRECTORY) {
		mode |= 0040000;
	}
	return mode;
}

static char* fat32_file_get_short_name(char* shortname, const char* longname) {
	strncpy(shortname, "           ", 12);
	const char* s = longname;
	int p = 0;
	while (*s) {
		if (*s == '.') {
			s++;
			p = 8;
		}
		if (*s >= 'a' && *s <= 'z') {
			shortname[p] = *s - ('a' - 'A');
		} else {
			shortname[p] = *s;
		}
		p++;
		s++;
		if (p == 11) {
			break;
		}
	}
	return shortname;
}

static int fat32_dir_insert_entry(int partition_id, unsigned int cluster,
								  struct FAT32DirEntry* dirent) {
	for (int i = 0;; i += 32) {
		struct FAT32DirEntry dir;
		unsigned int clus = fat32_offset_cluster(partition_id, cluster, i);
		if (clus == 0) {
			break;
		}
		if (fat32_read_cluster(partition_id, &dir, clus,
							   i % (fat32_superblock->sector_per_cluster * SECTORSIZE),
							   sizeof(dir)) < 0) {
			return ERROR_READ_FAIL;
		}
		if (dir.name[0] == 0xe5 || dir.name[0] == 0x00) {
			if (fat32_write_cluster(
					partition_id, dirent, clus,
					i % (fat32_superblock->sector_per_cluster * SECTORSIZE),
					sizeof(dir)) < 0) {
				return ERROR_WRITE_FAIL;
			}
			return 0;
		}
	}
	// allocate a new cluster and attrach
	unsigned int alloc = fat32_allocate_cluster(partition_id);
	if (alloc == 0) {
		return ERROR_OUT_OF_SPACE;
	}
	fat32_write_cluster(partition_id, dirent, alloc, 0, sizeof(struct FAT32DirEntry));
	fat32_append_cluster(partition_id, cluster, alloc);
	return 0;
}

int fat32_mkdir(int partition_id, struct VfsPath path) {
	// get target directory
	unsigned int cluster;
	char* filename;
	if (path.parts > 1) {
		struct VfsPath prevpath;
		prevpath.parts = path.parts - 1;
		prevpath.pathbuf = path.pathbuf;
		struct FAT32DirEntry dir;
		fat32_path_search(partition_id, prevpath, &dir);
		cluster = (dir.cluster_hi << 16) | dir.cluster_lo;
		filename = path.pathbuf + prevpath.parts * 128;
	} else {
		cluster = fat32_superblock->root_cluster;
		filename = path.pathbuf;
	}
	// check exist
	struct FAT32DirEntry search_dir;
	if (fat32_dir_search(partition_id, cluster, filename, &search_dir) >= 0) {
		return ERROR_EXIST;
	}
	// create inode
	char shortname[12];
	fat32_file_get_short_name(shortname, filename);
	struct FAT32DirEntry dir;
	memset(&dir, 0, sizeof(dir));
	memmove(dir.name, shortname, 11);
	unsigned int alloc = fat32_allocate_cluster(partition_id);
	if (alloc == 0) {
		return ERROR_OUT_OF_SPACE;
	}
	dir.cluster_lo = alloc & 0xffff;
	dir.cluster_hi = (alloc >> 16) & 0xffff;
	dir.attr = ATTR_DIRECTORY;
	int ret;
	if ((ret = fat32_dir_insert_entry(partition_id, cluster, &dir)) < 0) {
		return ret;
	}
	// dot entry
	memset(&dir, 0, sizeof(dir));
	memmove(dir.name, ".          ", 11);
	dir.cluster_lo = alloc & 0xffff;
	dir.cluster_hi = (alloc >> 16) & 0xffff;
	dir.attr = ATTR_DIRECTORY;
	if ((ret = fat32_dir_insert_entry(partition_id, alloc, &dir)) < 0) {
		return ret;
	}
	// dotdot entry
	memset(&dir, 0, sizeof(dir));
	memmove(dir.name, "..         ", 11);
	if (cluster != fat32_superblock->root_cluster) {
		// not root cluster
		dir.cluster_lo = cluster & 0xffff;
		dir.cluster_hi = (cluster >> 16) & 0xffff;
	}
	dir.attr = ATTR_DIRECTORY;
	return fat32_dir_insert_entry(partition_id, alloc, &dir);
}
