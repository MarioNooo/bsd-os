/*
 * Copyright (c) 1989, 1993
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

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)getttydesc.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <libdialer.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ttyent.h>
#include <unistd.h>

static char *ttysconf[] = { _PATH_TTYS_LOCAL, _PATH_TTYS_CONF, 0 };

static FILE *tf;
static ttydesc_t *_getttydesc __P((const char *));


ttydesc_t *
getttydescbyname(tty)
	const char *tty;
{
	register ttydesc_t *t;

	setttydesc();
	t = _getttydesc(tty);
	endttydesc();
	return (t);
}

ttydesc_t *
getttydesc()
{
	return(_getttydesc(NULL));
}

static ttydesc_t *
_getttydesc(name)
	const char *name;
{
	int magic = 0;
	static ttydesc_t tty = { 0, };
	register int c, i;
	register char *e, *p, *v, **nv;
#define	MAXLINELENGTH	100
	static char line[MAXLINELENGTH];
	static char *skip();
	cap_t *nc, *oc;

	if (!tf && !setttydesc())
		return (NULL);
	do {
		for (;;) {
			if (!fgets(p = line, sizeof(line), tf))
				return (NULL);
			/* skip lines that are too big */
			if (!index(p, '\n')) {
				while ((c = getc(tf)) != '\n' && c != EOF)
					;
				continue;
			}
			while (isspace(*p))
				++p;
			if (*p && *p != '#')
				break;
		}

		e = skip(p);
	} while (name && strcmp(p, name) != 0);

	tty.ty_name = p;
	tty.ty_getty = NULL;
	tty.ty_window = NULL;
	tty.ty_status = 0;

	if (tty.ty_cap)
		cap_free(tty.ty_cap);

	tty.ty_cap = getttycap(tty.ty_name);

#define	scmp(e)	!strncmp(p, e, sizeof(e) - 1) && isspace(p[sizeof(e) - 1])

	while (*(p = e)) {
		e = skip(p);

		switch (magic) {
		case 0:	/* processing second field */
			if (strcmp("none", p) == 0 || p[0] == '/' ||
			    (p[1] == '"' && p[1] == '/')) {
				/* we have an old style entry */
				magic = 1;
				if (cap_add(&tty.ty_cap, RESOURCE_COMMAND, p,
				    CM_NORMAL) < 0)
					return(NULL);
				break;
			}
			/** fall thru **/
		case 2: /* new style entry */
		case -1: /* new style entry */
			magic = -1;
			
			if ((v = strchr(p, '=')) != NULL) {
				*v++ = '\0';
				if (cap_add(&tty.ty_cap, p, v, CM_NORMAL) < 0)
					return(NULL);
			} else {
				nc = getttycap(p);
				if (nc == 0)
					return(NULL);
				cap_merge(&tty.ty_cap, nc, CM_NORMAL);
			}
			break;
		case 1: /* processing term type in old style entry */
			if (cap_add(&tty.ty_cap, RESOURCE_TERM, p, CM_NORMAL)<0)
				return(NULL);
			magic = 2;
			break;
		}
	}
	/*
	 * If we have an old style entry with just the command or
	 * command and terminal type, merge in the default entry
	 */
	if (magic > 0) {
		nc = getttycap("default");
		if (nc == 0)
			return(NULL);
		cap_merge(&tty.ty_cap, nc, CM_NORMAL);
	}

	/*
	 * Merge together the various stty opions (flow and modem control)
	 */
	if ((nc = cap_look(tty.ty_cap, RESOURCE_FLOWCONTROL)) && nc->value) {
		if (nc->value[0] == '\0' ||
	    	    strcasecmp(nc->value, "hw") == 0 ||
	    	    strcasecmp(nc->value, "hardware") == 0)
			cap_add(&tty.ty_cap, RESOURCE_STTYMODES,
			    VALUE_FLOWCONTROL_V, CM_UNION);
		else if (strcasecmp(nc->value, "sw") == 0 ||
	    	    strcasecmp(nc->value, "software") == 0)
			cap_add(&tty.ty_cap, RESOURCE_STTYMODES,
			    VALUE_SWFLOWCONTROL_V, CM_UNION);
		else if (strcasecmp(nc->value, "none") == 0)
			cap_add(&tty.ty_cap, RESOURCE_STTYMODES,
			    VALUE_NOFLOWCONTROL_V, CM_UNION);
	}

	if ((nc = cap_look(tty.ty_cap, "-" RESOURCE_FLOWCONTROL)) && nc->value)
		cap_add(&tty.ty_cap, RESOURCE_STTYMODES, VALUE_NOFLOWCONTROL_V,
		    CM_UNION);

	if ((nc = cap_look(tty.ty_cap, RESOURCE_MODEMCONTROL)) && nc->value)
		cap_add(&tty.ty_cap, RESOURCE_STTYMODES, VALUE_MODEMCONTROL_V,
		    CM_UNION);

	if ((nc = cap_look(tty.ty_cap, "-" RESOURCE_MODEMCONTROL)) && nc->value)
		cap_add(&tty.ty_cap, RESOURCE_STTYMODES, VALUE_NOMODEMCONTROL_V,
		    CM_UNION);

	/*
	 * smash dte and dce speeds together
	 */
	nc = cap_look(tty.ty_cap, RESOURCE_DTE);
	oc = cap_look(tty.ty_cap, RESOURCE_DCE);

	if (nc && nc->value && oc && oc->value && nc->values && oc->values) {
		for (i = 0; nc->values[i];) {
			for (nv = oc->values; *nv != NULL; ++nv)
				if (strcmp(nc->values[i], *nv) == 0)
					break;
			if (*nv == NULL)
				for (c = i; nc->values[c]; ++c)
					nc->values[c] = nc->values[c+1];
			else
				++i;
		}
		if (nc->values[0] == NULL) {
			free(nc->values);
			if (nc->value != _EMPTYVALUE)
				free(nc->value);
			nc->values = NULL;
			nc->value = NULL;
		} else if (nc->value != nc->values[0]) {
			strcpy(nc->value, nc->values[0]);
			nc->values[0] = nc->value;
		}
	}

	/*
	 * pull out various compatability flags
	 */
	if ((nc = cap_look(tty.ty_cap, RESOURCE_DIALIN)) && nc->value)
		tty.ty_status |= TD_IN;
	if ((nc = cap_look(tty.ty_cap, RESOURCE_DIALOUT)) && nc->value)
		tty.ty_status |= TD_OUT;
	if ((nc = cap_look(tty.ty_cap, RESOURCE_SECURE)) && nc->value)
		tty.ty_status |= TD_SECURE;

	/*
	 * Now keep init happy...
	 */
	if ((nc = cap_look(tty.ty_cap, RESOURCE_MANAGER)) &&
	    nc->value != NULL && strcmp(nc->value, MANAGER_INIT) == 0 &&
	    (tty.ty_status & TD_IN))
		tty.ty_status |= TD_INIT;

	if ((nc = cap_look(tty.ty_cap, RESOURCE_COMMAND)) && nc->value)
		tty.ty_getty = nc->value;
	if ((nc = cap_look(tty.ty_cap, RESOURCE_WINDOW)) && nc->value)
		tty.ty_window = nc->value;

	return (&tty);
}

