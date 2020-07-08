/*
 * dir program
 *
 * This file is part of HoleOS.
 *
 * HoleOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HoleOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HoleOS.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <holeos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define S_IFMT 0170000
#define S_IFDIR 0040000

char* dir_type_str(const char* name) {
	int mode = file_get_mode(name);
	if ((mode & S_IFMT) == S_IFDIR) {
		return "<DIR>";
	} else {
		return "     ";
	}
}

int main(int argc, char* argv[]) {
	char* dir_target;
	if (argc == 1) {
		dir_target = "/";
	} else {
		dir_target = argv[1];
	}
	int handle = dir_open(dir_target);
	if (handle < 0) {
		fputs("Directory not exist\n", stderr);
		exit(EXIT_FAILURE);
	}
	char buf[256];
	char fullname[256];
	int num = 0;
	while (dir_read(handle, buf)) {
		strcpy(fullname, dir_target);
		fullname[strlen(dir_target)] = '/';
		strcpy(fullname + strlen(dir_target) + 1, buf);
		printf("%s %s %d\n", dir_type_str(fullname), buf, file_get_size(fullname));
		num++;
	}
	printf("\nTotal: %d\n", num);
	return 0;
}
