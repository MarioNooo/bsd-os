/* mtest.c - MemTest-86 */

/* Copyright 1996,  Chris Brady
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without fee
 * is granted provided that the above copyright notice appears in all copies.
 * It is provided "as is" without express or implied warranty.
 */

#include <linux/tty.h>
#include <linux/sched.h>
#include <asm/io.h>
#include "mtest.h"

unsigned short memsz;

extern __inline__ void cache_off()
{
        __asm__("push %eax\n\t"
		"movl %cr0,%eax\n\t"
                "orl $0x60000000,%eax\n\t"
                "movl %eax,%cr0\n\t"
		".byte 0x0f,0x08\n\t"
		"pop  %eax\n\t");
}
extern __inline__ void cache_on()
{
        __asm__("push %eax\n\t"
		"movl %cr0,%eax\n\t"
                "andl $0x9fffffff,%eax\n\t"
                "movl %eax,%cr0\n\t"
		"pop  %eax\n\t");
}

struct segs {
	long *start;
	long *end;
} seg[10];

long *start;
long *end;
long p1, p2;
char buf[12];
int pass = 0;
int ecount = 0;
int segs, p0, emsg = 0;
long *eadr, exor;
long *p;
char *pp;
int i, cache_flag, ref_flag;
long m_lim;

char spin[] = {'/', '-', '\\', '|'};

asmlinkage void do_test(void)
{
	/*
	 * Make a spiffy title on a blue background
	 */
	for(i=0, pp=(char *)0xb8001; i<17; i++, pp+=2) {
		*pp = 0x017;
	}
	cprint(0, 0, " MEMTEST-86 v1.1 ");
	cprint(0, 31, "MemSz-BIOS:");
	dprint(0, 45, memsz+1024, 5);
	cprint(0, 50, "k");
	
	/*
	 * Find all segments of RAM
	 *
	 * We test for DRAM by reading a location and then writing  the
	 * complement. If any bits change then we assume that it is DRAM.
	 * To save time we only do this check every 64 bytes.
	 */
	cprint(2, 0, "Testing: ");

	i = 0;
	m_lim = (memsz * 1024) + (1024 * 1024);
	p = (long *)START_ADR;
	seg[i].start = p;
	dprint(2, 9, (long)p/1024, 5);
	cprint(2, 14, "k - ");
	while ((long)p < m_lim) {
		/*
		 * Skip over the display memory
		 */
		if (((long)p & 0xfffff000) == 0xb8000) {
			seg[i].end = p;
			dprint(2+i, 18, (long)p/1024, 5);
			cprint(2+i, 23, "k");
			p = (long *)0xc0000;
			i++;
			seg[i].start = 0;
			goto fstart;
		}
		p1 = *p;
		*p = ~p1;
		if (*p == p1) {
			/*
			 * ROM or nothing at this address, record end addrs
			 */
			seg[i].end = p;
			dprint(2 + i, 18, (long)p/1024, 5);
			cprint(2 + i, 23, "k");
			i++;
			seg[i].start = 0;
fstart:
			while ((long)p < m_lim) {
				/*
				 * Skip over video memory
				 */
				if (((long)p & 0xfffff000) == 0xb8000) {
					p = (long *)0xc0000;
				}
				p1 = *p;
				*p =  ~p1;
				if (*p != p1) {
					/*
					 * More RAM, record start addrs
					 */
					seg[i].start = p;
					dprint(2 + i, 9, (long)p/1024, 5);
					cprint(2 + i, 14, "k - ");
					break;
				}
				p += 64;
			}
		}
		p += 64;
	}
	if (seg[i].start) {
		seg[i].end = p;
		dprint(2 + i, 18, (long)p/1024, 5);
		cprint(2 + i, 23, "k");
		i++;
		seg[i].start = 0;
	}
	segs = i;

	cprint(1, 34, "Pattern:");
	cprint(2, 34, "Refresh:");
	cprint(2, 43, " Default");
	cprint(0, 63, "  Pass:");
	dprint(0, 71, pass, 5);
	cprint(1, 63, "Errors:");
	dprint(1, 71, ecount, 5);
	cprint(2, 63, " Cache:");
	cprint(2, 73, " ON");


	/*
	 * Main Loop
	 */
	while (1) {
#ifdef CACHE
		/*
		 * Alternate cache on and off for each pass
		 */
		if (cache_flag) {
			cache_flag = 0;
			cprint(2, 73, "OFF");
			cache_off();
		} else {
			cache_flag++;
			cprint(2, 73, " ON");
			cache_on();
#endif
#ifdef REFRESH
			/*
			 * For every 2 passes alternate with short and long
			 * refresh rates
			 */
			if (ref_flag) {
				ref_flag = 0;
				/* set refresh to 150ms */
				outb(0x74, 0x43);
				outb(0xb4, 0x41);
				outb(0x00, 0x41);
				cprint(2, 43, "Extended (150ms)");
			} else {
				ref_flag++;
				/* set refresh to 15ms */
				outb(0x74, 0x43);
				outb(0x12, 0x41);
				outb(0x00, 0x41);
				cprint(2, 43, "  Normal (15ms) ");
			}
#endif
#ifdef CACHE
		}
#endif
		/*
		 * Use a 4 bit wide walking ones pattern and it's complement.
		 * This will check out 4 bit wide chips.  This should be
		 * changed if chips more than 4 bits wide become available.
		 */
		p0 = 8;
		for (i=0; i<5; i++, p0=p0>>1) {
			p1 = p0 | (p0<<4) | (p0<<8) | (p0<<12) | (p0<<16) |
				(p0<<20) | (p0<<24) | (p0<<28);
			p2 = ~p1;
			check();
		
			/*
			 * Switch patterns
			 */
			p2 = p1;
			p1 = ~p2;
			check();
		}
		dprint(0, 71, ++pass, 5);
	}
}

