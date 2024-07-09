/* 
 * Copyright (c) 1994
 * Berkeley Software Design Inc.  All rights reserved.
 *
 *	BSDI template.c,v 2.8 2003/06/02 20:27:11 chrisk Exp
 */

#include <sys/param.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "template.h"

struct templateinfo templateinfo;

#define	MAXARGS	40

/*
 * Information passed between template() and token().
 */
struct tokinfo {
	/* these are simply passed on as given to template() */
	const struct template *templates;
	int	ntemplates;

	/* token() fills these values.  note, argc is in templateinfo */
	char	*tend;		/* just past end of token (& args if any) */
	struct	template *tt;	/* which template function to call */
	const char *av[MAXARGS+1]; /* arg vector space */
};

static int	tcompare __P((const void *, const void *));
static int	token __P((char *, struct tokinfo *));

int
template(fname, initf, flags, templates, ntemplates)
	const char *fname;
	int (*initf) __P((FILE *));
	int flags;
	const struct template *templates;
	int ntemplates;
{
	register char *cp, *lptr, *ofname;
	register FILE *ifp, *ofp;
	register int lineno;
	size_t l;
	struct templateinfo oldti;
	struct tokinfo tokinfo;
	char _ifname[200], *ifname;
	char line[LINE_MAX];
	const char *source = check("SYS");

	(void)sprintf(_ifname, "%s.%s", fname, require("ARCH"));
	ifname = expand(_ifname, 0);
	if ((ifp = fopen(ifname, "r")) == NULL) {
		/*
		 * If we failed to open the file in our current
		 * directory and we have specified the head of
		 * the system tree with "systree" (source != NULL)
		 * then look in $SYS/<ARCH>/conf/file.<ARCH>
		 * If we did not specifiy "systree" then we will
		 * just try to re-open the file that does not exist
		 * and then print the error.
		 */
		if (source != NULL) {
			(void)sprintf(_ifname, "%s/%s/conf/%s.%s",
			    source, require("ARCH"),
			    fname, require("ARCH"));
			ifname = expand(_ifname, 0);
		}
		if ((ifp = fopen(ifname, "r")) == NULL) {
			if (flags & T_OPTIONAL)
				return (0);
			(void)fprintf(stderr, "config: cannot read %s: %s\n",
			    ifname, strerror(errno));
			return (1);
		}
	}
	ofname = path(fname);
	if ((ofp = fopen(ofname, "w")) == NULL) {
		(void)fprintf(stderr, "config: cannot write %s: %s\n",
		    ofname, strerror(errno));
		free(ofname);
		(void)fclose(ifp);
		return (1);
	}
	oldti = templateinfo;
	templateinfo.ti_ifname = ifname;
	templateinfo.ti_lineno = lineno = 0;
	templateinfo.ti_argv = tokinfo.av;
	if ((*initf)(ofp) < 0)
		goto wrerror;
	tokinfo.templates = templates;
	tokinfo.ntemplates = ntemplates;
	while (fgets(line, sizeof(line), ifp) != NULL) {
		lineno++;
		lptr = line;
		if (flags & T_SIMPLE) {
			/*
			 * Simple (makefile-style) parse.  One token,
			 * at front of line, followed by end of line.
			 */
			if (*lptr != '%')
				goto copy;
			templateinfo.ti_lineno = lineno;
			if (token(lptr, &tokinfo))
				continue;
			cp = tokinfo.tend;
			if (*cp && *cp != '\n') {
				l = strlen(cp);
				if (l > 0 && cp[--l] == '\n')
					cp[l] = 0;
				xerror(ifname, lineno,
				    "garbage after %%%s: %s",
				    tokinfo.tt->t_name, cp);
			}
			if ((*tokinfo.tt->t_fn)(ofp))
				goto wrerror;
			continue;	/* nothing else to print */
		} else {
			/*
			 * Expand possibly-many embedded % escapes.
			 * First do a quick test to see if there are any.
			 */
			cp = strchr(line, '%');
			if (cp == NULL)
				goto copy;
			templateinfo.ti_lineno = lineno;
			for (lptr = line; cp != NULL; cp = strchr(cp, '%')) {
				l = cp - lptr;
				if (l > 0 && fwrite(lptr, 1, l, ofp) != l)
					goto wrerror;
				if (token(cp, &tokinfo) == 0 &&
				    (*tokinfo.tt->t_fn)(ofp))
					goto wrerror;
				cp = tokinfo.tend;
				lptr = cp;
			}
			/* Fall through to copy trailing text. */
		}
copy:
		if (fputs(lptr, ofp) < 0)
			goto wrerror;
	}
	if (ferror(ifp)) {
		(void)fprintf(stderr,
		    "config: error reading %s (at line %d): %s\n",
		    ifname, lineno, strerror(errno));
		goto bad;
	}
	if (fclose(ofp)) {
		ofp = NULL;
		goto wrerror;
	}
	(void)fclose(ifp);
	free(ofname);
	templateinfo = oldti;
	return (0);
wrerror:
	(void)fprintf(stderr, "config: error writing %s: %s\n",
	    ofname, strerror(errno));
bad:
	if (ofp != NULL)
		(void)fclose(ofp);
	/* (void)unlink(ofname); */
	free(ofname);
	templateinfo = oldti;
	return (1);
}

