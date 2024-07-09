/*	BSDI nlist.c,v 2.4 1998/09/08 05:37:08 torek Exp	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char sccsid[] = "@(#)nlist.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <a.out.h>
#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kvm.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"

typedef int (*nlist_callback_fn)(const struct nlist *, const char *, void *);
int _nlist_apply(nlist_callback_fn, void *, const void *, size_t);
off_t _nlist_offset_mem(unsigned long, void *, size_t);

static int callback(const struct nlist *, const char *, void *);
static void store_version(const char *, off_t, DB *, const void *, size_t);

static unsigned long version_address = -1;
static const char vrs_sym[] = VRS_SYM;
static const char vrs_key[] = VRS_KEY;

void
create_knlist(char *name, DB *db)
{
	int f;
	off_t version_offset;
	void *v;
	size_t size;
	struct stat st;

	if ((f = open(name, O_RDONLY)) == -1)
		err(1, "%s", name);
	if (fstat(f, &st))
		err(1, "stat(%s)", name);
	if (st.st_size > SIZE_T_MAX)
		errx(1, "%s: %s", name, strerror(EFBIG));
	size = st.st_size;
	if ((v = mmap(NULL, size, PROT_READ, MAP_SHARED, f, 0)) == MAP_FAILED)
		err(1, "%s: mmap(%lu)", name, (u_long)size);

	if (_nlist_apply(callback, db, v, size) == -1)
		err(1, "%s", name);

	if (version_address == (unsigned long)-1)
		errx(1, "%s: no version string", name);
	version_offset = _nlist_offset_mem(version_address, v, size);
	if (version_offset == -1)
		err(1, "%s: can't recover version string", name);
	store_version(name, version_offset, db, v, size);
	(void)munmap(v, size);

	close(f);
}

static int
callback(const struct nlist *s, const char *name, void *arg)
{
	DB *db = arg;
	DBT data, key;

	if (name == NULL || name[0] == '\0' ||
	    (s->n_type & N_EXT) != N_EXT || (s->n_type & N_STAB) != 0)
		return (0);

	key.data = (char *)name;
	key.size = strlen(name);
	data.data = (char *)s;
	data.size = sizeof (*s);
	if (db->put(db, &key, &data, 0))
		err(1, "error entering symbol table record");

	if (strcmp(name, vrs_sym) == 0 ||
	    (vrs_sym[0] == '_' && strcmp(name, &vrs_sym[1]) == 0))
		version_address = s->n_value;
	return (0);
}

static void
store_version(const char *name, off_t o, DB *db, const void *v, size_t size)
{
	DBT data, key;
	char *vers, *cp;

	if (o > size)
		errx(1, "%s: version string offset (%lu) > file size (%lu)",
		    name, (u_long)o, (u_long)size);

	vers = (char *)v + o;
	if ((cp = memchr(vers, '\n', size - (size_t)o)) == NULL)
		errx(1, "%s: can't find end of version string", name);

	key.data = (char *)vrs_key;
	key.size = sizeof (vrs_key) - 1;
	data.data = vers;
	data.size = (cp - vers) + 1;

	if (db->put(db, &key, &data, 0))
		err(1, "error entering version string record");
}
