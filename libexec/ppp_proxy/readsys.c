/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI readsys.c,v 1.3 1996/10/10 20:50:06 prb Exp
 */


#include <sys/types.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dial.h"

extern int debug;
extern dial_t dial;
/*
 * Capability types
 */
#define STR	0
#define NUM	1
#define BIT	6

#define	off(x)	(char **)(&x)

static struct captab {
	char	*oldname;
	int	type;
	char	**ptr;
	char	*name;
	u_long	bit;
} captab[] = {
	{ "e0",	STR,	off(dial.E[0]), },
	{ "e1",	STR,	off(dial.E[1]), },
	{ "e2",	STR,	off(dial.E[2]), },
	{ "e3",	STR,	off(dial.E[3]), },
	{ "e4",	STR,	off(dial.E[4]), },
	{ "e5",	STR,	off(dial.E[5]), },
	{ "e6",	STR,	off(dial.E[6]), },
	{ "e7",	STR,	off(dial.E[7]), },
	{ "e8",	STR,	off(dial.E[8]), },
	{ "e9",	STR,	off(dial.E[9]), },
	{ "f0",	STR,	off(dial.F[0]), },
	{ "f1",	STR,	off(dial.F[1]), },
	{ "f2",	STR,	off(dial.F[2]), },
	{ "f3",	STR,	off(dial.F[3]), },
	{ "f4",	STR,	off(dial.F[4]), },
	{ "f5",	STR,	off(dial.F[5]), },
	{ "f6",	STR,	off(dial.F[6]), },
	{ "f7",	STR,	off(dial.F[7]), },
	{ "f8",	STR,	off(dial.F[8]), },
	{ "f9",	STR,	off(dial.F[9]), },
	{ "s0",	STR,	off(dial.S[0]), },
	{ "s1",	STR,	off(dial.S[1]), },
	{ "s2",	STR,	off(dial.S[2]), },
	{ "s3",	STR,	off(dial.S[3]), },
	{ "s4",	STR,	off(dial.S[4]), },
	{ "s5",	STR,	off(dial.S[5]), },
	{ "s6",	STR,	off(dial.S[6]), },
	{ "s7",	STR,	off(dial.S[7]), },
	{ "s8",	STR,	off(dial.S[8]), },
	{ "s9",	STR,	off(dial.S[9]), },
	{ "t0",	NUM,	off(dial.T[0]), },
	{ "t1",	NUM,	off(dial.T[1]), },
	{ "t2",	NUM,	off(dial.T[2]), },
	{ "t3",	NUM,	off(dial.T[3]), },
	{ "t4",	NUM,	off(dial.T[4]), },
	{ "t5",	NUM,	off(dial.T[5]), },
	{ "t6",	NUM,	off(dial.T[6]), },
	{ "t7",	NUM,	off(dial.T[7]), },
	{ "t8",	NUM,	off(dial.T[8]), },
	{ "t9",	NUM,	off(dial.T[9]), },

	{ 0,	BIT,	off(debug),	"debug-all",	(u_long)-1, },

	{ 0,	0,	0 },
};

#undef	off

static int gstr();
static int gint();
static void getcap_internal();

void
getsyscap(host, file)
	register char *host;
	char *file;
{
	int stat;
	char *buf;
	char *capfiles[] = { file, 0 };

	if ((stat = cgetent(&buf, capfiles, host)) != 0) {
		switch (stat) {
		case 1:
			warnx("couldn't resolve 'tc' for %s in %s",
				host, capfiles[0]);
			break;
		case -1:
			warnx("host '%s' unknown in %s", host, capfiles[0]);
			break;
		case -2:
			warn("%s: getting host data in %s", host, capfiles[0]);
			break;
		case -3:
			warnx("'tc' reference loop for %s in %s",
				host, capfiles[0]);
			break;
		default:
			warnx("ppp: unexpected cgetent() error for %s in %s",
				host, capfiles[0]);
			break;
		}		
		exit(1);
	}

	getcap_internal(buf, captab);
}

static void
getcap_internal(buf, t)
	char *buf;
	struct captab *t;
{
	int stat;
	char *res;
	char **loc;
	char nb[64];

	for (; t->name || t->oldname; t++) {
		loc = t->ptr;
		res = 0;

		switch (t->type) {
		case STR:
			if (gstr(buf, t, &res))
				*loc = res;
			break;

		case NUM:
			if (gstr(buf, t, &res))
				*(int *)loc = strtol(res, 0, 0);
			else
				*(int *)loc = gint(buf, t);
			break;

		case BIT:
			stat =
			    t->name && cgetcap(buf, t->name, ':') != NULL ||
			    t->oldname && cgetcap(buf, t->oldname, ':') != NULL;
			if (stat) {
				*(u_long *)loc |= t->bit;
				break;
			}
			if (t->name) {
				snprintf(nb, sizeof(nb), "-%s", t->name);
				stat = cgetcap(buf, nb, ':') != NULL;
			}
			if (!stat && t->oldname) {
				snprintf(nb, sizeof(nb), "-%s", t->name);
				stat = cgetcap(buf, nb, ':') != NULL;
			}
			if (stat) {
				*(u_long *)loc &= ~t->bit;
				break;
			}
			break;
		}
	}
}

static int
gstr(buf, t, res)
	char *buf;
	struct captab *t;
	char **res;
{
	if (t->name && cgetstr(buf, t->name, res) >= 0 && *res && **res)
		return(1);
	if (t->oldname && cgetstr(buf, t->oldname, res) >= 0)
		return(1);
	return(0);
}

static int
gint(buf, t)
	char *buf;
	struct captab *t;
{
	long res;

	if (t->name && cgetnum(buf, t->name, &res) >= 0)
		return((int)res);
	if (t->oldname && cgetnum(buf, t->oldname, &res) >= 0)
		return((int)res);
	return(-1);
}
