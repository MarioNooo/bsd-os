/*	BSDI tokendb.c,v 1.2 2001/05/31 17:30:38 prb Exp */
/*-
//////////////////////////////////////////////////////////////////////////
//									//
//   Copyright (c) 1995 Migration Associates Corp. All Rights Reserved	//
//									//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MIGRATION ASSOCIATES	//
//	The copyright notice above does not evidence any   		//
//	actual or intended publication of such source code.		//
//									//
//	Use, duplication, or disclosure by the Government is		//
//	subject to restrictions as set forth in FAR 52.227-19,		//
//	and (for NASA) as supplemented in NASA FAR 18.52.227-19,	//
//	in subparagraph (c) (1) (ii) of Rights in Technical Data	//
//	and Computer Software clause at DFARS 252.227-7013, any		//
//	successor regulations or comparable regulations of other	//
//	Government agencies as appropriate.				//
//									//
//		Migration Associates Corporation			//
//			19935 Hamal Drive				//
//			Monument, CO 80132				//
//									//
//	A license is granted to Berkeley Software Design, Inc. by	//
//	Migration Associates Corporation to redistribute this software	//
//	under the terms and conditions of the software License		//
//	Agreement provided with this distribution.  The Berkeley	//
//	Software Design Inc. software License Agreement specifies the	//
//	terms and conditions for redistribution.			//
//									//
//	UNDER U.S. LAW, THIS SOFTWARE MAY REQUIRE A LICENSE FROM	//
//	THE U.S. COMMERCE DEPARTMENT TO BE EXPORTED.  IT IS YOUR	//
//	REQUIREMENT TO OBTAIN THIS LICENSE PRIOR TO EXPORTATION.	//
//									//
//////////////////////////////////////////////////////////////////////////
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <db.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "token.h"
#include "tokendb.h"

static	DB	*tokendb;

/*
 * Static function prototypes
 */

static	int	tokendb_open(void);
static	void	tokendb_close(void);

/*
 * Retrieve a user record from the token database file
 */

extern	int
tokendb_getrec(char *username, TOKENDB_Rec *tokenrec)
{
	DBT	key;
	DBT	data;
	int	status = 0;

	key.data = username;
	key.size = strlen(username) + 1;
	memset(&data, 0, sizeof(data));

	if (tokendb_open())
		return(-1);

	status = (tokendb->get)(tokendb, &key, &data, 0);

	switch (status) {
	case 1:
		tokendb_close();
		return(ENOENT);
	case -1:
		tokendb_close();
		return(-1);
	}
	memcpy(tokenrec, data.data, sizeof(TOKENDB_Rec));
	if ((tokenrec->flags & TOKEN_USEMODES) == 0)
		tokenrec->mode = tt->modes & ~TOKEN_RIM;
	tokendb_close();
	return (0);
}

/*
 * Put a user record to the token database file.
 */

extern	int
tokendb_putrec(char *username, TOKENDB_Rec *tokenrec)
{
	DBT	key;
	DBT	data;
	int	status = 0;

	key.data = username;
	key.size = strlen(username) + 1;

	if (tokenrec->mode)
		tokenrec->flags |= TOKEN_USEMODES;
	data.data = tokenrec;
	data.size = sizeof(TOKENDB_Rec);

	if (!tokendb_open()) {
		if (flock((tokendb->fd)(tokendb), LOCK_EX)) {
			tokendb_close();
			return (-1);
		}
		status = (tokendb->put)(tokendb, &key, &data, 0);
	}
	tokendb_close();
	return (status);
}

/*
 * Remove a user record from the token database file.
 */

extern	int
tokendb_delrec(char *username)
{
	DBT	key;
	DBT	data;
	int	status = 0;

	key.data = username;
	key.size = strlen(username) + 1;
	memset(&data, 0, sizeof(data));

	if (!tokendb_open()) {
		if (flock((tokendb->fd)(tokendb), LOCK_EX)) {
			tokendb_close();
			return (-1);
		}
		status = (tokendb->del)(tokendb, &key, 0);
	}
	tokendb_close();
	return (status);
}

/*
 * Open the token database.  In order to expedite access in
 * heavily loaded conditions, we employ a N1 lock method.
 * Updates should be brief, so all locks wait infinitely.
 * Wait for a read (shared) lock as all updates read first.
 */

