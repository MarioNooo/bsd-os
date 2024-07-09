/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI phone.c,v 1.5 2001/12/01 00:45:17 chrisk Exp
 */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libdialer.h>
#include "GenSym.h"
#include "phone.h"

struct rule_t {
	struct rule_t	*next;
	struct rule_t	*prev;
	char		*label;
	char		*pattern;
	argv_t		*actions;
};

typedef struct flist_t {
	struct flist_t	*next;
	char		*name;
	struct stat	stat;
} flist_t;

/*
 * To keep the % in VI working we use #defines for { and }
 */
#define	OPN	'{'
#define	CLS	'}'

static GenSym_t *variables = 0;
static GenSym_t *specials = 0;
static GenSym_t *rules = 0;
static flist_t *flist = NULL;

static char *expand(char *, char *, int);
static char *phone_macro(char *, char *, int, char *, char *, int);
static rule_t *parse_rule_set(FILE *, char *, int *);
static int match_pattern(char *, char *);
static char *ctopd(int, char *, char *, int);
static char *desep(char *);
static void free_rule_set(rule_t *);

static inline int issep(int c) { return(isspace(c) || c == '\n' || c == '\r'); }
static inline int isvar(int c) { return(isalnum(c) || c == '_' || c == '-'); }

#define	STREQ(b)	(strcasecmp(*av,b)==0)

#define	NM	1
#define	PAT	2
#define	VAR	3

/*
 * Return a ruleset by name
 */
rule_t *
phone_ruleset(char *name)
{
	GenSym_t *gs;
	if ((gs = GenSym(&rules, name, 0, 0, 0, 0)) == NULL)
		return(0);
	return((rule_t *)gs->gs_value);
}

/*
 * return a variable by name
 */
char *
phone_variable(char *v)
{
	GenSym_t *gs;
	if ((gs = GenSym(&variables, v, 0, 0, 0, 0)) == NULL)
		return(NULL);
	return((char *)gs->gs_value);
}

/*
 * return a variable by name
 */
char *
phone_variable_set(char *v, char *n)
{
	GenSym_t *gs;
	gs = GenSym(&variables, v, 0, n, strlen(n), GS_DYNAMIC|GS_REPLACE);
	if (gs == 0)
		return(NULL);
	return((char *)gs->gs_value);
}

/*
 * Process ``number'' according to ruleset ``r'' leaving the result
 * in ``result'' with at most ``resultlen'' bytes.
 * Returns the argument to the matching "resolved" directive in the ruleset.
 * Returns NULL if the number is rejected or could not be expanded.
 * phone_macro is called on the number before it is processed.
 *
 * If ``r'' is not defined then the default ruleset defined by "RULE"
 * is used.
 */
