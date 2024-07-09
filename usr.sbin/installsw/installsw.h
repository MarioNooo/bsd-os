/*-
 * Copyright (c) 1993, 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI installsw.h,v 2.12 1999/01/15 20:06:00 jch Exp
 */

#define F_NOFILE	'X'

#define PACKAGEDIR	"PACKAGES"
#define PACKAGEFILE	"PACKAGES"
#define	SCRIPTFILE	"INSTALLSW-SCRIPTS"

#define TMPMAP		"/var/tmp/installsw.maptmp"
#define	TMPPACK		"/var/tmp/installsw.packages"

#define STATUS		"/var/tmp/installsw.status"

#define	SCRIPT_DIR	"/var/tmp/installsw.script"
#define	SCRIPT_DELETE	"delete"
#define	SCRIPT_INFO	"information"
#define	SCRIPT_BEGIN	"begin_install"	/* Before all installation. */
#define	SCRIPT_END	"end_install"	/* After all installation. */
#define	SCRIPT_POST	"post_install"	/* After each package installation. */
#define	SCRIPT_PRE	"pre_install"	/* Before each package installation. */

/* Argument strings to pax should specify stdin as archive file. */
#define PAX_ARGS	"-r -v -pe"

#define TAPESLEEP	5		/* Sleep span between tape ops. */

typedef struct _pkg {
	TAILQ_ENTRY(_pkg) q;		/* Linked list of packages. */
	size_t size;	/* Size (in kbytes). */
	long file;	/* File number on the tape. */
	int  letter;	/* Selection character. */

	int  present;	/* 1 -> currently installed */
	int  previous;	/* 1 -> installed when installsw started. */
	int  script;	/* 1 -> run the scripts. */
	int  selected;	/* 1 -> chosen to install. */
	int  update;	/* 1 -> sentinel present, but not logged. */

#define P_DESIRABLE	'D'
#define P_HIDDEN	'H'
#define P_INITIAL	'I'
#define P_OPTIONAL	'O'
#define P_REQUIRED	'R'
	int  pref;	/* P_ flags. */

#define T_COMPRESSED	'Z'
#define T_GZIPPED	'z'
#define T_NORMAL	'N'
	int  type;	/* T_ flags. */

	char *desc;	/* nice printable name of the package */
	char *name;	/* name of this package */
	char *root;	/* root of the tree */
	char *sentinel;	/* existence -> this package is installed */
} PKG;
typedef struct _pkgh	PKGH;
TAILQ_HEAD(_pkgh, _pkg);

/* Globals and defaults. */
extern PKG     *spkg;			/* Script PKG file. */
extern int      devset;			/* If device path set. */
extern int	do_delete;		/* Delete mode. */
extern int	do_update;		/* Update mode. */
extern int	expert;			/* If the user is expert. */
extern int	express;		/* If doing an express install. */
extern int	tfileno;		/* Current tape file number. */
extern char    *remote_host;		/* Remote host. */
extern char    *remote_user;		/* Remote user. */

typedef enum { L_NONE=0, L_LOCAL, L_REMOTE } loc_t;
extern loc_t location;

typedef enum { S_NONE=0, S_DESIRABLE, S_OPTIONAL, S_REQUIRED, S_UPDATE } sel_t;
extern sel_t comsel;

#ifndef DEFAULT_MEDIA
#define DEFAULT_MEDIA	"cdrom"
#endif
typedef enum { M_NONE=0, M_CDROM, M_FLOPPY, M_TAPE } media_t;
extern media_t media;

#ifndef CD_MOUNTPT
#define CD_MOUNTPT	"/cdrom"
#endif
#ifndef FL_MOUNTPT
#define FL_MOUNTPT	"/a"
#endif
#ifndef TAPE_DEVICE
#define	TAPE_DEVICE	"/dev/nrst0"
#endif
extern char   *device;			/* CDROM/floppy mount, tape device. */ 

#ifndef DEF_ROOT
#define DEF_ROOT	"/"
#endif
extern char   *rootdir;			/* Root directory. */

/* Logging strings. */
#define	LOG_DELETE	"deleted"
#define	LOG_DFAILED	"delete FAILED"
#define	LOG_IFAILED	"install FAILED"
#define	LOG_INSTALL	"installed"

char	*build_extract_command __P((PKG *, char *, char **, int));
void	 clean_add __P((char *));
void	 clean_up __P((void));
void	 cur_end __P((void));
int	 cur_inter __P((PKGH *));
int	 delete __P((PKG *));
int	 determine_name_mapping __P((void));
int	 get_remote_file __P((char *, char *));
char	*hide_meta __P((char *));
int	 install __P((PKG *, char *, int));
void	 log __P((PKG *, char *));
char	*map_name __P((char *));
void	 mkdirp __P((char *));
void	 onintr __P((int));
int	 remote_local_command __P((char *, char *, int));
int	 run_script __P((PKG *, char *, char *));
void	 setup __P((void));
void	 tape_rew __P((void));
int	 tape_setup __P((int, int));
int	 yesno __P((char *));
