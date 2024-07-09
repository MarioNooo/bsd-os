/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_mknod.c,v 2.1 1995/02/03 15:14:50 polk Exp
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <err.h>
#include "emulate.h"
#include "sco.h"

/*
 * Support for mknod emulation.
 * Currently we only handle FIFOs.
 */

int
#ifdef __STDC__
sco_mknod(const char *filename, mode_t mode, dev_t dev)
#else
sco_mknod(filename, mode, dev)
	const char *filename;
	mode_t mode;
	dev_t dev;
#endif
{

	if (S_ISFIFO(mode))
		return (mkfifo(filename, mode &~ S_IFIFO));

	errx(1, "unsupported mknod system call");
}
