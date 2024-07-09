/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI cap.c,v 1.6 1997/04/28 17:39:11 prb Exp
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ucred.h>

#include <ctype.h>
#include <err.h>
#include <libdialer.h>
#include <string.h>
#include <stdlib.h>

char _EMPTYVALUE[] = { 0, };		/* Value of BOOLEAN only capabilities */

#define	FREEV(x)	if ((x) != NULL && (x) != _EMPTYVALUE) free(x)

/*
 * Given the capability buffer b, add all the entries to
 * the capabilities defined by c and return a pointer to
 * the new root.
 */
cap_t *
cap_make(char *b, cap_t *c)
{
	char *n, *v;
	int gotcolon;

	while (*b && *b != ':')
		if (*b++ == '\\' && *b)
			++b;
	if (*b == ':')
		++b;

	while (*b) {
		while (isspace(*b))
			++b;
		if (*b == ':') {
			++b;
			continue;
		}
		n = b;
		while (*b && *b != ':' && *b != '@'
			  && *b != '=' && *b != '#')
			++b;
		gotcolon = 0;
		switch (*b) {
		case '@':
			*b++ = 0;
			if (cap_add(&c, n, 0, CM_NORMAL))
				return(NULL);
			break;
		case ':':
			gotcolon = 1;
			*b++ = 0;
			if (*n)
				if (cap_add(&c, n, _EMPTYVALUE, CM_NORMAL))
					return(NULL);
			break;
		case '\0':
			if (*n)
				if (cap_add(&c, n, _EMPTYVALUE, CM_NORMAL))
					return(NULL);
			break;
		case '#': case '=':
			*b = 0;
			v = ++b;
			while (*b && *b != ':')
				if (*b++ == '\\' && *b)
					++b;
			if (*b)
				*b++ = '\0';
			if (*v == '@' && v[1] == '\0')
				v = 0;
			else
				cap_strrestore(v, C_PIPE);
			if (cap_add(&c, n, v, CM_NORMAL))
				return(NULL);
			continue;

		}
		if (!gotcolon)
			while (*b && *b != ':')
				if (*b++ == '\\' && *b)
					++b;
		if (*b == ':')
			++b;
	}
	return (c);
}

/*
 * Free the capability structure
 */
void
cap_free(cap_t *c)
{
	if (c == _EMPTYCAP)
		return;
	while (c) {
		cap_t *n = c->next;
		free(c->name);
		free(c->values);
		FREEV(c->value);
		free(c);
		c = n;
	}
}

/*
 * Add name n with value v to capability tree pointed to by cp.
 * The character '\377' is used to seperate values in n.  The
 * characters '\0' and '\377' are not allowed in individual values.
 */
