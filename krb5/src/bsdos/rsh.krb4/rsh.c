/*	BSDI	rsh.c,v 2.7 1997/11/19 12:42:59 jch Exp	*/

/*-
 * Copyright (c) 1983, 1990, 1993, 1994
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
"@(#) Copyright (c) 1983, 1990, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)rsh.c	8.4 (Berkeley) 4/29/95";
#endif /* not lint */

/*
 * /master/usr.bin/rsh/rsh.c,v
 * /master/usr.bin/rsh/rsh.c,v 2.7 1997/11/19 12:42:59 jch Exp
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pwd.h>
#include <signal.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <varargs.h>

#include "pathnames.h"

#ifdef KERBEROS
#include <kerberosIV/krb.h>

#include "krb.h"

#ifdef	CRYPT
CREDENTIALS cred;
Key_schedule schedule;
int doencrypt;
#endif
#endif

/*
 * rsh - remote shell
 */
static int	rfd2;
static pid_t	pid;

static char    *copyargs __P((char **));
static void	sendsig __P((int));
static void	termsig __P((int));
static void	talk __P((int, long, pid_t, int *));
static void	usage __P((void));
static void	warning __P(());

int
main(argc, argv)
	int argc;
	char **argv;
{
	struct passwd *pw;
	struct servent *sp;
	void (*catcher)(int);
	long omask;
	int asrsh, ch, one, rem;
	int dflag, nflag, xflag, Sflag, Nflag, Kflag;
	uid_t uid;
	char *args, *host, *p, *user, *kflag;

	asrsh = dflag = nflag = Sflag = Nflag = Kflag = xflag = 0;
	one = 1;
	host = user = kflag = NULL;
	catcher = sendsig;

	/* if called as something other than "rsh", use it as the host name */
	if ((p = strrchr(argv[0], '/')) != NULL)
		++p;
	else
		p = argv[0];
	if (strcmp(p, "rsh"))
		host = p;
	else
		asrsh = 1;

	/* handle "rsh host flags" */
	if (!host && argc > 2 && argv[1][0] != '-') {
		host = argv[1];
		optind++;
	}

#define	OPTIONS	"8KLNSdek:l:nwx"
	while ((ch = getopt(argc, argv, OPTIONS)) != EOF)
		switch(ch) {
		case 'K':
			Kflag = 1;
			break;
		case 'N':
			Nflag = 1;
			break;
		case 'S':
			Sflag = 1;
			catcher = termsig;
			break;
		case 'L':	/* -8Lew are ignored to allow rlogin aliases */
		case 'e':
		case 'w':
		case '8':
			break;
		case 'd':
			dflag = 1;
			break;
		case 'l':
			user = optarg;
			break;
		case 'k':
			kflag = optarg;
			break;
		case 'n':
			nflag = 1;
			break;
		case 'x':
			/* It just doesn't work */
			errx(1, "Encryption not supported");
			xflag = 1;
#if	defined(KERBEROS) && defined(CRYPT)
			des_set_key(cred.session, schedule);
#endif
			break;
		case '?':
		default:
			usage();
		}

	/* if haven't gotten a host yet, do so */
	if (!host && !(host = argv[optind++]))
		usage();

#ifndef	CRYPT
	if (xflag)
		errx(1, "Encryption is not available");
#endif	/* CRYPT */
#ifdef	KERBEROS
	if (Kflag) {
		if (Nflag)
			errx(1, "-N and -K flags conflict");
		if (kflag != NULL)
			errx(1, "-k flag and -K flags conflict");
#ifdef	CRYPT
		if (xflag)
			errx(1, "-x flag and -K flags conflict");
#endif	/* CRYPT */
	} else if (krb_configured() == KFAILURE) {
		if (Nflag || xflag)
			errx(1, "Kerberos is not configured");
		Kflag = 1;
	}
#else	/* KERBEROS */
	if (Nflag || xflag || kflag != NULL)
		errx(1, "Kerberos is not available");
#endif	/* KERBEROS */

	/* if no further arguments, must have been called as rlogin. */
	if (!argv[optind]) {
		if (asrsh)
			*argv = "rlogin";
		execv(_PATH_RLOGIN, argv);
		err(1, "can't exec %s", _PATH_RLOGIN);
	}

	argc -= optind;
	argv += optind;

	if (!(pw = getpwuid(uid = getuid())))
		errx(1, "unknown user id");
	/* Accept user1@host format, though "-l user2" overrides user1 */
	p = strchr(host, '@');
	if (p) {
		*p = '\0';
		if (!user && p > host)
			user = host;
		host = p + 1;
		if (*host == '\0')
			usage();
	}
	if (!user)
		user = pw->pw_name;

#if	defined(KERBEROS) && defined(CRYPT)
	/* -x turns off -n */
	if (xflag)
		nflag = 0;
#endif

	args = copyargs(argv);

	rem = -1;
#ifdef KERBEROS
	if (!Kflag) {
		char *msg, *shell;

		shell = xflag ? "ekshell" : "kshell";

		sp = getservbyname(shell, "tcp");
		if (sp == NULL) {
			if (xflag || Nflag)
				errx(1, "%s/tcp: unknown service", shell);
			warning("unable to get entry for %s/tcp service", shell);
		} else {

			errno = 0;
#ifdef CRYPT
			if (xflag) {
				rem = krcmd_mutual(&host, sp->s_port, user,
				    args, Sflag ? NULL : &rfd2, kflag, &cred,
				    schedule);
				if (rem >= 0)
					doencrypt = 1;
			} else
#endif
				rem = krcmd(&host, sp->s_port, user, args,
				    Sflag ? NULL : &rfd2, kflag);

			if (rem < 0) {
				switch (errno) {
				case ECONNREFUSED:
					msg = "remote host does not support Kerberos";
					break;
				case ENOENT:
					msg = "unable to provide Kerberos auth data";
				default:
					msg = NULL;
				}
				if (Nflag || xflag) {
					if (msg != NULL)
						errx(1, msg);
					exit(1);
				} else {
					if (msg != NULL)
						warning(msg);
				}
			}
		}
	}
#endif	/* KERBEROS */
	if (rem < 0) {
		sp = getservbyname("shell", "tcp");
		if (sp == NULL)
			errx(1, "shell/tcp: unknown service");
		rem = rcmd(&host, sp->s_port, pw->pw_name, user, args,
		    Sflag ? NULL : &rfd2);
	}

	if (rem < 0)
		exit(1);

	if (!Sflag) {
		if (rfd2 < 0)
			errx(1, "can't establish stderr");
	} else
		rfd2 = -1;

	if (dflag) {
		if (setsockopt(rem, SOL_SOCKET, SO_DEBUG, &one,
		    sizeof(one)) < 0)
			warn("setsockopt");
		if (rfd2 != -1 && setsockopt(rfd2, SOL_SOCKET, SO_DEBUG,
		    &one, sizeof(one)) < 0)
			warn("setsockopt");
	}

	(void)setuid(uid);
	omask = sigblock(sigmask(SIGINT)|sigmask(SIGQUIT)|sigmask(SIGTERM));
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void)signal(SIGINT, catcher);
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		(void)signal(SIGQUIT, catcher);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		(void)signal(SIGTERM, catcher);

	if (!nflag) {
		pid = fork();
		if (pid < 0)
			err(1, "fork");
	}

