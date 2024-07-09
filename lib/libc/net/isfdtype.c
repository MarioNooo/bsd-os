/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI isfdtype.c,v 2.1 1997/09/04 23:00:56 karels Exp
 */

#include <sys/stat.h>
#include <sys/errno.h>

/* determine whether the descriptor fd is of the specified type */
isfdtype(fd, type)
	int fd;
	int type;
{
	struct stat st;
	int ftype;

	if (fstat(fd, &st) == -1)
		return (-1);

	ftype = st.st_mode & S_IFMT;
	switch (type) {
	/*
	 * S_IFSOCK is defined by (draft) POSIX.1g;
	 * the remainder are obvious extensions.
	 */
	case S_IFSOCK:
	case S_IFIFO:
		return (ftype == S_IFSOCK || ftype == S_IFIFO);

	case S_IFCHR:
		return (ftype == S_IFCHR);

	case S_IFDIR:
		return (ftype == S_IFDIR);

	case S_IFBLK:
		return (ftype == S_IFBLK);

	case S_IFREG:
		return (ftype == S_IFREG);

#if 0 /* can't open a symlink, thus can't happen */
	case S_IFLNK:
		return (ftype == S_IFLNK);
#endif

#ifdef S_IFTTY
	case S_IFTTY:
		return (isatty(fd));
#endif

	default:
		errno = EINVAL;		/* XXX errno not defined by POSIX */
		return (-1);
	}
	/* NOTREACHED */
}
