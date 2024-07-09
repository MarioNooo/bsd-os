/*
 * Copyright (c) 1988, 1990, 1993
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
"@(#) Copyright (c) 1988, 1990, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)shutdown.c	8.4 (Berkeley) 4/28/95";
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tzfile.h>
#include <unistd.h>
#include <vis.h>

#include "pathnames.h"

#ifdef DEBUG
#undef	_PATH_NOLOGIN
#define	_PATH_NOLOGIN		"./nologin"
#undef	_PATH_SHUTDOWNPID
#define	_PATH_SHUTDOWNPID	"./shutdown.pid"
#endif

#define	H		* SECSPERHOUR
#define	M		* SECSPERMIN
#define	S		* 1
#define	NOLOG_TIME	5 * SECSPERMIN
struct interval {
	time_t timeleft, timetowait;
} tlist[] = {
	{ 10 H,  5 H },
	{ 5 H,  3 H },
	{ 2 H,  1 H },
	{ 1 H, 30 M },
	{ 30 M, 10 M },
	{ 20 M, 10 M },
	{ 10 M,  5 M },
	{ 5 M,  3 M },
	{ 2 M,  1 M },
	{ 1 M, 30 S },
	{ 30 S, 30 S },
	{ 0, 0 }
};
#undef H
#undef M
#undef S

static jmp_buf alarmbuf;
static time_t offset, shuttime;
static int dohalt, doreboot, killflg, mbuflen, nosync;
static char *whom, mbuf[8192];
static const char *action, *prog;
static const char *path_nologin = _PATH_NOLOGIN;
static const char *path_shutdownpid = _PATH_SHUTDOWNPID;

void	cancel __P((void));
void	die_you_gravy_sucking_pig_dog __P((void));
void	finish __P((int));
void	getoffset __P((char *));
int	isrunning __P((pid_t *, int));
void	logpid __P((void));
void	loop __P((void));
void	nolog __P((void));
void	timeout __P((int));
void	timewarn __P((int, int));
void	usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern int optind;
	struct passwd *pw;
	pid_t pid;
	size_t len;
	int cflag, ch, readstdin;
	char *p, *endp, *lp;

#ifndef DEBUG
	if (geteuid())
		errx(1, "%s", strerror(EPERM));
#endif
	nosync = 0;
	cflag = readstdin = 0;
	action = prog = "shutdown";
	while ((ch = getopt(argc, argv, "-cfhknpr")) != EOF)
		switch (ch) {
		case '-':
			readstdin = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		case 'f':
			/*
			 * As of BSD/OS 3.0, clean filesystems aren't checked,
			 * so the -f option is no longer needed.
			 */
			break;
		case 'p':
			dohalt++;
			/* FALL THROUGH */
		case 'h':
			dohalt++;
			action = "halt";
			prog = _PATH_HALT;
			break;
		case 'k':
			killflg = 1;
			break;
		case 'n':
			nosync = 1;
			break;
		case 'r':
			doreboot = 1;
			action = "reboot";
			prog = _PATH_REBOOT;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	whom = getlogin();
	if (whom == NULL && (pw = getpwuid(getuid())) != NULL)
		whom = pw->pw_name;
	if (whom == NULL)
		whom = "???";

	if (cflag) {
		if (readstdin || dohalt || killflg || nosync || doreboot) {
			warnx("incompatible switches: -c and anything else");
			usage();
		}
		if (argc != 0)
			usage();
		cancel();
		exit (0);
	}
	if (dohalt && doreboot) {
		warnx("incompatible switches: -h and -r");
		usage();
	}

	if (argc < 1)
		usage();
	
	getoffset(*argv++);

	p = mbuf;
	endp = mbuf + sizeof(mbuf) - 2;
	if (*argv) {
		if (readstdin) {
			warnx("- not compatible with additional arguments");
			usage();
		}
		do {
			len = strlen(*argv);
			if (p + len * 4 > endp) {
				warnx("message truncated at %s", *argv);
				break;
			}
			if (p != mbuf)
				*p++ = ' ';
			p += strvisx(p, *argv, len, VIS_SAFE);
		} while(*++argv);
	} else if (readstdin) {
		while ((lp = fgetln(stdin, &len)) != NULL) {
			if (lp[len - 1] != '\n') {
				lp[len++] = '\n';
			}
			if (p + len * 4 > endp) {
				warnx("message truncated at %s", *argv);
				break;
			}
			p += strvisx(p, lp, len, VIS_SAFE);
		}
		if (ferror(stdin))
			err(1, "reading stdin");
	}
	if ((mbuflen = p - mbuf)) {
		if (p[-1] != '\n') {
			*p = '\n';
			mbuflen++;
		}
	}

	/* Object if a shutdown is already running. */
	if (isrunning(&pid, 0))
		errx(1, "shutdown already running (pid %lu)", pid);

	if (offset)
		(void)printf("Shutdown at %.24s.\n", ctime(&shuttime));
	else
		(void)printf("Shutdown NOW!\n");

	/* If we get a signal, don't leave the PID and nologin files around. */
	(void)signal(SIGINT, finish);
	(void)signal(SIGHUP, finish);
	(void)signal(SIGQUIT, finish);
	(void)signal(SIGTERM, finish);

	(void)setpriority(PRIO_PROCESS, 0, PRIO_MIN);
	switch (pid = fork()) {
	case -1:
		err(1, "fork");
	case 0:
#ifndef DEBUG
		openlog(NULL, LOG_CONS | LOG_PID, LOG_AUTH);
#endif
		loop();
		break;
	default:
		(void)printf("shutdown: [pid %lu]\n", (u_long)pid);
		break;
	}
	exit(0);
}

