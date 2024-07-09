/*	BSDI ttyname.c,v 2.3 2001/05/08 17:02:32 torek Exp	*/

/*
 * Copyright (c) 1988, 1993
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

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)ttyname.c	8.2 (Berkeley) 1/27/94";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <db.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <string.h>
#include <termios.h>

static int __ttyname(int, char *, size_t);
static int __oldttyname(int, struct stat *, char *, size_t);

char *
ttyname(int fd)
{
	static char buf[sizeof(_PATH_DEV) + MAXNAMLEN];
	int error;

	if ((error = __ttyname(fd, buf, sizeof buf)) != 0) {
		errno = error;
		return (NULL);
	}
	return (buf);
}

int
ttyname_r(int fd, char *buf, size_t bufsize)
{
	int error, prev;

	/* this save/restore trick is sleazy, but should suffice */
	prev = errno;
	error = __ttyname(fd, buf, bufsize);
	errno = prev;
	return (error);
}

static int
__ttyname(int fd, char *buf, size_t bufsize)
{
	struct stat sb;
	struct termios tio;
	DB *db;
	DBT data, key;
	struct {
		mode_t type;
		dev_t dev;
	} bkey;

	/* Must be a terminal. */
	if (tcgetattr(fd, &tio) < 0)
		return (errno);	/* EBADF or ENOTTY, as appropriate */

	/* Must be a character device. */
	if (fstat(fd, &sb) || !S_ISCHR(sb.st_mode))
		return (ENOTTY);

	if ((db = dbopen(_PATH_DEVDB, O_RDONLY, 0, DB_HASH, NULL)) != NULL) {
		memset(&bkey, 0, sizeof(bkey));
		bkey.type = S_IFCHR;
		bkey.dev = sb.st_rdev;
		key.data = &bkey;
		key.size = sizeof(bkey);
		if (!(db->get)(db, &key, &data, 0)) {
			/* check length ('\0' included in data.size) */
			if (sizeof(_PATH_DEV) - 1 + data.size > bufsize)
				return (ERANGE);
			memcpy(buf, _PATH_DEV, sizeof(_PATH_DEV) - 1);
			memcpy(buf + sizeof(_PATH_DEV) - 1, data.data,
			    data.size);
			(void)(db->close)(db);
			return (0);
		}
		(void)(db->close)(db);
	}
	return (__oldttyname(fd, &sb, buf, bufsize));
}

static int
__oldttyname(int fd, struct stat *sb, char *buf, size_t bufsize)
{
	register struct dirent *dirp;
	register DIR *dp;
	struct stat dsb;
	size_t len;
	char namebuf[sizeof(_PATH_DEV) + MAXNAMLEN];

	if ((dp = opendir(_PATH_DEV)) == NULL)
		return (ENOENT);	/* ??? */

	memcpy(namebuf, _PATH_DEV, sizeof(_PATH_DEV) - 1);
	while ((dirp = readdir(dp)) != NULL) {
		if (dirp->d_fileno != sb->st_ino)
			continue;
		/* d_namlen does not include trailing '\0'; sizeof does. */
		memcpy(namebuf + sizeof(_PATH_DEV) - 1, dirp->d_name,
		    dirp->d_namlen + 1);
		if (stat(namebuf, &dsb) || sb->st_dev != dsb.st_dev ||
		    sb->st_ino != dsb.st_ino)
			continue;
		(void)closedir(dp);
		len = sizeof(_PATH_DEV) + dirp->d_namlen;
		if (len > bufsize)
			return (ERANGE);
		memcpy(buf, namebuf, len);
		return (0);
	}
	(void)closedir(dp);
	return (ENOENT);		/* ??? */
}
