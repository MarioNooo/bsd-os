/*	BSDI mv.c,v 2.6 2001/05/10 14:21:26 karels Exp	*/

/*
 * Copyright (c) 1989, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ken Smith of The State University of New York at Buffalo.
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
"@(#) Copyright (c) 1989, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)mv.c	8.2 (Berkeley) 4/2/94";
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pathnames.h"

int fflg, iflg;

int	move __P((char *, char *));
void	usage __P((void));
static int run __P((char *, char *, char *, char *, char *));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct stat sb;
	int baselen, ch, len, rval;
	char *p, *endp, path[MAXPATHLEN + 1];

	while ((ch = getopt(argc, argv, "-if")) != -1)
		switch (ch) {
		case 'i':
			iflg = 1;
			break;
		case 'f':
			fflg = 1;
			break;
		case '-':		/* Undocumented; for compatibility. */
			goto endarg;
		case '?':
		default:
			usage();
		}
endarg:	argc -= optind;
	argv += optind;

	if (argc < 2)
		usage();

	/*
	 * If the stat on the target fails or the target isn't a directory,
	 * try the move.  More than 2 arguments is an error in this case.
	 */
	if (stat(argv[argc - 1], &sb) || !S_ISDIR(sb.st_mode)) {
		if (argc > 2)
			usage();
		exit(move(argv[0], argv[1]));
	}

	/* It's a directory, move each file into it. */
	(void)strcpy(path, argv[argc - 1]);
	baselen = strlen(path);
	endp = &path[baselen];
	*endp++ = '/';
	++baselen;
	for (rval = 0; --argc; ++argv) {
		char *trailing_slash = NULL;

		/*
		 * Find the last component of the source pathname.  It
		 * may have trailing slashes.
		 */
		p = *argv + strlen(*argv);
		while (p != *argv && p[-1] == '/')
			trailing_slash = --p;
		while (p != *argv && p[-1] != '/')
			--p;
		/*
		 * Zap trailing slashes on directories, as we don't
		 * allow them on the target name (which is becoming a
		 * directory, but isn't yet).  Don't ignore them
		 * on non-directories, let them cause an error.
		 */
		if (trailing_slash && stat(*argv, &sb) == 0 &&
		    S_ISDIR(sb.st_mode))
			*trailing_slash = '\0';

		if ((baselen + (len = strlen(p))) >= MAXPATHLEN) {
			warnx("%s: destination pathname too long", *argv);
			rval = 1;
		} else {
			memmove(endp, p, len + 1);
			if (move(*argv, path))
				rval = 1;
		}
	}
	exit(rval);
}

int
move(from, to)
	char *from, *to;
{
	struct stat sb;
	int ask, ch;
	char modep[15];

	/*
	 * Check access.  If interactive and file exists, ask user if it
	 * should be replaced.  Otherwise if file exists but isn't writable
	 * make sure the user wants to clobber it.
	 */
	if (!fflg && !access(to, F_OK)) {
		ask = 0;
		if (iflg) {
			(void)fprintf(stderr, "overwrite %s? ", to);
			ask = 1;
		} else if (access(to, W_OK) && !stat(to, &sb)) {
			strmode(sb.st_mode, modep);
			(void)fprintf(stderr, "override %s%s%s/%s for %s? ",
			    modep + 1, modep[9] == ' ' ? "" : " ",
			    user_from_uid(sb.st_uid, 0),
			    group_from_gid(sb.st_gid, 0), to);
			ask = 1;
		}
		if (ask) {
			if ((ch = getchar()) != EOF && ch != '\n')
				while (getchar() != '\n');
			if (ch != 'y')
				return (0);
		}
	}
	errno = 0;
	if (!rename(from, to))				/* Intra device move. */
		return (0);
	if (errno != EXDEV) {
		warn("rename %s to %s", from, to);
		return (1);
	}

	/*
	 * !!!
	 * We don't want to depend on the error test order in the kernel,
	 * and we're about to remove the target -- the dangerous case is
	 * if the target is a directory and we failed for EXDEV before we
	 * tested for EISDIR.
	 */
	if (!stat(to, &sb) && S_ISDIR(sb.st_mode)) {
		errno = EISDIR;
		warn("rename %s to %s", from, to);
		return (1);
	}

	if (run(_PATH_RM, "mv", "-rf", to, NULL))	/* Remove the target. */
		return (1);
	if (run(_PATH_CP, "mv", "-PRp", from, to))	/* Inter device move. */
		return (1);
	if (run(_PATH_RM, "mv", "-rf", from, NULL))	/* Remove the source. */
		return (1);
	return (0);
}

/*
 * run --
 *	Run a program.
 */
static int
run(program, name, flags, arg1, arg2)
	char *program, *name, *flags, *arg1, *arg2;
{
	pid_t pid;
	int status;

	switch (pid = vfork()) {
	case -1:
		warn("fork");
		return (1);
	case 0:
		execl(program, name, flags, arg1, arg2, NULL);
		warn("%s", program);
		_exit(1);
	}
	if (waitpid(pid, &status, 0) == -1) {
		warn("%s: waitpid", program);
		return (1);
	}
	if (!WIFEXITED(status)) {
		warnx("%s: did not terminate normally", program);
		return (1);
	}
	return (WEXITSTATUS(status) ? 1 : 0);
}

void
usage()
{
	(void)fprintf(stderr,
"usage: mv [-if] src target;\n   or: mv [-if] src1 ... srcN directory\n");
	exit(1);
}
