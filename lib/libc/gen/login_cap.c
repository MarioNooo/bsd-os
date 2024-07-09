/*-
 * Copyright (c) 1995,1997 Berkeley Software Design, Inc. All rights reserved.
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
 *	This product includes software developed by Berkeley Software Design,
 *	Inc.
 * 4. The name of Berkeley Software Design, Inc.  may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	BSDI login_cap.c,v 2.16 2000/03/22 17:10:55 donn Exp
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <login_cap.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>


static	char *classfiles[] = { _PATH_LOGIN_CONF, 0 };
static	char *_authtypes[] = { LOGIN_DEFSTYLE, 0 };
static	void setuserpath __P((login_cap_t *, char *));
static	u_quad_t multiply __P((u_quad_t, u_quad_t));
static	u_quad_t strtolimit __P((char *, char **, int));
static	u_quad_t strtosize __P((char *, char **, int));

login_cap_t *
login_getclass(class)
	char *class;
{
	login_cap_t *lc;
	int res;

	for (res = 0; classfiles[res]; ++res)
		if (secure_path(classfiles[res]) < 0)
			return (0);

	if ((lc = malloc(sizeof(login_cap_t))) == NULL) {
		syslog(LOG_ERR, "%s:%d malloc: %m", __FILE__, __LINE__);
		return (0);
	}

	lc->lc_cap = 0;
	lc->lc_style = 0;

	if (class == NULL || class[0] == '\0')
		class = LOGIN_DEFCLASS;

    	if ((lc->lc_class = strdup(class)) == NULL) {
		syslog(LOG_ERR, "%s:%d strdup: %m", __FILE__, __LINE__);
		free(lc);
		return (0);
	}

	if ((res = cgetent(&lc->lc_cap, classfiles, lc->lc_class)) != 0 ) {
		lc->lc_cap = 0;
		switch (res) {
		case 1: 
			syslog(LOG_ERR, "%s: couldn't resolve 'tc'",
				lc->lc_class);
			break;
		case -1:
			if ((res = open(classfiles[0], 0)) >= 0)
				close(res);
			if (strcmp(lc->lc_class, LOGIN_DEFCLASS) == NULL &&
			    res < 0)
				return (lc);
			syslog(LOG_ERR, "%s: unknown class", lc->lc_class);
			break;
		case -2:
			syslog(LOG_ERR, "%s: getting class information: %m",
				lc->lc_class);
			break;
		case -3:
			syslog(LOG_ERR, "%s: 'tc' reference loop",
				lc->lc_class);
			break;
		default:
			syslog(LOG_ERR, "%s: unexpected cgetent error",
				lc->lc_class);
			break;
		}
		free(lc->lc_class);
		free(lc);
		return (0);
	}
	return (lc);
}

char *
login_getstyle(lc, style, atype)
	login_cap_t *lc;
	char *style;
	char *atype;
{
    	char **authtypes = _authtypes;
	char *auths, *ta;
    	char *f1, **f2;
	int i;

	f1 = 0;
	f2 = 0;

    	if (!atype || !(auths = login_getcapstr(lc, atype, NULL, NULL)))
		auths = login_getcapstr(lc, "auth", "", NULL);

	if (auths) {
		if (*auths) {
			f1 = ta = auths = strdup(auths);
			if (!auths) {
				syslog(LOG_ERR, "strdup: %m");
				return (0);
			}
			i = 2;
			while (*ta)
				if (*ta++ == ',')
					++i;
			f2 = authtypes = malloc(sizeof(char *) * i);
			if (!authtypes) {
				syslog(LOG_ERR, "malloc: %m");
				free(f1);
				return (0);
			}
			i = 0;
			while (*auths) {
				authtypes[i] = auths;
				while (*auths && *auths != ',')
					++auths;
				if (*auths)
					*auths++ = 0;
				if (!*authtypes[i])
					authtypes[i] = LOGIN_DEFSTYLE;
				++i;
			}
			authtypes[i] = 0;
			
		}
	}

	if (!style)
		style = authtypes[0];
		
	while (*authtypes && strcmp(style, *authtypes))
		++authtypes;

	if (*authtypes == NULL || (auths = strdup(*authtypes)) == NULL) {
		if (f1)
			free(f1);
		if (f2)
			free(f2);
		if (*authtypes)
			syslog(LOG_ERR, "strdup: %m");
		lc->lc_style = 0;
		return (0);
	}
	if (f2)
		free(f2);
	return (lc->lc_style = auths);
}

char *
login_getcapstr(lc, cap, def, e)
	login_cap_t *lc;
	char *cap;
	char *def;
	char *e;
{
	char *res;
	int stat;

	errno = 0;

    	if (!lc->lc_cap)
		return (def);

	switch (stat = cgetstr(lc->lc_cap, cap, &res)) {
	case -1:
		return (def);
	case -2:
		syslog(LOG_ERR, "%s: getting capability %s: %m",
		    lc->lc_class, cap);
		return (e);
	default:
		if (stat >= 0) 
			return (res);
		syslog(LOG_ERR, "%s: unexpected error with capability %s",
		    lc->lc_class, cap);
		return (e);
	}
}

quad_t
login_getcaptime(lc, cap, def, e)
	login_cap_t *lc;
	char *cap;
	quad_t def;
	quad_t e;
{
	char *ep;
	char *res, *sres;
	int stat;
	quad_t q, r;

	errno = 0;
    	if (!lc->lc_cap)
		return (def);

	switch (stat = cgetstr(lc->lc_cap, cap, &res)) {
	case -1:
		return (def);
	case -2:
		syslog(LOG_ERR, "%s: getting capability %s: %m",
		    lc->lc_class, cap);
		errno = ERANGE;
		return (e);
	default:
		if (stat >= 0) 
			break;
		syslog(LOG_ERR, "%s: unexpected error with capability %s",
		    lc->lc_class, cap);
		errno = ERANGE;
		return (e);
	}

	if (strcasecmp(res, "infinity") == 0)
		return (RLIM_INFINITY);

	errno = 0;

	q = 0;
	sres = res;
	while (*res) {
		r = strtoll(res, &ep, 0);
		if (!ep || ep == res ||
		    ((r == QUAD_MIN || r == QUAD_MAX) && errno == ERANGE)) {
invalid:
			syslog(LOG_ERR, "%s:%s=%s: invalid time",
			    lc->lc_class, cap, sres);
			errno = ERANGE;
			return (e);
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
			goto invalid;
		}
		res = ep;
		q += r;
	}
	return (q);
}

quad_t
login_getcapnum(lc, cap, def, e)
	login_cap_t *lc;
	char *cap;
	quad_t def;
	quad_t e;
{
	char *ep;
	char *res;
	int stat;
	quad_t q;

	errno = 0;
    	if (!lc->lc_cap)
		return (def);

	switch (stat = cgetstr(lc->lc_cap, cap, &res)) {
	case -1:
		return (def);
	case -2:
		syslog(LOG_ERR, "%s: getting capability %s: %m",
		    lc->lc_class, cap);
		errno = ERANGE;
		return (e);
	default:
		if (stat >= 0) 
			break;
		syslog(LOG_ERR, "%s: unexpected error with capability %s",
		    lc->lc_class, cap);
		errno = ERANGE;
		return (e);
	}

	if (strcasecmp(res, "infinity") == 0)
		return (RLIM_INFINITY);

	errno = 0;
    	q = strtoll(res, &ep, 0);
	if (!ep || ep == res || ep[0] ||
	    ((q == QUAD_MIN || q == QUAD_MAX) && errno == ERANGE)) {
		syslog(LOG_ERR, "%s:%s=%s: invalid number",
		    lc->lc_class, cap, res);
		errno = ERANGE;
		return (e);
	}
	return (q);
}

quad_t
login_getcapsize(lc, cap, def, e)
	login_cap_t *lc;
	char *cap;
	quad_t def;
	quad_t e;
{
	char *ep;
	char *res;
	int stat;
	quad_t q;

	errno = 0;

    	if (!lc->lc_cap)
		return (def);

	switch (stat = cgetstr(lc->lc_cap, cap, &res)) {
	case -1:
		return (def);
	case -2:
		syslog(LOG_ERR, "%s: getting capability %s: %m",
		    lc->lc_class, cap);
		errno = ERANGE;
		return (e);
	default:
		if (stat >= 0) 
			break;
		syslog(LOG_ERR, "%s: unexpected error with capability %s",
		    lc->lc_class, cap);
		errno = ERANGE;
		return (e);
	}

	errno = 0;
	q = strtolimit(res, &ep, 0);
	if (!ep || ep == res || (ep[0] && ep[1]) ||
	    ((q == QUAD_MIN || q == QUAD_MAX) && errno == ERANGE)) {
		syslog(LOG_ERR, "%s:%s=%s: invalid size",
		    lc->lc_class, cap, res);
		errno = ERANGE;
		return (e);
	}
	return (q);
}

int
login_getcapbool(lc, cap, def)
	login_cap_t *lc;
	char *cap;
	u_int def;
{
    	if (!lc->lc_cap)
		return (def);

	return (cgetcap(lc->lc_cap, cap, ':') != NULL);
}

void
login_close(lc)
	login_cap_t *lc;
{
	if (lc) {
		if (lc->lc_class)
			free(lc->lc_class);
		if (lc->lc_cap)
			free(lc->lc_cap);
		if (lc->lc_style)
			free(lc->lc_style);
		free(lc);
	}
}

#define	CTIME	1
#define	CSIZE	2
#define	CNUMB	3

static struct {
	int	what;
	int	type;
	char *	name;
} r_list[] = {
	{ RLIMIT_CPU,		CTIME, "cputime", },
	{ RLIMIT_FSIZE,		CSIZE, "filesize", },
	{ RLIMIT_DATA,		CSIZE, "datasize", },
	{ RLIMIT_STACK,		CSIZE, "stacksize", },
	{ RLIMIT_RSS,		CSIZE, "memoryuse", },
	{ RLIMIT_MEMLOCK,	CSIZE, "memorylocked", },
	{ RLIMIT_NPROC,		CNUMB, "maxproc", },
	{ RLIMIT_NOFILE,	CNUMB, "openfiles", },
	{ RLIMIT_CORE,		CSIZE, "coredumpsize", },
	{ -1, 0, 0 }
};

static int
gsetrl(lc, what, name, type)
	login_cap_t *lc;
	int what;
	char *name;
	int type;
{
	struct rlimit rl;
	struct rlimit r;
	char name_cur[32];
	char name_max[32];
    	char *v;

	/*
	 * If we have no capabilities then there is nothing to do and
	 * we can just return success.
	 */
	if (lc->lc_cap == NULL)
		return (0);

	sprintf(name_cur, "%s-cur", name);
	sprintf(name_max, "%s-max", name);

	if (getrlimit(what, &r)) {
		syslog(LOG_ERR, "getting resource limit: %m");
		return (-1);
	}

	/*
	 * We need to pre-fetch the 3 possible strings we will look
	 * up to see what order they come in.  If the one without
	 * the -cur or -max comes in first then we ignore any later
	 * -cur or -max entries.
	 * Note that the cgetent routines will always return failure
	 * on the entry "".  This will cause our login_get* routines
	 * to use the default entry.
	 */
	if ((v = cgetcap(lc->lc_cap, name, '=')) != NULL) {
		if (v < cgetcap(lc->lc_cap, name_cur, '='))
			name_cur[0] = '\0';
		if (v < cgetcap(lc->lc_cap, name_max, '='))
			name_max[0] = '\0';
	}

