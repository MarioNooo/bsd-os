/*
 * Copyright (c) 1992, 1995
 * Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/*
 * Utilities.
 */

#include <sys/param.h>
#include <sys/disklabel.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "diskdefect.h"

static int doio __P((struct ddstate *, void *, off_t, int,
			ssize_t (*)(int, void *, size_t)));

__dead void
panic(s)
	const char *s;
{

	errx(1, "panic: %s", s);
	/* NOTREACHED */
}

/*
 * Ask for a yes or no, being insistent.
 */
int
yesno()
{
	char line[_POSIX2_LINE_MAX];
	static int eofcnt;
	size_t l;

	if (yflag) {
		printf("yes\n");
		return ('y');
	}
	if (fgets(line, sizeof line, stdin) == NULL) {
		if (++eofcnt > 3) {
			printf("no\n");
			return ('n');
		}
		clearerr(stdin);
		return (-1);
	}
	eofcnt = 0;
	l = strlen(line);
	if (l > 0 && line[--l] == '\n')
		line[l] = '\0';
	do {
		line[l] = tolower(line[l]);
	} while (l-- != 0);
	if (strcmp(line, "yes") == 0 || strcmp(line, "y") == 0)
		return ('y');
	if (strcmp(line, "no") == 0 || strcmp(line, "n") == 0)
		return ('n');
	printf("Please answer yes or no\n");
	return (-1);
}

int
rsec(ds, buf, sn)
	struct ddstate *ds;
	void *buf;
	daddr_t sn;
{
	int ss = ds->d_dl.d_secsize;

	return (ratoff(ds, buf, (off_t)sn * ss, ss));
}

int
wsec(ds, buf, sn)
	struct ddstate *ds;
	void *buf;
	daddr_t sn;
{
	int ss = ds->d_dl.d_secsize;

	return (watoff(ds, buf, (off_t)sn * ss, ss));
}

int
ratoff(ds, buf, off, siz)
	struct ddstate *ds;
	void *buf;
	off_t off;
	int siz;
{

	return (doio(ds, buf, off, siz, read));
}

int
watoff(ds, buf, off, siz)
	struct ddstate *ds;
	void *buf;
	off_t off;
	int siz;
{
	static int skip_all_writes = 0;

	if (skip_all_writes)
		return (1);
	return (doio(ds, buf, off, siz,
	    (ssize_t (*)(int, void *, size_t))write));
}

/*
 * Do a read or write.
 */
static int
doio(ds, buf, off, size, fn)
	struct ddstate *ds;
	void *buf;
	off_t off;
	int size;
	ssize_t (*fn) __P((int, void *, size_t));
{
	int ns;
	extern int fake_errors;
	extern double error_rate;
	static off_t err = -1;
#define	frand() (rand() / (RAND_MAX + 1.0))	/* a number in [0.0,1.0) */

	if (fake_errors) {
		/*
		 * Make errors repeat once:  If this I/O covers
		 * the previous error spot, this one is also in error.
		 * This is only approximate (we may `move' an error
		 * below), but works well in practise.
		 */
		if (off <= err && off + size > err) {
			err = -1;
			errno = EIO;
			return (-1);
		}
		/*
		 * Our chance of error is determined by error_rate
		 * (which should be in [0.0,1.0]).
		 *
		 * We want the chance to be higher if we are doing a
		 * larger transfer, so convert the error rate to a
		 * success rate (SR = 1 - ER), and raise that to a
		 * power based on the transfer size.  The result is
		 * the success rate for that size xfer; subtract from
		 * one to get a new error rate.
		 */
		ns = (size + 511) >> 9;	/* 1 chance per 512-byte "sector" */
		if (frand() < 1.0 - pow(1.0 - error_rate, (double)ns)) {
			/* Pick an error spot within this xfer. */
			err = off + (int)(size * frand());
			errno = EIO;
			return (-1);
		}
	}

	if (no_io)		/* simulate successful I/O */
		return (0);

	if (lseek(ds->d_fd, off, L_SET) == (off_t)-1 ||
	    (*fn)(ds->d_fd, buf, size) != size)
		return (-1);
	return (0);
}

int
copysec(ds, ssn, dsn)
	struct ddstate *ds;
	daddr_t ssn, dsn;
{
	int try, lim, flag;

	flag = COPY_RDFAILED;
	lim = copytries ? copytries : 1;
	for (try = 0; try < lim; try++) {
		/* really only need to bzero on the last try */
		bzero(ds->d_secbuf, ds->d_dl.d_secsize);
		if (rsec(ds, ds->d_secbuf, ssn) == 0) {
			flag = 0;
			break;
		}
	}
	for (try = 0; try < lim; try++)
		if (wsec(ds, ds->d_secbuf, dsn) == 0)
			return (flag);
	return (flag | COPY_WRFAILED);
}

int
zerosec(ds, sn)
	struct ddstate *ds;
	daddr_t sn;
{

	bzero(ds->d_secbuf, ds->d_dl.d_secsize);
	return (wsec(ds, ds->d_secbuf, sn));
}

void *
emalloc(size)
	size_t size;
{
	void *p;

	if ((p = malloc(size)) == NULL)
		err(1, "malloc");
	return (p);
}

void *
erealloc(p, newsize)
	void *p;
	size_t newsize;
{

	if ((p = realloc(p, newsize)) == NULL)
		err(1, "realloc");
	return (p);
}