#define	QUOTED	1

/*
 * Skip over the current field, removing quotes, and return a pointer to
 * the next field.
 */
static char *
skip(p)
	register char *p;
{
	register char *t;
	register int c, q;

	for (q = 0, t = p; (c = *p) != '\0'; p++) {
		if (c == '"') {
			q ^= QUOTED;	/* obscure, but nice */
			continue;
		}
		if (q == QUOTED && *p == '\\' && *(p+1) == '"')
			p++;
		*t++ = *p;
		if (q == QUOTED)
			continue;
		if (c == '#') {
			*p = 0;
			break;
		}
		if (c == '\t' || c == ' ' || c == '\n') {
			*p++ = 0;
			while ((c = *p) == '\t' || c == ' ' || c == '\n')
				p++;
			if (c == '#')
				*p = 0;
			break;
		}
	}
	*--t = '\0';
	return (p);
}

int
setttydesc()
{

	if (tf) {
		(void)rewind(tf);
		return (1);
	} else if ((tf = fopen(_PATH_TTYS, "r")) != NULL)
		return (1);
	return (0);
}

int
endttydesc()
{
	int rval;

	if (tf) {
		rval = !(fclose(tf) == EOF);
		tf = NULL;
		return (rval);
	}
	return (1);
}

#define	WARNS(s)		write(STDERR_FILENO, s, sizeof(s) - 1)

static int WARN(char *s)	{ return(write(STDERR_FILENO, s, strlen(s))); }

char *
getttycapbuf(name)
	char *name;
{
    	char *capbuf;
	extern char *__progname;
	int fd;

	switch (cgetent(&capbuf, ttysconf, name)) {
	case 0:
		break;
	case 1:
		WARN(__progname);
		WARNS(": ");
		WARN(name);
		WARNS(": couldn't resolve 'tc'\n");
		capbuf = 0;
		break;
	case -1:
		capbuf = 0;
		break;
	case -2:
		/*
		 * We don't care if the local conf file is missing.
		 * How ever, we certainly complain if the system conf file
		 * is missing.
		 */
		if ((fd = open(ttysconf[1], 0)) < 0) {
			WARN(__progname);
			WARNS(": ");
			WARN(ttysconf[1]);
			WARNS(": ");
			WARN(strerror(errno));
			WARNS("\n");
			capbuf = 0;
		} else {
			close(fd);
			capbuf = _EMPTYVALUE;
		}
		
		break;
	case -3:
		WARN(__progname);
		WARNS(": ");
		WARN(name);
		WARNS(": couldn't resolve 'tc'\n");
		WARNS(": 'tc' reference loop\n");
		capbuf = 0;
		break;
	default:
		WARN(__progname);
		WARNS(": ");
		WARN(name);
		WARNS(": unexpected cgetent error\n");
		capbuf = 0;
		break;
	}
	return(capbuf);
}

cap_t *
getttycap(name)
	char *name;
{
	char *capbuf;
	cap_t *cp = (cap_t *)_EMPTYCAP;

	if ((capbuf = getttycapbuf(name)) != NULL) {
		if (capbuf == _EMPTYVALUE)
			return(cp);
		cp = cap_make(capbuf, (cap_t *)_EMPTYCAP);
		free(capbuf);
	}
	return(cp);
}
