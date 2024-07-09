/*	BSDI sched.x,v 1.1 2000/12/08 03:38:25 donn Exp	*/

include "types.xh"
include "sched.xh"
include "signal.xh"

%{

int __bsdi_sfork(int, void *, int);

/*
 * This hack causes the return value of __clone()'s func argument
 * to magically appear as the argument to exit_stub().
 */
static void exit_stub(int) __attribute__((__regparm__(1)));

static void
exit_stub(int r)
{

	__bsdi_syscall(SYS_exit, r);
}

%}

/*
 * The library clone() call has a very different interface from
 * the kernel one.  We don't currently implement the kernel syscall,
 * since nothing appears to use it (yet) and the stack manipulation
 * would be moderately tricky.
 */
int __library___clone(void *func, void *stack, clone_flags_t flags, void *arg)
{
	void **st;
	int sig;
	int r;

	if (func == NULL || stack == NULL)
		return (__bsdi_error_retval(EINVAL));

	sig = signal_t_in((flags >> 16) & LINUX_CSIGNAL);
	flags &= ~(LINUX_CSIGNAL << 16);

	/*
	 * The child in __bsdi_sfork() simply clears EBP and returns,
	 * which pops the stack and transfers control to func().
	 * When func() returns, it transfers control to exit_stub().
	 */
	st = stack;
	*--st = arg;
	*--st = (void *)&exit_stub;
	*--st = func;

	return (__bsdi_sfork(flags, st, sig));
}

/*
 * We fake out some current state.
 * At some point, we will need to implement these features
 * in the BSD/OS scheduler.
 */
int __sched_get_priority_max(int policy) = 0;
int __sched_get_priority_min(int policy) = 0;
int __sched_getscheduler(pid_t pid) = 0;

int __sched_getparam(pid_t pid, struct sched_param *param)
{
	param->sched_priority = 0;
	return (0);
}

int __sched_setscheduler(pid_t pid, int policy,
    const struct sched_param *param) = 0;

/*
 * Currently, this call is the only one that's for real.
 */
int sched_yield();