int
cap_add(cap_t **cp, char *n, char *v, int method)
{
	cap_t *c;
	cap_t *nc = *cp;
	cap_t *lc = 0;
	char *o, *p, *q;
	int i = -1, j, k;

	if (nc == _EMPTYCAP)
		nc = 0;

    	while (nc) {
		if ((i = strcmp(n, nc->name)) == 0) {
			if (method != CM_NORMAL)
				break;
			return(0);
		}
		if (i < 0)
			break;
		lc = nc;
		nc = nc->next;
	}
	if (i == 0) {
		switch (method) {
		case CM_UNION:
			if (v == NULL)
				return(0);
			if (nc->value == NULL) {
				c = nc;
				break;
			}
			if (nc->value == _EMPTYVALUE) {
				free(nc->values);
				c = nc;
				break;
			}
			for (i = 2, p = v; *p; ++p)
				if (*p == (char)0377)
					++i;
			k = p - v;
			for (j = 0; nc->values[j]; ++j, ++i)
				k += strlen(nc->values[j]) + 1;

			nc->values = realloc(nc->values, sizeof(char *) * i);
			if (nc->values == NULL) {
				cap_free(*cp);
				*cp = 0;
				return(-1);
			}
			q = nc->value;	/* save old value for relocation */
			nc->value = realloc(nc->value, k + 1);
			if (nc->value == NULL) {
				free(nc->values);
				nc->values = NULL;
				cap_free(*cp);
				*cp = 0;
				return(-1);
			}
			/*
			 * Relocate old values
			 */
			for (i = 0; i < j; ++i)
				nc->values[i] += nc->value - q;
			o = nc->value + k - (p - v);
			while (v && *v) {
				if (isspace(*v)) {
					++v;
					continue;
				}
				q = strchr(v, '\377');
				nc->values[j] = o;
				if (q) {
					memcpy(o, v, q - v);
					o += q - v;
					++q;
				} else {
					strcpy(o, v);
					while (o[1])
						++o;
				}
				while (isspace(*o))
					--o;
				*++o = '\0';
				++o;
				for (i = 0; i < j; ++i)
					if (strcmp(nc->values[j],
					    nc->values[i]) == 0)
						break;
				if (i == j)
					++j;
				v = q;
			}
			nc->values[j] = NULL;
			return(0);
			
		case CM_INTERSECTION:
			if (v == NULL) {
				FREEV(nc->value);
				if (nc->values)
					free(nc->values);
				nc->value = NULL;
				nc->values = NULL;
				return(0);
			}
			if (v == _EMPTYVALUE) {
				FREEV(nc->value);
				if (nc->values == NULL) {
					nc->values = malloc(sizeof(char *) * 2);
					if (nc->values == NULL) {
						nc->value = NULL;
						cap_free(*cp);
						*cp = NULL;
						return(-1);
					}
				}
				nc->values[0] = nc->value = _EMPTYVALUE;
				nc->values[1] = NULL;
				return(0);
			}
			if (nc->value == NULL || nc->value == _EMPTYVALUE)
				return(0);
			for (j = 0; nc->values[j]; ++j) {
				for (p = v; p && *p;) {
					if (isspace(*p)) {
						++p;
						continue;
					}
					q = strchr(p, '\377');
					if ((o = q) == NULL)
						for (o = p; *o; ++o)
						    ;
					else
						++q;

					while (isspace(*o))
						--o;
					++o;
					if (strlen(nc->values[j]) == o - p &&
					    strncmp(nc->values[j], p, o-p) == 0)
						break;
					p = q;
				}
				if (p == 0 || *p == '\0')
					for (i = j--; nc->values[i]; ++i)
						nc->values[i] = nc->values[i+1];
			}
			if (nc->values[0] == NULL) {
				free(nc->values);
				FREEV(nc->value);
				nc->values = NULL;
				nc->value = NULL;
			}
			return(0);

		default:
			return(0);
		}
	} else {
		if ((c = (cap_t *)malloc(sizeof(cap_t))) == NULL) {
			cap_free(*cp);
			*cp = 0;
			return(-1);
		}
		if ((c->name = strdup(n)) == NULL) {
			free(c);
			cap_free(*cp);
			*cp = 0;
			return(-1);
		}
	}

    	if (v && v != _EMPTYVALUE && (v = strdup(v)) == NULL) {
		free(c->name);
		free(c);
		cap_free(*cp);
		*cp = 0;
		return(-1);
	}
	c->value = v;
	if (c->value) {
		i = 2;
		for (p = c->value; *p; ++p)
			if (*p == (char)0377)
				++i;
		if ((c->values = (char **)malloc(sizeof(char *) * i)) == NULL) {
			FREEV(c->value);
			free(c->name);
			free(c);
			cap_free(*cp);
			*cp = 0;
			return(-1);
		}
		if (i == 2) {
			c->values[0] = c->value;
			c->values[1] = 0;
		} else {
			p = c->value;
			i = 0;
			/*
			 * Strip spaces around the separator as we go.
			 */
			while (*p) {
				c->values[i] = p;
				while (*p && *p != (char)0377)
					++p;
				if (*p) {
					for (q = p; q > c->values[i]; --q)
						if (!isspace(q[-1]))
							break;
					while (isspace(*p))
						++p;
					*q = '\0';
					++p;
				}
				++i;
			}
			c->values[i] = 0;
		}
	} else
		c->values = 0;
	if (c != nc) {
		c->next = nc;
		if (lc)
			lc->next = c;
		else
			*cp = c;
	}
	return(0);
}

