/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI rebuild.c,v 2.5 2002/04/24 01:28:11 chrisk Exp
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

#include <sys/types.h>
#include <sys/stat.h>

#include <assert.h>
#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <paths.h>
#include <unistd.h>

#include "libpasswd.h"

static char * pwd_rbend __P((struct pwinfo *, int));
static char * pwd_rbpass __P((struct pwinfo *, int, u_int, int, int));
static char errmsg[1024];

/*
 * Set up to rebuild the databases from scratch.
 * Need input file name.  No new files are created just yet; see pw_rbpass.
 */
char *
pwd_rebuild(pw, master, cachesize)
	struct pwinfo *pw;
	char *master;
	u_int cachesize;
{
	int omask, pass, lines, len, kdlen, i;
	size_t msize, nlen;
	FILE *fp;
	struct passwd pwd;
	char *msg, *ln;

	assert((pw->pw_flags & (PW_INPLACE | PW_REBUILD)) == 0);
	/*
	 * This is similar to pw_lock, but we do not need to lock the file.
	 * We will go ahead and set close-on-exec, out of sheer paranoia.
	 */
	if ((fp = fopen(master, "r")) == NULL) {
		snprintf(errmsg, sizeof(errmsg), "cannot read %s: %s",
		    master, strerror(errno));
		return (errmsg);
	}
	(void)fcntl(fileno(fp), F_SETFD, FD_CLOEXEC);
	(void)fchmod(fileno(fp), S_IRUSR | S_IWUSR);	/* force security */
	pw->pw_master.pf_name = master;
	pw->pw_master.pf_fp = fp;

	/* Just construct the names -- we build the files later. */
	if ((msg = pwd_path(pw, _PATH_MP_DB, ".tmp",
						&pw->pw_idb.pf_name)) != NULL ||
	    (msg = pwd_path(pw, _PATH_SMP_DB, ".tmp",
						&pw->pw_sdb.pf_name)) != NULL) {
		(void)fclose(fp);
		return (msg);
	}
	if ((pw->pw_flags & PW_MAKEOLD) && (msg = pwd_path(pw, _PATH_PASSWD,
	    ".tmp", &pw->pw_old.pf_name)) != NULL) {
		(void)fclose(fp);
		return (msg);
	}
	pw->pw_flags |= PW_REBUILD;

	/* Use exact permissions. */
	omask = umask(0);

	/* Get a count of the number of lines in the master file. */
	lines = 0;
	msize = 0;
	nlen = 0;
	rewind(pw->pw_master.pf_fp);
	while ((ln = fgetln(pw->pw_master.pf_fp, &len)) != NULL) {
		++lines;
		nlen += index(ln, ':') - ln - 1;
		msize += len - 1;
	}
	/*
	 * Key data pair length = Ave. Key length + Ave. line size (data).
	 * There are 3 entries per line, 2 of which have an ASCII character
	 * and a 4 byte integer as a key. The other has the username as a key.
	 */
	kdlen = ((2 * 5 * lines + nlen)/(3*lines)) + msize/lines;
	rewind(pw->pw_master.pf_fp);

	/* Now run two rebuild passes; 0 does insecure, 1 does secure. */
	for (pass = 0; pass < 2; pass++) {
		if ((msg = pwd_rbpass(pw, pass, cachesize, lines, kdlen))
		    != NULL)
			goto bad;
		while ((msg = pwd_next(pw, &pwd)) == NULL)
			if ((msg = pwd_ent1(pw, NULL, &pwd, pass)) != NULL)
				goto bad;
		if (*msg || (msg = pwd_rbend(pw, pass)) != NULL)
			goto bad;
	}

	(void)umask(omask);
	return (NULL);

bad:
	(void)umask(omask);
	return (msg);
}

static HASHINFO openinfo = {
	4096,		/* bsize */
	32,		/* ffactor */
	256,		/* nelem */
	0,		/* cachesize -- set in rbpass */
	NULL,		/* hash() */
	0		/* lorder */
};

