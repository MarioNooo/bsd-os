/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI strip.c,v 2.10 2002/02/18 11:04:57 benoitp Exp
 */

/*
 * Copyright (c) 1988, 1993
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
static char copyright[] =
"@(#) Copyright (c) 1988, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)strip.c	8.3 (Berkeley) 5/16/95";
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <a.out.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#include "strip.h"

static struct {
	int (*is) __P((const void *, size_t));
	ssize_t (*stab) __P((void *, size_t));
	ssize_t (*sym) __P((void *, size_t));
} op[] = {
#if defined(OMAGIC) && !defined (TORNADO)
	{ is_aout, aout_stab, aout_sym },
#endif
	{ is_elf32, elf32_stab, elf32_sym },
#if !defined(TORNADO)
	{ is_elf64, elf64_stab, elf64_sym },
#endif
};

void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct stat s;
	int fd, ofd;
	void *v;
	ssize_t (*sfcn) __P((void *, size_t));
	ssize_t size;
	ssize_t n;
	size_t i;
	int error, perms;
	int ch, eval = 0;
	int dflag = 0;
	char *fn;
	char *ofn = 0;

	while ((ch = getopt(argc, argv, "dgo:")) != -1)
		switch(ch) {
		case 'd':
		case 'g':	/* GNU compatibility */
			dflag = 1;
			break;
		case 'o':
			ofn = optarg;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

    	if (argc == 0 || (argc > 1 && ofn))
		usage();
	ofd = -1;

	while ((fn = *argv++) != NULL) {
		error = 0;
		ofd = -1;
		perms = 0;
		v = MAP_FAILED;

		if (error = lstat(fn, &s))
			warn("%s", fn);

		if (!error && s.st_size > SIZE_T_MAX) {
			warnx("%s: %s", fn, strerror(EFTYPE));
			error = 1;
		}

#ifdef TORNADO
		/* can't open the file RDWR: change perms if needed */

		if (!error && !ofn && !(s.st_mode & S_IWUSR)) {
			if (error = chmod(fn, (s.st_mode & 07777) | S_IWUSR))
				warn("%s", fn);
			else
				perms = 1;
		}
#endif

		if ((fd = open(fn, (ofn ? O_RDONLY : O_RDWR)|O_BINARY)) < 0) {
			warn("%s", fn);
			error = 1;
		}
		if (ofn &&
		    (ofd = open(ofn, O_WRONLY|O_CREAT|O_EXCL|O_BINARY, 
				0755)) < 0) {
			warn("%s", ofn);
			error = 1;
		}


		if (!error && (v = mmap(0, (size_t)s.st_size,
		    PROT_READ|PROT_WRITE, ofn ? MAP_PRIVATE : MAP_SHARED,
		    fd, 0)) == MAP_FAILED) {
			warn("%s", fn);
			error = 1;
		}

		if (!error) {
			sfcn = NULL;
			for (i = 0; i < sizeof (op) / sizeof (op[0]); ++i)
				if (op[i].is(v, (size_t)s.st_size)) {
					sfcn = dflag ? op[i].stab : op[i].sym;
					break;
				}
			if (sfcn == NULL) {
				warnx("%s: %s", fn, strerror(EFTYPE));
				error = 1;
			}
		}

		if (!error) {
			size = sfcn(v, (size_t)s.st_size);
			if (ofn) {
				if ((n = write(ofd, v, size)) != size) {
					error = 1;
					if (n == -1)
						warn("%s", ofn);
					else
						warnx("%s: short write", ofn);
				}
			}
		}

		if (v != MAP_FAILED && munmap(v, (size_t)s.st_size) == -1) {
			warn("%s", fn);
			error = 1;
		}
 
		if (ofd != -1 && close(ofd) == -1) {
			warn("%s", ofn);
			error = 1;
		}
		if (ofd != -1 && error)
			unlink(ofn);

		if (!error && !ofn && size != s.st_size &&
		    (error = ftruncate(fd, size)))
			warn("%s", fn);

		if (fd != -1 && close(fd) == -1) {
			warn("%s", fn);
			error = 1;
		}

#ifdef TORNADO
		/* restore perms */
		if (perms)
			chmod(fn, s.st_mode & 07777);
#endif

		if (error)
			eval = 1;
	}

	return (eval);
}

void
usage()
{
#ifdef TORNADO
	errx(1, "usage: bsdstrip [-d] [-o outfile] file ...");
#else
	errx(1, "usage: strip [-d] [-o outfile] file ...");
#endif
}
