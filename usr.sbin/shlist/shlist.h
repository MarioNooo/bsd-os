/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI shlist.h,v 2.2 1997/10/26 15:00:30 donn Exp
 */

int _aout_shlist(const char *, void *, size_t);
int _elf32_shlist(const char *, void *, size_t);
int _elf64_shlist(const char *, void *, size_t);

long md_ltaddr(void *);

extern int aflag;

struct ldtab {
	char *name;
	void *address;
};
