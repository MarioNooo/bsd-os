/*	BSDI cmds.c,v 2.5 1999/10/13 22:07:54 jch Exp	*/

/*-
 * Copyright (c) 1980, 1992, 1993
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
static char sccsid[] = "@(#)cmds.c	8.2 (Berkeley) 4/29/95";
#endif /* not lint */

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include "systat.h"
#include "extern.h"

static int init __P((struct cmdtab *));

void
command(cmd)
        char *cmd;
{
        register struct cmdtab *p;
        register char *cp;
	char *err;
	int interval, omask;

	omask = sigblock(sigmask(SIGALRM));
        for (cp = cmd; *cp && !isspace(*cp); cp++)
                ;
        if (*cp)
                *cp++ = '\0';
	if (*cmd == '\0')
		return;
	for (; *cp && isspace(*cp); cp++)
		;
        if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "q") == 0)
                die(0);
	if (strcmp(cmd, "load") == 0) {
		load();
		goto done;
	}
        if (strcmp(cmd, "stop") == 0) {
                alarm(0);
                mvaddstr(CMDLINE, 0, "Refresh disabled.");
                clrtoeol();
		goto done;
        }
	if (strcmp(cmd, "help") == 0) {
		int col, len;

		move(CMDLINE, col = 0);
		p = curcmd;
		if (p != NULL && p->c_subhelp != NULL) {
			addstr(p->c_subhelp);
			addstr("; ");
			col = strlen(curcmd->c_subhelp) + 2;
		}
		for (p = cmdtab; p->c_name; p++) {
			len = strlen(p->c_name);
			if (col + len > COLS)
				break;
			addstr(p->c_name);
			col += len;
			if (++col < COLS)
				addch(' ');
		}
		clrtoeol();
		goto done;
	}
	interval = atoi(cmd);
        if (interval <= 0 &&
	    (strcmp(cmd, "start") == 0 || strcmp(cmd, "interval") == 0)) {
		interval = *cp ? atoi(cp) : naptime;
                if (interval <= 0) {
			error("%d: bad interval.", interval);
			goto done;
                }
	}
	if (interval > 0) {
                alarm(0);
                naptime = interval;
                display(0);
                status();
		goto done;
        }
	p = lookup(cmd, &err);
	if (p == NULL && *err == 'A') {
		error("%s: %s", cmd, err);
		goto done;
	}
        if (p) {
                if (curcmd == p)
			goto done;
                alarm(0);
		(*curcmd->c_close)(wnd);
		wnd = (*p->c_open)();
		if (wnd == 0 || !init(p)) {
			error("Couldn't open new display");
			p = curcmd;
			wnd = (*p->c_open)();
			if (wnd == 0) {
				error("Couldn't change back to previous cmd");
				exit(1);
			}
		}
                curcmd = p;
		labels();
                display(0);
                status();
		goto done;
        }
	if (curcmd->c_cmd == NULL || !(*curcmd->c_cmd)(cmd, cp))
		error("%s: Unknown command.", cmd);
done:
	sigsetmask(omask);
	return;
}

static int
init(p)
	struct cmdtab *p;
{

	if (p->c_flags & CF_INIT)
		return (1);
	if ((*p->c_init)()) {
		p->c_flags |= CF_INIT;
		return (1);
	}
	return (0);
}

struct cmdtab *
lookup(name, errp)
	register char *name;
	char **errp;
{
	register char *p, *q;
	register struct cmdtab *c, *found;
	register int nmatches, longest;

	longest = 0;
	nmatches = 0;
	found = NULL;
	for (c = cmdtab; (p = c->c_name); c++) {
		for (q = name; *q == *p++; q++)
			if (*q == 0)		/* exact match? */
				return (c);
		if (!*q) {			/* the name was a prefix */
			if (q - name > longest) {
				longest = q - name;
				nmatches = 1;
				found = c;
			} else if (q - name == longest)
				nmatches++;
		}
	}
	if (nmatches > 1) {
		*errp = "Ambiguous.";
		return (NULL);
	}
	if (found == NULL)
		*errp = "Unknown command.";
	return (found);
}

void
resize(signo)
	int signo;
{
	endwin();
	if (wload != NULL) {
		delwin(wload);
		wload = NULL;
	}
	alarm(0);
	wclear(wnd);
	(*curcmd->c_close)(wnd);
	if (initscr() == NULL) {
		fprintf(stderr,
		    "Unable to reinit screen\n");
		exit(1);
	}
	wnd = (*curcmd->c_open)();
	if (wnd == 0) {
		error("Couldn't open new display");
		exit(1);
	}
	CMDLINE = LINES - 1;
#ifdef	NCURSES_VERSION
	getmaxyx(wnd, maxy, maxx);
#else
	maxx = wnd->maxx;
	maxy = wnd->maxy;
#endif
	labels();
	display(0);
	noecho();
	crmode();
}

void
status()
{

        error("Showing %s, refresh every %d seconds.",
          curcmd->c_name, naptime);
}

int
prefix(s1, s2)
        register char *s1, *s2;
{

        while (*s1 == *s2) {
                if (*s1 == '\0')
                        return (1);
                s1++, s2++;
        }
        return (*s1 == '\0');
}