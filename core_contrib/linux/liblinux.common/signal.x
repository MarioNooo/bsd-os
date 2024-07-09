/*	BSDI signal.x,v 1.5 2003/09/23 17:46:34 donn Exp	*/

/*
 * Signal.h function transforms.
 */

include "types.xh"
include "signal.xh"

int kill(pid_t pid, signal_t sig);

int sigqueue(pid_t pid, signal_t sig, int value);

int sigwaitinfo(const struct sigset *ss, struct siginfo_t *si);

int sigtimedwait(const struct sigset *ss, struct siginfo_t *si,
    const struct timespec *ts);

/*
 * For 'emulation' of Linux-only signals, we simply use static variables.
 * This won't look right after we exec, but it's unlikely to be a problem.
 * XXX NOT RE-ENTRANT!
 */
int sigaction(LINUX_SIGSTKFLT, const struct sigaction *sa,
    struct sigaction *osa)
{
	static struct sigaction sa_sigstkflt;

	if (osa != NULL)
		*osa = sa_sigstkflt;
	if (sa != NULL)
		sa_sigstkflt = *sa;
	return (0);
}

int sigaction(LINUX_SIGPWR, const struct sigaction *sa, struct sigaction *osa)
{
	static struct sigaction sa_sigpwr;

	if (osa != NULL)
		*osa = sa_sigpwr;
	if (sa != NULL)
		sa_sigpwr = *sa;
	return (0);
}

int sigaction(signal_t sig, const struct sigaction *sa, struct sigaction *osa);

/* XXX Does the library use SIGSTKFLT or SIGPWR internally?  */
int __kernel_sigaction(signal_t sig, const struct kernel_sigaction *sa,
    struct kernel_sigaction *osa);

int sigpending(struct sigset *ss);

int sigprocmask(sigprocmask_t op, const struct sigset *ss, struct sigset *oss);

int sigsuspend(const struct sigset *ss);

/*
 * XXX int sigreturn(struct sigcontext *scp);
 */