#define	RCUR	r.rlim_cur
#define	RMAX	r.rlim_max

	switch (type) {
	case CTIME:
		RCUR = login_getcaptime(lc, name, RCUR, RCUR);
		RMAX = login_getcaptime(lc, name, RMAX, RMAX);
		rl.rlim_cur = login_getcaptime(lc, name_cur, RCUR, RCUR);
		rl.rlim_max = login_getcaptime(lc, name_max, RMAX, RMAX);
		break;
	case CSIZE:
		RCUR = login_getcapsize(lc, name, RCUR, RCUR);
		RMAX = login_getcapsize(lc, name, RMAX, RMAX);
		rl.rlim_cur = login_getcapsize(lc, name_cur, RCUR, RCUR);
		rl.rlim_max = login_getcapsize(lc, name_max, RMAX, RMAX);
		break;
	case CNUMB:
		RCUR = login_getcapnum(lc, name, RCUR, RCUR);
		RMAX = login_getcapnum(lc, name, RMAX, RMAX);
		rl.rlim_cur = login_getcapnum(lc, name_cur, RCUR, RCUR);
		rl.rlim_max = login_getcapnum(lc, name_max, RMAX, RMAX);
		break;
	default:
		return (-1);
	}

	if (setrlimit(what, &rl)) {
		syslog(LOG_ERR, "%s: setting resource limit %s: %m",
		    lc->lc_class, name);
		return (-1);
	}
