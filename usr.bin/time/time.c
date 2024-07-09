/*
 * Copyright (c) 1987, 1988, 1993
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
"@(#) Copyright (c) 1987, 1988, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)time.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void lreport __P((struct rusage *));
void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern int optind;
	struct rusage ru;
	struct timeval before, after;
	pid_t pid;
	int ch, eval, status, lflag;

	lflag = 0;
	while ((ch = getopt(argc, argv, "l")) != EOF)
		switch (ch) {
		case 'l':
			lflag = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();

	gettimeofday(&before, NULL);
	switch (pid = vfork()) {
	case -1:			/* error */
		err(1, "vfork");
		/* NOTREACHED */
	case 0:				/* child */
		execvp(*argv, argv);
		warn("%s", *argv);
		_exit(127);
		/* NOTREACHED */
	}

	/* parent */
	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);
	(void)wait4(pid, &status, 0, &ru);
	gettimeofday(&after, NULL);

	if (WIFSIGNALED(status))
		fprintf(stderr, "%s: signaled: %s\n",
		    *argv, sys_siglist[WTERMSIG(status)]);
	if (WCOREDUMP(status))
		fprintf(stderr, "%s: core dumped\n", *argv);
	if (WIFEXITED(status)) {
		eval = WEXITSTATUS(status);
		if (eval != 0 && eval != 127)
			fprintf(stderr,
			    "%s: terminated abnormally: %d\n", *argv, eval);
	} else
		eval = 126;

	after.tv_sec -= before.tv_sec;
	after.tv_usec -= before.tv_usec;
	if (after.tv_usec < 0) {
		after.tv_sec--;
		after.tv_usec += 1000000;
	}
	fprintf(stderr, "%9ld.%02ld real ",
	    after.tv_sec, after.tv_usec / 10000);
	fprintf(stderr, "%9ld.%02ld user ",
	    ru.ru_utime.tv_sec, ru.ru_utime.tv_usec / 10000);
	fprintf(stderr, "%9ld.%02ld sys\n",
	    ru.ru_stime.tv_sec, ru.ru_stime.tv_usec / 10000);

	if (lflag)
		lreport(&ru);
	exit (eval);
}

void
lreport(rp)
	struct rusage *rp;
{
	struct clockinfo clk;
	size_t len;
	long ticks;
	int mib[3];

	mib[0] = CTL_KERN;
	mib[1] = KERN_CLOCKRATE;

	len = sizeof(clk);
	if (sysctl(mib, 2, &clk, &len, NULL, NULL) == -1)
		err(1, "sysctl: clockrate");

	ticks = clk.hz * (rp->ru_utime.tv_sec + rp->ru_stime.tv_sec) +
	     clk.hz * (rp->ru_utime.tv_usec + rp->ru_stime.tv_usec) / 1000000;
	if (ticks == 0)
		ticks = 1;

	fprintf(stderr,
	    "%10ld  maximum resident set size\n", rp->ru_maxrss);
	fprintf(stderr,
	    "%10ld  average shared memory size\n", rp->ru_ixrss / ticks);
	fprintf(stderr,
	    "%10ld  average unshared data size\n", rp->ru_idrss / ticks);
	fprintf(stderr,
	    "%10ld  average unshared stack size\n", rp->ru_isrss / ticks);
	fprintf(stderr, "%10ld  page reclaims\n", rp->ru_minflt);
	fprintf(stderr, "%10ld  page faults\n", rp->ru_majflt);
	fprintf(stderr, "%10ld  swaps\n", rp->ru_nswap);
	fprintf(stderr, "%10ld  block input operations\n", rp->ru_inblock);
	fprintf(stderr, "%10ld  block output operations\n", rp->ru_oublock);
	fprintf(stderr, "%10ld  messages sent\n", rp->ru_msgsnd);
	fprintf(stderr, "%10ld  messages received\n", rp->ru_msgrcv);
	fprintf(stderr, "%10ld  signals received\n", rp->ru_nsignals);
	fprintf(stderr, "%10ld  voluntary context switches\n", rp->ru_nvcsw);
	fprintf(stderr, "%10ld  involuntary context switches\n", rp->ru_nivcsw);
}

void
usage()
{
	fprintf(stderr, "usage: time [-l] command [arg ...].\n");
	exit(1);
}
