/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI copy.c,v 2.2 1995/07/23 17:01:20 bostic Exp
 */
#include <pwd.h>
#include <stdlib.h>
#include <string.h>

static int
dupfield(f)
	char **f;
{
	return (*f == NULL || (*f = strdup(*f)) != NULL);
}

struct passwd *
pw_copy(pwd)
	struct passwd *pwd;
{
	struct passwd *new;

	if ((new = malloc(sizeof(struct passwd))) != NULL) {
		*new = *pwd;
		if (dupfield(&new->pw_name) == NULL ||
		    dupfield(&new->pw_passwd) == NULL ||
		    dupfield(&new->pw_class) == NULL ||
		    dupfield(&new->pw_gecos) == NULL ||
		    dupfield(&new->pw_dir) == NULL ||
		    dupfield(&new->pw_shell) == NULL)
			return (NULL);
	}
	return(new);
}
