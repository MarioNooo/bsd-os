/*-
 * Copyright (c) 1983, 1993
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
char copyright[] =
"@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)uudecode.c	8.2 (Berkeley) 4/2/94";
#endif /* not lint */

/*
 * uudecode [file ...]
 *
 * create the specified file, decoding as you go.
 * used with uuencode.
 */
#include <sys/param.h>
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum { UU_DEF, UU_MIME, UU_FILE } uu_t;

int	decode __P((char *, FILE *, int));
int	input __P((char *, char *, uu_t));
int	mime_decode __P((char *, FILE *, int));
void	usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	uu_t uutype;
	int ch, rval;
	char *ofile;

	ofile = NULL;
	uutype = UU_FILE;
	while ((ch = getopt(argc, argv, "dmo:")) != -1)
		switch (ch) {
		case 'd':
			uutype = UU_DEF;
			break;
		case 'm':
			uutype = UU_MIME;
			break;
		case 'o':
			ofile = optarg;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if ((uutype != UU_FILE || ofile != NULL) && argc > 1)
		errx(1,
		    "more than one file argument with a -d, -m or -o option");

	/* For each argument, but at least once... */
	rval = 0;
	do {
		if (input(*argv, ofile, uutype))
			rval = 1;
	} while (*argv != NULL && *++argv != NULL);

	exit(rval);
}

/* 0666 */
#define	MAXPERM	(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

int
input(ifile, ofile, uutype)
	char *ifile, *ofile;
	uu_t uutype;
{
	struct passwd *pw;
	struct stat sb;
	FILE *fp;
	int fd, mime, mode, n, n1, rval;
	char *p, buf[MAXPATHLEN], path[MAXPATHLEN];

#define	HEAD_DEF	"begin "
#define	HEAD_MIME	"begin-base64 "

	/* Open the input file, if specified. */
	if (ifile == NULL)
		ifile = "stdin";
	else
		if (!freopen(ifile, "r", stdin)) {
			warn("%s", ifile);
			return (1);
		}

	/* If reading from a formatted file... */
	if (uutype == UU_FILE) {
		/* Search for the header line. */
		do {
			if (fgets(buf, sizeof(buf), stdin) == NULL) {
				warnx("%s: no \"%s\" or \"%s\" line found",
				    ifile, HEAD_DEF, HEAD_MIME);
				return (1);
			}
		} while
		    (memcmp(buf, HEAD_DEF, sizeof(HEAD_DEF) - 1) &&
		     memcmp(buf, HEAD_MIME, sizeof(HEAD_MIME) - 1));
		mime = memcmp(buf, HEAD_DEF, sizeof(HEAD_DEF) - 1);

		/* Parse the file mode and pathname. */
		p = strchr(buf, ' ');
		if (sscanf(p, "%o %s", &mode, path) != 2) {
			warnx("illegal header: %s", buf);
			return (1);
		}

		/* Get the target pathname, handle ~user/file format. */
		if (ofile == NULL) {
			if (path[0] == '~') {
				if (!(p = strchr(path, '/'))) {
					warnx("%s: missing path in %s",
					    ifile, path);
					return (1);
				}
				*p++ = '\0';
				if ((pw = getpwnam(path + 1)) == NULL) {
					warnx("%s: %s: unknown user",
					    ifile, path);
					return (1);
				}
				n = strlen(pw->pw_dir);
				n1 = strlen(p);
				if (n + n1 + 2 > MAXPATHLEN) {
					warnx("%s: path too long", ifile);
					return (1);
				}
				memmove(path + n + 1, p, n1 + 1);
				memmove(path, pw->pw_dir, n);
				path[n] = '/';
			}
			ofile = path;
		}
	} else {
		mime = uutype == UU_MIME ? 1 : 0;
		mode = MAXPERM;
	}

	/* Open the output file, if any, and set the mode. */
	if (ofile != NULL) {
		if ((fd = open(ofile, O_WRONLY | O_CREAT | O_TRUNC, 0)) < 0 ||
		    fstat(fd, &sb) ||
		    S_ISREG(sb.st_mode) && fchmod(fd, mode & MAXPERM) ||
		    (fp = fdopen(fd, "w")) == NULL) {
			warn("%s", ofile);
			if (fd >= 0)
				(void)close(fd);
			return (1);
		}
	} else
		fp = stdout;

	/* Decode the file. */
	rval = mime ?
	    mime_decode(ifile, fp, uutype == UU_FILE) :
	    decode(ifile, fp, uutype == UU_FILE);

	/* Check for errors. */
	if (ferror(fp)) {
		warn("%s", ofile);
		rval = 1;
	}
	if (fclose(fp)) {
		warn("%s", ofile);
		rval = 1;
	}
	return (rval);
}

/*
 * decode --
 *	Decode the historic uuencode format.
 */