/*
 * merge capabilities from ``op'' into ``cp''.
 * ``op'' is consumed and should neary be refered to again.
 * Note that the absence of an option implies that there are
 * no restrictions on that option.
 */
void
cap_merge(cap_t **cp, cap_t *op, int method)
{
	cap_t *t;
	cap_t *c;
	cap_t *lc;
	char **nv;
	char *p;
	int i, j;

	if (*cp == _EMPTYCAP)
		*cp = 0;

	if (!*cp) {
		*cp = op;
		return;
	}

	switch (method) {
	case CM_NORMAL:
	case CM_UNION:
	case CM_INTERSECTION:
		if (op == _EMPTYCAP)
			return;
		break;

	case CM_PERMISSIONS:
		if (op == 0) {
			cap_free(*cp);
			*cp = NULL;
			return;
		}
		if (op == _EMPTYCAP)
			return;


		while (op && (strcmp(op->name, (*cp)->name) < 0)) {
			t = op->next;
			FREEV(op->value);
			free(op->values);
			free(op);
			op = t;
		}
		break;
	default:
		if (op && op != _EMPTYCAP)
			cap_free(op);
		return;
	}

	lc = 0;
	c = *cp;

	while (op && c) {
		i = strcmp(op->name, c->name);
		if (i < 0) {			/* op is before c */
			t = op->next;
			switch (method) {
			default:
				if (lc) {
					op->next = c;
					lc->next = op;
				} else {
					op->next = *cp;
					*cp = op;
				}
				lc = op;
				break;

			case CM_PERMISSIONS:
				FREEV(op->value);
				free(op->values);
				free(op);
				break;
			}
			op = t;
			continue;
		}
		if (i > 0) {			/* op is after c */
			t = c->next;
			switch (method) {
			case CM_PERMISSIONS:
				FREEV(c->value);
				free(c->values);
				if (lc)
					lc->next = c->next;
				else
					*cp = c->next;
				free(c);
				break;
			default:
				break;
			}
			lc = c;
			c = t;
			continue;
		}

		/* op and c have the same element */

		switch (method) {
		case CM_NORMAL:	/* we just toss op */
			break;
		case CM_INTERSECTION: /* only keep the common values */
			if (c->value == _EMPTYVALUE || c->value == NULL)
				break;
			if (op->value == _EMPTYVALUE || op->value == NULL) {
				if (c->values)
					free(c->values);
				c->value = op->value;
				c->values = op->values;
				t = op->next;
				free(op);
				op = t;
				lc = c;
				c = c->next;
				continue;
			}
			for (i = 0; c->values[i];) {
				for (nv = op->values; *nv; ++nv)
					if (strcmp(c->values[i], *nv) == 0)
						break;
				if (*nv == NULL) {
					for (j = i; c->values[j]; ++j)
						c->values[j] = c->values[j+1];
				} else
					++i;
			}
			if (c->values[0] == NULL) {
				free(c->values);
				FREEV(c->value);
				c->values = NULL;
				c->value = NULL;
			} else if (c->value != c->values[0]) {
				strcpy(c->value, c->values[0]);
				c->values[0] = c->value;
			}
			break;
			
		case CM_UNION:
			if (c->value == _EMPTYVALUE || c->value == NULL) {
				if (c->values)
					free(c->values);
				c->value = op->value;
				c->values = op->values;
				t = op->next;
				free(op);
				op = t;
				lc = c;
				c = c->next;
				continue;
			}
			if (op->value == _EMPTYVALUE || op->value == NULL)
				break;	/* toss empty op's */
			/** FALL THRU TO CM_PERMISSIONS **/
		case CM_PERMISSIONS:
			if (c->value == _EMPTYVALUE || op->value == _EMPTYVALUE) {
				FREEV(op->value);
				free(op->values);
				t = op->next;
				free(op);
				op = t;

				FREEV(c->value);
				free(c->values);
				t = c->next;
				free(c);
				c = t;
				if (lc)
					lc->next = c;
				else
					*cp = c;
				continue;
			}
			j = 0;
			for (nv = c->values; *nv; ++nv) {
				j += strlen(*nv) + 1;
				++i;
			}
			for (nv = op->values; *nv; ++nv) {
				j += strlen(*nv) + 1;
				++i;
			}
			++i;
			nv = malloc(sizeof(char *) * i);
			if (nv == NULL) {
				warn("out of memory");
				cap_free(op);
				cap_free(*cp);
				*cp = 0;
				return;
			}
			p = malloc(j);
			if (p == NULL) {
				warn("out of memory");
				free(nv);
				cap_free(op);
				cap_free(*cp);
				*cp = 0;
				return;
			}
			for (j = 0; c->values[j]; ++j) {
				strcpy(p, c->values[j]);
				nv[j] = p;
				/*
				 * delete duplicates
				 */
				for (i = 0; op->values[i]; ++i) {
					if (strcmp(p, op->values[i]) == 0)
						op->values[i] = _EMPTYVALUE;
				}
				while (*p++)
					;
			}

			free(c->values);
			FREEV(c->value);
			c->values = nv;
			c->value = nv[0];

			nv += j;

			for (j = 0; op->values[j]; ++j)
				if (op->values[j] != _EMPTYVALUE) {
					strcpy(p, op->values[j]);
					*nv++ = p;
					while (*p++)
						;
				}
			*nv = 0;
			break;
		}

		t = op->next;
		free(op->values);
		FREEV(op->value);
		free(op);
		op = t;
		lc = c;
		c = c->next;
	}
	switch (method) {
	case CM_PERMISSIONS:
		cap_free(c);
		cap_free(op);
		if (lc)
			lc->next = 0;
		else
			*cp = _EMPTYCAP; /* NULL would mean an error */
		break;
	default:
		if (c) {
			while (c->next)
				c = c->next;
			c->next = op;
		} else if (lc)
			lc->next = op;
		else
			*cp = op;
	}
}

