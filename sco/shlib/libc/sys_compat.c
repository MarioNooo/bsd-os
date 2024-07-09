/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sys_compat.c,v 2.3 1996/08/20 17:39:34 donn Exp
 */

/*
 * Compatibility hacks for system calls in the emulated SCO shared C library.
 * This code is NOT general and is present solely to support the use
 * of BSD library objects under the SCO emulator.
 *
 * XXX If the C library changes, we can get screwed...
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "shlib.h"

/*
 * The following routines can be handled by BSD syscall stubs:
 *
 * _exit close dup2 getpid kill lseek read sbrk select write
 *
 * The remainder require some sort of data type conversion from SCO
 * back to BSD.  Yes, a couple of redundant conversions occur...
 */

int
dup2(int old, int new)
{

	close(new);
	return (sco_fcntl(old, F_DUPFD, new));
}

int
fstat(int f, struct stat *bsp)
{
	struct sco_stat ss;

	if (sco_fstat(f, &ss) == -1)
		return (-1);

	/* we only convert the fields that we actually need */
	bsp->st_mode = ss.sst_mode;
	bsp->st_size = ss.sst_size;
	bsp->st_blksize = NBPG;			/* XXX */

	return (0);
}

int
getdtablesize(void)
{

	if (sco_nfile == 0)
		set_sco_nfile();
	return (sco_nfile);
}

/* XXX to prevent threaded version from being linked in... */
int
_syscall_sys_getdtablesize(void)
{

	if (sco_nfile == 0)
		set_sco_nfile();
	return (sco_nfile);
}

int
getpagesize(void)
{

	return (NBPG);
}

#define	SCO_O_CREAT	0x0100
#define	SCO_O_TRUNC	0x0200
#define	SCO_O_EXCL	0x0400

int
open(const char *path, int flags, ...)
{
	int sco_flags = flags & ~(O_CREAT|O_TRUNC|O_EXCL);
	int n;
	va_list ap;

	va_start(ap, flags);

	/* we only convert flag bits that are actually used */
	sco_flags |= flags & O_CREAT ? SCO_O_CREAT : 0;
	sco_flags |= flags & O_TRUNC ? SCO_O_TRUNC : 0;
	sco_flags |= flags & O_EXCL ? SCO_O_EXCL : 0;

	if (flags & O_CREAT)
		n = sco_open(path, sco_flags, va_arg(ap, int));
	else
		n = sco_open(path, sco_flags);

	va_end(ap);
	return (n);
}

char *
sbrk(int n)
{

	return (sco_sbrk(n));
}

sig_t
signal(int sig, sig_t func)
{

	/* used only in abort() with SIGABRT, which is the same in SCO & BSD */
	return (sco_signal(sig, func));
}

/* from iBCS2 p 6-50 */
#define	SCO_SIG_SETMASK	0

int
sigprocmask(int cookie, const sigset_t *mask, sigset_t *omask)
{

	/* used only in abort() with SIG_SETMASK and SIGABRT */
	return (sco_sigprocmask(SCO_SIG_SETMASK, mask, omask));
}

/* XXX to defeat threaded version... */
int
_syscall_sys_sigprocmask(int cookie, const sigset_t *mask, sigset_t *omask)
{

	/* used only in abort() with SIG_SETMASK and SIGABRT */
	return (sco_sigprocmask(SCO_SIG_SETMASK, mask, omask));
}

int
stat(const char *p, struct stat *bsp)
{
	struct sco_stat ss;

	if (sco_stat(p, &ss) == -1)
		return (-1);

	/* we only convert the fields that we actually need */
	bsp->st_mode = ss.sst_mode;

	return (0);
}
