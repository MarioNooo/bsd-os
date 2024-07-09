/*	BSDI fcntl.h,v 1.1 1995/07/10 18:21:02 donn Exp	*/

#ifndef _SCO_FCNTL_H_
#define	_SCO_FCNTL_H_

/* from iBCS2 p6-35 - 6-36 */
#define	O_RDONLY	0x0000
#define	O_WRONLY	0x0001
#define	O_RDWR		0x0002
#define	O_NDELAY	0x0004
#define	O_APPEND	0x0008
#define	O_SYNC		0x0010
#define	O_NONBLOCK	0x0080
#define	O_CREAT		0x0100
#define	O_TRUNC		0x0200
#define	O_EXCL		0x0400
#define	O_NOCTTY	0x0800

#define	O_ACCMODE	0x0003

#define	FNDELAY		O_NDELAY	/* XXX needed by X11r5 */

#define	F_DUPFD		0
#define	F_GETFD		1
#define	F_SETFD		2
#define	F_GETFL		3
#define	F_SETFL		4
#define	F_GETLK		5
#define	F_SETLK		6
#define	F_SETLKW	7

struct flock {
	short	l_type;
	short	l_whence;
	long	l_start;
	long	l_len;
	short	l_sysid;
	short	l_pid;
};

typedef struct flock flock_t;

#define	F_RDLCK		1
#define	F_WRLCK		2
#define	F_UNLCK		3

#define	FD_CLOEXEC	1

/* prototypes required by POSIX.1 */

#include <sys/cdefs.h>

int creat __P((const char *, unsigned short));
#if 0
int fcntl __P((int, int, ...));
int open __P((const char *, int, ...));
#else
int fcntl();
int open();
#endif

#endif /* _SCO_FCNTL_H_ */