int
decode(ifile, fp, format)
	char *ifile;
	FILE *fp;
	int format;
{
	int n;
	char ch, *p, buf[1024];

	/* For each input line. */
	for (;;) {
		if (fgets(p = buf, sizeof(buf), stdin) == NULL) {
			if (format) {
				warnx("%s: short file", ifile);
				return (1);
			}
			return (0);
		}
#define	DEC(c)	(((c) - ' ') & 077)		/* single character decode */
		/*
		 * `n' is used to avoid writing out all the characters
		 * at the end of the file.
		 */
		if ((n = DEC(*p)) <= 0)
			break;
		for (++p; n > 0; p += 4, n -= 3)
			if (n >= 3) {
				ch = DEC(p[0]) << 2 | DEC(p[1]) >> 4;
				if (putc(ch, fp) == EOF)
					goto ioerr;
				ch = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
				if (putc(ch, fp) == EOF)
					goto ioerr;
				ch = DEC(p[2]) << 6 | DEC(p[3]);
				if (putc(ch, fp) == EOF)
					goto ioerr;
			}
			else {
				if (n >= 1) {
					ch = DEC(p[0]) << 2 | DEC(p[1]) >> 4;
					if (putc(ch, fp) == EOF)
						goto ioerr;
				}
				if (n >= 2) {
					ch = DEC(p[1]) << 4 | DEC(p[2]) >> 2;
					if (putc(ch, fp) == EOF)
						goto ioerr;
				}
				if (n >= 3) {
					ch = DEC(p[2]) << 6 | DEC(p[3]);
					if (putc(ch, fp) == EOF)
						goto ioerr;
				}
			}
	}
	if (format &&
	    (fgets(buf, sizeof(buf), stdin) == NULL || strcmp(buf, "end\n"))) {
		warnx("%s: missing \"end\" line", ifile);
		return (1);
	}
	return (0);

ioerr:	warn("I/O output error");
	return (1);
}

#define	BAD	64
static u_int8_t mtab[] = {
	BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD,	/*   0 -   7 */
	BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD,	/*   8 -  15 */
	BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD,	/*  16 -  23 */
	BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD,	/*  24 -  31 */
	BAD, BAD, BAD, BAD, BAD, BAD, BAD, BAD,	/*  32 -  39 */
	BAD, BAD, BAD,  62, BAD, BAD, BAD,  63,	/*  40 -  47 */
	 52,  53,  54,  55,  56,  57,  58,  59,	/*  48 -  55 */
	 60,  61, BAD, BAD, BAD, BAD, BAD, BAD,	/*  56 -  63 */
	BAD,   0,   1,   2,   3,   4,   5,   6,	/*  64 -  71 */
	  7,   8,   9,  10,  11,  12,  13,  14,	/*  72 -  79 */
	 15,  16,  17,  18,  19,  20,  21,  22,	/*  80 -  87 */
	 23,  24,  25, BAD, BAD, BAD, BAD, BAD,	/*  88 -  95 */
	BAD,  26,  27,  28,  29,  30,  31,  32,	/*  96 - 103 */
	 33,  34,  35,  36,  37,  38,  39,  40,	/* 104 - 111 */
	 41,  42,  43,  44,  45,  46,  47,  48,	/* 112 - 119 */
	 49,  50,  51, BAD, BAD, BAD, BAD, BAD,	/* 120 - 127 */
};

/*
 * mime_decode --
 *	Decode the MIME Base64 uuencode format.
 *
 * See Internet MIME RFC 1341.
 */
int
mime_decode(ifile, fp, format)
	char *ifile;
	FILE *fp;
	int format;
{
	static int needarray[] = { 0, 0, 4, 0, 6, 0, 8, 0, 2 };
	int cval, need, val;

	for (need = 8, val = 0;;) {
		if ((cval = getchar()) == EOF) {
			if (format) {
				warnx("%s: short file", ifile);
				return (1);
			}
			return (0);
		}
		while (cval == '\n')
			if ((cval = getchar()) == '=' &&
			    (cval = getchar()) == '=' &&
			    (cval = getchar()) == '=' &&
			    (cval = getchar()) == '=' &&
			    (cval = getchar()) == '\n')
				return (0);
		if (cval == EOF || (cval = mtab[cval]) == BAD)
			continue;
		switch (need) {
		case 8:
			val = cval << 2;
			break;
		case 6:
			val |= cval;
			cval = 0;
			break;
		case 4:
			val |= (cval & 0x3c) >> 2;
			cval = (cval & 0x03) << 6;
			break;
		case 2:
			val |= (cval & 0x30) >> 4;
			cval = (cval & 0x0f) << 4;
			break;
		default:
			abort();
		}

		if (need != 8) {
			if (putc(val, fp) == EOF) {
				warn("I/O output error");
				return (1);
			}
			val = cval;
		}
		need = needarray[need];
	}
	return (0);
}

void
usage()
{
	(void)fprintf(stderr, "usage: uudecode [-dm] [-o output] [file ...]\n");
	exit(1);
}
