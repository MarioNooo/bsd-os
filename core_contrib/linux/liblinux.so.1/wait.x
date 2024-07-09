/*	BSDI wait.x,v 1.1.1.1 1998/12/23 19:57:20 prb Exp	*/

/*
 * Wait.h function transforms.
 * In libc5, all the wait variants appear to use inline syscalls.
 */

include "types.xh"
include "resource.xh"

/* XXX Old versions of BSD returned status in EDX; does Linux do this?  */

int wait(int *status)
{
	return (__bsdi_syscall(SYS_wait4, -1, status, 0, NULL));
}

int wait3(int *status, int options, struct rusage *rup)
{
	return (__bsdi_syscall(SYS_wait4, -1, status, options, rup));
}

int waitpid(pid_t pid, int *status, int options)
{
	return (__bsdi_syscall(SYS_wait4, pid, status, options, NULL));
}

int wait4(pid_t p, int *status, int options, struct rusage *rup);
