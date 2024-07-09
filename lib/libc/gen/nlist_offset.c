/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI nlist_offset.c,v 2.4 2001/10/03 17:29:52 polk Exp
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <a.out.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include "nlist_var.h"

static off_t (* const offset[])(unsigned long, const void *, size_t) = {
#ifdef OMAGIC
	_aout_nlist_offset,
#else
	NULL,
#endif
	_elf32_nlist_offset,
#ifdef NLIST64
	_elf64_nlist_offset,
#else
	NULL,
#endif
};

/*
 * Given an open file descriptor pointing to an executable, return the
 * file offset associated with the given run-time address.
 *
 * (This used to do some nonsense with the current file offset, but that
 * made its callers have to lseek() back to 0, so we omit it.)
 */
off_t
_nlist_offset(int f, unsigned long address)
{
	off_t ret;
	int saverr;
	void *v;
	size_t size;
	struct stat st;

	if (fstat(f, &st) < 0)
		return (-1);
	if (st.st_size > SIZE_T_MAX) {
		errno = EFBIG;
		return (-1);
	}
	size = st.st_size;
	if ((v = mmap(NULL, size, PROT_READ, MAP_SHARED, f, 0)) == MAP_FAILED)
		return (-1);
	ret = _nlist_offset_mem(address, v, size);
	saverr = errno;
	(void)munmap(v, size);
	errno = saverr;
	return (ret);
}

/*
 * Given a run-time address and a memory image of an executable,
 * return the file offset associated with the given run-time address.
 * (Add this to the offset from which the image was read to get the
 * "final" file offset.)
 */
off_t
_nlist_offset_mem(unsigned long address, const void *base, size_t size)
{
	enum _nlist_filetype type;

	/* We classify the file according to its executable format...  */
	type = _nlist_classify(base, size);
	if ((unsigned)type >= sizeof (offset) / sizeof (offset[0]) ||
	    offset[type] == NULL) {
		errno = EFTYPE;
		return (-1);
	}

	/* ... then run the appropriate offset function on it.  */
	return (offset[type](address, base, size));
}
