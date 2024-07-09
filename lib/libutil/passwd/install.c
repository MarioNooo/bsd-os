/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI install.c,v 2.2 1997/08/08 18:38:27 prb Exp
 */

#include <assert.h>
#include <db.h>
#include <err.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "libpasswd.h"

static char * pwd_mv __P((char *, char *));
static char errmsg[1024];

char *
pwd_install(pw)
	struct pwinfo *pw;
{
	char *cp, *p, *msg;

	assert(pw->pw_flags & (PW_INPLACE | PW_REBUILD));

	/*
	 * We always move the master file last, since it holds the
	 * lock (see pw_lock()).
	 */
	if (pw->pw_flags & PW_MAKEOLD &&
	    ((msg = pwd_path(pw, _PATH_PASSWD, NULL, &p)) != NULL ||
	     (msg = pwd_mv(pw->pw_old.pf_name, p)) != NULL))
		return (msg);
	pw->pw_flags &= ~PW_RMOLD;
	if (pw->pw_flags & PW_INPLACE) {
		cp = pw->pw_new.pf_name;
	} else {
		if ((msg = pwd_path(pw, _PATH_MP_DB, NULL, &p)) != NULL ||
		    (msg = pwd_mv(pw->pw_idb.pf_name, p)) != NULL)
			return (msg);
		pw->pw_flags &= ~PW_RMIDB;
		if ((msg = pwd_path(pw, _PATH_SMP_DB, NULL, &p)) != NULL ||
		    (msg = pwd_mv(pw->pw_sdb.pf_name, p)) != NULL)
			return (msg);
		pw->pw_flags &= ~PW_RMSDB;
		cp = pw->pw_master.pf_name;
	}
	if (!(pw->pw_flags & PW_NOLINK)
	    && ((msg = pwd_path(pw, _PATH_MASTERPASSWD, NULL, &p)) != NULL ||
	    (msg = pwd_mv(cp, p)) != NULL))
		return (msg);
	pw->pw_flags &= ~PW_RMNEW;
	pw_unlock(pw);
	return (NULL);
}

static char *
pwd_mv(from, to)
	char *from, *to;
{

	if (rename(from, to)) {
		snprintf(errmsg, sizeof(errmsg), "rename %s to %s: %s",
		    from, to, strerror(errno));
		return (errmsg);
	}
	return (NULL);
}
