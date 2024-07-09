#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>


#define USAGE "Usage: %s [flags]\n\
	-s	Memory to test (MB) (def: 1MB)\n\
	-p	Pass count (0 == don't stop, def=10)\n\
	-q	Supress verbose progress reports\n"

void *memstart;
void *memend;
u_long memsize = 1024 * 1024;
int vflag = 1;
u_long p1;
u_long p2;
u_long ecount = 0;

int main ( int ac, char **av );
void usage ( char *self );
void do_pass ( void );
void addrtest( void );
void bitpat ( void );
void inv ( void );
int check ( void );
void error ( char *desc, u_long *adr, u_long exp, u_long act );

int
main(int ac, char **av)
{
	int c;
	int pcount = 10;

	setvbuf(stdout, NULL, _IONBF, 0);
	while ((c = getopt(ac, av, "s:vp:")) != EOF) {
		switch (c) {
		case 's':
			memsize = 1024 * 1024 * strtol(optarg, NULL, 0);
			break;
		case 'q':
			vflag = 0;
			break;
		case 'p':
			pcount = strtol(optarg, NULL, 0);
			break;
		default:
			usage(av[0]);
		}
	}
	if (ac != optind)
		usage(av[0]);
	memstart = malloc(memsize);
	if (memstart == NULL)
		err(1, "Couldn't allocate 0x%x bytes (%d MB) of memory",
		    memsize, memsize / (1024 * 1024));
	memend = (u_char *)memstart + memsize;
	printf("Testing 0x%x bytes (%d MB) of memory\n", memsize, 
	    memsize / (1024 * 1024));
	printf("Pass:");
	if (pcount == 0) {
		for (;;) {
			printf(" %d", ++pcount);
			do_pass();
		}
	} else {
		while (pcount >= 0) {
			printf(" %d", pcount);
			do_pass();
			pcount--;
		}
		printf("\n");
	}
}

void
usage(char *self)
{
	fprintf(stderr,USAGE,self);
	exit(1);
}

void
do_pass()
{
	if (vflag)
		printf("B");
	bitpat();
	if (vflag)
		printf("A");
	addrtest();
	if (vflag)
		printf("I");
	inv();
}



u_long pats[] = {
	0,
	0xffffffff,
	0xaaaaaaaa,
	0x55555555,
	0xcccccccc,
	0x33333333,
};
#define	NPATS	(sizeof(pats) / sizeof(u_long))

/*
 * Simple test that fills and checks several bit patterns
 */
void
bitpat()
{
	u_long *p;
	int spos = 0;
	u_long val;
	u_long act;

	for (spos = 0; spos < NPATS; spos++) {
		val = pats[spos];
		/* Fill */
		for (p = (u_long *)memstart; p < (u_long *)memend; p++) {
			*p = val;
			act = *p;
			if (val != act)
				error("bit fill", p, val, act);
		}
		/* Check */
		for (p = (u_long *)memstart; p < (u_long *)memend; p++) {
			act = *p;
			if (val != act)
				error("bit check", p, val, act);
		}
	}
}

/*
 * Simple address test
 */
void
addrtest()
{
	u_long *p;
	int spos = 0;
	u_long val;
	u_long act;

	/* Fill with address dependent data */
	for (p = (u_long *)memstart; p < (u_long *)memend; p++) {
		val = (u_long)p ^ ((u_long)p << 16);
		*p = val;
		act = *p;
		if (val != act)
			error("addr fill", p, val, act);
	}
	/* Check */
	for (p = (u_long *)memstart; p < (u_long *)memend; p++) {
		val = (u_long)p ^ ((u_long)p << 16);
		act = *p;
		if (val != act)
			error("addr check", p, val, act);
	}
}

/* mtest.c - MemTest-86 */

/* Copyright 1996,  Chris Brady
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without fee
 * is granted provided that the above copyright notice appears in all copies.
 * It is provided "as is" without express or implied warranty.
 */

void
inv()
{
	int i;
	int p0;
	int ipass;

	/*
	 * Main Loop
	 */
	for (ipass = 0; ipass < 1; ipass++) {
		/*
		 * Use a 4 bit wide walking ones pattern and it's complement.
		 * This will check out 4 bit wide chips.  This should be
		 * changed if chips more than 4 bits wide become available.
		 * First walk the bit to the left and then right.
		 */
		p0 = 1;
		for (i=0; i<4; i++, p0=p0<<1) {
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
	}
}

/*
 * Test all of memory using a "moving inversions" algorithm using the
 * pattern in p1 and it's complement in p2.
 */
check()
{
	register int i, j;
	register u_long *p;
	u_long *start;
	u_long *end;

	/*
	 * Initialize memory with the initial pattern.
	 */
	start = memstart;
	end = memstart + memsize;
	for (p = start; p < end; p++) {
		*p = p1;
	}

	/*
	 * Do moving inversions test. Check for initial pattern and then
	 * write the complement for each memory location. Test from bottom
	 * up and then from the top down.
	 */
	for (i=0; i<4; i++) {
		start = memstart;
		end = memend;
		for (p = start; p < end; p++) {
			if (*p != p1) {
				error("inv1 fwd", p, p1, *p);
			}
			*p = p2;
		}
		start = memstart;
		end = memend;
		for (p = end - 1; p >= start; p--) {
			if (*p != p2) {
				error("inv back", p, p2, *p);
			}
			*p = p1;
		}
	}
}

/*
 * Display data error message
 */
void
error(char *desc, u_long *adr, u_long exp, u_long act)
{
	printf("\n%8.8x: Exp=%8.8x Act=%8.8x Bad=%8.8x %s\n",
			adr, exp, act, exp ^ act, desc);
}
