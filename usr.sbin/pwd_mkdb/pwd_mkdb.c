/*	BSDI pwd_mkdb.c,v 2.4 2001/01/22 23:19:31 geertj Exp	*/

/*-
 * Copyright (c) 1991, 1993, 1994
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
static char copyright[] =
"@(#) Copyright (c) 1991, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)pwd_mkdb.c	8.5 (Berkeley) 4/20/94";
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/wait.h>

#include <err.h>
#include <pwd.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <db.h>

#include <libpasswd.h>

void	usage __P((void));

char *usagestring = "pwd_mkdb [-dlpr] [-c cachesize] file";
char *opts = "c:dlprv";

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct pwinfo pw;

	int ch;
    	char *p;
    	u_int cachesize = 2048;
    	int flags = PW_WARNROOT;
    	char *newmaster;

	while ((ch = getopt(argc, argv, opts)) != EOF)
		switch (ch) {
		case 'c':
			cachesize = strtoul(optarg, &p, 0);
			if (cachesize == 0)
				usage();
		    	break;
		case 'r':
			flags &= ~PW_WARNROOT;
			break;
		case 'd':
			flags |= PW_STRIPDIR;
			break;
		case 'l':
			flags |= PW_NOLINK;
			break;
		case 'p':
			flags |= PW_MAKEOLD;
			break;
		case 'v':		/* Backwards compat */
			break;
		case '?':
		default:
			usage();
		}
	
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	newmaster = argv[0];

    	pw_init(&pw, flags);
	pw_sigs(NULL, NULL);
    	pw_unlimit();

	/*
	 * XXX pw_lock ignores PW_STRIPDIR and hence breaks in
	 * non-root mode. The correct fix is to rebuild the library. Later..
	 */
	if ((flags & PW_STRIPDIR) == 0) {
		if (pw_lock(&pw, O_NONBLOCK) < 0) {
	    		if (errno == EWOULDBLOCK)
				errx(1, "Password file already locked");
			err(1, "Locking password file");
		}
    	}

	if ((p = pwd_rebuild(&pw, newmaster, cachesize)) != NULL) {
		warnx("%s", p);
		pw_abort(&pw);
		pw_unlock(&pw);
		errx(1, "%s: could not build passwd file", newmaster);
	}

    	if ((p = pwd_install(&pw)) != NULL) {
		warnx("%s", p);
		err(1, "failed to install %s", newmaster);
	}

	exit(0);
}

void
usage()
{

	(void)fprintf(stderr, "%s\n", usagestring);
	exit(1);
}
