/*-
 * Copyright (c) 1992, 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI utils.c,v 2.3 1997/11/19 01:58:35 sanders Exp
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>

#include "pathnames.h"
#include "at.h"
#include "errlib.h"

char *
bfpath(JobQ p)
{
	static char buf[MAXPATHLEN];
	snprintf(buf, MAXPATHLEN, "%sbroken.%d.%d", AT_DIR, p->id, p->when);
	return buf;
}

char *
qfpath(JobQ p)
{
	static char buf[MAXPATHLEN];
	snprintf(buf, MAXPATHLEN, "%s%d.%d", AT_DIR, p->id, p->when);
	return buf;
}

char *
xfpath(JobQ p)
{
	static char buf[MAXPATHLEN];
	snprintf(buf, MAXPATHLEN, "%s%s/%d.%d", AT_DIR, AT_LOCKDIR, p->id, p->when);
	return buf;
}

char *
outputpath(JobQ p)
{
	static char buf[MAXPATHLEN];
	snprintf(buf, MAXPATHLEN, "%s%s/%d.output", AT_DIR, AT_LOCKDIR, p->id);
	return buf;
}

char *
escstr(char *s, char match, char *repl)
{
	static char buf[ARG_MAX];
	int rlen = strlen(repl);
	char *end = buf + ARG_MAX - 1;
	char *p = buf;

	while (*s && end >= p)
		if (*s == match)
			strncpy(p, repl, end-p), p += rlen, s++;
		else
			*p++ = *s++;
	*p++ = '\0';
	if (p > end)
		Pexit("environment string %s too long\n", s);
	return buf;
}

char *
shellesc(char *s)
{
	return escstr(s, '\'', "'\\''");
}

void
swrite(int fd, char *fmt, ...)
{
	char buf[ARG_MAX];
	va_list ap;

	va_start(ap, fmt);
	if (vsnprintf(buf, ARG_MAX, fmt, ap) >= ARG_MAX-1)
		Perror("string %s too long\n", buf);
	if (write(fd, buf, strlen(buf)) <= 0)
		Perror("write in swrite failed on fd=%d", fd);
	va_end(ap);
}

#ifdef NEED_USLEEP
void
usleep(long usec)
{
	struct	timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = usec;
	select(0, NULL, NULL, NULL, &tv);
}
#endif

/*
 * Lcck the at directory and get the next job number
 */

static int
atlock()
{
	char	lockfile[MAXPATHLEN];
	int	lockid;
	mode_t	mode = O_RDWR|O_EXLOCK;

	snprintf(lockfile, MAXPATHLEN, "%s%s", AT_DIR, AT_LOCK);
	seteuid(0);
	while(1) {
		errno = 0;
		lockid = open(lockfile, mode, 0600);
		if (lockid >= 0) break;
		if (errno == ENOENT) {
			mode |= O_CREAT;
			errno = ENOLCK;
		}
		if (errno != ENOLCK)
			Perror("lockfile %s", lockfile);
		usleep(50000);
	}
	seteuid(job->owner);

	/* create and init */
	if (mode & O_CREAT) {
		swrite(lockid, "1\n");
		lseek(lockid, 0, SEEK_SET);
	}
	return lockid;
}

static void
atunlock(int fd)
{
	close(fd);
}

long
getjobid()
{
#define	JOBLEN 12
	char buf[JOBLEN+1];
	int fd;
	long jobid;

	fd = atlock();

	if (read(fd, buf, JOBLEN) < 1)
		Perror("read at lockfile");
	buf[JOBLEN] = '\0';
	if ((jobid = atol(buf)) == 0)
		Pexit("lockfile job# invalid\n");
	if (lseek(fd, 0, SEEK_SET) < 0)
		Perror("seek on at lockfile");
	swrite(fd, "%d\n", jobid + 1);
	
	atunlock(fd);
	return jobid;
#undef	JOBLEN
}
