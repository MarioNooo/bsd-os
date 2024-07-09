/*	BSDI liblinux.h,v 1.1 2000/12/08 03:58:49 donn Exp	*/

#define	LINUX_PZERO		20

#ifndef ASSEMBLY

/*
 * Here we declare all of the optional functions in bsdi.s.
 * Declarations for mandatory functions are inserted by transform.
 */

pid_t __bsdi_fork(void);
int __bsdi_getpriority(int, int);
int __bsdi_pipe(int *);
int __bsdi_sigprocmask(int, const void *, void *);
int __bsdi_vfork(void);

#else

/* XXX Unfortunately <signal.h> and <sys/sfork.h> don't support -DLOCORE.  */
#define	SIGCHLD		20
#define	SF_WAITCHILD	0x80000000

#endif