#undef	RCUR
#undef	RMAX
	return (0);
}

int
setclasscontext(class, flags)
	char *class;
	u_int flags;
{
	int ret;
	login_cap_t *lc;

	flags &= LOGIN_SETRESOURCES | LOGIN_SETPRIORITY | LOGIN_SETUMASK |
	    LOGIN_SETPATH;

	lc = login_getclass(class);
	ret = lc ? setusercontext(lc, NULL, 0, flags) : -1;
	login_close(lc);
	return (ret);
}

int
setusercontext(lc, pwd, uid, flags)
	login_cap_t *lc;
	struct passwd *pwd;
	uid_t uid;
	u_int flags;
{
	login_cap_t *flc;
	quad_t p;
	int i;

	flc = NULL;

	if (!lc && !(flc = lc = login_getclass(pwd ? pwd->pw_class : NULL)))
		return (-1);

	/*
	 * Without the pwd entry being passed we cannot set either
	 * the group or the login.  We could complain about it.
	 */
	if (pwd == NULL)
		flags &= ~(LOGIN_SETGROUP|LOGIN_SETLOGIN);

	if (flags & LOGIN_SETRESOURCES)
		for (i = 0; r_list[i].name; ++i) 
			if (gsetrl(lc, r_list[i].what, r_list[i].name,
			    r_list[i].type))
				/* XXX - call syslog()? */;

	if (flags & LOGIN_SETPRIORITY) {
		p = login_getcapnum(lc, "priority", 0, 0);

		if (setpriority(PRIO_PROCESS, 0, (int)p) < 0)
			syslog(LOG_ERR, "%s: setpriority: %m", lc->lc_class);
	}

	if (flags & LOGIN_SETUMASK) {
		p = login_getcapnum(lc, "umask", LOGIN_DEFUMASK,LOGIN_DEFUMASK);
		umask((mode_t)p);
	}

	if (flags & LOGIN_SETGROUP) {
		if (setgid(pwd->pw_gid) < 0) {
			syslog(LOG_ERR, "setgid(%d): %m", pwd->pw_gid);
			login_close(flc);
			return (-1);
		}

		if (initgroups(pwd->pw_name, pwd->pw_gid) < 0) {
			syslog(LOG_ERR, "initgroups(%s,%d): %m",
			    pwd->pw_name, pwd->pw_gid);
			login_close(flc);
			return (-1);
		}
	}

	if (flags & LOGIN_SETLOGIN)
		if (setlogin(pwd->pw_name) < 0) {
			syslog(LOG_ERR, "setlogin(%s) failure: %m",
			    pwd->pw_name);
			login_close(flc);
			return (-1);
		}

	if (flags & LOGIN_SETUSER)
		if (setuid(uid) < 0) {
			syslog(LOG_ERR, "setuid(%d): %m", uid);
			login_close(flc);
			return (-1);
		}

	if (flags & LOGIN_SETPATH)
		setuserpath(lc, pwd ? pwd->pw_dir : "");

	login_close(flc);
	return (0);
}