char *
process_number(char *number, char *result, int resultlen, rule_t *r)
{
	char **av;
	rule_t *g;
	char *s, *b;
	flist_t *f;
	struct stat sb;
	char *file = NULL;
	int n;
	int flip = 0;
	char pre[1024];
	char post[1024];
	char _num[2][1024];
#define	App	_num[flip^1]
#define	Num	_num[flip]
			
	if (r == 0) {
		for (f = flist; f; f = f->next) {
			if (file != NULL ||
			    (stat(f->name, &sb) == 0 &&
			    (sb.st_size != f->stat.st_size ||
			     sb.st_mtime != f->stat.st_mtime)))
				file = f->name;
		}

		if (file != NULL) {
			while (variables)
				GenSymDelete(&variables, variables->gs_name);
			while (specials)
				GenSymDelete(&specials, specials->gs_name);
			while (rules) {
				free_rule_set(rules->gs_value);
				GenSymDelete(&rules, rules->gs_name);
			}
			if (parse_rule_file(file))
				return(0);
		}

		s = phone_variable("RULE");
		if (s == NULL || *s == 0) {
			warnx("RULE variable must defined and not empty");
			return(0);
		}
		if ((r = phone_ruleset(s)) == NULL) {
			warnx("%s: ruleset missing", s);
			return(0);
		}
	}

	if (phone_macro(number, App, sizeof(App), 0, 0, NM) == NULL)
		return(NULL);

	GenSymDelete(&variables, "NUMBER");

	while (r) {
		flip ^= 1;
		if (r->pattern)
			n = match_pattern(Num, r->pattern);
		else
			n = -1;
		g = r->next;
		if (n > 0 && Num[n] == '<' && strncmp(Num + n, "<EXT>", 5)) {
			if ((b = strchr(Num + n, '>')) == NULL)
				return(0);
			n = ++b - Num;
		}

		if (n < 0 || r->actions == 0) {
			r = g;
			if (n > 0)
				strcpy(App, Num + n);
			else
				flip ^= 1;
			continue;
		}
		
		memcpy(pre, Num, n);
		pre[n] = 0;
		strcpy(post, Num + n);
		App[0] = 0;

		av = r->actions->argv;
		while (*av) {
			if ((s = strchr(*av, '=')) != NULL) {
				if (s[-1] == '?')
					--s;
				for (b = *av; b < s; ++b)
					if (!isvar(*b)) {
						warn("%s: invalid variable", *av);
						return(0);
					}
				b = s + 1;
				if (*s == '?')
					++b;
				b = phone_macro(b, Num, sizeof(Num), pre, post, VAR);
				if (b == 0)
					return(0);
				if (GenSym(&variables,
				    *av, s - *av, b, strlen(b),
				    GS_DYNAMIC |
				    (*s == '?' ? GS_INSERT : GS_REPLACE)) == 0){
					warnx("out of memory");
					return(0);
				}
			} else if (STREQ("goto")) {
				if (*++av == NULL) {
					warnx("goto missing label");
					return(0);
				}
				for (g = r; g; g = g->next)
					if (g->label && STREQ(g->label))
						break;
				if (g == 0)
				for (g = r; g->next; g = g->prev)
					if (g->label && STREQ(g->label))
						break;
				if (g->next == 0) {
					warnx("%s: no such label", g->label);
					return(0);
				}
			} else if (STREQ("prepend")) {
				if (*++av == NULL) {
					warnx("prepend missing value");
					return(0);
				}
				n = strlen(App);
				b = App + n;
				n = sizeof(App) - n;
				if (phone_macro(*av, b, n, pre, post, VAR) == 0)
					return(0);
			} else if (STREQ("resolved")) {
				if (*++av == NULL)
					warnx("resolved missing type");
				return(expand(*av, result, resultlen));
			} else if (STREQ("reject"))
				return(0);
			else {
				warnx("%s: invalid action", *av);
				return(0);
			}
			++av;
		}
		strcat(App, post);
		r = g;
	}
#undef	App
#undef	Num
	return(0);
}

/*
 * parse a ruleset file.
 * directly handle all lines except an actual ruleset.
 * The ';' character introduces a comment
 * spaces are ignored, just like fortran (except within the actual ruleset)
 */
