/*	BSDI bpatch.c,v 2.6 1997/10/25 15:52:02 donn Exp	*/

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
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
char copyright[] =
"@(#) Copyright (c) 1991 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "from @(#)dmesg.c	5.9 (Berkeley) 5/2/91";
#endif /* not lint */

/*
 * bpatch: binary patch
 * Patches a.out files, including kernel, or /dev/kmem for running kernel.
 * Usage:
 *	bpatch [ type-opt ] [ -r ] location [ value ]
 *
 *	type-options (chose at most one, 'l' is default):
 *	    -b	byte as number
 *	    -c	byte as character
 *	    -w	short/u_short
 *	    -l	long/u_long
 *	    -q	quad_t/u_quad_t
 *	    -s	string
 *
 *	Other options:
 *	    -r patch running kernel rather than a.out
 *	    -N namelist	name for kernel namelist file
 *	    -M memfile	name for memory file
 *
 *	location
 *	    a symbol name, or a decimal/hex/octal number
 *
 *	value
 *	    depending on the type, a character, string,
 *	    or a decimal/hex/octal number.
 *	    If none, the current value is printed.
 */

#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/errno.h>

#include <a.out.h>
#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <kvm.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int __fdnlist(int, struct nlist *);
off_t _nlist_offset(int, unsigned long);

struct nlist nl[2];

void usage();

enum { UNSPEC, BYTE, CHAR, WORD, LONG, QUAD, STRING } type = UNSPEC;

#define	MAXSTRING	1024

u_int8_t	cbuf;
u_int16_t	wbuf;
u_int32_t	lbuf;
u_int64_t	qbuf;

u_char	sbuf[MAXSTRING + 1];
u_char	vbuf[MAXSTRING + 1];
int	checkversion = 1;	/* check version strings, namelist vs kmem */

int
main(int argc, char **argv)
{
	int ch;
	char *namelist;
	char *core;
	char *file;
	char *locname;
	char *valname;
	int writing;
	int fd;
	int nfd;
	int missing;
	u_long loc;
	void *p;
	off_t foff;
	char *ep;
	size_t len;

	namelist = _PATH_KERNEL;
	core = NULL;
	writing = 0;
	while ((ch = getopt(argc, argv, "bcwlqsrM:N:")) != EOF)
		switch (ch) {
		case 'b':
			if (type != UNSPEC)
				usage();
			type = BYTE;
			p = &cbuf;
			len = sizeof(cbuf);
			break;
		case 'c':
			if (type != UNSPEC)
				usage();
			type = CHAR;
			p = &cbuf;
			len = sizeof(cbuf);
			break;
		case 'w':
			if (type != UNSPEC)
				usage();
			type = WORD;
			p = &wbuf;
			len = sizeof(wbuf);
			break;
		case 'l':
			if (type != UNSPEC)
				usage();
			type = LONG;
			p = &lbuf;
			len = sizeof(lbuf);
			break;
		case 'q':
			if (type != UNSPEC)
				usage();
			type = QUAD;
			p = &qbuf;
			len = sizeof(qbuf);
			break;
		case 's':
			if (type != UNSPEC)
				usage();
			type = STRING;
			p = sbuf;
			len = MAXSTRING;
			break;
		case 'r':
			if (core == NULL)
				core = _PATH_KMEM;
			break;
		case 'M':
			core = optarg;
			if (strcmp(core, _PATH_KMEM) != 0)
				checkversion = 0;
			break;
		case 'N':
			namelist = optarg;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;
	if (type == UNSPEC) {
		type = LONG;
		p = &lbuf;
		len = sizeof(lbuf);
	}

	if (argc < 1 || argc > 2)
		usage();
	writing = (argc == 2);
	if (core == NULL)
		checkversion = 0;
	locname = argv[0];
	valname = argv[1];	/* NULL if not writing */

	file = core ? core : namelist;
	if ((fd = open(file, writing ? O_RDWR : O_RDONLY)) == -1)
		err(1, "%s", file);
	if (core) {
		if ((nfd = open(namelist, O_RDONLY)) == -1)
			err(1, "%s", namelist);
	} else
		nfd = fd;

	nl[0].n_un.n_name = VRS_SYM;
	if (checkversion && __fdnlist(nfd, nl) == 0) {
		if ((foff = _nlist_offset(nfd, nl[0].n_value)) == -1)
			err(1, "%s", namelist);
		if (lseek(nfd, foff, SEEK_SET) == -1)
			err(1, "%s: lseek to version", namelist);
		if (read(nfd, vbuf, MAXSTRING) != MAXSTRING)
			errx(1, "%s: can't read version", namelist);
		if (lseek(fd, nl[0].n_value, SEEK_SET) == -1)
			err(1, "%s: lseek to version", core);
		if (read(fd, sbuf, MAXSTRING) != MAXSTRING)
			errx(1, "%s: can't read version", core);
		if (strcmp(vbuf, sbuf))
			errx(1, "version string mismatch in %s and %s",
			    namelist, core);
		lseek(nfd, 0, SEEK_SET);
	}

	/*
	 * See whether the given location is numeric or symbolic.
	 */
	errno = 0; 
	loc = strtoul(locname, &ep, 0);
	if (errno == ERANGE || *ep != '\0') {
		/*
		 * Value must be symbolic.  If not found,
		 * try prepending underscore.
		 */
		nl[0].n_un.n_name = locname;
		missing = __fdnlist(nfd, nl);
		if (missing) {
			nl[0].n_un.n_name = malloc(strlen(locname) + 2);
			if (nl[0].n_un.n_name == NULL)
				err(1, NULL);
			sprintf(nl[0].n_un.n_name, "_%s", locname);
			missing = __fdnlist(nfd, nl);
		}
		if (missing)
			errx(1, "%s not defined in namelist", locname);
		lseek(nfd, 0, SEEK_SET);
        	loc = nl[0].n_value;
	}

	/*
	 * If patching the binary file, need to figure out file offsets.
	 * If checking version, also need exec header.
	 */
	if (core)
		foff = loc;
	else if ((foff = _nlist_offset(fd, loc)) == -1)
		err(1, "%s", file);
 
 	if (!writing) {
 		if (lseek(fd, foff, SEEK_SET) == -1)
 			err(1, "%s", file);
        	if ((size_t)read(fd, p, len) != len)
 			errx(1, "%s: can't read at %s", file, locname);
        	switch (type) {
        	case BYTE:
        		printf("0x%x = %u\n", cbuf, cbuf);
        		break;
        	case CHAR:
        		printf("%c\n", cbuf);
        		break;
        	case WORD:
        		printf("0x%x = %u\n", wbuf, wbuf);
        		break;
        	case LONG:
        		printf("0x%lx = %lu\n", (u_long)lbuf, (u_long)lbuf);
        		break;
        	case QUAD:
        		printf("0x%qx = %qu\n", (u_quad_t)qbuf, (u_quad_t)qbuf);
        		break;
        	case STRING:
        		printf("%s\n", sbuf);
        		break;
        	}
		return (0);
	}

	errno = 0;
	switch (type) {
	case BYTE:
		lbuf = strtoul(valname, NULL, 0);
		if (lbuf > UCHAR_MAX || errno == ERANGE)
			errx(1, "%s: value out of range", valname);
		cbuf = lbuf;
		break;
	case CHAR:
		cbuf = valname[0];
		break;
	case WORD:
		lbuf = strtoul(valname, NULL, 0);
		if (lbuf > USHRT_MAX || errno == ERANGE)
			errx(1, "%s: value out of range", valname);
		wbuf = lbuf;
		break;
	case LONG:
		/* XXX Worry about overflow if sizeof (long) == 8?  */
		lbuf = strtoul(valname, NULL, 0);
		if (errno == ERANGE)
			errx(1, "%s: value out of range", valname);
		break;
	case QUAD:
		qbuf = strtouq(valname, NULL, 0);
		if (errno == ERANGE)
			errx(1, "%s: value out of range", valname);
		break;
	case STRING:
		p = valname;
		len = strlen(p) + 1;
		break;
	}
	if (lseek(fd, foff, SEEK_SET) == -1)
		err(1, "%s", file);
	if ((size_t)write(fd, p, len) != len)
		err(1, "%s: can't write at %s", file, locname);

	return (0);
}

void
usage()
{
	fprintf(stderr,
"usage: bpatch [-b|c|w|l|q|s] [-r] [-M core] [-N system] var [ val ]\n");
	exit(1);
}
