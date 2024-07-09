/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI i386.c,v 2.2 1997/10/26 15:00:29 donn Exp
 */

#include <sys/types.h>
#include <a.out.h>
#include <stdlib.h>

#include "shlist.h"

#define	N_CODEOFF(e) \
	(N_TXTOFF(e) + (__HEADER_IN_TEXT(e) ? sizeof(e) : 0))

static const char codemagic[] = { 0x6a, 0, 0x6a, 0, 0xbe };

long
md_ltaddr(void *v)
{
	char *code = (char *)v + N_CODEOFF(*(struct exec *)v);
	long ltaddr;

	if (memcmp(code, codemagic, sizeof (codemagic)) != 0)
		return (0);

	memcpy(&ltaddr, code + sizeof (codemagic), sizeof (ltaddr));
	return (ltaddr);
}