int
parse_rule_file(char *file)
{
	static int depth = 0;
	char buf[1024];
	char *b, *n;
	int line = 0;
	FILE *fp;
	rule_t *r;
	flist_t *f;

	if ((fp = fopen(file, "r")) == NULL) {
		warn("%s", file);
		return(-1);
	}

	if (depth++ == 0) {
		while (flist) {
			f = flist->next;
			free(flist->name);
			free(flist);
			flist = f;
		}
	}

	if ((f = malloc(sizeof(flist_t))) == NULL) {
		warn("%s", file);
		goto error;
	}

	f->next = flist;
	if ((f->name = strdup(file)) == NULL) {
		free(f);
		warn("%s", file);
		goto error;
	}

	if (fstat(fileno(fp), &f->stat) < 0) {
		free(f);
		warn("%s", file);
		goto error;
	}
	flist = f;
	
	while (fgets(buf, sizeof(buf), fp)) {
		++line;
		if ((n = strchr(buf, ';')) != NULL)
			*n = 0;
		if (desep(buf) == NULL)
			continue;
		if (strncmp(buf, "#include", 8) == 0) {
			/* do include */
			b =  buf + 8;
			if (*b++ != '<' ||
			    (n = strchr(b, '>')) == 0 ||
			    n[1])
				goto invalid;
			*n = 0;
			if (parse_rule_file(b) < 0)
				goto error;
		} else if (*buf == '<') {
			/* handle <SPECIAL>= */
			if ((n = strchr(buf, '>')) < buf+2 ||
			    n[1] != '=' || n[2] == 0 || n[3])
				goto invalid;
			for (b = buf + 1; b < n - 1; ++b)
				if (!isvar(*b))
					goto invalid;
			if(GenSym(&specials, n+2, 1, buf, (n + 1) - buf,
			    GS_DYNAMIC|GS_REPLACE) == 0) {
				warnx("%s:%d: out of memory",file,line);
				goto error;
			}
		} else if (isvar(*buf) && (n = strchr(buf, '='))) {
			/* handle VAR=value or VAR?=value */
			if (n[-1] == '?')
				--n;
			for (b = buf; b < n; ++b)
				if (!isvar(*b))
					goto invalid;
			b = n + 1;
			if (*n == '?')
				++b;
			if (GenSym(&variables, buf, n - buf, b, strlen(b),
			    GS_DYNAMIC |
			    (*n == '?' ? GS_INSERT : GS_REPLACE)) == 0) {
				warnx("%s:%d: out of memory", file, line);
				goto error;
			}
		} else if (isvar(*buf) && (n = strchr(buf, OPN))) {
			/* handle RuleName { *//* for vi: } */
			if (n[1])
				goto invalid;
			for (b = buf; b < n; ++b)
				if (!isvar(*b))
					goto invalid;
			if ((r = parse_rule_set(fp, file, &line)) == NULL)
				goto error;
			if (GenSym(&rules, buf, n - buf, r, 0, GS_INSERT) == 0){
				warnx("%s:%d: out of memory", file, line);
				goto error;
			}
		} else {
invalid:
			warnx("%s:%d: syntax error", file, line);
			goto error;
		}
	}
	fclose(fp);
	--depth;
	return(0);
error:
	fclose(fp);
	--depth;
	return(-1);
}

/*
 * Process a rule set from ``file'' which is opened on ``fp''.
 * ``pline'' points to the current line number.
 * Processing continues until a } is encountered on a line by itself.
 */
static rule_t *
parse_rule_set(FILE *fp, char *file, int *pline)
{
	char buf[1024];
	char *b;
	char *n;
	char *p;
	int line = *pline;

	rule_t *r, *br;

	br = 0;
	r = (rule_t *)malloc(sizeof(rule_t));
	memset(r, 0, sizeof(rule_t));

	while ((b = fgets(buf, sizeof(buf), fp)) != NULL) {
		if ((n = strchr(b, ';')) != NULL)
			*n = 0;
		++line;
		while (issep(*b))
			++b;
		if (*b == CLS) {
			n = b + 1;
			while (issep(*n))
				++n;
			if (*n == 0) {
				*pline = line;
				return(br);
			}
		}
		while (*b) {
			while (issep(*b))
				++b;
			if (*b == 0)
				break;
			n = b;
			while (*b && !issep(*b) && *b != ':')
				++b;
			if (*b == ':') {
				if (isdigit(*n)) {
					warnx(
	"%s:%d: labels cannot start with a digit", file, line);
					goto failed;
				}
				for (p = n; p < b; ++p) {
					if (*p != '_' && !isalnum(*p)) {
						warnx(
	"%s:%d: labels can only contain [_a-zA-Z0-9]", file, line);
						goto failed;
					}
				}
				
				if (r->label) {
					warnx(
	"%s:%d: only one label per line", file, line);
					goto failed;
				}
				if (r->pattern) {
					warnx(
	"%s:%d: labels must be at start of line", file, line);
					goto failed;
				}
				if ((r->label = malloc(b + 1 - n)) == 0) {
					warnx(
	"%s:%d: %s\n", file, line, strerror(errno));
					goto failed;
				}
				memcpy(r->label, n, b - n);
				r->label[b - n] = 0;
				++b;
			} else if (r->pattern == NULL) {
				if ((r->pattern = malloc(b + 1 - n)) == 0) {
					warnx(
	"%s:%d: %s\n", file, line, strerror(errno));
					goto failed;
				}
				memcpy(r->pattern, n, b - n);
				r->pattern[b - n] = 0;
			} else {
				if (r->actions == 0 &&
				    (r->actions = start_argv(0)) == 0) {
					warnx(
	"%s:%d: %s\n", file, line, strerror(errno));
					goto failed;
				}
				if (add_argvn(r->actions, n, b - n) == 0) {
					warnx(
	"%s:%d: %s\n", file, line, strerror(errno));
					goto failed;
				}
			}
		}
		if (r->label || r->pattern) {
			if (br) {
				r->prev = br->prev;
				br->prev->next = r;
			} else
				br = r;
			br->prev = r;

			r = (rule_t *)malloc(sizeof(rule_t));
			memset(r, 0, sizeof(rule_t));
		}
	}
	warnx("%s:%d: premature EOF", file, line);
failed:
	*pline = line;
	return(0);
}