static	int
tokendb_open(void)
{
	int	must_set_perms = 0;
	struct	stat	statb;

	if (stat(tt->db, &statb) < 0) {
		if (errno != ENOENT)
			return (-1);
		must_set_perms++;
	} else {
		if (statb.st_uid != 0 || statb.st_gid != 0) {
#ifdef PARANOID
			printf("Authentication disabled\n");
			fflush(stdout);
			syslog(LOG_ALERT,
			    "POTENTIAL COMPROMISE of %s. Owner was %d, "
			    "Group was %d", tt->db, statb.st_uid, statb.st_gid);
			return (-1);
#else
			must_set_perms++;
#endif
		}
		if ((statb.st_mode & 0777) != 0600) {
#ifdef PARANOID
			printf("Authentication disabled\n");
			fflush(stdout);
			syslog(LOG_ALERT,
			    "POTENTIAL COMPROMISE of %s. Mode was %o",
			    tt->db, statb.st_mode);
			return (-1);
#else
			must_set_perms++;
#endif
		}
	}
	if (!(tokendb =
	    dbopen(tt->db, O_CREAT | O_RDWR, 0600, DB_BTREE, 0)) )
		return (-1);

	if (flock((tokendb->fd)(tokendb), LOCK_SH)) {
		(tokendb->close)(tokendb);
		return (-1);
	}
	if (must_set_perms && chown(tt->db, 0, 0))
		syslog(LOG_INFO,
		    "Can't set owner/group of %s errno=%m", tt->db);

	return (0);
}

/*
 * Close the token database.  We are holding an unknown lock.
 * Release it, then close the db. Since little can be done 
 * about errors, we ignore them.
 */

static	void
tokendb_close(void)
{
	(void)flock((tokendb->fd)(tokendb), LOCK_UN);
	(tokendb->close)(tokendb);
}

/*
 * Retrieve the first user record from the database, leaving the
 * database open for the next retrieval. If the march thru the
 * the database is aborted before end-of-file, the caller should
 * call tokendb_close to release the read lock.
 */

extern	int
tokendb_firstrec(int reverse_flag, TOKENDB_Rec *tokenrec)
{
	DBT	key;
	DBT	data;
	int	status = 0;

	memset(&data, 0, sizeof(data));

	if (!tokendb_open()) {
		status = (tokendb->seq)(tokendb, &key, &data,
				reverse_flag ? R_LAST : R_FIRST);
	}
	if (status) {
		tokendb_close();
		return (status);
	}
	if (!data.data) {
		tokendb_close();
		return (ENOENT);
	}
	memcpy(tokenrec, data.data, sizeof(TOKENDB_Rec));
	if ((tokenrec->flags & TOKEN_USEMODES) == 0)
		tokenrec->mode = tt->modes & ~TOKEN_RIM;
	return (0);
}

/*
 * Retrieve the next sequential user record from the database. Close
 * the database only on end-of-file or error.
 */


extern	int
tokendb_nextrec(int reverse_flag, TOKENDB_Rec *tokenrec)
{
	DBT	key;
	DBT	data;
	int	status;

	memset(&data, 0, sizeof(data));

	status = (tokendb->seq)(tokendb, &key, &data, 
		reverse_flag ? R_PREV : R_NEXT);

	if (status) {
		tokendb_close();
		return (status);
	}
	if (!data.data) {
		tokendb_close();
		return (ENOENT);
	}
	memcpy(tokenrec, data.data, sizeof(TOKENDB_Rec));
	if ((tokenrec->flags & TOKEN_USEMODES) == 0)
		tokenrec->mode = tt->modes & ~TOKEN_RIM;
	return (0);
}

/*
 * Retrieve and lock a user record.  Since there are no facilities in
 * BSD for record locking, we hack a bit lock into the user record.
 */

extern	int
tokendb_lockrec(char *username, TOKENDB_Rec *tokenrec, unsigned recflags)
{
	DBT	key;
	DBT	data;
	int	status;

	key.data = username;
	key.size = strlen(username) + 1;
	memset(&data, 0, sizeof(data));

	if (tokendb_open())
		return(-1);

	if (flock((tokendb->fd)(tokendb), LOCK_EX)) {
		tokendb_close();
		return(-1);
	}
	switch ((tokendb->get)(tokendb, &key, &data, 0)) {
	case 1:
		tokendb_close();
		return (ENOENT);
	case -1:
		tokendb_close();
		return(-1);
	}
	memcpy(tokenrec, data.data, sizeof(TOKENDB_Rec));

	if ((tokenrec->flags & TOKEN_LOCKED)||(tokenrec->flags & recflags)) {
		tokendb_close();
		return(1);
	}
	data.data = tokenrec;
	data.size = sizeof(TOKENDB_Rec);

	time(&tokenrec->lock_time);
	tokenrec->flags |= recflags;
	status = (tokendb->put)(tokendb, &key, &data, 0);
	tokendb_close();
	if (status)
		return(-1);
	if ((tokenrec->flags & TOKEN_USEMODES) == 0)
		tokenrec->mode = tt->modes & ~TOKEN_RIM;

	return(0);
}