void
loop()
{
	struct interval *tp;
	time_t sleep_time;
	int logged;

	logpid();
	if (offset <= NOLOG_TIME) {
		nolog();
		logged = 1;
	} else
		logged = 0;

	tp = tlist;
	if (tp->timeleft < offset)
		(void)sleep((u_int)(offset - tp->timeleft));
	else {
		while (offset < tp->timeleft)
			++tp;
		/*
		 * Warn now, if going to sleep more than a fifth of
		 * the next wait time.
		 */
		if ((sleep_time = offset - tp->timeleft) != 0) {
			if (sleep_time > tp->timetowait / 5)
				timewarn(offset, 0);
			(void)sleep(sleep_time);
		}
	}
	for (;; ++tp) {
		timewarn(tp->timeleft, 0);
		if (!logged && tp->timeleft <= NOLOG_TIME) {
			nolog();
			logged = 1;
		}
		(void)sleep((u_int)tp->timetowait);
		if (!tp->timeleft)
			break;
	}
	die_you_gravy_sucking_pig_dog();
	finish(0);
}

void
timewarn(timeleft, cancel)
	int timeleft, cancel;
{
	static int first;
	static char hostname[MAXHOSTNAMELEN + 1];
	FILE *pf;
	char wcmd[MAXPATHLEN + 4];

	if (!first++)
		(void)gethostname(hostname, sizeof(hostname));

#ifdef DEBUG
	(void)snprintf(wcmd, sizeof(wcmd), "%s ", _PATH_CAT);
#else
	/* undoc -n option to wall suppresses normal wall banner */
	(void)snprintf(wcmd, sizeof(wcmd), "%s -n", _PATH_WALL);
#endif
	if ((pf = popen(wcmd, "w")) == NULL) {
		syslog(LOG_ERR, "%s: %m", _PATH_WALL);
		warn("%s", _PATH_WALL);
		return;
	}

	if (cancel)
		(void)fprintf(pf,
		    "\007*** System shutdown CANCELLED by %s@%s ***\007\n",
		    whom, hostname);
	else {
		(void)fprintf(pf,
		    "\007*** %sSystem shutdown message from %s@%s ***\007\n",
		    timeleft ? "": "FINAL ", whom, hostname);

		if (timeleft > 10 * SECSPERMIN)
			(void)fprintf(pf, "System going down at %5.5s\n\n",
			    ctime(&shuttime) + 11);
		else if (timeleft > SECSPERMIN - 1)
			(void)fprintf(pf,
			    "System going down in %d minute%s\n\n",
			    timeleft / SECSPERMIN,
			    (timeleft > SECSPERMIN) ? "s" : "");
		else if (timeleft)
			(void)fprintf(pf,
			    "System going down in 30 seconds\n\n");
		else
			(void)fprintf(pf, "System going down IMMEDIATELY\n\n");

		if (mbuflen)
			(void)fwrite(mbuf, sizeof(*mbuf), mbuflen,
				     pf);
	}

	/*
	 * Play some games, just in case wall doesn't come back.  Probably
	 * unecessary, given that wall is careful.
	 */
	if (!setjmp(alarmbuf)) {
		(void)signal(SIGALRM, timeout);
		(void)alarm((u_int)30);
		(void)pclose(pf);
		(void)alarm((u_int)0);
		(void)signal(SIGALRM, SIG_DFL);
	}
}

void
timeout(signo)
	int signo;
{
	longjmp(alarmbuf, 1);
}

void
die_you_gravy_sucking_pig_dog()
{
#ifndef	DEBUG
	const char *p;
#endif

#ifndef	DEBUG
	syslog(LOG_NOTICE, "%s by %s: %s", action, whom, mbuf);
	(void)sleep(2);
#endif

	(void)printf("\r\nSystem shutdown time has arrived\007\007\r\n");
	if (killflg) {
		(void)printf("\rbut you'll have to do it yourself\r\n");
		return;
	}
#ifdef DEBUG
	(void)printf("exec %s %s %s\n", prog, action, nosync ? nosync : "");
	if (nosync)
		(void)printf(" no sync");
	(void)printf("\n");
#else
	if (dohalt || doreboot) {
		if ((p = strrchr(prog, '/')) != NULL)
			p++;
		else
			p = prog;
		execle(prog, p, "-l", nosync ? "-ln" : "-l",
		    (dohalt > 1) ? "-lh" : "-l", NULL, NULL);
		syslog(LOG_ERR, "%s: %m", prog);
		warn("%s", prog);
	} else {
		kill(1, SIGTERM);
		syslog(LOG_ERR, "kill init (SIGTERM): %m");
		warn("kill init (SIGTERM)");
	}
#endif
}

