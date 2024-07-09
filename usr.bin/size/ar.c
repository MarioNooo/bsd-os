/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI ar.c,v 2.6 2001/10/03 17:29:57 polk Exp
 */

#include <sys/param.h>
#include <a.out.h>
#include <ar.h>
#include <err.h>
#include <errno.h>
#include <ranlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "size.h"

static int (* const asize[])(const char *, const void *, size_t) = {
#ifdef	NMAGIC
	_aout_size,
#endif
	_elf32_size,
	_elf64_size
};

/*
 * Print sizes for members of an archive.
 * Return 0 if this isn't an archive;
 * return 1 otherwise.
 */
int
_ar_size(const char *name, const void *v, size_t len)
{
	const char *base = v;
	const char *arhdr;
	const char *member_data;
	const struct ar_hdr *ar;
	size_t namelen = strlen(name);
	const char *dict = NULL;
	size_t dictlen;
	const char *member;
	size_t memberlen;
	char *fullname;
	const char *p;
	const char ranlibmag[] = RANLIBMAG;
	const size_t ranlibmaglen = sizeof (ranlibmag) - 1;
	size_t o;
	size_t i;
	size_t arlen;
#ifdef _STRICT_ALIGN
	void *copy;
#endif

	if (len < SARMAG)
		return (0);
	if (memcmp(base, ARMAG, SARMAG) != 0)
		return (0);

	for (arhdr = base + SARMAG;
	    (member_data = arhdr + sizeof *ar) <= base + len;
	    arhdr += sizeof *ar + o) {
		ar = (const struct ar_hdr *)arhdr;
		o = roundup(strtoul(ar->ar_size, NULL, 10), 2);

		if (memcmp(ar->ar_name, ranlibmag, ranlibmaglen) == 0 &&
		    ar->ar_name[ranlibmaglen] == ' ')
			continue;
		if (ar->ar_name[0] == '/' && ar->ar_name[1] == ' ')
			continue;

		if (ar->ar_name[0] == '/' && ar->ar_name[1] == '/' &&
		    ar->ar_name[2] == ' ') {
			dict = member_data;
			dictlen = o;
			if (dict + dictlen > base + len)
				dict = NULL;
			continue;
		}

		if (dict != NULL && ar->ar_name[0] == '/') {
			member = dict + strtoul(&ar->ar_name[1], NULL, 10);
			if (member >= dict + dictlen)
				/* warning? */
				continue;
			memberlen = (dict + dictlen) - member;
			if ((p = memchr(member, '/', memberlen)) != NULL)
				memberlen = p - member;
		} else {
			member = ar->ar_name;
			memberlen = sizeof (ar->ar_name);
			if ((p = memchr(ar->ar_name, ' ', memberlen)) != NULL)
				memberlen = p - member;
			if (member[memberlen - 1] == '/')
				--memberlen;
		}
		if ((fullname = malloc(namelen + memberlen + 2)) == NULL)
			err(1, NULL);
		strcpy(fullname, name);
		fullname[namelen] = ':';
		memcpy(fullname + namelen + 1, member, memberlen);
		fullname[namelen + memberlen + 1] = '\0';

		arlen = (base + len) - member_data;
		if (o < arlen)
			arlen = o;

#ifdef _STRICT_ALIGN
		/*
		 * Enforce machine alignment on archive member data.
		 * Sometimes this may be overkill (e.g., 8-byte or 16-byte
		 * alignment on a 4-byte format), but there is no easy
		 * way to tell.
		 */
		copy = NULL;
		if (member_data != (const char *)ALIGN(member_data)) {
			if ((copy = malloc(arlen)) == NULL)
				err(1, NULL);
			memcpy(copy, member_data, arlen);
			member_data = copy;
		}
#endif

		for (i = 0; i < sizeof (asize) / sizeof (asize[0]); ++i) {
			if ((asize[i])(fullname, member_data, arlen) != 0)
				break;
		}

#ifdef _STRICT_ALIGN
		if (copy != NULL)
			free(copy);
#endif

		free(fullname);
	}

	return (1);
}
