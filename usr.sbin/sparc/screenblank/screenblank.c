/*	BSDI screenblank.c,v 1.3 1998/06/10 04:18:58 torek Exp	*/

#define	USE_STAT	1		/* use stat on kbd & mouse, not fstat */

#ifndef lint
static char lbl_rcsid[] =
    "@(#)Header: /usr/src/local/sbin/screenblank/RCS/screenblank.c,v 1.11 1995/05/24 06:02:11 leres Exp (LBL)";
#endif
/*
 * screenblank.c - screen blanking daemon *
 *
 * Comments to: *   jef@well.sf.ca.us *   {ucbvax, apple}!well!jef *
 *
 * Copyright (C) 1989, 1990, 1991 by Jef Poskanzer. *
 *
 * Permission to use, copy, modify, and distribute this software and its *
 * documentation for any purpose and without fee is hereby granted, provided *
 * that the above copyright notice appear in all copies and that both that *
 * copyright notice and this permission notice appear in supporting *
 * documentation.  This software is provided "as is" without express or *
 * implied warranty.
 */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/fbio.h>

#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

/* Definitions. */

#define DEFAULT_WAITING_SECS 600	/* ten minutes */
#define BLANKING_USECS 200000		/* give times a second */


#define min(a,b) ( (a) < (b) ? (a) : (b) )
#define max(a,b) ( (a) > (b) ? (a) : (b) )

#ifndef __P
#ifdef __STDC__
#define __P(protos)	protos		/* full-blown ANSI C */
#else
#define __P(protos)	()		/* traditional C preprocessor */
#endif
#endif

struct list {
	char *name;			/* name of device */
	dev_t rdev;			/* special device identifier */
	int fd;				/* file descriptor, if open */
	int revocable;			/* e.g., /dev/console */
};

#define MAX_FBS 64
static struct list fblist[MAX_FBS];
static int nfblist;

static struct list idlelist[3];
static int nidlelist;

/* Variables. */

static char *argv0;

static int ign_mouse;
static int ign_keyboard;
static int debug;
static unsigned waiting_secs;
static time_t curr_time;


#ifdef __GNUC__
volatile void exit();
#endif

/* Forward routines. */

static void find_fbs __P((void));
static void sig_handle __P((int));
static void blank __P((void));
static void unblank __P((void));
static unsigned idle_time __P((void));
static int retry __P((struct list *, struct stat *));
static void set_video __P((struct list *, int));
static int varcmp __P((char *, char *));

/* Routines. */

int
main(argc, argv)
	int argc;
	char **argv;
{
	int argn;
	unsigned idle;
	char *usage = "usage: %s [-mouse] [-keyboard] [-delay <seconds>] [-fb <framebuffer>]\n";
	register struct list *fb, *id;

	argv0 = *argv;

	/* Parse flags. */
	ign_mouse = 0;
	ign_keyboard = 0;
	waiting_secs = DEFAULT_WAITING_SECS;
	nfblist = 0;
	debug = 0;
	fb = fblist;
	for (argn = 1; argn < argc && argv[argn][0] == '-'; ++argn) {
		if (varcmp(argv[argn], "-delay") == 0) {
			++argn;
			if (argn >= argc ||
			    sscanf(argv[argn], "%d", &waiting_secs) != 1) {
				(void) fprintf(stderr, usage, argv0);
				exit(1);
			}
			continue;
		}
		if (varcmp(argv[argn], "-mouse") == 0) {
			ign_mouse = 1;
			continue;
		}
		if (varcmp(argv[argn], "-keyboard") == 0) {
			ign_keyboard = 1;
			continue;
		}
		if (varcmp(argv[argn], "-fb") == 0) {
			++argn;
			if (argn >= argc) {
				(void) fprintf(stderr, usage, argv0);
				exit(1);
			}
			fb->name = argv[argn];
			fb->fd = open(fb->name, O_RDWR);
			fb->revocable = 0;
			if (fb->fd < 0) {
				syslog(LOG_ERR, "open %s: %m", fb->name);
				continue;
			}
			++fb;
			++nfblist;
			continue;
		}
		if (varcmp(argv[argn], "-debug") == 0) {
			debug = 1;
			continue;
		}
		(void) fprintf(stderr, usage, argv0);
		exit(1);
	}
	if (argn != argc) {
		(void) fprintf(stderr, usage, argv0);
		exit(1);
	}

	if (!debug)
		daemon(0, 0);
	openlog("screenblank", LOG_PID | LOG_NOWAIT, LOG_DAEMON);

	/* If no fb's specified, search through /dev and find them all. */
	if (nfblist == 0)
		find_fbs();

	/* Open idle devices */
	id = idlelist;
	id->name = _PATH_CONSOLE;
	id->fd = open(id->name, O_RDWR | O_NOCTTY);
	if (id->fd < 0) {
		syslog(LOG_ERR, "open %s: %m", id->name);
		exit(1);
	}
	id->revocable = 1;
	++id;
	++nidlelist;
	if (!ign_mouse) {
		id->name = "/dev/mouse";
#ifdef USE_STAT
		id->fd = -1;
#else
		id->fd = open(id->name, O_RDWR);
		if (id->fd < 0) {
			syslog(LOG_ERR, "open %s: %m", id->name);
			exit(1);
		}
#endif
		id->revocable = 0;
		++id;
		++nidlelist;
	}
	if (!ign_keyboard) {
		id->name = "/dev/kbd";
#ifdef USE_STAT
		id->fd = -1;
#else
		id->fd = open(id->name, O_RDWR);
		if (id->fd < 0) {
			syslog(LOG_ERR, "open %s: %m", id->name);
			exit(1);
		}
#endif
		id->revocable = 0;
		++id;
		++nidlelist;
	}

	/* Insure all displays are unblanked */
	unblank();

	(void) signal(SIGHUP, sig_handle);
	(void) signal(SIGINT, sig_handle);
	(void) signal(SIGTERM, sig_handle);

	/* Main loop. */
	for (;;) {
		/* Wait for idleness to be reached. */
		idle = 0;
		do {
			sleep(waiting_secs - idle);
		}
		while ((idle = idle_time()) < waiting_secs);

		/* Ok, we're idle.  Blank it. */
		blank();

		/* Watch for non-idleness. */
		do {
			usleep(BLANKING_USECS);
		} while (idle_time() >= waiting_secs);

		/* No longer idle.  Clean up. */
		unblank();
	}
}

