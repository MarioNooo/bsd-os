/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI emulate_break.c,v 2.1 1995/02/03 15:13:29 polk Exp
 */

#include <sys/param.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>

#include "emulate.h"

extern char end;

/*
 * Code for the emulator's internal brk()/sbrk() routines.
 * This gives us a working malloc() in the emulator,
 * and thus our private stdio.
 */

vm_offset_t emulate_break_low;
vm_offset_t emulate_break_high;

/*
 * Initialize global variables for our internal fake brk()/sbrk().
 */
void
emulate_init_break()
{

	emulate_break_low = (vm_offset_t)&end;
	emulate_break_high = emulate_break_low;
}

/*
 * Brk() returns 0 on success...
 */
char *
brk(p)
	const char *p;
{
	vm_offset_t e = i386_round_page(emulate_break_high);
	vm_offset_t v = i386_round_page((vm_offset_t)p);

	if ((vm_offset_t)p < emulate_break_low) {
		errno = ENOMEM;
		return ((char *)-1);
	}

	if (v < e) {
		if (munmap((caddr_t)v, e - v) == -1)
		    	return ((char *)-1);
	}

	else if (v > e) {
		if (mmap((caddr_t)e, v - e, PROT_EXEC|PROT_READ|PROT_WRITE,
		    MAP_ANON|MAP_FIXED|MAP_PRIVATE, -1, 0) == (caddr_t)-1)
		    	return ((char *)-1);
	}

	emulate_break_high = (vm_offset_t)p;
	return ((char *)0);
}

/*
 * ... but sbrk() returns the previous break!
 */
char *
sbrk(n)
	int n;
{
	char *e = (char *)emulate_break_high;

	if (brk(e + n) == (char *)-1)
		return ((char *)-1);
	return (e);
}
