/*-
 * Copyright (c) 1999 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI rlimit.c,v 1.1 1999/09/08 19:16:59 prb Exp
 */

#include <sys/types.h>
#include <time.h>
#include <sys/resource.h>

#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

void setlimits(uid_t pid, char *limit, char *value);
void getlimits(uid_t, char *);
char *printlimit(quad_t n, char *buf, char *);
static u_quad_t strtolimit(char *str, char **endptr, int radix);
static quad_t strtotime(char *res, char **eptr);
static u_quad_t multiply(u_quad_t n1, u_quad_t n2);

int hard, verbose, printunits;

main(int ac, char **av)
{
	int c;
	uid_t pid;
	char *e;

	pid = getppid();	/* affect our parent by default */

	while ((c = getopt(ac, av, "hp:uv")) != EOF) {
		switch (c) {
		case 'h':
			++hard;
			break;

		case 'p':
			errno = 0;
			pid = strtol(optarg, &e, 10);
			if (errno)
				err(1, "%s", optarg);
			if (e == optarg || *e)
				err(1, "%s: invalid number", optarg);
			break;

		case 'v':
			++verbose;
			break;

		case 'u':
			++printunits;
			break;

		default: usage:
			fprintf(stderr, "usage: plimit [-huv] [-p pid] [resource [maximum-use]]\n");
			exit (1);
		}

	}
	if (optind >= ac)
		getlimits(pid, NULL);
	else if (optind + 1 == ac)
		getlimits(pid, av[optind]);
	else if (optind + 2 == ac)
		setlimits(pid, av[optind], av[optind+1]);
	else
		goto usage;
	exit(0);
}

struct {
	int limit;
	char *handle;
	char *units;
	char *name;
} limits[] = {
	{ RLIMIT_CORE,		"coredumpsize",	" bytes",	"core file size", }  ,
	{ RLIMIT_CPU,		"cputime",	" seconds",	"cpu time in seconds", },
	{ RLIMIT_DATA,		"datasize",	" bytes",	"data size", },
	{ RLIMIT_FSIZE,		"filesize",	" bytes",	"maximum file size", },
	{ RLIMIT_NPROC,		"maxproc",	"",		"number of processes", } ,
	{ RLIMIT_MEMLOCK,	"memorylocked",	" bytes",	"locked-in-memory address space", },
	{ RLIMIT_RSS,		"memoryuse",	" bytes",	"resident set size", },
	{ RLIMIT_NOFILE,	"openfiles",	"",		"number of open files", },
	{ RLIMIT_STACK,		"stacksize",	" bytes",	"stack size", },
	{ -1,			NULL, },
};

void
setlimits(uid_t pid, char *limit, char *value)
{
	struct rlimit rl;
	int r;
	u_quad_t q;
	char *e;

	for (r = 0; limits[r].limit >= 0; ++r)
		if (strcasecmp(limits[r].handle, limit) == 0)
			break;
	if (limits[r].limit < 0) {
		warn("%s: no such limit", limit);
		return;
	}

	errno = 0;

	if (r != RLIMIT_CPU)
		q = strtolimit(value, &e, 0);
	else
		q = strtotime(value, &e);

	if (errno || e == value || *e) {
		warn("%s: invalid value", value);
		return;
	}
	if (getprlimit(P_PID, pid, limits[r].limit, &rl) < 0) {
		warn("getprlimit(%s)", limits[r].handle);
		return;
	}
	if (hard)
		rl.rlim_max = q;
	else
		rl.rlim_cur = q;
	if (rl.rlim_cur > rl.rlim_max)
		rl.rlim_max = rl.rlim_cur;

	if (setprlimit(P_PID, pid, limits[r].limit, &rl) < 0)
		warn("setprlimit(%s)", limits[r].handle);
}


