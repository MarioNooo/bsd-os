/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1994,1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI loader.c,v 2.8 2001/10/03 17:30:00 polk Exp
 */

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/mman.h>

#include <machine/shlib.h>

ENTRY

/*
 * Internal bootstrap for fixed-address static shared libraries.
 * Called from the main bootstrap loader (embedded in the shared C library).
 */

void
loader(struct ldtab *lp, int f, int pagesz, struct initfini *ip)
{

	LOAD_SEGMENTS(lp, f, pagesz);
	DO_INIT(ip);

	/* library-specific initializations would go here */
}

LIBRARY
