/*	BSDI resource.x,v 1.3 2000/12/08 04:21:37 donn Exp	*/

/*
 * Resource.h syscall transformations.
 */

include "types.xh"
include "resource.xh"

int getpriority(int which, int who)
{

	/*
	 * Since the BSD getpriority() syscall may return a negative value,
	 * and the Linux errno convention uses negative values to return
	 * error numbers, we must implement this call in assembly.
	 */
	return (__bsdi_getpriority(which, who));
}

/* XXX Is this reasonable?  */
int getrlimit(LINUX_RLIMIT_AS, struct rlimit *rlp) {
	__bsdi_unimpl("getrlimit(RLIMIT_AS, struct rlimit *)\n");
	return (__bsdi_error_retval(EINVAL));
}
int getrlimit(rlimit_t resource, struct rlimit *rlp);

int getrusage(LINUX_RUSAGE_BOTH, struct rusage *rusage)
{
	struct rusage ruchildren;
	int r;

	if (__bsdi_error(r = __bsdi_syscall(RUSAGE_SELF, rusage)))
		return (r);
	if (__bsdi_error(__bsdi_syscall(RUSAGE_CHILDREN, &ruchildren)))
		return (0);

	rusage->ru_utime.tv_usec += ruchildren.ru_utime.tv_sec;
	while (rusage->ru_utime.tv_usec > 1000000) {
		rusage->ru_utime.tv_usec -= 1000000;
		++rusage->ru_utime.tv_sec;
	}
	rusage->ru_utime.tv_sec += ruchildren.ru_utime.tv_sec;
	rusage->ru_stime.tv_usec += ruchildren.ru_stime.tv_sec;
	while (rusage->ru_stime.tv_usec > 1000000) {
		rusage->ru_stime.tv_usec -= 1000000;
		++rusage->ru_stime.tv_sec;
	}
	rusage->ru_stime.tv_sec += ruchildren.ru_stime.tv_sec;
	rusage->ru_ixrss += ruchildren.ru_ixrss;
	rusage->ru_idrss += ruchildren.ru_idrss;
	rusage->ru_isrss += ruchildren.ru_isrss;
	rusage->ru_minflt += ruchildren.ru_minflt;
	rusage->ru_majflt += ruchildren.ru_majflt;
	rusage->ru_nswap += ruchildren.ru_nswap;
	rusage->ru_inblock += ruchildren.ru_inblock;
	rusage->ru_oublock += ruchildren.ru_oublock;
	rusage->ru_msgsnd += ruchildren.ru_msgsnd;
	rusage->ru_msgrcv += ruchildren.ru_msgrcv;
	rusage->ru_nsignals += ruchildren.ru_nsignals;
	rusage->ru_nvcsw += ruchildren.ru_nvcsw;
	rusage->ru_nivcsw += ruchildren.ru_nivcsw;

	return (0);
}
int getrusage(rusage_t who, struct rusage *rusage);

int setpriority(int which, int who, int prio);

int setrlimit(rlimit_t resource, const struct rlimit *rlp);
