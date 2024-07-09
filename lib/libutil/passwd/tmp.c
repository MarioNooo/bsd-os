/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI tmp.c,v 2.2 1997/08/08 18:38:28 prb Exp
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <db.h> 
#include <err.h> 
#include <errno.h> 
#include <pwd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
		
#include "libpasswd.h"
static char errmsg[1024];
		
int
pwd_tmp(pw, path, plen, msg)
	struct pwinfo *pw;
	char *path;
	int plen;
	char **msg;
{
	int fd;
	char *p; 
	
	if ((pw->pw_flags & PW_STRIPDIR) &&
	    (p = strrchr(_PATH_MASTERPASSWD, '/')) != NULL)
		++p;
	else
		p = _PATH_MASTERPASSWD;

    	strncpy(path, p, plen);

	if (p = strrchr(path, '/'))
		++p;
	else
		p = path;
	strncpy(p, "pw.XXXXXX", plen - (p - path));
	if ((fd = mkstemp(path)) == -1) {
		snprintf(errmsg, sizeof(errmsg), "%s: %s", path,
		    strerror(errno));
		*msg = errmsg;
		return (-1);
	}
    	(void)fchmod(fd, PW_PERM_SECURE);
	*msg = NULL;
	return (fd);
}
