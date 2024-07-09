/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI lock.c,v 2.3 2002/02/12 18:49:58 prb Exp
 */

/*-
 * Copyright (c) 1990, 1993, 1994
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

#include <assert.h>
#include <db.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <paths.h>
#include <sys/stat.h>

#include "libpasswd.h"

/*
 * Lock against other changes.
 * On successful return, the master file is open for reading.
 */
int
pw_lock(pw, flags)
	struct pwinfo *pw;
	int flags;
{
    	struct stat sb1, sb2;
	int fd, saverr;
	FILE *fp;
	int tries = 2;

	assert((flags & ~O_NONBLOCK) == 0);
	/*
	 * Get an exclusive lock on /etc/master.passwd.  The C library
	 * takes a shared lock, so this coordinates properly.  Set the
	 * close-on-exec bit in the underlying file descriptor, so that
	 * if our caller runs other processes they cannot see the passwords.
	 *
	 * Verify that the file we have locked is indeed still the
	 * /etc/master.passwd.  If we fail to find /etc/master.passwd,
	 * sleep for a bit and try a couple more times.  We just might
	 * catch someone doing:
	 *	unlink("/etc/master.passwd");
	 *	link("foo", "/etc/master.passwd");
	 *	unlink("foo");
	 * though no one should be doing that these days.
	 */
	fd = -1;
	do {
		if (fd >= 0)
			close(fd);
		fd = open(_PATH_MASTERPASSWD, O_RDONLY | O_EXLOCK | flags, 0);
		if (fd < 0) {
			if (errno == ENOENT && tries--) {
				sleep(1);
				continue;
			}
			/*
			 * If the filesystem does not support locking then
			 * we would never work.  This is not a very safe
			 * way to operate, however, it does let us work
			 * in those environments.
			 */
			if (errno == EOPNOTSUPP)
				fd = open(_PATH_MASTERPASSWD, O_RDONLY | flags,
				    0);
			if (fd < 0)
				return (fd);
		}
		if (fstat(fd, &sb1) < 0 || stat(_PATH_MASTERPASSWD, &sb2) < 0) {
			if (errno == ENOENT && tries--) {
				sleep(1);
				continue;
			}
			close(fd);
			return (-1);
		}
		tries = 2;
	} while (sb1.st_dev != sb2.st_dev || sb1.st_ino != sb2.st_ino);
	
	if (fcntl(fd, F_SETFD, 1) == -1 || (fp = fdopen(fd, "r")) == NULL) {
		saverr = errno;
		(void)close(fd);
		errno = saverr;
		fd = -1;
	} else {
		pw->pw_lock.pf_name = _PATH_MASTERPASSWD;
		pw->pw_lock.pf_fp = fp;
		pw->pw_flags |= PW_LOCKED;
	}
	return (fd);
}