#if	defined(KERBEROS) && defined(CRYPT)
	if (!doencrypt)
#endif
	{
		if (rfd2 != -1)
			(void)ioctl(rfd2, FIONBIO, &one);
		(void)ioctl(rem, FIONBIO, &one);
	}

	talk(nflag, omask, pid, &rem);

	if (!nflag)
		(void)kill(pid, SIGKILL);
	exit(0);
}

static void
talk(nflag, omask, pid, remp)
	int nflag;
	long omask;
	pid_t pid;
	int *remp;
{
	int cc, rem, wc;
	fd_set readfrom, ready, rembits;
	char *bp, buf[BUFSIZ];

	rem = *remp;

	if (!nflag && pid == 0) {
		if (rfd2 != -1)
			(void)close(rfd2);

reread:		errno = 0;
		if ((cc = read(0, buf, sizeof buf)) <= 0)
			goto done;
		bp = buf;

rewrite:	
		FD_ZERO(&rembits);
		FD_SET(rem, &rembits);
		if (select(rem + 1, 0, &rembits, 0, 0) < 0) {
			if (errno != EINTR)
				err(1, "select");
			goto rewrite;
		}
		if (!FD_ISSET(rem, &rembits))
			goto rewrite;
#if	defined(KERBEROS) && defined(CRYPT)
		if (doencrypt)
			wc = des_write(rem, bp, cc);
		else
#endif
			wc = write(rem, bp, cc);
		if (wc < 0) {
			if (errno == EWOULDBLOCK)
				goto rewrite;
			goto done;
		}
		bp += wc;
		cc -= wc;
		if (cc == 0)
			goto reread;
		goto rewrite;
done:
		(void)shutdown(rem, 1);
		exit(0);
	}

	(void)sigsetmask(omask);
	FD_ZERO(&readfrom);
	if (rfd2 != -1)
		FD_SET(rfd2, &readfrom);
	FD_SET(rem, &readfrom);
	do {
		ready = readfrom;
		if (select(MAX(rfd2, rem) + 1, &ready, 0, 0, 0) < 0) {
			if (errno != EINTR)
				err(1, "select");
			continue;
		}
		if (rfd2 != -1 && FD_ISSET(rfd2, &ready)) {
			errno = 0;
#if	defined(KERBEROS) && defined(CRYPT)
			if (doencrypt)
				cc = des_read(rfd2, buf, sizeof buf);
			else
#endif
				cc = read(rfd2, buf, sizeof buf);
			if (cc <= 0) {
				if (errno != EWOULDBLOCK) {
					FD_CLR(rfd2, &readfrom);
					(void)close(rfd2);
					rfd2 = -1;
				}
			} else
				(void)write(STDERR_FILENO, buf, cc);
		}
		if (rem != -1 && FD_ISSET(rem, &ready)) {
			errno = 0;
#if	defined(KERBEROS) && defined(CRYPT)
			if (doencrypt)
				cc = des_read(rem, buf, sizeof buf);
			else
#endif
				cc = read(rem, buf, sizeof buf);
			if (cc <= 0) {
				if (errno != EWOULDBLOCK) {
					FD_CLR(rem, &readfrom);
					(void)close(rem);
					*remp = rem = -1;
				}
			} else
				(void)write(STDOUT_FILENO, buf, cc);
		}
	} while (rfd2 != -1 || rem != -1);
}

