/*
 * Window manager
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

#include <panicos.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_XRES 1024
#define DEFAULT_YRES 768
#define CURSOR_WIDTH 5
#define CURSOR_HEIGHT 5

extern unsigned char font16[256 * 16];

struct DisplayControl {
	int xres;
	int yres;
	void* framebuffer;
};

typedef struct {
	uint8_t b, g, r;
} __attribute__((packed)) COLOUR;

struct Window {
	int width, height;
	COLOUR* buffer;
	char title[64];
};

struct Sheet {
	int x, y, width, height;
	COLOUR* buffer;
	struct Window* window;
	struct Sheet* next;
};

// common used colour
COLOUR light_blue = {.r = 40, .g = 200, .b = 255};
COLOUR dark_blue = {.r = 0, .g = 0, .b = 255};
COLOUR red = {.r = 255, .g = 0, .b = 0};
COLOUR green = {.r = 0, .g = 255, .b = 0};
COLOUR gray = {.r = 200, .g = 200, .b = 200};
COLOUR white = {.r = 255, .g = 255, .b = 255};

COLOUR* fb; // framebuffer
int xres, yres;
int cur_x = 200, cur_y = 200;
struct Sheet* sheet_list = NULL; // sheets linked list

void wm_print_char(char c, COLOUR* buf, int x, int y, int width, COLOUR colour) {
	for (int i = 0; i < 16; i++) {
		COLOUR* p = buf + (y + i) * width + x;
		if (font16[c * 16 + i] & 1) {
			p[7] = colour;
		}
		if (font16[c * 16 + i] & 2) {
			p[6] = colour;
		}
		if (font16[c * 16 + i] & 4) {
			p[5] = colour;
		}
		if (font16[c * 16 + i] & 8) {
			p[4] = colour;
		}
		if (font16[c * 16 + i] & 16) {
			p[3] = colour;
		}
		if (font16[c * 16 + i] & 32) {
			p[2] = colour;
		}
		if (font16[c * 16 + i] & 64) {
			p[1] = colour;
		}
		if (font16[c * 16 + i] & 128) {
			p[0] = colour;
		}
	}
}

void wm_print_string(COLOUR* buf, const char* str, int x, int y, int width,
					 COLOUR colour) {
	for (int i = 0; (str[i] != '\0') && (str[i] != '\n'); i++) {
		wm_print_char(str[i], buf, x + i * 8, y, width, colour);
	}
}

void wm_fill_buffer(COLOUR* buf, int x, int y, int w, int h, int width, COLOUR colour) {
	for (int i = 0; i < w; i++) {
		for (int j = 0; j < h; j++) {
			buf[(y + j) * width + (x + i)] = colour;
		}
	}
}

void wm_copy_buffer(COLOUR* dest, int dest_x, int dest_y, int dest_width,
					const COLOUR* src, int src_x, int src_y, int src_width, int width,
					int height) {
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			dest[(dest_y + j) * dest_width + (dest_x + i)] =
				src[(src_y + j) * src_width + (src_x + i)];
		}
	}
}

void wm_render_window(struct Sheet* sht, COLOUR title_colour) {
	wm_fill_buffer(sht->buffer, 0, 0, sht->width, 32, sht->width, title_colour);
	wm_print_string(sht->buffer, sht->window->title, 8, 8, sht->width, white);
	wm_fill_buffer(sht->buffer, sht->width - 28, 4, 24, 24, sht->width, red);
	wm_print_string(sht->buffer, "X", sht->width - 20, 8, sht->width, white);
	wm_copy_buffer(sht->buffer, 0, 32, sht->width, sht->window->buffer, 0, 0,
				   sht->window->width, sht->window->width, sht->window->height);
}

struct Sheet* wm_create_sheet(int x, int y, int width, int height) {
	// allocate a Sheet object
	struct Sheet* sht = malloc(sizeof(struct Sheet));
	sht->next = NULL;
	sht->x = x;
	sht->y = y;
	sht->width = width;
	sht->height = height;
	sht->window = NULL;
	sht->buffer = malloc(sizeof(COLOUR) * width * height);
	// add to linked list
	if (sheet_list == NULL) {
		sheet_list = sht;
	} else {
		struct Sheet* l = sheet_list;
		while (l->next) {
			l = l->next;
		}
		if (l->window) {
			wm_render_window(l, gray);
		}
		l->next = sht;
	}
	return sht;
}

struct Sheet* wm_create_window(int width, int height, int x, int y) {
	struct Sheet* sht = wm_create_sheet(x, y, width, height + 32);
	sht->window = malloc(sizeof(struct Window));
	sht->window->width = width;
	sht->window->height = height;
	sht->window->title[0] = '\0'; // empty title
	sht->window->buffer = malloc(sizeof(COLOUR) * width * height);
	wm_fill_buffer(sht->window->buffer, 0, 0, width, height, width, white);
	wm_render_window(sht, dark_blue);
	return sht;
}

void wm_window_set_title(struct Sheet* sht, const char* title) {
	strcpy(sht->window->title, title);
	if (!sht->next) {
		wm_render_window(sht, dark_blue);
	} else {
		wm_render_window(sht, gray);
	}
}

void wm_draw_sheet(struct Sheet* sht) {
	wm_copy_buffer(fb, sht->x, sht->y, xres, sht->buffer, 0, 0, sht->width, sht->width,
				   sht->height);
}

void* wm_modeswitch(int xres, int yres) {
	struct DisplayControl dc = {.xres = xres, .yres = yres};
	if (kcall("display", (unsigned int)&dc) < 0) {
		fputs("wm: no display found\n", stderr);
		exit(EXIT_FAILURE);
	}
	return dc.framebuffer;
}

void sheet_remove(struct Sheet* to_remove) {
	free(to_remove->buffer);
	if (to_remove == sheet_list) {
		sheet_list = sheet_list->next;
	} else {
		struct Sheet* sht = sheet_list;
		while (sht->next != to_remove) {
			sht = sht->next;
			if (!sht) {
				return;
			}
		}
		sht->next = sht->next->next;
	}
	// cover old sheet
	wm_fill_buffer(fb, to_remove->x, to_remove->y, to_remove->width, to_remove->height,
				   xres, light_blue);
	free(to_remove);
	struct Sheet* todraw = sheet_list;
	while (todraw) {
		wm_draw_sheet(todraw);
		todraw = todraw->next;
	}
}

void window_remove(struct Sheet* to_remove) {
	free(to_remove->window->buffer);
	free(to_remove->window);
	sheet_remove(to_remove);
}

void window_move_top(struct Sheet* to_move) {
	if (to_move == sheet_list) {
		sheet_list = sheet_list->next;
	} else {
		struct Sheet* lst = sheet_list;
		while (lst->next != to_move) {
			lst = lst->next;
			if (!lst) {
				return;
			}
		}
		lst->next = lst->next->next;
	}

	// attach to end of linked lsit
	struct Sheet* sht = sheet_list;
	while (sht && sht->next) {
		sht = sht->next;
	}
	if (sht->window) {
		wm_render_window(sht, gray);
	}
	sht->next = to_move;
	wm_render_window(to_move, dark_blue);
	to_move->next = NULL;
	// draw buffers
	struct Sheet* todraw = sheet_list;
	while (todraw) {
		wm_draw_sheet(todraw);
		todraw = todraw->next;
	}
}

struct Sheet* win_moving = NULL;
unsigned int prev_x = 0, prev_y = 0;

void window_onclick(struct Sheet* sht, unsigned int btn) {
	if (btn & 1 && cur_y < sht->y + 32 && !win_moving) {
		if (cur_x >= sht->x + sht->width - 28 && cur_x < sht->x + sht->width - 4 &&
			cur_y >= sht->y + 4 && cur_y < sht->y + 28) {
			// close button
			window_remove(sht);
		} else {
			// start to move the window
			win_moving = sht;
			prev_x = cur_x - sht->x;
			prev_y = cur_y - sht->y;
			// move window to top
			if (sht->next) {
				window_move_top(sht);
			}
		}
	} else if (win_moving && !(btn & 1)) {
		win_moving = NULL;
	}
}

void mouse_cursor_event(void) {
	struct Sheet* sht = win_moving;
	if (win_moving && cur_y < sht->y + 32) {
		// cover old window
		wm_fill_buffer(fb, sht->x, sht->y, sht->width, sht->height, xres, light_blue);
		sht->x = cur_x - prev_x;
		sht->y = cur_y - prev_y;
		// draw buffers
		struct Sheet* todraw = sheet_list;
		while (todraw) {
			wm_draw_sheet(todraw);
			todraw = todraw->next;
		}
	}
}

void mouse_button_event(unsigned char btn) {
	struct Sheet* sht = sheet_list;
	while (sht) {
		if (cur_x >= sht->x && cur_x < sht->x + sht->width && cur_y >= sht->y &&
			cur_y < sht->y + sht->height) {
			if (sht->window) {
				window_onclick(sht, btn);
			}
			return;
		}
		sht = sht->next;
	}
}

int main(int argc, char* argv[]) {
	if (argc == 1) {
		xres = DEFAULT_XRES;
		yres = DEFAULT_YRES;
	} else if (argc == 3) {
		xres = atoi(argv[1]);
		yres = atoi(argv[2]);
	} else {
		fputs("Usage: wm [xres yres]\n", stderr);
		exit(EXIT_FAILURE);
	}
	fb = wm_modeswitch(xres, yres);

	for (int x = 0; x < xres; x++) {
		for (int y = 0; y < yres; y++) {
			fb[y * xres + x].r = 40;
			fb[y * xres + x].g = 200;
			fb[y * xres + x].b = 255;
		}
	}

	struct Sheet* win_a = wm_create_window(300, 300, 100, 200);
	wm_print_string(win_a->window->buffer, "Window A", 10, 10, 300, green);
	wm_window_set_title(win_a, "Window A");
	struct Sheet* win_b = wm_create_window(250, 400, 500, 200);
	wm_print_string(win_b->window->buffer, "Window B", 10, 10, 250, green);
	wm_window_set_title(win_b, "Window B");
	// draw sheets
	struct Sheet* sht = sheet_list;
	while (sht) {
		wm_draw_sheet(sht);
		sht = sht->next;
	}
	// main loop
	for (;;) {
		int m;
		kcall("mouse", (unsigned int)&m);
		if (!m) {
			continue;
		}
		// remove old cursor
		wm_fill_buffer(fb, cur_x, cur_y, CURSOR_WIDTH, CURSOR_HEIGHT, xres, light_blue);
		// draw sheets
		struct Sheet* sht = sheet_list;
		while (sht) {
			wm_draw_sheet(sht);
			sht = sht->next;
		}
		// mouse cursor
		char movex = (char)((m >> 16) & 0xff);
		char movey = (char)((m >> 8) & 0xff);
		cur_x += movex;
		cur_y -= movey;
		if (movex || movey) {
			mouse_cursor_event();
		}
		// draw cursor
		wm_fill_buffer(fb, cur_x, cur_y, CURSOR_WIDTH, CURSOR_HEIGHT, xres, red);
		// mouse buttons
		int btn = (m >> 24) & 7;
		if (btn & 1) {
			wm_fill_buffer(fb, 0, 0, 50, 50, xres, red);
		} else {
			wm_fill_buffer(fb, 0, 0, 50, 50, xres, green);
		}
		if (btn & 4) {
			wm_fill_buffer(fb, 50, 0, 50, 50, xres, red);
		} else {
			wm_fill_buffer(fb, 50, 0, 50, 50, xres, green);
		}
		if (btn & 2) {
			wm_fill_buffer(fb, 100, 0, 50, 50, xres, red);
		} else {
			wm_fill_buffer(fb, 100, 0, 50, 50, xres, green);
		}
		static int prevbtn = 0;
		if (prevbtn != btn) {
			mouse_button_event(btn);
			prevbtn = btn;
		}
	}

	return 0;
}