/*
 * Compare template names via strcmp.
 * (maybe use strcasecmp?)
 */
static int
tcompare(key, t0)
	const void *key, *t0;
{
	register const struct template *t = t0;

	return (strcmp((const char *)key, t->t_name));
}

/*
 * %-escape token parsing.
 *
 * Crude.
 */
static int
token(cp, ip)
	register char *cp;
	register struct tokinfo *ip;
{
	register char *tstart, *copyp;
	register struct template *tt;
	register struct templateinfo *ti;
	register char ch;
	int ac;
	char argspace[LINE_MAX];

	/* whitespace, not including newline */
#define	WHITE(c) ((c) == ' ' || (c) == '\t')

	ti = &templateinfo;

	/*
	 * Save token start (pointer to % char marking token).
	 * Token is text up to non-alpha, non-[-_] char.
	 */
	tstart = cp++;
	while (isalnum((u_char)*cp) || *cp == '-' || *cp == '_')
		cp++;
	ip->tend = cp;		/* until further notice */

	/*
	 * Look up the template function based on the token.
	 * Note that the table does NOT include the '%'.
	 * Return error if not found; otherwise check to see if
	 * we parse arguments as well.
	 */
	ch = *cp;
	*cp = '\0';
	tt = bsearch(tstart + 1, ip->templates, ip->ntemplates,
	    sizeof *ip->templates, tcompare);
	if (tt == NULL) {
		xerror(ti->ti_ifname, ti->ti_lineno,
		    "unknown escape %s", tstart);
		*cp = ch;
		return (1);
	}
	*cp = ch;
	ip->tt = tt;
	ti->ti_argc = 0;
	ip->av[0] = NULL;
	if (tt->t_maxargs == 0)
		return (0);

	/*
	 * Skip white space, find open paren.  Args are non-white stuff,
	 * separated by commas, between parens.
	 */
	while (WHITE(*cp))
		cp++;
	if (*cp++ != '(') {
		if (tt->t_minargs == 0)
			return (0);
		xerror(ti->ti_ifname, ti->ti_lineno,
		    "%%%s needs %s%d argument", tt->t_name,
		    tt->t_maxargs > tt->t_minargs ? "at least " : "",
		    tt->t_minargs, tt->t_minargs == 1 ? "" : "s");
		return (1);
	}
	ac = 0;
	do {
		/* copy args, compressing out whitespace (i.e., cheating) */
		tstart = copyp = argspace;
		while ((ch = *cp++) != ',' && ch != ')') {
			if (ch == '\0') {
				xerror(ti->ti_ifname, ti->ti_lineno,
				    "%%%s: unclosed `('", tt->t_name);
				ip->tend = cp - 1;
				return (1);
			}
			if (!WHITE(ch))
				*copyp++ = ch;
		}
		*copyp++ = '\0';
		if (ac < MAXARGS)
			ip->av[ac++] = intern(argspace);
	} while (ch != ')');
	ip->tend = cp;
	if (ac >= MAXARGS) {
		xerror(ti->ti_ifname, ti->ti_lineno,
		    "%%%s: too many args", tt->t_name);
		return (1);
	}
	if (ac < tt->t_minargs || ac > tt->t_maxargs) {
		if (tt->t_minargs == tt->t_maxargs)
			xerror(ti->ti_ifname, ti->ti_lineno,
			    "%%%s: wrong number of arguments (need %d)",
			    tt->t_name, tt->t_minargs);
		else
			xerror(ti->ti_ifname, ti->ti_lineno,
	    "%%%s: wrong number of arguments (must be between %d and %d)",
			    tt->t_name, tt->t_minargs, tt->t_maxargs);
		return (1);
	}
	ip->av[ac] = NULL;	/* just like UNIX :-) */
	ti->ti_argc = ac;
	return (0);
}