void
getlimits(uid_t pid, char *lim)
{
	struct rlimit rl;
	int r;
	char b1[64], b2[64];

	for (r = 0; limits[r].limit >= 0; ++r) {
		if (lim && strcasecmp(lim, limits[r].handle) != 0)
			continue;
		if (getprlimit(P_PID, pid, limits[r].limit, &rl) < 0)
			err(1, "getprlimit(%s)", limits[r].name);
		if (verbose)
			printf("%-32s %-10s %s\n", limits[r].name,
				printlimit(rl.rlim_cur, b1, ""),
				printlimit(rl.rlim_max, b2, limits[r].units));
		else if (hard)
			printf("%-15s %s\n", limits[r].handle,
				printlimit(rl.rlim_max, b2, limits[r].units));
		else
			printf("%-15s %s\n", limits[r].handle,
				printlimit(rl.rlim_cur, b1, limits[r].units));
	}
}

char *
printlimit(quad_t n, char *buf, char *units)
{
	if (n == RLIM_INFINITY)
		return ("infinity");
	if (!printunits && n > (quad_t)10 * 1024 * 1024 * 1024)
		sprintf(buf, "%qdG%s", n >> 30, units);
	else if (!printunits && n > 10 * 1024 * 1024)
		sprintf(buf, "%qdM%s", n >> 20, units);
	else if (!printunits && n > 10 * 1024)
		sprintf(buf, "%qdK%s", n >> 10, units);
	else
		sprintf(buf, "%qd%s", n, units);
	return (buf);
}


/*
 * The following is duplicated from lib/libc/gen/login_cap.c.
 * When login_cap was written it was decided that strtosize was
 * just too unique for anyone else to use, so it was made a static.
 * now we can't get to it.
 */

/*
 * Convert an expression of the following forms
 * 	1) A number.
 *	2) A number followed by a b (mult by 512).
 *	3) A number followed by a k (mult by 1024).
 *	5) A number followed by a m (mult by 1024 * 1024).
 *	6) A number followed by a g (mult by 1024 * 1024 * 1024).
 *	7) A number followed by a t (mult by 1024 * 1024 * 1024 * 1024).
 *	8) Two or more numbers (with/without k,b,m,g, or t).
 *	   seperated by x (also * for backwards compatibility), specifying
 *	   the product of the indicated values.
 */
static
u_quad_t
strtosize(str, endptr, radix)
	char *str;
	char **endptr;
	int radix;
{
	u_quad_t num, num2, t;
	char *expr, *expr2;

	errno = 0;
	num = strtouq(str, &expr, radix);
	if (errno || expr == str) {
		if (endptr)
			*endptr = expr;
		return (num);
	}

	switch(*expr) {
	case 'b': case 'B':
		num = multiply(num, (u_quad_t)512);
		++expr;
		break;
	case 'k': case 'K':
		num = multiply(num, (u_quad_t)1024);
		++expr;
		break;
	case 'm': case 'M':
		num = multiply(num, (u_quad_t)1024 * 1024);
		++expr;
		break;
	case 'g': case 'G':
		num = multiply(num, (u_quad_t)1024 * 1024 * 1024);
		++expr;
		break;
	case 't': case 'T':
		num = multiply(num, (u_quad_t)1024 * 1024);
		num = multiply(num, (u_quad_t)1024 * 1024);
		++expr;
		break;
	}

	if (errno)
		goto erange;

	switch(*expr) {
	case '*':			/* Backward compatible. */
	case 'x':
		t = num;
		num2 = strtosize(expr+1, &expr2, radix);
		if (errno) {
			expr = expr2;
			goto erange;
		}

		if (expr2 == expr + 1) {
			if (endptr)
				*endptr = expr;
			return (num);
		}
		expr = expr2;
		num = multiply(num, num2);
		if (errno)
			goto erange;
		break;
	}
	if (endptr)
		*endptr = expr;
	return (num);
erange:
	if (endptr)
		*endptr = expr;
	errno = ERANGE;
	return (UQUAD_MAX);
}

static
u_quad_t
strtolimit(str, endptr, radix)
	char *str;
	char **endptr;
	int radix;
{
	if (strcasecmp(str, "infinity") == 0 || strcasecmp(str, "inf") == 0 ||
	    strcasecmp(str, "unlimited") == 0) {
		if (endptr)
			*endptr = str + strlen(str);
		return ((u_quad_t)RLIM_INFINITY);
	}
	return (strtosize(str, endptr, radix));
}

