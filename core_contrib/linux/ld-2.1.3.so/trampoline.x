/*	BSDI trampoline.x,v 1.2 2003/09/23 17:46:34 donn Exp	*/

include "types.xh"
include "signal.xh"

%{

#include <string.h>
#include <signal.h>
#include <machine/ucontext.h>
#include <machine/npx.h>

asm (
"	.section \".text\"\n"
"	.align 4\n"
"jmp_tramp:\n"
"	movl $0xffffffff,%eax\n"
"	jmp *%eax\n"
"end_jmp_tramp:");

static void c_jmp_tramp(void) __attribute__((__alias__("jmp_tramp")));
static void c_end_jmp_tramp(void) __attribute__((__alias__("end_jmp_tramp")));

/*
 * XXX STOLEN FROM /sys/i386/i386/machdep.c.
 */
struct normalsigframe {
	int	sf_signum;
	int	sf_code;
	struct	sigcontext *sf_scp;
	sig_t	sf_handler;

	int	sf_type;		/* sf_type == 0 */
	int	sf_eax;
	int	sf_edx;
	int	sf_ecx;
	struct	save87 sf_fpu;
	struct	sigcontext sf_sc;
};

struct siginfoframe {
	int             sf_signum;
	siginfo_t       *sf_siginfo;    /* Points to .sf_si */
	void            *sf_ucontext;   /* Points to .sf_uc */
	sig_t		sf_handler;
	int		sf_type;	/* sf_type == SA_SIGINFO */

	struct normalsigframe sf_normal;

	siginfo_t sf_si;
	ucontext_t sf_uc;
};

static void siginfo_translate(struct siginfoframe);

asm (
"	.section \".text\"\n"
"	.align 4\n"
"trampoline:\n"
"	cmpl $0,20(%esp)\n"
"	je 1f\n"
"	call siginfo_translate\n"
"	jmp 2f\n"
"1:	popl %eax\n"
"	movzbl 0xffffffff(,%eax,1),%eax\n"
"	pushl %eax\n"
"	call *12(%esp)\n"
"2:	movl $103,%eax\n"
"	lcall $7,$0\n"
"	hlt\n"
"end_trampoline:");

static void c_trampoline(void) __attribute__((__alias__("trampoline")));
static void c_end_trampoline(void) __attribute__((__alias__("end_trampoline")));

/* We copy the trampoline code to a writable buffer and edit it there.  */
static char trampoline_buffer[128];
static char signal_out[NSIG];

static caddr_t orig_tramp;

/*
 * Tricky...  We want the address of the signal trampoline,
 * so we send ourselves a signal and check the calling frame.
 */
static void
handler(int sig)
{

	/*
	 * Some magic (sigh)...  XXX
	 * We dig out that EIP and subtract a magic value
	 * to get the address of the start of the trampoline.
	 */
	orig_tramp = (caddr_t)*(&sig - 1) - 6;
}

/*
 * Call this function when the emulation library is loaded.
 * Note that we get called AFTER the C library has been loaded,
 * so we can call C library functions.
 */
static void init_trampoline(void) __attribute__((__constructor__));

/*
 * Install a replacement signal trampoline that transforms signal numbers.
 */
static void
init_trampoline(void)
{
	struct sigaction sa;
	struct sigaction osa;
	sigset_t omask;
	sigset_t mask;
	caddr_t addr_signal_out;
	caddr_t addr_trampoline_buffer;
	caddr_t v;
	size_t len;
	int i;

	/* Initialize the fast signal transformation table.  */
	for (i = 1; i < NSIG; ++i)
		signal_out[i] = signal_t_out(i);

	/*
	 * Initialize the trampoline code.
	 * We copy the trampoline instructions to a writable buffer,
	 * then update the pointer to the fast signal transformation table.
	 * We assume that the first instance of the 0xff byte
	 * is in the offset to the mov instruction that
	 * loads the translated signal number.
	 */
	len = (caddr_t)&c_end_trampoline - (caddr_t)&c_trampoline;
	memcpy(trampoline_buffer, &c_trampoline, len);
	v = (caddr_t)memchr(trampoline_buffer, 0xff, len);
	addr_signal_out = (caddr_t)&signal_out[0];
	memcpy(v, &addr_signal_out, sizeof (addr_signal_out));

	/*
	 * Send ourselves a signal to get the address of
	 * the trampoline code that the kernel installed for us.
	 * We use SIGALRM since it's fatal and gdb ignores it.
	 * In theory no signals are blocked or caught at this point,
	 * but we save and restore state anyway, because it's easy.
	 */
	sa.sa_handler = handler;
	sa.sa_mask = mask;
	sa.sa_flags = 0;
	__bsdi_syscall(SYS_sigaction, SIGALRM, &sa, &osa);
	SIGEMPTYSET(mask);
	SIGADDSET(mask, SIGALRM);
	__bsdi_syscall(SYS_sigprocmask, SIG_BLOCK, &mask, &omask);
	__bsdi_syscall(SYS_kill, __bsdi_syscall(SYS_getpid), SIGALRM);
	mask = omask;
	SIGDELSET(mask, SIGALRM);
	__bsdi_syscall(SYS_sigsuspend, &mask);
	__bsdi_syscall(SYS_sigaction, SIGALRM, &osa, NULL);
	__bsdi_syscall(SYS_sigprocmask, SIG_SETMASK, &omask, NULL);

	/*
	 * We want to replace the existing trampoline code
	 * with code that probably won't fit in the space provided,
	 * so we install a jump in place of the regular trampoline,
	 * with the target being the trampoline code that we copied.
	 * As above, we assume that the 0xff byte first appears in the offset.
	 */
	len = (caddr_t)&c_end_jmp_tramp - (caddr_t)&c_jmp_tramp;
	memcpy(orig_tramp, &c_jmp_tramp, len);
	v = (caddr_t)memchr(orig_tramp, 0xff, len);
	addr_trampoline_buffer = (caddr_t)&trampoline_buffer[0];
	memcpy(v, &addr_trampoline_buffer, sizeof (addr_trampoline_buffer));
}

/* Translate SA_SIGINFO frames.  */
static void
siginfo_translate(struct siginfoframe sf)
{
	struct linux_siginfo_t linux_siginfo;
	void (*handler)(int, struct linux_siginfo_t *, void *) = 
	    (void (*)(int, struct linux_siginfo_t *, void *))sf.sf_handler;

	siginfo_t_out(&sf.sf_si, &linux_siginfo, sizeof linux_siginfo);
	(*handler)(sf.sf_signum, &linux_siginfo, &sf.sf_uc);
}

%}
