/*-
 * Copyright (c) 1988, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char sccsid[] = "@(#)util.c	8.4 (Berkeley) 4/2/94";
#endif /* not lint */

#include <sys/types.h>

#include <ctype.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>

#include "chpass.h"
#include "pathnames.h"

static char *months[] =
	{ "January", "February", "March", "April", "May", "June",
	  "July", "August", "September", "October", "November",
	  "December", NULL };

char *
ttoa(tval)
	time_t tval;
{
	struct tm *tp;
	static char tbuf[50];

	if (tval) {
		tp = localtime(&tval);
		(void)snprintf(tbuf, sizeof(tbuf),
		    "%s %d, %d", months[tp->tm_mon],
		    tp->tm_mday, tp->tm_year + TM_YEAR_BASE);
	} else
		*tbuf = '\0';
	return (tbuf);
} 

int
atot(p, store)
	char *p;
	time_t *store;
{
	static int first = 1;
	struct tm lt;
	char *t, **mp;

	if (*p == '\0') {
		*store = 0;
		return (0);
	}
	if (first) {
		first = 0;
		unsetenv("TZ");
	}
	if ((t = strtok(p, " \t")) == NULL)
		return (1);
	for (mp = months;; ++mp) {
		if (*mp == NULL)
			return (1);
		if (!strncasecmp(*mp, t, 3)) {
			lt.tm_mon = mp - months;
			break;
		}
	}
	if ((t = strtok(NULL, " \t,")) == NULL || !isdigit(*t))
		return (1);
	lt.tm_mday = atoi(t);
	if ((t = strtok(NULL, " \t,")) == NULL || !isdigit(*t))
		return (1);
	lt.tm_year = atoi(t);
	if (lt.tm_year > 1900)
		lt.tm_year -= 1900;

	lt.tm_sec = 0;
	lt.tm_min = 0;
	lt.tm_hour = 0;
	lt.tm_isdst = -1;
	return ((*store = mktime(&lt)) == -1);
}

char *
ok_shell(name)
	char *name;
{
	char *p, *sh;

	setusershell();
	while ((sh = getusershell()) != NULL) {
		if (!strcmp(name, sh))
			return (name);
		/* allow just shell name, but use "real" path */
		if ((p = strrchr(sh, '/')) != NULL && strcmp(name, p + 1) == 0)
			return (sh);
	}
	return (NULL);
}
