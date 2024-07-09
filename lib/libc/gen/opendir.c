/*	BSDI opendir.c,v 2.8 2001/05/08 17:02:31 torek Exp	*/

/*
 * Copyright (c) 1983, 1993
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
static char sccsid[] = "@(#)opendir.c	8.8 (Berkeley) 5/1/95";
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dirvar.h"

#include "thread_private.h"

/*
 * Open a directory.
 */
DIR *
opendir(name)
	const char *name;
{

	return (__opendir2(name, DTF_HIDEW|DTF_NODUP));
}

DIR *
__opendir2(name, flags)
	const char *name;
	int flags;
{
	DIR *dirp;
	int fd;
	int incr = DIRBUFSIZE;
	int nfsdir;
	int unionstack;
	struct stat statb;
	struct statfs sfb;
	int sv_errno;

	if ((fd = open(name, O_RDONLY)) == -1)
		return (NULL);
	if (fstat(fd, &statb) || !S_ISDIR(statb.st_mode)) {
		close(fd);
		errno = ENOTDIR;
		return (NULL);
	}
	if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1 ||
	    (dirp = malloc(sizeof(DIR))) == NULL) {
		sv_errno = errno;
		close(fd);
		errno = sv_errno;
		return (NULL);
	}

	/*
	 * Determine whether this directory is the top of a stack (that
	 * may contain a union)
	 */
	if (fstatfs(fd, &sfb) < 0) {
		sv_errno = errno;
		free(dirp);
		close(fd);
		errno = sv_errno;
		return (NULL);
	}
	if ((flags & DTF_NODUP) && (sfb.f_flags & MNT_UNIONDIR))
		unionstack = 1;
	else
		unionstack = 0;

	nfsdir = strncmp(sfb.f_fstypename, "nfs", MFSNAMELEN) == 0;

	if (unionstack || nfsdir) {
		int len;
		int space;
		char *buf;
		char *ddptr;
		char *ddeptr;
		int n;
		struct dirent **dpv;

	retry:
		len = space = 0;
		buf = ddptr = 0;

		/*
		 * The strategy here is to read all the directory
		 * entries into a buffer, sort the buffer, and
		 * remove duplicate entries by setting the inode
		 * number to zero.
		 *
		 * For directories on an NFS mounted filesystem, we try
	 	 * to get a consistent snapshot by trying until we have
		 * successfully read all of the directory without errors
		 * (i.e. 'bad cookie' errors from the server because
		 * the directory was modified). These errors should not
		 * happen often, but need to be dealt with.
		 */

		do {
			/*
			 * Always make at least DIRBUFSIZE bytes
			 * available to getdirentries
			 */
			if (space < DIRBUFSIZE) {
				space += incr;
				len += incr;
				buf = realloc(buf, len);
				if (buf == NULL) {
					sv_errno = errno;
					free(dirp);
					close(fd);
					errno = sv_errno;
					return (NULL);
				}
				ddptr = buf + (len - space);
			}

			n = getdirentries(fd, ddptr, space, &dirp->dd_seek);
			/*
			 * For NFS: EINVAL means a bad cookie error
			 * from the server. Keep trying to get a
			 * consistent view, in this case this means
			 * starting all over again.
			 */
			if (n == -1 && errno == EINVAL && nfsdir) {
				free(buf);
				lseek(fd, 0, SEEK_SET);
				goto retry;
			}
			if (n > 0) {
				ddptr += n;
				space -= n;
			}
		} while (n > 0);

		ddeptr = ddptr;
		flags |= __DTF_READALL;

		/*
		 * Re-open the directory.
		 * This has the effect of rewinding back to the
		 * top of the union stack and is needed by
		 * programs which plan to fchdir to a descriptor
		 * which has also been read -- see fts.c.
		 */
		if (flags & DTF_REWIND) {
			(void) close(fd);
			if ((fd = open(name, O_RDONLY)) == -1 ||
			    fcntl(fd, F_SETFD, FD_CLOEXEC) == -1) {
				sv_errno = errno;
				if (fd != -1)
					close(fd);
				free(buf);
				free(dirp);
				errno = sv_errno;
				return (NULL);
			}
		}

		/*
		 * There is now a buffer full of (possibly) duplicate
		 * names.
		 */
		dirp->dd_buf = buf;

		/*
		 * Go round this loop twice...
		 *
		 * Scan through the buffer, counting entries.
		 * On the second pass, save pointers to each one.
		 * Then sort the pointers and remove duplicate names.
		 */
		for (dpv = 0; unionstack;) {
			n = 0;
			ddptr = buf;
			while (ddptr < ddeptr) {
				struct dirent *dp;

				dp = (struct dirent *) ddptr;
				if ((int)dp & 03)
					break;
				if ((dp->d_reclen <= 0) ||
				    (dp->d_reclen > (ddeptr + 1 - ddptr)))
					break;
				ddptr += dp->d_reclen;
				if (dp->d_fileno) {
					if (dpv)
						dpv[n] = dp;
					n++;
				}
			}

			if (dpv) {
				struct dirent *xp;

				/*
				 * This sort must be stable.
				 */
				mergesort(dpv, n, sizeof(*dpv), alphasort);

				dpv[n] = NULL;
				xp = NULL;

				/*
				 * Scan through the buffer in sort order,
				 * zapping the inode number of any
				 * duplicate names.
				 */
				for (n = 0; dpv[n]; n++) {
					struct dirent *dp = dpv[n];

					if ((xp == NULL) ||
					    strcmp(dp->d_name, xp->d_name)) {
						xp = dp;
					} else {
						dp->d_fileno = 0;
					}
					if (dp->d_type == DT_WHT &&
					    (flags & DTF_HIDEW))
						dp->d_fileno = 0;
				}

				free(dpv);
				break;
			} else {
				dpv = malloc((n+1) * sizeof(struct dirent *));
				if (dpv == NULL)
					break;
			}
		}

		dirp->dd_len = len;
		dirp->dd_size = ddptr - dirp->dd_buf;
		dirp->dd_threadlock = NULL;
	} else {
		dirp->dd_len = incr;
		dirp->dd_buf = malloc(dirp->dd_len);
		if (dirp->dd_buf == NULL) {
			sv_errno = errno;
			free(dirp);
			close (fd);
			errno = sv_errno;
			return (NULL);
		}
		dirp->dd_seek = 0;
		flags &= ~DTF_REWIND;
		/* NB: you really have to set this to NULL before init'ing */
		dirp->dd_threadlock = NULL;
		(void) _thread_qlock_init(&dirp->dd_threadlock);
	}

	dirp->dd_loc = 0;
	dirp->dd_fd = fd;
	dirp->dd_flags = flags;

	return (dirp);
}