/*
 * Free the storage associated with the specified rule set
 */
static void
free_rule_set(rule_t *r)
{
	rule_t *n;
	while (r) {
		n = r->next;
		if (r->label)
			free(r->label);
		if (r->pattern)
			free(r->pattern);
		if (r->actions)
			free_argv(r->actions);
		r = n;
	}
}

/*
 * Check to see if ``number'' matches ``pattern''.
 * return the number of bytes of number which matched
 * the pattern.  -1 is returned if it did not match.
 */
static int
match_pattern(char *number, char *pattern)
{
	char buf[256];
	char *s, *b;
	int n;

	b = pattern;
	s = number;

	if ((b = phone_macro(pattern, buf, sizeof(buf), 0, 0, PAT)) == NULL)
		errx(1, "Invalid pattern\n");

	while (*b == '-')
		++b;

	while (*b && *b != '$' && *s && strncmp(s, "<EXT>", 5)) {
		if (*s == '<') {
			while (*s && *s != '>')
				++s;
			if (*s == '>')
				++s;
			continue;
		}
		if (*b == '-') {
			++b;
			continue;
		}
		if (*b == *s) {
			++b;
			++s;
			continue;
		}
		if (*b == '[') {
			while (*++b && *b != ']') {
				if (*b == '[')
					errx(1, "Invalid pattern\n");

				if (*b == *s ||
				    (*b == '-' && *s <= b[1] &&
				     (b[-1] == '[' || b[-1] <= *s)))
					break;
			}
			if (*b && *b != ']') {
			    ++s;
			    while (*b && *b != ']')
				    ++b;
			    if (*b)
				    ++b;
			    continue;
			}
		}
		break;
	}
	while (*b == '-')
		++b;
	if ((n = strncmp(s, "<EXT>", 5)) == 0)
		s += 5;
	else if (*s == 0)
		n = 0;
	if (*b == '$')
		n = n ? -1 : s - number;
	else
		n = *b ? -1 : s - number;
	return(n);
}

#define	APPEND(c)	if (b < (buf + buflen)) *b++ = c; else ++oflow

/*
 * expand the string pointed to by ``n'' placing the result in ``buf''
 * which is ``buflen'' bytes long.  ``pre'' and ``post'' contain the
 * already processed and yet to be processed data.
 * Mode is one of:
 *	NM	Expanding number from the user
 *	PAT	Expanding a pattern
 *	VAR	Expanding a variable
 * This function is recursive but stops after 20 levels of recursion
 */
static char *
phone_macro(char *n, char *buf, int buflen, char *pre, char *post, int mode)
{
	static int recurse = 0;

	GenSym_t *gs;
	char *b = buf;
	char *v;
	int plus = 0;
	int oflow = 0;
	int c;

	if (recurse++ > 20) {
		--recurse;
		return(0);
	}

        while (!oflow && *n) {
		if ((v = ctopd(c = *n, pre, post, mode)) != NULL)
			while (*v)
			    APPEND(*v++);
		else
			switch (*n) {
			case '-':
				break;
			case '+':
				if (plus++ == 0) {
					APPEND('+');
				}
				break;
			case '<':
				do
					APPEND(*n);
				while (*n != '>' && *++n);
				break;
			case OPN:
				v = ++n;
				while (*n && *n != CLS)
					++n;
				gs = GenSym(&variables, v, n - v, 0, 0, 0);
				if (gs && (v = (char *)gs->gs_value)) {
					v = phone_macro(v, b, buflen - (b-buf),
					    pre, post, mode);
					if (v == 0)
						oflow = 0;
					else
						while (*b)
							++b;
				}
				if (*n == 0)
					--n;
				break;
			default:
				break;
			}
		++n;
	}
	APPEND(0);
	--recurse;
	if (oflow || plus > 1)
		return(NULL);
	return(buf);
}

