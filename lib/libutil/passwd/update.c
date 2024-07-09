/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI update.c,v 2.5 1997/09/06 21:37:42 prb Exp
 */

#include <sys/types.h>
#include <db.h>
#include <err.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "libpasswd.h"
static char errmsg[1024];

static int
comp(s1, s2)
	char *s1;
	char *s2;
{
	if (!s1 && !s2)
		return (0);
	if (!s1 || !s2)
		return (1);
	return (strcmp(s1, s2));
}

char *
pwd_update(old, new, flags)
	struct passwd *old;
	struct passwd *new;
	int flags;
{
	struct passwd *chk;
    	struct passwd current;
    	struct pwinfo pw;
	int c, t;
	char *msg;

	if (new && !(new = pw_copy(new))) {
		snprintf(errmsg, sizeof(errmsg), "pwd_update: %s",
		    strerror(errno));
		return (errmsg);
	}
	if (!(flags & PW_SILENT))
		warnx("updating passwd database");
	if (new->pw_uid == 0)
		flags |= PW_WARNROOT;
	pw_init(&pw, flags);			/* Initialize datebase */
	if (pw_lock(&pw, O_NONBLOCK) < 0) {
		if (flags & PW_NONBLOCK) {
			return("The password database is currently locked.");
		}
		if (!(flags & PW_SILENT)) {
			printf("The password database is currently locked.\n");
			printf("Wait for lock to be released? [y] ");
			fflush(stdout);
			c = t = getchar();
			while (t != EOF && t != '\n')
				t = getchar();
			/*
			 * Since we are talking to the user we might
			 * as well allow the errx here.
			 */
			if (c == 'n')
				errx(0, "entry not changed");
		}
		if (pw_lock(&pw, 0) < 0) {
		    	return("could not aquire lock.  Contact your system administrator");
		}
    	}

	if (getuid() && old && (chk = getpwnam(old->pw_name))) {
		if (strcmp(old->pw_passwd, chk->pw_passwd)) {
			pw_abort(&pw);
			pw_unlock(&pw);
			return("password changed elsewhere.  no changes made");
		}
		if (old->pw_uid != chk->pw_uid || old->pw_gid != chk->pw_gid) {
			pw_abort(&pw);
			pw_unlock(&pw);
			return("uid/gid changed elsewhere.  no changes made");
		}
		if (old->pw_expire != chk->pw_expire) {
			pw_abort(&pw);
			pw_unlock(&pw);
			return("account expiration changed elsewhere.  no changes made");
		}
		if (comp(old->pw_class, chk->pw_class) ||
		    comp(old->pw_gecos, chk->pw_gecos) ||
		    comp(old->pw_dir, chk->pw_dir) ||
		    comp(old->pw_shell, chk->pw_shell)) {
			pw_abort(&pw);
			pw_unlock(&pw);
			return("account changed elsewhere.  no changes made");
		}
	}

	pw_sigs(NULL, NULL);			/* Block signals */
	pw_unlimit();				/* Crank up all usage limits */

	if ((msg = pwd_inplace(&pw)) != NULL) {
		pw_abort(&pw);
		pw_unlock(&pw);
		return(msg);
	}

	while ((msg = pwd_next(&pw, &current)) == NULL) {
		if (old && current.pw_uid == old->pw_uid &&
	      	    strcmp(current.pw_name, old->pw_name) == 0)
			msg = pwd_enter(&pw, &current, new);
		else
			msg = pwd_enter(&pw, &current, &current);
		if (msg) {
			pw_abort(&pw);
			pw_unlock(&pw);
			return("failure rebuilding passwd entry.  contact your system administrator");
		}
    	}
	if (*msg) {
		pw_abort(&pw);
		pw_unlock(&pw);
		return("failure reading passwd file.  contact your system administrator");
	}

	if (!old && ++pw.pw_line && (msg = pwd_enter(&pw, NULL, new)) != NULL) {
		pw_abort(&pw);
		pw_unlock(&pw);
		return("failure in adding new password entry.");
	}

	if ((msg = pwd_ipend(&pw)) != NULL) {
		pw_abort(&pw);
		pw_unlock(&pw);
		return("failure in closing password file.  contact your system administrator");
	}

	if ((msg = pwd_install(&pw)) != NULL) {
		pw_abort(&pw);
		pw_unlock(&pw);
		return("failure in installing password file.  contact your system administrator");
	}
	if (!(flags & PW_SILENT))
		warnx("done");
	if (new)
		free(new);
	return (NULL);
}