/*
 * return maximum numeric value of all the values defined in the
 * given capability
 */
int
cap_max(cap_t *cp)
{
	int m, n;
	char **av = cp->values;

	m = strtol(*av, 0, 0);

	while (*++av) {
		n = strtol(*av, 0, 0);
		if (m < n)
			m = n;
	}
	return (m);
}

/*
 * Look for the first matching value in s and p.
 * values can be either numeric or strings
 */
char *
cap_check(cap_t *s, cap_t *p)
{
	char **v1;
	char **v2;
	char *e1, *e2;
	int n1, n2;

	if (p->value == _EMPTYVALUE)
		return(s->value);

	for (v1 = s->values; *v1; ++v1) {
		n1 = strtol(*v1, &e1, 0);
		for (v2 = p->values; *v2; ++v2) {
			if (e1 != *v1 && !*e1) {
				n2 = strtol(*v2, &e2, 0);
				if (e2 != *v2 && !*e2 && n1 == n2)
					return (*v1);
			} else if (strcmp(*v1, *v2) == 0)
				return (*v1);
		}
	}
	return (0);
}

/*
 * Verify that every value in s is also in p
 */
int
cap_verify(cap_t *s, cap_t *p)
{
	char **v1;
	char **v2;

	if (p->value == _EMPTYVALUE)
		return (1);

	/*
	 * go through the requested call types
	 * if even one does not match, reject the request.
	 */
	for (v1 = s->values; *v1; ++v1) {
		for (v2 = p->values; *v2; ++v2)
			if (strcmp(*v1, *v2) == 0)
				break;
		if (!*v2)
			return (0);
	}
	return (1);
}