static char *fb_names[] = {
	"/dev/fb",
	"/dev/bwone0",		"/dev/bwone1",
	"/dev/bwtwo0",		"/dev/bwtwo1",
	"/dev/cgone0",		"/dev/cgone1",
	"/dev/cgtwo0",		"/dev/cgtwo1",
	"/dev/cgthree0",	"/dev/cgthree1",
	"/dev/cgfour0",		"/dev/cgfour1",
	"/dev/cgfive0",		"/dev/cgfive1",
	"/dev/cgsix0",		"/dev/cgsix1",
	"/dev/cgeight0",	"/dev/cgeight1",
	"/dev/cgnine0",		"/dev/cgnine1",
	"/dev/gpone0a",		"/dev/gpone1a",
	NULL
};

/* Add any frame buffers found from our list */
static void
find_fbs()
{
	register int fd;
	register char **fbn;
	register struct list *fb, *fb2;
	struct stat sbuf;

	fb = fblist;
	for (fbn = fb_names; *fbn != NULL; ++fbn) {
		fd = open(*fbn, O_RDWR);
		if (fd < 0)
			continue;
		if (fstat(fd, &sbuf) < 0)
			continue;

		for (fb2 = fblist; fb2 != fb; ++fb2)
			if (fb2->rdev == sbuf.st_rdev) {
				/* Already have this guy open */
				(void)close(fd);
				fd = -1;
				break;
			}

		if (fd < 0)
			continue;
		fb->name = *fbn;
		fb->fd = fd;
		fb->rdev = sbuf.st_rdev;
		fb->revocable = 0;
		++nfblist;
		++fb;
	}
}

/* ARGSUSED */
static void
sig_handle(sig)
	int sig;
{

	unblank();
	exit(0);
}

static void
blank()
{
	register struct list *fb;
	register int n;

	for (fb = fblist, n = 0; n < nfblist; ++n, ++fb)
		(void) set_video(fb, 0);
}

static void
unblank()
{
	register struct list *fb;
	register int n;

	for (fb = fblist, n = 0; n < nfblist; ++n, ++fb)
		(void) set_video(fb, 1);
}

static unsigned
idle_time()
{
	register struct list *id;
	long mintime;
	struct stat sbuf;
	int n;
	long t;

	curr_time = time(0L);
	mintime = -1;

	for (id = idlelist, n = 0; n < nidlelist; ++n, ++id) {
#ifdef USE_STAT
		if (id->fd < 0) {
			if (stat(id->name, &sbuf) >= 0)
				goto havestat;
			syslog(LOG_ERR, "stat: %s: %m", id->name);
			exit(1);
		}
#endif
		if (fstat(id->fd, &sbuf) < 0) {
			if (retry(id, &sbuf))
				goto havestat;
			syslog(LOG_ERR, "fstat: %s: %m", id->name);
			exit(1);
		}
	havestat:
		t = min(curr_time - sbuf.st_atime, curr_time - sbuf.st_mtime);
		if (mintime < 0 || t < mintime)
			mintime = t;
	}

	return (mintime);
}

static void
set_video(fb, b)
	register struct list *fb;
	register int b;
{
	int v;

	v = b ? FBVIDEO_ON : FBVIDEO_OFF;
	if (ioctl(fb->fd, FBIOSVIDEO, (char *)&v) < 0)
		syslog(LOG_ERR, "set_video %s %s failed: %m",
		    b ? "on" : "off", fb->name);
}

static int
retry(id, sb)
	struct list *id;
	struct stat *sb;
{
	int fd, saverr;

	if (!id->revocable)
		return (0);
	saverr = errno;
	fd = open(id->name, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		errno = saverr;
		return (0);
	}
	(void)close(id->fd);
	id->fd = fd;
	return (fstat(fd, sb) >= 0);
}

static int
varcmp(arg, str)
	register char *arg, *str;
{
	register int len;

	len = strlen(arg);
	if (len < 2)
		len = 2;
	return (strncmp(arg, str, len));
}
