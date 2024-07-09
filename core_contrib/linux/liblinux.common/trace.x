/*	BSDI trace.x,v 1.2 2003/09/23 17:46:34 donn Exp	*/

%{

#include <signal.h>
#include <unistd.h>

void
__bsdi_unimpl(const char *s)
{
	sigset_t ss;

	sigfillset(&ss);
	sigdelset(&ss, SIGABRT);

	__bsdi_syscall(SYS_write, STDERR_FILENO, s, strlen(s));
	__bsdi_syscall(SYS_kill, __bsdi_syscall(SYS_getpid), SIGABRT);
	__bsdi_syscall(SYS_sigsuspend, &ss);
}

%}
