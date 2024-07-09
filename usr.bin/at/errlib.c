/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI errlib.c,v 2.3 1997/11/19 01:58:34 sanders Exp
 */

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include "errlib.h"

void
Perror(char *fmt, ...)
{
	int errcode = errno;
	va_list ap;

	va_start(ap, fmt);
	(void)fprintf(stderr, "%s: ", PROGNAME);
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (errcode != 0)
		(void)fprintf(stderr, ": %s", strerror(errcode));
	(void)fprintf(stderr, "\n");
	exit(1);
}

void
Pwarn(char *fmt, ...)
{
	int errcode = errno;
	va_list ap;

	va_start(ap, fmt);
	(void)fprintf(stderr, "%s: ", PROGNAME);
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (errcode != 0)
		(void)fprintf(stderr, ": %s", strerror(errcode));
	(void)fprintf(stderr, "\n");
}

void
Perrmsg(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void
Pexit(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	(void)fprintf(stderr, "%s: ", PROGNAME);
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(1);
}

void
__Pmsg(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	(void)vfprintf(stdout, fmt, ap);
	va_end(ap);
}
