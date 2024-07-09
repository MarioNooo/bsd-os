/*	BSDI who.c,v 2.3 1998/09/22 05:49:12 torek Exp	*/

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Michael Fischbein.
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
static char copyright[] =
"@(#) Copyright (c) 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)who.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>

#include <err.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utmp.h>

FILE	*file __P((char *));
void	 output __P((struct utmp *));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct utmp usr;
	struct passwd *pw;
	FILE *ufp;
	char *p, *t;

	switch (argc) {
	case 1:					/* who */
		ufp = file(_PATH_UTMP);
		fseek(ufp, sizeof (usr), SEEK_SET);

		/* only entries with both name and line fields */
		while (fread(&usr, sizeof(usr), 1, ufp) == 1)
			if (*usr.ut_name && *usr.ut_line)
				output(&usr);
		break;
	case 2:					/* who utmp_file */
		ufp = file(argv[1]);
		fseek(ufp, sizeof (usr), SEEK_SET);

		/* all entries */
		while (fread(&usr, sizeof(usr), 1, ufp) == 1)
			output(&usr);
		break;
	case 3:					/* who am i */
		ufp = file(_PATH_UTMP);
		fseek(ufp, sizeof (usr), SEEK_SET);

		/* search through the utmp and find an entry for this tty */
		if ((p = ttyname(STDIN_FILENO)) != NULL) {
			/* strip any directory component */
			if ((t = strrchr(p, '/')) != NULL)
				p = t + 1;
			while (fread(&usr, sizeof(usr), 1, ufp) == 1)
				if (*usr.ut_name && !strcmp(usr.ut_line, p)) {
					output(&usr);
					exit(0);
				}

			/* well, at least we know what the tty is */
			(void)strncpy(usr.ut_line, p, UT_LINESIZE);
		} else
			(void)strcpy(usr.ut_line, "tty??");
		pw = getpwuid(getuid());
		(void)strncpy(usr.ut_name, pw ? pw->pw_name : "?", UT_NAMESIZE);
		usr.ut_time = time(NULL);
		*usr.ut_host = '\0';
		output(&usr);
		break;
	default:
		(void)fprintf(stderr, "usage: who [ file ]\n       who am i\n");
		exit(1);
	}
	exit(0);
}

void
output(up)
	struct utmp *up;
{
	time_t tmp;

	(void)printf("%-*.*s %-*.*s", UT_NAMESIZE, UT_NAMESIZE, up->ut_name,
	    UT_LINESIZE, UT_LINESIZE, up->ut_line);
	tmp = up->ut_time;
	(void)printf("%.12s", ctime(&tmp) + 4);
	if (*up->ut_host)
		printf("\t(%.*s)", UT_HOSTSIZE, up->ut_host);
	(void)putchar('\n');
}

FILE *
file(name)
	char *name;
{
	FILE *ufp;

	if ((ufp = fopen(name, "r")) == NULL)
		err(1, "%s", name);
	return(ufp);
}