#define	ATOI2(p)	(p[0] - '0') * 10 + (p[1] - '0'); p += 2;

void
getoffset(timearg)
	char *timearg;
{
	struct tm *lt;
	time_t now;
	char *p;

	if (!strcasecmp(timearg, "now")) {		/* now */
		offset = 0;
		return;
	}

	(void)time(&now);
	if (*timearg == '+') {				/* +minutes */
		if (!isdigit(*++timearg))
			goto time_err;
		offset = atoi(timearg) * 60;
		shuttime = now + offset;
		return;
	}

	/* handle hh:mm by getting rid of the colon */
	for (p = timearg; *p; ++p)
		if (!isascii(*p) || !isdigit(*p))
			if (*p == ':' && strlen(p) == 3) {
				p[0] = p[1];
				p[1] = p[2];
				p[2] = '\0';
			}
			else
				goto time_err;

	unsetenv("TZ");					/* OUR timezone */
	lt = localtime(&now);				/* current time val */

	switch (strlen(timearg)) {
	case 10:
		lt->tm_year = ATOI2(timearg);
		if (lt->tm_year < 70)
			lt->tm_year += 100;		/* year 2000 */
		/* FALLTHROUGH */
	case 8:
		lt->tm_mon = ATOI2(timearg);
		if (--lt->tm_mon < 0 || lt->tm_mon > 11)
			goto time_err;
		/* FALLTHROUGH */
	case 6:
		lt->tm_mday = ATOI2(timearg);
		if (lt->tm_mday < 1 || lt->tm_mday > 31)
			goto time_err;
		/* FALLTHROUGH */
	case 4:
		lt->tm_hour = ATOI2(timearg);
		if (lt->tm_hour < 0 || lt->tm_hour > 23)
			goto time_err;
		lt->tm_min = ATOI2(timearg);
		if (lt->tm_min < 0 || lt->tm_min > 59)
			goto time_err;
		lt->tm_sec = 0;
		if ((shuttime = mktime(lt)) == -1)
			goto time_err;
		if ((offset = shuttime - now) < 0)
			errx(1, "that time is already past");
		break;
	default:
time_err:	errx(1, "bad time format");
	}
}

/*
 * nolog --
 *	Turn off logins on the system.
 */
void
nolog()
{
	FILE *fp;
	char *ct;

	/* Delete the nologin file in case it's linked to another file. */
	(void)unlink(path_nologin);

	if ((fp = fopen(path_nologin, "w")) == NULL) {
		warn("%s", path_nologin);
		return;
	}

#define	NOMSG	"\n\nNO LOGINS: The system is going down at "
	ct = ctime(&shuttime);
	(void)fprintf(fp, "%s%.5s\n\n%s", NOMSG, ct + 11, mbuf);
	(void)fclose(fp);
}

/*
 * logpid --
 *	Log the process id of a running shutdown.
 */
void
logpid()
{
	FILE *fp;

	/* Write out the current PID. */
	if ((fp = fopen(path_shutdownpid, "w")) == NULL)
		err(1, "%s", path_shutdownpid);
	(void)fprintf(fp, "%lu\n", (u_long)getpid());
	(void)fclose(fp);
}

/*
 * cancel --
 *	Cancel an impending shutdown.
 */
void
cancel()
{
	pid_t pid;

	/*
	 * If the shutdown is still running, kill it.  Don't object if it
	 * doesn't exist, it may have died for other reasons and the user
	 * is trying to clean up.
	 */
	if (isrunning(&pid, 1) && kill(pid, SIGTERM))
		err(1, "killing process %lu", (u_long)pid);

	/* Cancel the shutdown. */
	timewarn(0, 1);

	/*
	 * Delete the shutdown PID and nologin files.
	 *
	 * XXX
	 * We don't know that the nologin file was created by the running
	 * shutdown.
	 */
	finish(0);
}

/*
 * isrunning --
 *	Return the PID of any already running shutdown.
 */
int
isrunning(pidp, fatal)
	pid_t *pidp;
	int fatal;
{
	FILE *fp;
	pid_t pid;
	u_long val;

	/* Read the PID of the running shutdown -- object if none exists. */
	if ((fp = fopen(path_shutdownpid, "r")) == NULL) {
		if (fatal)
			err(1, "%s", path_shutdownpid);
		return (0);
	}
	(void)fscanf(fp, "%lu", &val);
	(void)fclose(fp);

	pid = val;
	if (pidp != NULL)
		*pidp = pid;
	return (kill(pid, 0) == 0);
}

/*
 * finish --
 *	Cleanup and exit, possibly after signal receipt.
 */
void
finish(signo)
	int signo;
{
	/* Delete the shutdown PID file. */
	(void)unlink(path_shutdownpid);

	/* Delete the nologin file. */
	if (!killflg)
		(void)unlink(path_nologin);
	exit(0);
}

void
usage()
{
	fprintf(stderr, "usage: shutdown [-hknr] shutdowntime [ message ]\n");
	exit(1);
}
