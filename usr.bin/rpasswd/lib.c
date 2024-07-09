/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI lib.c,v 1.3 1997/10/09 17:25:32 tks Exp
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include "extern.h"
#include "pathnames.h"

static DB *db = NULL;
static int dbmode = 0;
static int nflag;

static DB *rpasswd_open(int);
static char *rpasswd_data(DBT *);

static DB *
rpasswd_open(int mode)
{
	if (db != NULL)
		rpasswd_close();
	if (mode & ~(O_RDONLY | O_RDWR | O_CREAT)) {
		errno = EINVAL;
		return (NULL);
	}
	mode |= (mode & O_RDWR) ? O_EXLOCK : O_SHLOCK;

	db = dbopen(_PATH_RPASSWD, mode, S_IRUSR | S_IWUSR, DB_BTREE, NULL);
	if (db != NULL) {
		dbmode = mode & ~(O_EXLOCK | O_SHLOCK);
		nflag = R_FIRST;
	}
	return (db);
}

int
rpasswd_close(void)
{
	int rval = 0;

	if (db != NULL)
		rval = db->close(db);
	dbmode = 0;
	db = NULL;
	return (rval);
}

char *
rpasswd_next(char **datap)
{
	DBT nd, dd;
	char *data, *user;
	
	if (db == NULL && rpasswd_open(O_RDONLY) == NULL)
		return (NULL);

	user = NULL;
	if (db->seq(db, &nd, &dd, nflag) == 0) {
		nflag = R_NEXT;
		if ((user = rpasswd_data(&nd)) == NULL ||
		    (data = rpasswd_get(user)) == NULL)
			return (NULL);
		if (datap != NULL)
			*datap = data;
	}
	return (user);
}

void
rpasswd_rewind(void)
{
	nflag = R_FIRST;
}

char *
rpasswd_get(char *name)
{
	DBT dd, nd;

	if (db == NULL && rpasswd_open(O_RDONLY) == NULL)
		return (NULL);

	nd.data = name;
	nd.size = strlen(name) + 1;

	switch (db->get(db, &nd, &dd, 0)) {
	case -1:
		warn("%s: get failed", name);
		return (NULL);
	case 0:
		return (rpasswd_data(&dd));
	default:
		return (NULL);
	}
}

int
rpasswd_delete(char *name)
{
	DBT nd;

	if (db != NULL && dbmode != (O_RDWR | O_CREAT))
		rpasswd_close();

	if (db == NULL && rpasswd_open(O_RDWR | O_CREAT) == NULL)
		return (-1);

	nd.data = name;
	nd.size = strlen(name) + 1;

	switch (db->del(db, &nd, 0)) {
	case -1:
		return (-1);
	case 0:
		return (0);
	default:
		errno = ENOENT;
		return (-1);
	}
}

int
rpasswd_insert(char *name, char *data)
{
	DBT dd, nd;

	if (db != NULL && dbmode != (O_RDWR | O_CREAT))
		rpasswd_close();

	if (db == NULL && rpasswd_open(O_RDWR | O_CREAT) == NULL)
		return (NULL);

	nd.data = name;
	nd.size = strlen(name) + 1;
	dd.data = data;
	dd.size = strlen(data) + 1;
	return (db->put(db, &nd, &dd, 0));
}

static char *
rpasswd_data(DBT *d)
{
	char *data;

	if (d->size) {
		data = d->data;
		if (data[d->size-1] != '\0' &&
		    (d->size <= 11 || data[d->size-11] != '\0'))
			return (NULL);
		return (data);
	}
	return ("");
}
