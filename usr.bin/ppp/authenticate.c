/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI authenticate.c,v 2.5 1997/11/06 20:43:37 chrisk Exp
 */

#include <sys/param.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_dl.h>

#include <login_cap.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <syslog.h>
#include <bsd_auth.h>

#include "pbuf.h"
#include "ppp.h"
#include "ppp_proto.h"
#include "ppp_var.h"

char *
pap_authenticate(char *user, char *style, char *passwd)
{
	struct passwd *pwd;
	login_cap_t *lc;

	pwd = getpwnam(user);
	if ((lc = login_getclass(pwd ? pwd->pw_class : PAP_CLASS)) == NULL)
		return("unable to locate login class entry for user");

	style = login_getstyle(lc, style, "auth-pap");

	if (!style) {
		login_close(lc);
		return("authentication style not available");
	}

	if (auth_response(user, lc->lc_class, style, "response", NULL, "",
	    passwd) > 0)
		return(0);

	return("authentication denied");
}

char *
login_approve (char *user, char *defclass)
{
	auth_session_t *as;
	struct passwd *pwd, _pwd;
	login_cap_t *lc;

	/*
	 * Find out who the user is.
	 * Use a fake entry for accounts that have no passwd entry
	 * And use the default class passed in with that accounts
	 * These accounts never expire.
	 */

	pwd = getpwnam(user);
	if ((lc = login_getclass(pwd ? pwd->pw_class : defclass)) == NULL)
		return("unable to locate login class entry for user");

	if (pwd == NULL) {
		memset(&_pwd, 0, sizeof(_pwd));
		pwd = &_pwd;
		pwd->pw_name = user;
	}

	if ((as = auth_open()) == NULL)
		return("Resource error in determining approval");

	auth_setpwd(as, pwd);

	if (auth_approval(as, lc, pwd->pw_name, "ppp") == NULL) {
		dprintf(D_DT, "       {approval script failed, account expired, no home dir or logins not allowed}\n");
		auth_close(as);
		return("login not approved");
	}

	auth_close(as);
	return (0);
}
