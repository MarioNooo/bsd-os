/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_compat.c,v 2.3 1996/03/24 21:35:02 donn Exp
 */

/*
 * Random SCO compatibility routines.
 */

#include <sys/param.h>
#include <sys/signal.h>
#include <stdlib.h>

#include "shlib.h"

char **environ;
extern char ***_libc_environ;

char *
sco_getenv(const char *v)
{

	environ = *_libc_environ;

	return (getenv(v));
}

extern char *_libc_end;
int sco_brk(const char *);

char *
sco_sbrk(int n)
{
	static char *current;
	char *old;

	if (!current)
		current = _libc_end;
	if (sco_brk(current + n) == -1)
		return ((char *)-1);
	old = current;
	current += n;
	return (old);
}

/* from iBCS2 p 3-36 */

#define	SCO_SIGSET		0x0100
#define	SCO_SIGHOLD		0x0200
#define	SCO_SIGRELSE		0x0400
#define	SCO_SIGIGNORE		0x0800
#define	SCO_SIGPAUSE		0x1000

int
sco_sighold(int sig)
{

	return ((int)sco_signal(SCO_SIGHOLD | sig, (sig_t)0));
}

int
sco_sigignore(int sig)
{

	return ((int)sco_signal(SCO_SIGIGNORE | sig, (sig_t)0));
}

int
sco_sigrelse(int sig)
{

	return ((int)sco_signal(SCO_SIGRELSE | sig, (sig_t)0));
}

int
sco_sigpause(int sig)
{

	return ((int)sco_signal(SCO_SIGPAUSE | sig, (sig_t)0));
}

sig_t
sco_sigset(int sig, sig_t handler)
{

	return (sco_signal(SCO_SIGSET | sig, handler));
}

/*
 * SCO-compatible isatty() routine to supplant the BSD one
 * called by the BSD stdio routines.
 */

#define	SCO_TCGETA	0x5401

int
isatty(int f)
{
	int dummy[5];	/* should be struct sco_termio */

	return (sco_ioctl(f, SCO_TCGETA, dummy) != -1);
}
