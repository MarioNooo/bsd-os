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
static char sccsid[] = "@(#)uuencode.c	8.2 (Berkeley) 4/2/94";
#endif /* not lint */

/*
 * uuencode [input] output
 *
 * Encode a file so it can be mailed to a remote system.
 */
#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>

void	encode __P((char *, char *, u_long));
void	mime_encode __P((char *, char *, u_long));
void	usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct stat sb;
	mode_t mode;
	int ch, mime;
	char *ifile;

	mime = 0;
	while ((ch = getopt(argc, argv, "m")) != -1)
		switch (ch) {
		case 'm':
			mime = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	ifile = "stdin";
	switch (argc) {
	case 2:			/* optional first argument is input file */
		ifile = *argv;
		if (!freopen(ifile, "r", stdin) || fstat(fileno(stdin), &sb))
			err(1, "%s", ifile);
#define	RWX	(S_IRWXU | S_IRWXG | S_IRWXO)
		mode = sb.st_mode & RWX;
		++argv;
		break;
	case 1:
#define	RW	(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
		mode = RW & ~umask(RW);
		break;
	default:
		usage();
	}

	if (mime)
		mime_encode(ifile, *argv, (u_long)mode);
	else
		encode(ifile, *argv, (u_long)mode);
	exit (0);
}

/* ENC is the basic 1 character encoding function to make a char printing */
#define	ENC(c) ((c) ? ((c) & 077) + ' ' : '`')

/*
 * Historic format: copy from in to out, encoding as you go along.
 */
void
encode(ifile, name, mode)
	char *ifile, *name;
	u_long mode;
{
	int ch, n;
	char *p, buf[80];

	(void)printf("begin %lo %s\n", mode, name);
	while ((n = fread(buf, 1, 45, stdin)) > 0) {
		ch = ENC(n);
		if (putchar(ch) == EOF)
			goto ioerr;
		for (p = buf; n > 0; n -= 3, p += 3) {
			ch = *p >> 2;
			ch = ENC(ch);
			if (putchar(ch) == EOF)
				goto ioerr;
			ch = (*p << 4) & 060 | (p[1] >> 4) & 017;
			ch = ENC(ch);
			if (putchar(ch) == EOF)
				goto ioerr;
			ch = (p[1] << 2) & 074 | (p[2] >> 6) & 03;
			ch = ENC(ch);
			if (putchar(ch) == EOF)
				goto ioerr;
			ch = p[2] & 077;
			ch = ENC(ch);
			if (putchar(ch) == EOF)
				goto ioerr;
		}
		if (putchar('\n') == EOF)
			goto ioerr;
	}
	if (ferror(stdin))
		err(1, "%s", ifile);

	ch = ENC('\0');
	(void)putchar(ch);
	(void)printf("\nend\n");
	if (ferror(stdout))
ioerr:		err(1, "stdout");
}

static u_int8_t mtab[64] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b',
	'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
	'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
};

/*
 * Mime Base64 format: copy from in to out, encoding as you go along.
 */
void
mime_encode(ifile, name, mode)
	char *ifile, *name;
	u_long mode;
{
	u_int32_t bits;
	int ch, n, pad;

	(void)printf("begin-base64 %lo %s\n", mode, name);

	for (n = pad = 0;;) {
		if ((ch = getchar()) == EOF)
			break;
		bits = ch << 16;
		if ((ch = getchar()) == EOF)
			++pad;
		else
			bits |= ch << 8;
		if ((ch = getchar()) == EOF)
			++pad;
		else
			bits |= ch;

#define	MAXLINELEN	76
#define	OUTC {								\
	if (putchar(ch) == EOF)						\
		goto ioerr;						\
	if (++n == MAXLINELEN - 1) {					\
		n = 0;							\
		if (putchar('\n') != '\n')				\
			break;						\
	}								\
}
		ch = mtab[(bits & 0x00fc0000) >> 18];
		OUTC;
		ch = mtab[(bits & 0x0003f000) >> 12];
		OUTC;
		ch = pad > 1 ? '=' : mtab[(bits & 0x00000fc0) >> 6];
		OUTC;
		ch = pad > 0 ? '=' : mtab[bits & 0x0000003f];
		OUTC;
	}

	if (ferror(stdin))
		err(1, "%s", ifile);
	if (n != 0)
		(void)putchar('\n');
	(void)printf("====\n");
	if (ferror(stdout))
ioerr:		err(1, "stdout");
}

void
usage()
{
	(void)fprintf(stderr,"usage: uuencode [-m] [infile] remotefile\n");
	exit(1);
}
