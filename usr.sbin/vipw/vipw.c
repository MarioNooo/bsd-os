/*	BSDI vipw.c,v 2.5 1998/04/26 21:27:11 prb Exp	*/

/*
 * Copyright (c) 1987, 1993, 1994
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
"@(#) Copyright (c) 1987, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)vipw.c	8.3 (Berkeley) 4/2/94";
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

int	copyfile __P((int, int));
void	usage __P((void));
int	pw_prompt __P((void));
int	pw_edit __P((char *));
void	cleanup __P((int));
int	checksum __P((char *, u_long *));

#define	PWE_WRITE	1	/* Failed on a write */
#define	PWE_READ	2	/* Failed on a read */
#define	PWE_EDIT	3	/* Failed on edit */

struct pwinfo pw;
char tmppath[MAXPATHLEN] = { 0 };

char *usagestring = "usage: vipw";
char *opts = "";

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int pfd, tfd, e;
    	sigset_t sigs;
    	struct sigaction act;
	u_long cksum, ncksum;
	int ch;
    	char *p;
    	u_int cachesize = 2048;
    	int flags = PW_WARNROOT | PW_MAKEOLD;

	while ((ch = getopt(argc, argv, opts)) != EOF)
		switch (ch) {
		case '?':
		default:
			usage();
		}
	
	argc -= optind;
	argv += optind;

	if (argc != 0)
		usage();

    	pw_init(&pw, flags);

    	act.sa_handler = cleanup;
    	act.sa_flags = 0;
	pw_sigs(&sigs, &act);

    	pw_unlimit();
    	if ((pfd = pw_lock(&pw, O_NONBLOCK)) < 0) {
	    	if (errno == EWOULDBLOCK)
			errx(1, "password file already locked");
		err(1, "locking password file");
    	}

	tfd = pwd_tmp(&pw, tmppath, MAXPATHLEN, &p);
	if (p)
		errx(1, "%s", p);
	(void)fchmod(tfd, PW_PERM_SECURE);
	if (e = copyfile(pfd, tfd)) {
		pw_abort(&pw);
		switch (e) {
		case PWE_WRITE:
			err(1, "%s", _PATH_MASTERPASSWD);
		case PWE_READ:
			err(1, "%s", tmppath);
		default:
			err(1, "unknown error");
		}
	}
	(void)close(tfd);

	for (;;) {
		if (checksum(tmppath, &cksum) < 0) {
			pw_abort(&pw);
			pw_unlock(&pw);
			errx(1, "checksum computation failed on %s", tmppath);
		}
		if (pw_edit(tmppath)) {
			pw_abort(&pw);
			pw_unlock(&pw);
			errx(1, "editor failed on %s", tmppath);
		}
		if (checksum(tmppath, &ncksum) < 0) {
			pw_abort(&pw);
			pw_unlock(&pw);
			errx(1, "checksum recomputation failed on %s", tmppath);
		}
		if (cksum == ncksum) {
			warnx("no changes made");
			pw_abort(&pw);
			pw_unlock(&pw);
			unlink(tmppath);
			exit(0);
		}

		if ((p = pwd_rebuild(&pw, tmppath, cachesize)) == NULL)
			break;
		warnx("%s", p);
		pw_abort(&pw);
		if (pw_prompt()) {
			warnx("no changes made");
			pw_abort(&pw);
			pw_unlock(&pw);
			unlink(tmppath);
			exit(0);
		}
	}
	(void)sigprocmask(SIG_BLOCK, &sigs, NULL);
    	if ((p = pwd_install(&pw)) != NULL) {
		warnx("%s", p);
		err(1, "failed to install %s", tmppath);
	}

	exit(0);
}

void
cleanup(sig)
	int sig;
{

	pw_abort(&pw);
	pw_unlock(&pw);
    	if (tmppath[0])
		unlink(tmppath);
	errx(1, "signal: %s: no changes made", (unsigned)sig >= NSIG ?
	    "(unknown)" : sys_siglist[sig]);
}

int
copyfile(from, to)
	int from, to;
{
	int nr, nw, off;
	char buf[8*1024];
	
	while ((nr = read(from, buf, sizeof(buf))) > 0)
		for (off = 0; off < nr; nr -= nw, off += nw)
			if ((nw = write(to, buf + off, nr)) < 0) {
				return(PWE_WRITE);
			}
	if (nr < 0)
		return(PWE_READ);
    	return(0);
}

int
checksum(file, cval)
	char *file;
	u_long *cval;
{
	int fd;
	u_long clen;

	if ((fd = open(file, O_RDONLY)) < 0)
		return (-1);
	if (crc(fd, cval, &clen) < 0) {
		close(fd);
		return (-1);
	}
	close(fd);
	return (0);
}

void
usage()
{

	(void)fprintf(stderr, "%s\n", usagestring);
	exit(1);
}