static void
sendsig(sig)
	int sig;
{
	char signo;

	signo = sig;
#if	defined(KERBEROS) && defined(CRYPT)
	if (doencrypt)
		(void)des_write(rfd2, &signo, 1);
	else
#endif
		(void)write(rfd2, &signo, 1);
}

static void
termsig(sig)
{
	if (pid > 0)
		(void)kill(pid, SIGKILL);
	exit(0);
}

#ifdef KERBEROS
/* VARARGS */
static void
warning(va_alist)
va_dcl
{
	va_list ap;
	char *fmt;

	(void)fprintf(stderr, "rsh: warning, using standard rsh: ");
	va_start(ap);
	fmt = va_arg(ap, char *);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, ".\n");
}
#endif

static char *
copyargs(argv)
	char **argv;
{
	int cc;
	char **ap, *args, *p;

	cc = 0;
	for (ap = argv; *ap; ++ap)
		cc += strlen(*ap) + 1;
	if (!(args = malloc((u_int)cc)))
		err(1, NULL);
	for (p = args, ap = argv; *ap; ++ap) {
		(void)strcpy(p, *ap);
		for (p = strcpy(p, *ap); *p; ++p);
		if (ap[1])
			*p++ = ' ';
	}
	return (args);
}

static void
usage()
{
	char *flags;
	
#ifdef KERBEROS
	if (krb_configured() == KSUCCESS)
#ifdef CRYPT
		flags = "dKNSnx] [-k realm";
#else
		flags = "dKNSn] [-k realm";
#endif
	else
#endif
		flags = "dn";

	(void)fprintf(stderr,
	    "usage: rsh [-%s] [-l login] [login@]host [command]\n",
	    flags);
	exit(1);
}