/*
 * Test all of memory using a "moving inversions" algorithm using the
 * pattern in p1 and it's complement in p2.
 */
check()
{
	register int i, j;
	register long *p;
	char *s = spin;

        hprint(1, 43, p1);

	/*
	 * Initialize memory with the initial pattern.
	 */
	for (j=0; j<segs; j++) {
		start = seg[j].start;
		end = seg[j].end;
		for (p = start; p < end; p++) {
			*p = p1;
		}
	}
	/*
	 * Do moving inversions test. Check for initial pattern and then
	 * write the complement for each memory location. Test from bottom
	 * up and then from the top down.
	 */
	for (i=0; i<3; i++) {
		for (j=0; j<segs; j++) {
			start = seg[j].start;
			end = seg[j].end;
			for (p = start; p < end; p++) {
				if (*p != p1) {
					error(p, p1, *p);
				}
				*p = p2;
			}
		}
		do_spin();
		for (j=segs-1; j>=0; j--) {
			start = seg[j].start;
			end = seg[j].end;
			for (p = end - 1; p >= start; p--) {
				if (*p != p2) {
					error(p, p2, *p);
				}
				*p = p1;
			}
		}
		do_spin();
	}
}

/*
 * Display data error message. Don't display repeat duplicate errors.
 */
error(adr, good, bad)
long *adr;
long good;
long bad;
{
	long xor;

	xor = good ^ bad;
	if (emsg < 23-segs && (adr != eadr || xor != exor)) {
		cprint(emsg+segs+3, 0, "Data Error - Addrs: ");
		hprint(emsg+segs+3, 20, adr);
		cprint(emsg+segs+3, 30, "Good: ");
		hprint(emsg+segs+3, 36, good);
		cprint(emsg+segs+3, 46, "Bad: ");
		hprint(emsg+segs+3, 51, bad);
		cprint(emsg+segs+3, 61, "Xor: ");
		hprint(emsg+segs+3, 66, xor);
		eadr = adr;
		exor = xor;
		emsg++;
	}
	dprint(1, 71, ++ecount, 5);
}

/*
 * Print characters on screen
 */
cprint(y, x, text)
int y;
int x;
char *text;
{
	register int i;
	char *dptr;

	dptr = (char *)(0xb8000 + (160*y) + (2*x));
	for (i=0; i < text[i]; i++) {
		*dptr = text[i];
		dptr += 2;
	}
}

/*
 * Print a decimal number on screen
 */
dprint(x, y, val, len)
int x;
int y;
long val;
int len;
{
        long j, k;
        int i, flag=0;
        char c;

        for(i=0, j=1; i<len-1; i++) {
                j *= 10;
        }
        for (i=0; j>0; j/=10) {
                k = val/j;
                if (k > 9) {
                        j *= 100;
                        continue;
                }
                if (flag || k || j == 1) {
                        buf[i++] = k + '0';
			flag++;
                } else {
			buf[i++] = ' ';
		}
                val -= k * j;
        }
        buf[i] = 0;
	cprint(x, y, buf);
}

/*
 * Print a hex number on screen
 */
hprint(x, y, val)
int x;
int y;
long val;
{
	long j;
	int i, idx, flag = 0;
	char c;

        for (i=0, idx=0; i<8; i++) {
                j = val >> (28 - (4 * i));
		j &= 0xf;
		if (j < 10) {
			if (flag || j || i == 7) {
		                buf[idx++] = j + '0';
				flag++;
			} else {
				buf[idx++] = ' ';
			}
		} else {
			buf[idx++] = j + 'a' - 10;
			flag++;
		}
        }
        buf[idx] = 0;
	cprint(x, y, buf);
}
	
/*
 * Display a spinning pattern to show progress
 */
do_spin()
{
	static int s=0;
	char *dptr = (char *)0xb8024;

	*dptr = spin[s]&0x7f;
	dptr++;
	*dptr = 0xf;
	if (++s > 3) {
		s = 0;
	}
}
