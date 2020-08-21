/*
 * Kernel module loader
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

#include <common/spinlock.h>
#include <defs.h>
#include <filesystem/vfs/vfs.h>
#include <proc/exec/elf.h>

#define MAX_MODULES 64

struct ModuleInfo {
	char name[64];
	unsigned int base;
} module_info[MAX_MODULES];

static int module_elf_load(pde_t* pgdir, unsigned int base, const char* name,
						   unsigned int* entry) {
	struct FileDesc fd;
	if (vfs_fd_open(&fd, name, O_READ) < 0) {
		return -1;
	}

	struct elfhdr elf;
	if (vfs_fd_read(&fd, (char*)&elf, sizeof(elf)) != sizeof(elf)) {
		vfs_fd_close(&fd);
		return -1;
	}
	if (elf.magic != ELF_MAGIC) {
		vfs_fd_close(&fd);
		return -1;
	}
	*entry = elf.entry;

	struct proghdr ph;
	unsigned int sz = 0;
	for (unsigned int off = elf.phoff; off < elf.phoff + elf.phnum * sizeof(ph);
		 off += sizeof(ph)) {
		if (vfs_fd_seek(&fd, off, SEEK_SET) < 0) {
			vfs_fd_close(&fd);
			return -1;
		}
		if (vfs_fd_read(&fd, (char*)&ph, sizeof(ph)) != sizeof(ph)) {
			vfs_fd_close(&fd);
			return -1;
		}
		if (ph.type != ELF_PROG_LOAD) {
			continue;
		}
		if (allocuvm(pgdir, base + ph.vaddr - ph.vaddr % PGSIZE,
					 base + ph.vaddr + ph.memsz,
					 (ph.flags & ELF_PROG_FLAG_WRITE) ? PTE_W : 0) == 0) {
			vfs_fd_close(&fd);
			return -1;
		}
		if (ph.vaddr + ph.memsz > sz) {
			sz = ph.vaddr + ph.memsz;
		}
		if (loaduvm(pgdir, (char*)base + ph.vaddr - ph.vaddr % PGSIZE, &fd,
					ph.off - ph.vaddr % PGSIZE, ph.vaddr % PGSIZE + ph.filesz) < 0) {
			vfs_fd_close(&fd);
			return -1;
		}
	}

	vfs_fd_close(&fd);

	return sz;
}

static unsigned int module_base = PROC_MODULE_BOTTOM;
extern pde_t* kpgdir;

void module_info_add(const char* name, unsigned int base) {
	for (int i = 0; i < MAX_MODULES; i++) {
		if (!module_info[i].base) {
			strncpy(module_info[i].name, name, 64);
			module_info[i].base = base;
			break;
		}
	}
}

int module_load(const char* name) {
	unsigned int entry;
	unsigned int load_base = module_base;
	int ret = module_elf_load(kpgdir, load_base, name, &entry);
	if (ret < 0) {
		return ret;
	}
	module_info_add(name, load_base);
	acquire(&ptable.lock);
	for (int i = 0; i < NPROC; i++) {
		if (ptable.proc[i].state != UNUSED && ptable.proc[i].state != EMBRYO) {
			if (copypgdir(ptable.proc[i].pgdir, kpgdir, load_base, load_base + ret) ==
				0) {
				panic("module load copy failed");
			}
		}
	}
	release(&ptable.lock);
	module_base += PGROUNDUP(ret);
	void (*module_entry_point)(void) = (void (*)(void))(load_base + entry);
	module_entry_point();
	return 0;
}

void module_set_pgdir(pde_t* pgdir) {
	if (copypgdir(pgdir, kpgdir, PROC_MODULE_BOTTOM, module_base) == 0) {
		panic("module_set_pgdir failed");
	}
}

void module_print(void) {
	cprintf("Kernel modules:\n");
	for (int i = 0; i < MAX_MODULES; i++) {
		if (module_info[i].base) {
			cprintf("%x %s\n", module_info[i].base, module_info[i].name);
		}
	}
}

static struct KernerServiceTable {
	// basic functions
	void (*cprintf)(const char*, ...);
}* kernsrv = (void*)0x80010000;

void module_init(void) {
	memset(module_info, 0, sizeof(module_info));
	kernsrv->cprintf = cprintf;
}