static u_quad_t
multiply(n1, n2)
	u_quad_t n1;
	u_quad_t n2;
{
	static int bpw = 0;
	u_quad_t m;
	u_quad_t r;
	int b1, b2;

	/*
	 * Get rid of the simple cases
	 */
	if (n1 == 0 || n2 == 0)
		return (0);
	if (n1 == 1)
		return (n2);
	if (n2 == 1)
		return (n1);

	/*
	 * sizeof() returns number of bytes needed for storage.
	 * This may be different from the actual number of useful bits.
	 */
	if (!bpw) {
		bpw = sizeof(u_quad_t) * 8;
		while (((u_quad_t)1 << (bpw-1)) == 0)
			--bpw;
	}

	/*
	 * First check the magnitude of each number.  If the sum of the
	 * magnatude is way to high, reject the number.  (If this test
	 * is not done then the first multiply below may overflow.)
	 */
	for (b1 = bpw; (((u_quad_t)1 << (b1-1)) & n1) == 0; --b1)
		; 
	for (b2 = bpw; (((u_quad_t)1 << (b2-1)) & n2) == 0; --b2)
		; 
	if (b1 + b2 - 2 > bpw) {
		errno = ERANGE;
		return (UQUAD_MAX);
	}

	/*
	 * Decompose the multiplication to be:
	 * h1 = n1 & ~1
	 * h2 = n2 & ~1
	 * l1 = n1 & 1
	 * l2 = n2 & 1
	 * (h1 + l1) * (h2 + l2)
	 * (h1 * h2) + (h1 * l2) + (l1 * h2) + (l1 * l2)
	 *
	 * Since h1 && h2 do not have the low bit set, we can then say:
	 *
	 * (h1>>1 * h2>>1 * 4) + ...
	 *
	 * So if (h1>>1 * h2>>1) > (1<<(bpw - 2)) then the result will
	 * overflow.
	 *
	 * Finally, if MAX - ((h1 * l2) + (l1 * h2) + (l1 * l2)) < (h1*h2)
	 * then adding in residual amout will cause an overflow.
	 */

	m = (n1 >> 1) * (n2 >> 1);

	if (m >= ((u_quad_t)1 << (bpw-2))) {
		errno = ERANGE;
		return (UQUAD_MAX);
	}

	m *= 4;

	r = (n1 & n2 & 1)
	  + (n2 & 1) * (n1 & ~(u_quad_t)1)
	  + (n1 & 1) * (n2 & ~(u_quad_t)1);

	if ((u_quad_t)(m + r) < m) {
		errno = ERANGE;
		return (UQUAD_MAX);
	}
	m += r;

	return (m);
}

quad_t
strtotime(char *res, char **eptr)
{
	char *ep;
	char *sres;
	int stat;
	quad_t q, r;

	errno = 0;

	if (strcasecmp(res, "infinity") == 0)
		return (RLIM_INFINITY);

	errno = 0;

	q = 0;
	sres = res;
	while (*res) {
		r = strtoq(res, &ep, 0);
		if (!ep || ep == res ||
		    ((r == QUAD_MIN || r == QUAD_MAX) && errno == ERANGE)) {
invalid:
			if (eptr)
				*eptr = res;
			errno = ERANGE;
			return (-1);
		}
		switch (*ep++) {
		case '\0':
			--ep;
			break;
		case 's': case 'S':
			break;
		case 'm': case 'M':
			r *= 60;
			break;
		case 'h': case 'H':
			r *= 60 * 60;
			break;
		case 'd': case 'D':
			r *= 60 * 60 * 24;
			break;
		case 'w': case 'W':
			r *= 60 * 60 * 24 * 7;
			break;
		case 'y': case 'Y':	/* Pretty absurd */
			r *= 60 * 60 * 24 * 365;
			break;
		default:
			res = ep;
			goto invalid;
		}
		res = ep;
		q += r;
	}
	if (eptr)
		*eptr = res;
	return (q);
}
