/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sparc.c,v 2.3 1998/09/10 21:52:19 torek Exp
 */

#include <sys/types.h>
#include <a.out.h>
#include <stdlib.h>

#include "shlist.h"

#define	N_CODEOFF(e) \
	(N_TXTOFF(e) + ((e).a_magic == ZMAGIC ? sizeof(e) : 0))

static const char codemagic[] =
    { 0x9d, 0xe3, 0xbf, 0x90, 0xb4, 0x10, 0x00, 0x01 };

#define	HI22(x)	((x) & ((1 << 22) - 1))
#define	LO10(x)	((x) & ((1 << 10) - 1))

long
md_ltaddr(void *v)
{
	char *code = (char *)v + N_CODEOFF(*(struct exec *)v);
	unsigned int *ip;
	long ltaddr;

	if (memcmp(code, codemagic, sizeof (codemagic)) != 0)
		return (0);

	ip = (unsigned int *)(code + sizeof (codemagic));
	ltaddr = HI22(*ip++) << 10;
	ltaddr |= LO10(*ip);
	return (ltaddr);
}
