/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI enter.c,v 2.2 1997/08/08 18:38:26 prb Exp
 */

#include <db.h>
#include <err.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>

#include "libpasswd.h"

/*
 * Enter a passwd line in both databases.  (Hides shadowing from caller.)
 */
char *
pwd_enter(pw, old, new)
	struct pwinfo *pw;
	struct passwd *old, *new;
{
	char *msg;

	if ((msg = pwd_ent1(pw, old, new, 0)) == NULL)
		msg = pwd_ent1(pw, old, new, 1);
	return (msg);
}