static void
setuserpath(lc, home)
	login_cap_t *lc;
	char *home;
{
	int hlen, plen;
	int cnt = 0;
	char *path;
	char *p, *q;

	hlen = strlen(home);

	if (p = path = login_getcapstr(lc, "path", NULL, NULL)) {
		while (*p)
			if (*p++ == '~')
				++cnt;
		plen = (p - path) + cnt * (hlen + 1) + 1;
		p = path;
		if (q = path = malloc(plen)) {
			while (*p) {
				p += strspn(p, " \t");
				if (*p == '\0')
					break;
				plen = strcspn(p, " \t");
				if (hlen == 0 && *p == '~') {
					p += plen;
					continue;
				}
				if (q != path)
					*q++ = ':';
				if (*p == '~') {
					strcpy(q, home);
					q += hlen;
					++p;
					--plen;
				}
				memcpy(q, p, plen);
				p += plen;
				q += plen;
			}
			*q = '\0';
		} else
			path = _PATH_DEFPATH;
	} else
		path = _PATH_DEFPATH;
	if (setenv("PATH", path, 1))
		warn("could not set PATH");
}

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
	num = strtoull(str, &expr, radix);
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
	if (strcasecmp(str, "infinity") == 0 || strcasecmp(str, "inf") == 0) {
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

int
secure_path(char *path)
{
	struct stat sb;

	/*
	 * If not a regular file, or is owned/writeable by someone
	 * other than root, quit.
	 */
	if (lstat(path, &sb) < 0) {
		syslog(LOG_ERR, "cannot stat %s: %m", path);
		return (-1);
	} else if (!S_ISREG(sb.st_mode)) {
		syslog(LOG_ERR, "%s: not a regular file", path);
		return (-1);
	} else if (sb.st_uid != 0) {
		syslog(LOG_ERR, "%s: not owned by root", path);
		return (-1);
	} else if (sb.st_mode & (S_IWGRP | S_IWOTH)) {
		syslog(LOG_ERR, "%s: writeable by non-root", path);
		return (-1);
	}
	return (0);
}