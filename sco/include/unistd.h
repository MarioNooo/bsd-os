/*	BSDI unistd.h,v 1.1 1995/07/10 18:22:35 donn Exp	*/

#ifndef _SCO_UNISTD_H_
#define	_SCO_UNISTD_H_

#define	F_OK		0x00
#define	X_OK		0x01
#define	W_OK		0x02
#define	R_OK		0x04

#define	F_ULOCK		0
#define	F_LOCK		1
#define	F_TLOCK		2
#define	F_TEST		3

#define	SEEK_SET	0
#define	SEEK_CUR	1
#define	SEEK_END	2

#define	_SC_ARG_MAX		 0
#define	_SC_CHILD_MAX		 1
#define	_SC_CLK_TCK		 2
#define	_SC_NGROUPS_MAX		 3
#define	_SC_OPEN_MAX		 4
#define	_SC_JOB_CONTROL		 5
#define	_SC_SAVED_IDS		 6
#define	_SC_VERSION		 7

#define	_PC_LINK_MAX		 0
#define	_PC_MAX_CANON		 1
#define	_PC_MAX_INPUT		 2
#define	_PC_NAME_MAX		 3
#define	_PC_PATH_MAX		 4
#define	_PC_PIPE_BUF		 5
#define	_PC_CHOWN_RESTRICTED	 6
#define	_PC_NO_TRUNC		 7
#define	_PC_VDISABLE		 8

#define	STDIN_FILENO	0
#define	STDOUT_FILENO	1
#define	STDERR_FILENO	2

/* prototypes required by POSIX.1 */

#include <machine/ansi.h>

#ifdef _SCO_SIZE_T_
typedef _SCO_SIZE_T_ size_t;
#undef _SCO_SIZE_T_
#endif

#include <sys/cdefs.h>

void _exit __P((int));
int access __P((const char *, int));
unsigned int alarm __P((unsigned int));
int chdir __P((const char *));
int chown __P((const char *, int, int));
int close __P((int));
size_t confstr __P((int, char *, size_t));
int dup __P((int));
int dup2 __P((int, int));
int execl __P((const char *, const char *, ...));
int execle __P((const char *, const char *, ...));
int execlp __P((const char *, const char *, ...));
int execv __P((const char *, char * const *));
int execve __P((const char *, char * const *, char * const *));
int execvp __P((const char *, char * const *));
int fork __P((void));
long fpathconf __P((int, int));
char *getcwd __P((char *, size_t));
int getegid __P((void));
int geteuid __P((void));
int getgid __P((void));
int getgroups __P((int, unsigned short []));
char *getlogin __P((void));
int getpgrp __P((void));
short getpid __P((void));
int getppid __P((void));
int getuid __P((void));
int isatty __P((int));
int link __P((const char *, const char *));
long lseek __P((int, long, int));
long pathconf __P((const char *, int));
int pause __P((void));
int pipe __P((int *));
int read __P((int, char *, size_t));
int rmdir __P((const char *));
int setgid __P((int));
int setpgid __P((int, int));
int setsid __P((void));
int setuid __P((int));
unsigned int sleep __P((unsigned int));
long sysconf __P((int));
int tcgetpgrp __P((int));
int tcsetpgrp __P((int, int));
char *ttyname __P((int));
int unlink __P((const char *));
int write __P((int, const char *, size_t));

#endif	/* _SCO_UNISTD_H_ */
