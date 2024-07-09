/*	BSDI trap.x,v 1.2 2003/09/23 17:46:34 donn Exp	*/

%{

/*
 * Handle the built-in syscall trap in _start().
 * XXX Deal with machine dependencies?
 */

#include <sys/types.h>
#include <machine/npx.h>
#include <sys/syscall.h>
#include <signal.h>
#include <stddef.h>

/* XXX Extracted from the kernel.  */
struct sigframe {
	int	sf_signum;
	int	sf_code;
	struct	sigcontext *sf_scp;
	sig_t	sf_handler;
	int	sf_eax;
	int	sf_edx;
	int	sf_ecx;
	struct	save87 sf_fpu;
	struct	sigcontext sf_sc;
};

#include "../D/asm/unistd.h"

#define	INT80_0		0xcd
#define	INT80_1		0x80

static struct sigaction saved_sigsegv_handler;

static void install_sigsegv_handler() __attribute__ ((__constructor__));
static void sigsegv_handler(struct sigframe);

static void
install_sigsegv_handler()
{
	struct sigaction sa;

	sa.sa_handler = (sig_t)sigsegv_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	__bsdi_syscall(SYS_sigaction, SIGSEGV, &sa, &saved_sigsegv_handler);
}

static void
sigsegv_handler(struct sigframe sf)
{
	struct sigaction sa;
	sigset_t ss;
	u_char *cp;

	/* Restore the original SIGSEGV state.  */
	__bsdi_syscall(SYS_sigaction, SIGSEGV, &saved_sigsegv_handler, NULL);

	/* If we weren't blocking SIGSEGV before, then unblock it.  */
	if (!sigismember(&sf.sf_sc.sc_mask, SIGSEGV)) {
		sigemptyset(&ss);
		sigaddset(&ss, SIGSEGV);
		__bsdi_syscall(SYS_sigprocmask, SIG_UNBLOCK, &ss, NULL);
	}

	cp = (u_char *)sf.sf_sc.sc_pc;
	if (cp[0] != INT80_0 || cp[1] != INT80_1)
		return;

	switch (sf.sf_eax) {
	case __NR_personality:
		sf.sf_eax = 0;
		break;
	case __NR_brk:
		/*
		 * XXX - the brk value is in ebx, which we do not have.
		 *     The only program I have seen that needs this does
		 *     a brk of 0.
		 */
		sf.sf_eax = brk(0);
		break;
	default:
		/* Re-execute the INT instruction without the handler.  */
		return;
	}

	/*
	 * Re-install the handler, there can be more than one call
	 */
	sa.sa_handler = (sig_t)sigsegv_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	__bsdi_syscall(SYS_sigaction, SIGSEGV, &sa, &saved_sigsegv_handler);

	/* Skip the INT instruction.  */
	sf.sf_sc.sc_pc += 2;
}

%}
