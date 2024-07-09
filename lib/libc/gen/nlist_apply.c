/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1995,1997,1998 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI nlist_apply.c,v 2.4 2001/10/03 17:29:52 polk Exp
 */

/*
 * Copyright (c) 1989, 1993
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

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <a.out.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "nlist_var.h"

static int (* const scan[])(nlist_callback_fn, void *, const void *, size_t) = {
#ifdef OMAGIC
	_aout_nlist_scan,
#else
	NULL,
#endif
	_elf32_nlist_scan,
#ifdef NLIST64
	_elf64_nlist_scan,
#else
	NULL,
#endif
};

/*
 * We look for an executable binary file in the given memory image.
 * We call the given function on every symbol+name in the given file.
 * We stop if the function returns nonzero; a positive nonzero value
 * means we've finished early, while -1 indicates an error.
 * XXX Note that _nlist_apply() must be listed in libc.except in order
 * XXX for nm and kvm_mkdb to use it.
 */
int
_nlist_apply(nlist_callback_fn callback, void *arg,
    const void *base, size_t len)
{
	enum _nlist_filetype type;
	int error = 0;

	/* We classify the file according to its executable format...  */
	type = _nlist_classify(base, len);
	if ((unsigned)type >= sizeof (scan) / sizeof (scan[0]) ||
	    scan[type] == NULL)
		error = EFTYPE;

	/* ... then run the appropriate scan function on it.  */
	if (error == 0)
		error = scan[type](callback, arg, base, len);

	if (error) {
		errno = error;
		return (-1);
	}

	return (0);
}
