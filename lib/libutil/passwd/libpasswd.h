/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI libpasswd.h,v 2.3 1997/08/08 18:38:27 prb Exp
 */

#ifndef	_LIBPASSWD_H_
#define	_LIBPASSWD_H_

#include <db.h>
#include <stdio.h>

struct pwf {
	char	*pf_name;	/* name (e.g., for error messages) */
	union {
		FILE	*un_fp;	/* stdio file */
		DB	*un_db;	/* or database, whichever is appropriate */
	} pf_un;
#define	pf_fp	pf_un.un_fp
#define	pf_db	pf_un.un_db
};

struct pwinfo {
	volatile int pw_flags;	/* below */
	int	pw_line;	/* line in master.passwd */
	struct	pwf pw_lock;	/* lock info */
	struct	pwf pw_master;	/* master.passwd info */
	struct	pwf pw_new;	/* new master.passwd, if one is being built */
	struct	pwf pw_idb;	/* insecure database (pwd.db) */
	struct	pwf pw_sdb;	/* secure database (spwd.db) */
	struct	pwf pw_old;	/* `old style' passwd file, if desired */
};

				/* these flags are for pw_init */
#define	PW_MAKEOLD	0x0001	/* make old-style passwd file */
#define	PW_STRIPDIR	0x0002	/* work in . instead of /etc */
#define	PW_WARNROOT	0x0004	/* warn about unusual "root" entry */
#define	PW_NOLINK	0x0008	/* don't link to master.passwd */
#define	PW_SILENT	0x0100	/* Do not display verbose messages */
#define	PW_NONBLOCK	0x0200	/* Must not block on lock */

#define	PW__USER	(PW_MAKEOLD | PW_STRIPDIR | PW_WARNROOT | PW_NOLINK | PW_SILENT | PW_NONBLOCK)

				/* these flags are shared */
#define	PW_RMIDB	0x0010	/* need to remove temporary pw_idb */
#define	PW_RMSDB	0x0020	/* need to remove temporary pw_sdb */
#define	PW_RMOLD	0x0040	/* need to remove temporary pw_old */
#define	PW_RMNEW	0x0080	/* need to remove temporary pw_new */


				/* these flags are for internal use only */
#define	PW_LOCKED	0x1000	/* password file is locked */
#define	PW_INPLACE	0x2000	/* update is in-place */
#define	PW_REBUILD	0x4000	/* update is complete rebuild */

#define	PW_PERM_INSECURE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define	PW_PERM_SECURE		(S_IRUSR | S_IWUSR)

#include <sys/cdefs.h>

struct passwd;
struct sigaction;

__BEGIN_DECLS
void	pw_abort __P((struct pwinfo *));
struct passwd *
	pw_copy __P((struct passwd *));
int	pw_edit __P((char *));
void	pw_init __P((struct pwinfo *, int));
int	pw_lock __P((struct pwinfo *, int));
int	pw_prompt __P((void));
void	pw_sigs __P((sigset_t *, struct sigaction *));
char   *pw_split __P((char *, struct passwd *, int));
void	pw_unlimit __P((void));
void	pw_unlock __P((struct pwinfo *));

/* Internal use only! */
char   *pwd_enter __P((struct pwinfo *, struct passwd *, struct passwd *));
char   *pwd_install __P((struct pwinfo *));
char   *pwd_inplace __P((struct pwinfo *));
char   *pwd_ipend __P((struct pwinfo *));
char   *pwd_next __P((struct pwinfo *, struct passwd *));
char   *pwd_rebuild __P((struct pwinfo *, char *, u_int));
int	pwd_tmp __P((struct pwinfo *, char *, int, char **));
char   *pwd_update __P((struct passwd *, struct passwd *, int));

char   *pwd_ent1 __P((struct pwinfo *, struct passwd *, struct passwd *, int));
char   *pwd_path __P((struct pwinfo *, char *, char *, char **));
__END_DECLS

#endif /* !_LIBPASSWD_H_ */
