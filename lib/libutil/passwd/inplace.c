/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI inplace.c,v 2.3 2000/04/03 16:21:20 donn Exp
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

static char * pwd_opendb __P((struct pwinfo *, struct pwf *, char *));
static char errmsg[1024];

/*
 * Set up to update the databases in place.
 * We open up the databases for modification and make new flat files
 * (because the old flat files may have some lines replaced).
 */
char *
pwd_inplace(pw)
	struct pwinfo *pw;
{
	int fd, omask;
	char *cp;
	FILE *fp;
	struct stat st;
	char *msg;

	assert((pw->pw_flags & (PW_INPLACE | PW_REBUILD)) == 0);

	/* Open input master file, or use lock file if input locked. */
	pwd_path(pw, _PATH_MASTERPASSWD, NULL, &cp);
	if (pw->pw_flags & PW_LOCKED && strcmp(cp, pw->pw_lock.pf_name) == 0)
		pw->pw_master = pw->pw_lock;
	else {
		if ((fp = fopen(cp, "r")) == NULL) {
			snprintf(errmsg, sizeof(errmsg), "cannot read %s: %s",
			    cp, strerror(errno));
			return (errmsg);
		}
		fcntl(fileno(fp), F_SETFD, FD_CLOEXEC);
		pw->pw_master.pf_name = cp;
		pw->pw_master.pf_fp = fp;
	}

	/* Create with exact permissions (umask restored at end). */
	omask = umask(0);

	/* These are not removed on error (good, bad, ??? -- use copies?) */
	if ((msg = pwd_opendb(pw, &pw->pw_idb, _PATH_MP_DB)) != NULL) {
		goto bad;
	}
	if ((msg = pwd_opendb(pw, &pw->pw_sdb, _PATH_SMP_DB)) != NULL) {
		goto bad;
	}

	/* Build old-style flat file iff one exists now. */
	pwd_path(pw, _PATH_PASSWD, NULL, &cp);
	if (stat(cp, &st)) {
		if (errno == ENOENT)
			pw->pw_flags &= ~PW_MAKEOLD;
		else {
			snprintf(errmsg, sizeof(errmsg), "%s: %s", cp,
			    strerror(errno));
			msg = errmsg;
			goto bad;
		}
	} else
		pw->pw_flags |= PW_MAKEOLD;

	if (pw->pw_flags & PW_MAKEOLD) {
		/* Build new old-style file, to be removed on error. */
		if ((msg = pwd_path(pw, _PATH_PASSWD, ".tmp", &cp)) != NULL) {
			goto bad;
		}
		pw->pw_old.pf_name = cp;
		pw->pw_flags |= PW_RMOLD;
		(void)unlink(cp);
		fd = open(cp, O_WRONLY | O_CREAT | O_EXCL, PW_PERM_INSECURE);
		if (fd < 0) {
			snprintf(errmsg, sizeof(errmsg), "%s: %s", cp,
			    strerror(errno));
			msg = errmsg;
			goto bad;
		}
		if ((fp = fdopen(fd, "w")) == NULL) {
			snprintf(errmsg, sizeof(errmsg), "%s: %s", cp,
			    strerror(errno));
			msg = errmsg;
			(void)close(fd);
			goto bad;
		}
		fcntl(fd, F_SETFD, FD_CLOEXEC);
		pw->pw_old.pf_fp = fp;
	}

	/* Build new master file, to be removed on error. */
	if ((msg = pwd_path(pw, _PATH_MASTERPASSWD, ".tmp", &cp)) != NULL) {
		goto bad;
	}
	pw->pw_new.pf_name = cp;
	pw->pw_flags |= PW_RMNEW;
	fd = open(cp, O_WRONLY | O_CREAT | O_EXCL, PW_PERM_SECURE);
	if (fd < 0) {
		snprintf(errmsg, sizeof(errmsg), "%s:%s", cp, strerror(errno));
		msg = errmsg;
		goto bad;
	}
	if ((fp = fdopen(fd, "w")) == NULL) {
		snprintf(errmsg, sizeof(errmsg), "%s:%s", cp, strerror(errno));
		msg = errmsg;
		(void)close(fd);
		goto bad;
	}
	fcntl(fd, F_SETFD, FD_CLOEXEC);
	pw->pw_new.pf_fp = fp;

	/* Phew, made it. */
	(void)umask(omask);
	pw->pw_flags |= PW_INPLACE;
	return (NULL);

bad:
	/* Something went wrong -- clean up. */
	(void)umask(omask);
	return (msg);
}

static char *
pwd_opendb(pw, pf, path)
	struct pwinfo *pw;
	struct pwf *pf;
	char *path;
{
	char *cp;
	char *msg;

	if ((msg = pwd_path(pw, path, NULL, &cp)) != NULL)
		return (msg);
	pf->pf_name = cp;
	/* No need for permissions; we just want to open existing db. */
	if ((pf->pf_db = dbopen(cp, O_RDWR, 0, DB_HASH, NULL)) == NULL) {
		snprintf(errmsg, sizeof(errmsg), "%s: %s", cp, strerror(errno));
		return (errmsg);
	}
	return (NULL);
}

/*
 * End in-place update.
 */
char *
pwd_ipend(pw)
	struct pwinfo *pw;
{
	struct pwf *pf;
	DB *db;
	int status = 0;

	assert(pw->pw_flags & PW_INPLACE);

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
	db = pf->pf_db;
	if (db->close(db)) {
		snprintf(errmsg, sizeof(errmsg), "%s: %s",
		    pf->pf_name, strerror(errno));
		status = -1;
	}
	pf->pf_db = NULL;

	pf = &pw->pw_sdb;
	db = pf->pf_db;
	if (db->close(db)) {
		snprintf(errmsg, sizeof(errmsg), "%s: %s",
		    pf->pf_name, strerror(errno));
		status = -1;
	}
	pf->pf_db = NULL;

	/* The fsync is paranoia. */
	pf = &pw->pw_new;
	if (fflush(pf->pf_fp) || fsync(fileno(pf->pf_fp)) ||
	    fclose(pf->pf_fp)) {
		snprintf(errmsg, sizeof(errmsg), "%s: %s",
		    pf->pf_name, strerror(errno));
		status = -1;
	}
	pf->pf_fp = NULL;

	pf = &pw->pw_master;
	if (pf->pf_fp != pw->pw_lock.pf_fp) {
		/* Different (or no) lock; close input. */
		(void)fclose(pf->pf_fp);
		pf->pf_fp = NULL;
	}

	return (status ? errmsg : NULL);
}
