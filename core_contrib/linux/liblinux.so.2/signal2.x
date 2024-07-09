/*	BSDI signal2.x,v 1.4 2001/01/23 04:04:13 donn Exp	*/

/*
 * Signal.h material that's new in glibc 2.1.2.
 */

include "types.xh"
include "signal.xh"

int sigaltstack(const struct sigaltstack *sas, struct sigaltstack *osas);

/* Force the library to fall back on emulation.  */
int __kernel_rt_sigaction() = ENOSYS;
int __kernel_rt_sigpending() = ENOSYS;
int __kernel_rt_sigprocmask() = ENOSYS;
int __kernel_rt_sigsuspend() = ENOSYS;