/*
 * Set up for a rebuild pass (insecure or secure, as indicated).
 */
static char *
pwd_rbpass(pw, secure, cachesize, lines, kdlen)
	struct pwinfo *pw;
	int secure;
	u_int cachesize;
	int lines;
	int kdlen;
{
	int fd, perm;
	char *cp, c;
	DB *db;
	FILE *fp;
	struct pwf *pf;
	struct stat sb;

	/* Create the database. */
	/* Set the cachesize. */
	openinfo.cachesize = (cachesize ? cachesize : 1024) * 1024;
	/* Set nelem to 3 (byname, byuid, byline) times lines. */
	openinfo.nelem = 3*lines > openinfo.nelem ? 3*lines : openinfo.nelem;
	/* Determine the bsize and ffactor. Try to keep 64 entries in a block */
	for (openinfo.bsize = 4096; openinfo.bsize < 65536;
	    openinfo.bsize *= 2) {
		openinfo.ffactor = openinfo.bsize/(kdlen + 8);
		if (openinfo.ffactor >= 64)
			break;
	}

	if (!secure) {
		/* Create insecure database, and maybe old-style flat file. */
		if (pw->pw_flags & PW_MAKEOLD) {
			cp = pw->pw_old.pf_name;
			pw->pw_flags |= PW_RMOLD;
			fd = open(cp, O_WRONLY | O_CREAT | O_EXCL,
			    PW_PERM_INSECURE);
			if (fd < 0) {
				snprintf(errmsg, sizeof(errmsg),
			    	   "cannot create %s: %s", cp, strerror(errno));
				return (errmsg);
			}
			(void)fcntl(fd, F_SETFD, FD_CLOEXEC);
			if ((fp = fdopen(fd, "w")) == NULL) {
				snprintf(errmsg, sizeof(errmsg),
			    	    "%s: %s", cp, strerror(errno));
				(void)close(fd);
				return (errmsg);
			}
			pw->pw_old.pf_fp = fp;
		}
		pf = &pw->pw_idb;
		perm = PW_PERM_INSECURE;
		pw->pw_flags |= PW_RMIDB;
	} else {
		/* Create secure database; no flat file. */
		pf = &pw->pw_sdb;
		perm = PW_PERM_SECURE;
		pw->pw_flags |= PW_RMSDB;
	}
	(void)unlink(pf->pf_name);
	db = dbopen(pf->pf_name, O_RDWR | O_CREAT | O_EXCL, perm,
	    DB_HASH, &openinfo);
	if (db == NULL) {
		snprintf(errmsg, sizeof(errmsg), "cannot create %s: %s",
		    pf->pf_name, strerror(errno));
		return (errmsg);
	}
	pf->pf_db = db;

	/* Start at the beginning of the input master file. */
	rewind(pw->pw_master.pf_fp);
	pw->pw_line = 0;

	/* Ready to go. */
	return (NULL);
}

/*
 * End rebuild pass.
 */
static char *
pwd_rbend(pw, secure)
	struct pwinfo *pw;
	int secure;
{
	struct pwf *pf;
	DB *db;
	int status = 0;

	if (!secure) {
		if (pw->pw_flags & PW_MAKEOLD) {
			pf = &pw->pw_old;
			if (fflush(pf->pf_fp) || fclose(pf->pf_fp)) {
				snprintf(errmsg, sizeof(errmsg), "%s: %s",
				    pf->pf_name, strerror(errno));
				status = -1;
			}
			pf->pf_fp = NULL;
		}
		pf = &pw->pw_idb;
	} else {
		pf = &pw->pw_sdb;
	}
	db = pf->pf_db;
	if (db->close(db)) {
		snprintf(errmsg, sizeof(errmsg), "%s: %s",
		    pf->pf_name, strerror(errno));
		status = -1;
	}
	pf->pf_db = NULL;

	return (status ? errmsg : NULL);
}
