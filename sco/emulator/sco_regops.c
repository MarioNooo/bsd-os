/*
 * Copyright (c) 1993,1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_regops.c,v 2.1 1995/02/03 15:15:07 polk Exp
 */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "emulate.h"
#include "sco_ops.h"
#include "sco.h"
#include "sco_sig_state.h"

/*
 * Table of file ops for regular files.
 */

extern struct fdops regops;

static void
reg_init(f, filename, flags)
	int f;
	const char *filename;
	int flags;
{

	fd_register(f);
	if ((fdtab[f] = malloc(sizeof (struct fdbase))) == 0)
		err(1, "can't initialize regular file");
	fdtab[f]->f_ops = &regops;
}

int
reg_close(f)
	int f;
{

	/* XXX currently only lingering sockets return EINTR */
	if (commit_close(f) == -1)
		return (-1);
	free(fdtab[f]);
	fdtab[f] = 0;
	return (0);
}

int
reg_dup(f)
	int f;
{
	int r;

	if ((r = dup(f)) == -1)
		return (-1);
	fdtab[f]->f_ops->init(r, "", O_RDWR);		/* XXX */
	return (r);
}

int
reg_fcntl(f, c, arg)
	int f, c, arg;
{
	struct flock fl;
	int a = arg;
	int r;

	if (c == F_SETLKW) {
		flock_in((struct flock *)arg, &fl);
		return (commit_fcntl(f, c, (int)&fl));
	}

	sig_state = SIG_POSTPONE;

	switch (c) {

	case F_GETLK:
	case F_SETLK:
		flock_in((struct flock *)arg, &fl);
		a = (int)&fl;
		break;

	case F_SETFL:
		a = open_in(arg);
		if (a & X_O_NDELAY)
			/* best effort */
			a = (a &~ X_O_NDELAY) | O_NONBLOCK;
		break;

	/* case F_CHKFL? */
	}

	if ((r = fcntl(f, c, a)) == -1)
		return (-1);

	switch (c) {

	case F_DUPFD:
		fdtab[f]->f_ops->init(r, "", O_RDWR);	/* XXX */
		break;

	case F_GETLK:
		flock_out(&fl, (struct flock *)arg);
		break;

	case F_GETFL:
		r = open_out(r);
		break;
	}

	return (r);
}

struct fdops regops = {
	0,
	0,
	reg_close,
	0,
	reg_dup,
	0,
	0,
	0,
	0,
	0,
	reg_fcntl,
	0,
	fpathconf,
	fstat,
	/* XXX handle statfs buffer length? */
	(int (*) __P((int, struct statfs *, int, int)))fstatfs,
	0,
	0,
	enotdir,
	0,
	enostr,
	0,
	0,
	0,
	enotty,
	reg_init,
	enotty,
	0,
	lseek,
	0,
	0,
	enostr,
	commit_read,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	enotty,
	commit_write,
	0,
};
