/*
 * Console input and output
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

// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include <common/sleeplock.h>
#include <common/spinlock.h>
#include <common/x86.h>
#include <core/mmu.h>
#include <core/proc.h>
#include <core/traps.h>
#include <defs.h>
#include <driver/uart.h>
#include <memlayout.h>
#include <param.h>

// comment of the following lines for release build
#define USE_DEBUGCON
#define DEBUGCON_ADDR 0xe9

static void consputc(int);

static int panicked = 0;

static struct {
	struct spinlock lock;
	int locking;
} cons;

static void printint(int xx, int base, int sign) {
	static char digits[] = "0123456789abcdef";
	char buf[16];
	int i;
	unsigned int x;

	if (sign && (sign = xx < 0))
		x = -xx;
	else
		x = xx;

	i = 0;
	do {
		buf[i++] = digits[x % base];
	} while ((x /= base) != 0);

	if (sign)
		buf[i++] = '-';

	while (--i >= 0)
		consputc(buf[i]);
}

// Print to the console. only understands %d, %x, %p, %s.
void cprintf(const char* fmt, ...) {
	int i, c, locking;
	unsigned int* argp;
	char* s;

	locking = cons.locking;
	if (locking)
		acquire(&cons.lock);

	if (fmt == 0)
		panic("null fmt");

	argp = (unsigned int*)(void*)(&fmt + 1);
	for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
		if (c != '%') {
			consputc(c);
			continue;
		}
		c = fmt[++i] & 0xff;
		if (c == 0)
			break;
		switch (c) {
		case 'd':
			printint(*argp++, 10, 1);
			break;
		case 'x':
		case 'p':
			printint(*argp++, 16, 0);
			break;
		case 's':
			if ((s = (char*)*argp++) == 0)
				s = "(null)";
			for (; *s; s++)
				consputc(*s);
			break;
		case '%':
			consputc('%');
			break;
		default:
			// Print unknown % sequence to draw attention.
			consputc('%');
			consputc(c);
			break;
		}
	}

	if (locking)
		release(&cons.lock);
}

void panic(const char* s) {
	int i;
	unsigned int pcs[10];

	cli();
	cons.locking = 0;
	// use lapiccpunum so that we can call panic from mycpu()
	cprintf("lapicid %d: panic: ", lapicid());
	cprintf(s);
	cprintf("\n");
	getcallerpcs(&s, pcs);
	for (i = 0; i < 10; i++)
		cprintf(" %p", pcs[i]);
	panicked = 1; // freeze other CPU
	for (;;)
		;
}

// PAGEBREAK: 50
#define BACKSPACE 0x100
#define CRTPORT 0x3d4
static unsigned short* crt = (unsigned short*)P2V(0xb8000); // CGA memory

static void cgaputc(int c) {
	static int pos = 0;

	if (c == '\n')
		pos += 80 - pos % 80;
	else if (c == BACKSPACE) {
		if (pos > 0)
			--pos;
	} else
		crt[pos++] = (c & 0xff) | 0x0700; // black on white

	if (pos < 0 || pos > 25 * 80)
		panic("pos under/overflow");

	if ((pos / 80) >= 24) { // Scroll up.
		memmove(crt, crt + 80, sizeof(crt[0]) * 23 * 80);
		pos -= 80;
		memset(crt + pos, 0, sizeof(crt[0]) * (24 * 80 - pos));
	}

	crt[pos] = ' ' | 0x0700;
}

void consputc(int c) {
	if (panicked) {
		cli();
		for (;;)
			;
	}

	if (c == BACKSPACE) {
		uart_putc('\b');
		uart_putc(' ');
		uart_putc('\b');
	} else
		uart_putc(c);
	cgaputc(c);
#ifdef USE_DEBUGCON
	outb(DEBUGCON_ADDR, c);
#endif
}

#define INPUT_BUF 128
struct {
	char buf[INPUT_BUF];
	unsigned int r; // Read index
	unsigned int w; // Write index
	unsigned int e; // Edit index
} input;

#define C(x) ((x) - '@') // Control-x

void consoleintr(int (*getc)(void)) {
	int c;

	acquire(&cons.lock);
	while ((c = getc()) >= 0) {
		switch (c) {
		case C('U'): // Kill line.
			while (input.e != input.w && input.buf[(input.e - 1) % INPUT_BUF] != '\n') {
				input.e--;
				consputc(BACKSPACE);
			}
			break;
		case C('H'):
		case '\x7f': // Backspace
			if (input.e != input.w) {
				input.e--;
				consputc(BACKSPACE);
			}
			break;
		default:
			if (c != 0 && input.e - input.r < INPUT_BUF) {
				c = (c == '\r') ? '\n' : c;
				input.buf[input.e++ % INPUT_BUF] = c;
				consputc(c);
				if (c == '\n' || c == C('D') || input.e == input.r + INPUT_BUF) {
					input.w = input.e;
					wakeup(&input.r);
				}
			}
			break;
		}
	}
	release(&cons.lock);
}

int consoleread(char* dst, int n) {
	unsigned int target;
	int c;

	target = n;
	acquire(&cons.lock);
	while (n > 0) {
		while (input.r == input.w) {
			if (myproc()->killed) {
				release(&cons.lock);
				return -1;
			}
			sleep(&input.r, &cons.lock);
		}
		c = input.buf[input.r++ % INPUT_BUF];
		if (c == C('D')) { // EOF
			if (n < target) {
				// Save ^D for next time, to make sure
				// caller gets a 0-byte result.
				input.r--;
			}
			break;
		}
		*dst++ = c;
		--n;
		if (c == '\n')
			break;
	}
	release(&cons.lock);

	return target - n;
}

int consolewrite(char* buf, int n) {
	int i;

	acquire(&cons.lock);
	for (i = 0; i < n; i++)
		consputc(buf[i] & 0xff);
	release(&cons.lock);

	return n;
}

void consoleinit(void) {
	initlock(&cons.lock, "console");
	cons.locking = 1;
}
