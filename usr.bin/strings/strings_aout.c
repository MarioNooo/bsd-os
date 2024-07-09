/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI strings_aout.c,v 2.3 2001/10/03 17:29:58 polk Exp
 */

#include <sys/types.h>
#include <a.out.h>
#include <stdio.h>

#include "strings_private.h"

#ifdef	OMAGIC
enum aout_state { TEXT=0, DATA, DONE };
static enum aout_state state;

static struct exec a;

/*
 * Arrange to scan through the text and data sections of an a.out binary.
 */
static off_t
aout_section(off_t *op)
{
	off_t size;

	switch (state) {
	case TEXT:
		*op = N_TXTOFF(a);
		size = a.a_text;
		state = DATA;
		break;
	case DATA:
		*op = N_DATOFF(a);
		size = a.a_data;
		state = DONE;
		break;
	case DONE:
		*op = 0;
		size = 0;
		break;
	}

	return (size);
}

/*
 * See whether this is an a.out binary file.
 * If so, tell our caller to use aout_section() to scan the file.
 */
sec_t
check_aout(FILE *f, off_t *op)
{
	size_t n;

	state = TEXT;

	/*
	 * If we can reject this file based on just the magic number,
	 * do so and save the effort of rejecting the entire header.
	 */
	if ((n = fread((char *)&a.a_magic, 1, sizeof (a.a_magic), f)) == 0)
		return (NULL);
	if (n != sizeof (a.a_magic) || N_BADMAG(a)) {
		unget(&a.a_magic, n, f);
		return (NULL);
	}

	/* Read the rest of the a.out header.  */
	n += fread((char *)(&a.a_magic + 1), 1, sizeof (a) - n, f);
	if (ferror(f))
		return (NULL);
	if (n != sizeof (a)) {
		unget(&a, n, f);
		return (NULL);
	}

	*op = sizeof (a);

	return (aout_section);
}
#endif
