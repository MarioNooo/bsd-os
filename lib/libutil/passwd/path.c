/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI path.c,v 2.2 1997/08/08 18:38:27 prb Exp
 */

#include <db.h>
#include <err.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libpasswd.h"

/*
 * Construct a string holding the given path name, with directory
 * prefix stripped if appropriate, and with the given suffix appended.
 * We special case the empty suffix (which cannot fail).
 */
char *
pwd_path(pw, name, suf, pathp)
	struct pwinfo *pw;
	char *name, *suf, **pathp;
{
	char *p;
	size_t ln, ls;
	static char errmsg[1024];

	*pathp = NULL;
	if (pw->pw_flags & PW_STRIPDIR) {
		if ((p = strrchr(name, '/')) != NULL)
			name = p + 1;
	}
	if (suf == NULL || (ls = strlen(suf)) == 0) {
		*pathp = name;
		return (NULL);
	}
	ln = strlen(name);
	p = malloc(ln + ls + 1);
	if (p == NULL) {
		snprintf(errmsg, sizeof(errmsg),
		    "cannot save name %s%s: %s", name, suf, strerror(errno));
		return (errmsg);
	}
	memmove(p, name, ln);
	memmove(p + ln, suf, ls + 1);
	*pathp = p;
	return (NULL);
}
