/*	BSDI ppp.h,v 2.13 1997/11/06 20:43:39 chrisk Exp	*/

/*
 * Includes
 */
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <setjmp.h>
#include <unistd.h>
#include <errno.h>
#include "pathnames.h"

typedef struct session_t {
	char	*sysname;	/* name of system */
	char	*sysfile;	/* name of ppp.sys file */
	char	ifname[30];	/* name of interface being used */
	char	pifname[30];	/* name of parallel interface being used */
	char	*buf;		/* capabilities buffer */
	int	fd;
	char	*proxy;		/* proxy to dial through */
	char	*device;
	int	speed;
	char	*sttymodes;
	int	interface;
	int	flags;
	int	oldld;

#define	F_SLIP		0x0001	/* This is a slip link */
#define	F_IMMEDIATE	0x0002	/* Immediate Connection */
#define	F_DIALIN	0x0004	/* Dialin */
#define	F_DIALOUT	0x0008	/* Dialout */
#define	F_DIRECT	0x0010	/* Direct connection */
#define	F_NOLASTLOG	0x0020	/* Don't make a lastlog entry */

	char	*linkup;
	char	*linkdown;
	char	*linkinit;
	char	*linkfailed;
	char	*linkoptions;
	char	*linkopts[64];
	char	**destinations;
	char	**nextdst;

	void	*private;	/* procotol specific */
} session_t;

extern int Socket;      /* control socket */
extern char *sysname;   /* system name */

char *uucplock;         /* UUCP lock file name */

extern int debug;

/* Functions */
extern char *printable(char *);
extern char *Dial(session_t *);

extern char *tgetstr();

#define	equal(a, b)	(strcmp(a,b)==0)/* A nice function to string compare */

#define value(x) x
#define number(x) x
#define boolean(x) x

#define VERBOSE debug
#define DIALTIMEOUT 60
#define BAUDRATE BR
#define	WAITTIMEOUT 60