/*
 * expand all variables in ``value'', placing the result in ``res''
 * which is ``len'' bytes.
 * return a pointer to the result if all goes well.
 */
static char *
expand(char *value, char *res, int len)
{
	char *ret = res;
	char *last = res + len - 1;
	char *n;
	GenSym_t *gs;

	while (*value && res < last) {
		if (*value == OPN) {
			n = ++value;
			while (*value && *value != CLS)
				++value;
			if (*value != CLS)
				return(0);
			gs = GenSym(&variables, n, value - n, 0, 0, 0);
			if (gs == NULL)
				return(0);
			if (gs->gs_value) {
				if (expand((char *)gs->gs_value, res,
				    last - res) == NULL)
					return(0);
				while (*res)
					++res;
			}
		} else
			*res++ = *value;
		++value;
	}
	if (res < ret + len - 1) {
		*res = 0;
		return(ret);
	}
	return(0);
}

/*
 * convert character to phone digit
 * Returns pointer to string which the character maps to.
 * pre and post arse used when converting & and $ during variable expansion.
 * The mode determines why ctopd is being call:
 *
 *	NM	Number expansion (from the user)
 *	VAR	Variable expansion
 *	PAT	Pattern expansion
 */
static char *
ctopd(int c, char *pre, char *post, int mode)
{
	static char cb[2];
	GenSym_t *gs;

	cb[0] = c;
	cb[1] = 0;

	switch (c) {
	case '+': case '<': case OPN:
		return(0);
	case '?':
		if (mode == PAT)
			return("[0-9]");
		return(0);
	case '[': case '-': case ']':
		if (mode != NM)
			return(cb);
		return(0);
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	case '#': case '*':
		return(cb);
	case 'a': case 'b': case 'c':
	case 'A': case 'B': case 'C':
		if (mode != VAR)
			cb[0] = '2';
		return(cb);
	case 'd': case 'e': case 'f':
	case 'D': case 'E': case 'F':
		if (mode != VAR)
			cb[0] = '3';
		return(cb);
	case 'g': case 'h': case 'i':
	case 'G': case 'H': case 'I':
		if (mode != VAR)
			cb[0] = '4';
		return(cb);
	case 'j': case 'k': case 'l':
	case 'J': case 'K': case 'L':
		if (mode != VAR)
			cb[0] = '5';
		return(cb);
	case 'm': case 'n': case 'o':
	case 'M': case 'N': case 'O':
		if (mode != VAR)
			cb[0] = '6';
		return(cb);
	case 'p': case 'r': case 's':
	case 'P': case 'R': case 'S':
		if (mode != VAR)
			cb[0] = '7';
		return(cb);
	case 't': case 'u': case 'v':
	case 'T': case 'U': case 'V':
		if (mode != VAR)
			cb[0] = '8';
		return(cb);
	case 'w': case 'x': case 'y':
	case 'W': case 'X': case 'Y':
		if (mode != VAR)
			cb[0] = '9';
		return(cb);
	case '&':
		if (mode == VAR)
			return(pre);
		if (mode == NM)
			goto special;
		return(0);
	case '$':
		if (mode == VAR)
			return(post);
		if (mode == NM)
			goto special;
		return(cb);
	default:
		if (mode != NM)
			return(0);
	special:
		gs = GenSym(&specials, cb, 1, 0, 0, 0);
		return(gs ? (char *)gs->gs_value : 0);
	}
}

/*
 * remove separators (white space) from a line
 */
char *
desep(char *buf)
{
	char *n;
	char *b;

	b = n = buf;
	while (*b) {
		if (!issep(*b))
			*n++ = *b;
		++b;
	}
	*n = 0;
	return(*buf ? buf : 0);
}
