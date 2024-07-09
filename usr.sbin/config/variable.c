/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved.
 *
 *      BSDI variable.c,v 2.10 2002/04/24 19:16:01 torek Exp
 */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

#define	LBRACE	'{'
#define	RBRACE	'}'

typedef struct var_t var_t;

struct var_t {
	var_t		*next;
	const char	*name;
	const char	*value;
	int		hard;
};

static var_t *vars;

const char *
assign(const char *name, const char *value, int hard)
{
	var_t *v;
	name = intern(name);

	for (v = vars; v; v = v->next) {
		if (v->name == name) {
			if (hard || v->hard == 0) {
				v->value = intern(value);
				v->hard = hard;
			}
			return(v->value);
		}
	}
	v = emalloc(sizeof(*v));
	v->name = name;
	v->value = intern(value);
	v->hard = hard;
	v->next = vars;
	vars = v;
	return(v->value);
}

const char *
check(const char *name)
{
	var_t *v;
	const char *e;

	for (v = vars; v; v = v->next)
		if (strcmp(v->name, name) == 0)
			return (v->value);
	if (importenv && (e = getenv(name)) != NULL)
		return (assign(name, convertposix(e), 0));
	return (NULL);
}

const char *
require(const char *name)
{
	const char *r;

	if ((r = check(name)) == NULL)
		errx(1, "%s: required variable not defined", name);
	return (r);
}

static char *
_expand(const char *string, int line)
{
	extern const char *srcfile;
	char *buf = NULL;

	const char *s;
	const char *n, *v;
	char *d;
	int cnt;

	/*
	 * First count up the space needed
	 */
	cnt = 0;
	for (s = string; *s; ++s) {
		if (*s == '$') {
			if (*++s != LBRACE)
				errx(1, "%s:%d: $ requires %c",
				    srcfile, line, LBRACE);
			n = ++s;
			while (*s && *s != RBRACE)
				++s;
			if (*s != RBRACE)
				errx(1, "%s:%d: $%c%.*s missing closing %c",
				    srcfile, line, LBRACE, s - n, n, RBRACE);
			n = internn(n, s - n);
			/*
			 * Ugh!  On first required use of sys we
			 * set it if not already set.
			 * Should we require it?
			 */
			if (strcmp(n, "SYS") == 0 ||
			    strcmp(n, "CONFDIR") == 0) {
				v = check(n);
				if (v == NULL) {
					v = "../..";
					assign(n, v, 0);
				}
			} else
				v = require(n);
			cnt += strlen(v);
		} else
			++cnt;
	}
	buf = emalloc(++cnt);

	for (d = buf, s = string; *s; ++s) {
		if (*s == '$') {
			++s;
			n = ++s;
			while (*s && *s != RBRACE)
				++s;
			n = internn(n, s - n);
			n = require(n);
			while (*n)
				*d++ = *n++;
		} else
			*d++ = *s;
	}
	*d = '\0';
	return (buf);
}

char *
expand(const char *string, int line)
{
	char *p, *s;

	s = _expand(string, line);

	while ((p = strchr(s, '$')) != NULL && p[1] == LBRACE &&
	    strchr(p, RBRACE) != NULL)
		s = _expand(s, line);

	return (s);
}

int
emitvars(FILE *fp)
{
	int n;
	var_t *v;
	const char *confdir;

	if ((confdir = check("CONFDIR")) != NULL)
		if (fprintf(fp, "%s=%s\n", "CONFDIR", confdir) < 0)
			return (-1);

	for (v = vars; v; v = v->next) {
		if (strcmp(v->name, "CONFDIR") == 0)
			continue;
		if (confdir && strcmp(v->name, "SYS") == 0 &&
		    *v->value != '/' && *v->value != '$')
			n = fprintf(fp, "%s=${CONFDIR}/%s\n", v->name,v->value);
		else
			n = fprintf(fp, "%s=%s\n", v->name, v->value);
		if (n < 0)
			return (-1);
	}
	return (0);
}
