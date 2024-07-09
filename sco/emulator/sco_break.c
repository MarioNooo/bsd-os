/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_break.c,v 2.1 1995/02/03 15:13:56 polk Exp
 */

#include <sys/param.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>

#include "emulate.h"
#include "sco.h"

/*
 * SVr3.2 brk()/sbrk().
 * We have to simulate this with mmap(),
 * since the break in a COFF execution environment
 * is in an inconvenient location for BSD.
 * Currently this is almost identical to emulate_break().
 */

vm_offset_t sco_break_low;
vm_offset_t sco_break_high;

int
sco_break(c)
	const char *c;
{
	vm_offset_t u = (vm_offset_t)c;
	vm_offset_t e = i386_round_page(sco_break_high);
	vm_offset_t v = i386_round_page(u);

	if (u < sco_break_low) {
		errno = ENOMEM;
		return (-1);
	}

	if (v < e) {
		if (munmap((caddr_t)v, e - v) == -1)
		    	return (-1);
	}

	else if (v > e) {
		if (mmap((caddr_t)e, v - e, PROT_EXEC|PROT_READ|PROT_WRITE,
		    MAP_ANON|MAP_FIXED|MAP_PRIVATE, -1, 0) == (caddr_t)-1)
		    	return (-1);
	}

	sco_break_high = u;
	return (0);
}