cap_t *
cap_look(cap_t *c, char *name)
{
	if (c == _EMPTYCAP)
		return(NULL);
	while (c && strcmp(c->name, name) != 0)
		c = c->next;
	return(c);
}

/*
 * Dump the capabilities into buf, which has nbytes of spaces
 * Return pointer to buf on success or NULL if there is not
 * enough space.
 */
char *
cap_dump(cap_t *c, char *buf, int nbytes)
{
	char **v;
	char *obuf = buf;
	char *p, *t;

#define	APPEND(x)	{ if (nbytes < 2) return(NULL); *buf++ = x; --nbytes; }

	if (c == _EMPTYCAP)
		return(_EMPTYVALUE);

	while (c) {
		p = c->name;
		while (*p)
			APPEND(*p++)
		if (c->value != _EMPTYVALUE && c->values) {
			char sep = '=';
			for (v = c->values; *v; ++v) {
				char *p = *v;

				APPEND(sep)
				sep = '|';
				while (*p) {
					t = cap_strencode(*p, C_PIPE) - 1;
					while (*++t)
						APPEND(*t)
					++p;
				}
			}
		}
		APPEND(':');
		c = c->next;
	}
	*buf++ = '\n';
	*buf++ = '\0';
	return(obuf);
}

/*
 * Remove \ escapes
 * Special processing according to flags:
 *
 * C_PIPE	convert |'s into '\377'
 * C_SPACE	ignore leanding (they must be escaped)
 */
void
cap_strrestore(char *s, int flags)
{
	char *p;

	p = s;

	if (flags && C_SPACE)
		while (isspace(*s))
			++s;

	while (*s) {
		if ((*p++ = *s) == '\\') {
			if ('0' <= *s && *s <= '7') {
				register int n, i;
	 
				n = 0;
				i = 3;  /* maximum of three octal digits */
				do { 
					n = n * 8 + (*s++ - '0');
				} while (--i && '0' <= *s && *s <= '7');
				p[-1] = n;
			} else switch (*++s) {
			case 'b': case 'B':
				p[-1] = '\b';
				break;
			case 't': case 'T':
				p[-1] = '\t';
				break;
			case 'n': case 'N':
				p[-1] = '\n';
				break;
			case 'f': case 'F':
				p[-1] = '\f';
				break;
			case 'r': case 'R':
				p[-1] = '\r';
				break;
			case 'e': case 'E':
				p[-1] = '\033';
				break;
			case 'c': case 'C':
				p[-1] = ':';
				break;
			default:
				/*
				 * Catches '\', '^', and
				 *  everything else.
				 */
				p[-1] = *s;
			break;
			}
		} else if ((flags && C_PIPE) && *s == '|')
			p[-1] = (char)0377;
		++s;
	}
	*p = 0;
}

char *
cap_strencode(int c, int flags)
{
	static char rbuf[8];

	switch (c) {
	case '\b':
		return("\\B");
		break;
	case '\t':
		return("\\T");
		break;
	case '\n':
		return("\\N");
		break;
	case '\f':
		return("\\F");
		break;
	case '\r':
		return("\\R");
		break;
	case '\033':
		return("\\E");
		break;
	case ':':
		return("\\C");
		break;
	case '|':
		if ((flags & C_PIPE) == 0)
			goto character;
		return("\\|");
		break;
	case ' ':
		if ((flags & C_SPACE) == 0)
			goto character;
		return("\\ ");
		break;
	default: character:
		if (isprint(c)) {
			rbuf[0] = c;
			rbuf[1] = 0;
		} else {
			rbuf[0] = '\\';
			rbuf[1] = ((c >> 6) & 03) + '0';
			rbuf[2] = ((c >> 3) & 07) + '0';
			rbuf[3] = ( c       & 07) + '0';
			rbuf[4] = 0;
		}
		return(rbuf);
	}
}
