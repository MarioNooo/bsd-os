/*
 * Copyright (c) 1993,1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_signal.c,v 2.2 1995/11/29 02:22:50 donn Exp
 */

#include <sys/param.h>
#include <sys/signal.h>
#include <machine/npx.h>	/* XXX */
#include <err.h>
#include <errno.h>

#include "emulate.h"
#include "sco.h"
#include "sco_sig_state.h"

/*
 * Support for SCO signal interfaces.
 */

/* from iBCS2 p 6-50 */
const int procmask_in_map[] = {
	SIG_SETMASK,
	SIG_BLOCK,
	SIG_UNBLOCK,
};
const int max_procmask_in_map = sizeof (procmask_in_map) / sizeof (int);

/*
 * Signal numbers and masks.
 */

/* from signal(5) in the SVr4.2 SFDR p 356 */
#define	SCO_SIGUSR1	16
#define	SCO_SIGUSR2	17
#define	SCO_SIGCLD	18
#define	SCO_SIGPWR	19
#define	SCO_SIGWINCH	20
#define	SCO_SIGURG	21
#define	SCO_SIGPOLL	22
#define	SCO_SIGSTOP	23
#define	SCO_SIGTSTP	24
#define	SCO_SIGCONT	25
#define	SCO_SIGTTIN	26
#define	SCO_SIGTTOU	27
#define	SCO_SIGVTALRM	28
#define	SCO_SIGPROF	29
#define	SCO_SIGXCPU	30
#define	SCO_SIGXFSZ	31

#define	SIGPWR_DUMMY	32

const int sig_in_map[NSIG] = {
	0,
	SIGHUP,
	SIGINT,
	SIGQUIT,
	SIGILL,
	SIGTRAP,
	SIGABRT,
	SIGEMT,
	SIGFPE,
	SIGKILL,
	SIGBUS,
	SIGSEGV,
	SIGSYS,
	SIGPIPE,
	SIGALRM,
	SIGTERM,
	SIGUSR1,
	SIGUSR2,
	SIGCHLD,
	SIGPWR_DUMMY,
	SIGWINCH,
	SIGURG,
	SIGIO,		/* signal(5) in SVr4.2 SFDR p 357 */
	SIGSTOP,
	SIGTSTP,
	SIGCONT,
	SIGTTIN,
	SIGTTOU,
	SIGVTALRM,
	SIGPROF,
	SIGXCPU,
	SIGXFSZ,
};
const int max_sig_in_map = NSIG;

/* used in signal glue code, to fix the signal argument to the handler */
const int sig_out_map[NSIG] = {
	0,
	SIGHUP,
	SIGINT,
	SIGQUIT,
	SIGILL,
	SIGTRAP,
	SIGABRT,
	SIGEMT,
	SIGFPE,
	SIGKILL,
	SIGBUS,
	SIGSEGV,
	SIGSYS,
	SIGPIPE,
	SIGALRM,
	SIGTERM,
	SCO_SIGURG,
	SCO_SIGSTOP,
	SCO_SIGTSTP,
	SCO_SIGCONT,
	SCO_SIGCLD,
	SCO_SIGTTIN,
	SCO_SIGTTOU,
	SCO_SIGPOLL,
	SCO_SIGXCPU,
	SCO_SIGXFSZ,
	SCO_SIGVTALRM,
	SCO_SIGPROF,
	SCO_SIGWINCH,
	0,		/* no equivalent for SIGINFO */
	SCO_SIGUSR1,
	SCO_SIGUSR2,
};
const int max_sig_out_map = NSIG;

static unsigned long const sigset_in_bits[] = {
	sigmask(SCO_SIGCONT),	sigmask(SIGCONT),
	sigmask(SCO_SIGTSTP),	sigmask(SIGTSTP),
	sigmask(SCO_SIGWINCH),	sigmask(SIGWINCH),
	sigmask(SCO_SIGCLD),	sigmask(SIGCHLD),
	sigmask(SCO_SIGUSR1),	sigmask(SIGUSR1),
	sigmask(SCO_SIGUSR2),	sigmask(SIGUSR2),
	sigmask(SCO_SIGURG),	sigmask(SIGURG),
	sigmask(SCO_SIGPOLL),	sigmask(SIGIO),
	sigmask(SCO_SIGTTIN),	sigmask(SIGTTIN),
	sigmask(SCO_SIGTTOU),	sigmask(SIGTTOU),
	sigmask(SCO_SIGVTALRM),	sigmask(SIGVTALRM),
	sigmask(SCO_SIGPROF),	sigmask(SIGPROF),
	sigmask(SCO_SIGXCPU),	sigmask(SIGXCPU),
	sigmask(SCO_SIGXFSZ),	sigmask(SIGXFSZ),
	sigmask(SCO_SIGSTOP),	sigmask(SIGSTOP),
	0
};

static unsigned long const sigset_out_bits[] = {
	sigmask(SIGCONT),	sigmask(SCO_SIGCONT),
	sigmask(SIGTSTP),	sigmask(SCO_SIGTSTP),
	sigmask(SIGWINCH),	sigmask(SCO_SIGWINCH),
	sigmask(SIGCHLD),	sigmask(SCO_SIGCLD),
	sigmask(SIGUSR1),	sigmask(SCO_SIGUSR1),
	sigmask(SIGUSR2),	sigmask(SCO_SIGUSR2),
	sigmask(SIGURG),	sigmask(SCO_SIGURG),
	sigmask(SIGIO),		sigmask(SCO_SIGPOLL),
	sigmask(SIGTTIN),	sigmask(SCO_SIGTTIN),
	sigmask(SIGTTOU),	sigmask(SCO_SIGTTOU),
	sigmask(SIGVTALRM),	sigmask(SCO_SIGVTALRM),
	sigmask(SIGPROF),	sigmask(SCO_SIGPROF),
	sigmask(SIGXCPU),	sigmask(SCO_SIGXCPU),
	sigmask(SIGXFSZ),	sigmask(SCO_SIGXFSZ),
	sigmask(SIGSTOP),	sigmask(SCO_SIGSTOP),
	0
};

#define	COMMON_SIGS \
	(sigmask(SIGHUP)|sigmask(SIGINT)|sigmask(SIGQUIT)| \
	 sigmask(SIGILL)|sigmask(SIGTRAP)|sigmask(SIGABRT)| \
	 sigmask(SIGEMT)|sigmask(SIGFPE)|sigmask(SIGKILL)| \
	 sigmask(SIGBUS)|sigmask(SIGSEGV)|sigmask(SIGSYS)| \
	 sigmask(SIGPIPE)|sigmask(SIGTERM))

const struct xbits sigset_in_xbits = { COMMON_SIGS, sigset_in_bits };
const struct xbits sigset_out_xbits = { COMMON_SIGS, sigset_out_bits };

/* from iBCS2 p 6-50 */
#define	SCO_SIG_DFL		((sig_t)0)
#define	SCO_SIG_IGN		((sig_t)1)
#define	SCO_SIG_HOLD		((sig_t)2)
#define	SCO_SA_NOCLDSTOP	1

/* from iBCS2 p 3-36; these are or'ed together to make SCO_SIG_MASK */
#define	SCO_SIGSET		0x0100
#define	SCO_SIGHOLD		0x0200
#define	SCO_SIGRELSE		0x0400
#define	SCO_SIGIGNORE		0x0800
#define	SCO_SIGPAUSE		0x1000

/*
 * The following data structures must be visible to external glue code.
 * SVr4.2 NSIG is the same as BSD NSIG; we assume initial SIG_DFL.
 */
struct sigaction sco_action[NSIG + 1];
sigset_t sco_bsd_sa_mask[NSIG + 1];
sigset_t sco_reset_on_sig;	/* reset to SIG_DFL on signal receipt */

extern void sco_sig_handler_glue __P((int));

#ifdef DEBUG
/* XXX Only for gdb's sake...  It will eventually become out-of-date. */
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
#endif

sig_t sco_signal __P((int, sig_t));

int
sco_sigignore(sig)
	int sig;
{

	if (sco_signal(sig, SIG_IGN) == SIG_ERR)
		return (-1);
	return (0);
}

int
sco_sighold(sig)
	int sig;
{
	sigset_t s;

	if (sig >= NSIG)
		return (0);
	sigemptyset(&s);
	sigaddset(&s, sig);
	return (commit_sigprocmask(SIG_BLOCK, &s, 0));
}

int
sco_sigrelse(sig)
	int sig;
{
	sigset_t s;

	if (sig >= NSIG)
		return (0);
	sigemptyset(&s);
	sigaddset(&s, sig);
	return (commit_sigprocmask(SIG_UNBLOCK, &s, 0));
}

int
sco_sigpause(sig)
	int sig;
{
	sigset_t s;

	sigprocmask(SIG_SETMASK, 0, &s);
	if ((unsigned)sig < NSIG)
		sigdelset(&s, sig);
	return (commit_sigsuspend(&s));
}

sig_t
sco_sigset(sig, handler)
	int sig;
	sig_t handler;
{
	struct sigaction sa, osa;

	if (handler == SCO_SIG_HOLD) {
		if (sco_sighold(sig) == -1)
			return (SIG_ERR);
		return (sco_action[sig].sa_handler);
	}

	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sco_sigaction(sig, &sa, &osa) == -1)
		return (SIG_ERR);
	if (sa.sa_handler != SIG_IGN)
		sco_sigrelse(sig);
	return (osa.sa_handler);
}

/*
 * The old unsafe signal() call.
 */
sig_t
sco_signal(sig, handler)
        int sig;
        sig_t handler;
{
	struct sigaction sa;
	void (*ohandler) __P((int));

	switch (sig & SCO_SIG_MASK) {
	case 0:
		break;

	case SCO_SIGSET:
		return (sco_sigset(sig & ~SCO_SIG_MASK, handler));

	case SCO_SIGHOLD:
		return ((sig_t)sco_sighold(sig & ~SCO_SIG_MASK));

	case SCO_SIGRELSE:
		return ((sig_t)sco_sigrelse(sig & ~SCO_SIG_MASK));

	case SCO_SIGIGNORE:
		return ((sig_t)sco_sigignore(sig & ~SCO_SIG_MASK));

	case SCO_SIGPAUSE:
		return ((sig_t)sco_sigpause(sig & ~SCO_SIG_MASK));

	default:
		errno = EINVAL;
		return (SIG_ERR);
	}

	switch ((long)handler) {
	case (long)SCO_SIG_DFL:
		sa.sa_handler = SIG_DFL;
		break;
	case (long)SCO_SIG_IGN:
		sa.sa_handler = SIG_IGN;
		break;
	case (long)SCO_SIG_HOLD:
		errx(1, "sco_sigaction: SIG_HOLD");
	default:
		sa.sa_handler = sco_sig_handler_glue;
		break;
	}
	sigfillset(&sa.sa_mask);	/* block everything */
#ifdef DEBUG
	sigdelset(&sa.sa_mask, SIGTRAP);
#endif
#if 0
	sa.sa_flags = SA_NOCLDSTOP;
#else
	/* sure looks like sample programs expect this */
	sa.sa_flags = 0;
#endif
	if (sig != SIGPWR_DUMMY) {
		if (sigaction(sig, &sa, (struct sigaction *)0) == -1)
			return (SIG_ERR);
		sigaddset(&sco_reset_on_sig, sig);
	}
	ohandler = sco_action[sig].sa_handler;
	sco_action[sig].sa_handler = handler;
	sigemptyset(&sco_action[sig].sa_mask);
	sigemptyset(&sco_bsd_sa_mask[sig]);
	sco_action[sig].sa_flags = 0;
	return ((sig_t)ohandler);
}

int
sco_sigaction(sig, ssa, ossa)
        int sig;
        const struct sigaction *ssa;
        struct sigaction *ossa;
{
	struct sigaction sa;

	if (ssa == 0) {
		if (ossa)
			/* EFAULT? */
			*ossa = sco_action[sig];
		return (0);
	}

	switch ((long)ssa->sa_handler) {
	case (long)SCO_SIG_DFL:
		sa.sa_handler = SIG_DFL;
		break;
	case (long)SCO_SIG_IGN:
		sa.sa_handler = SIG_IGN;
		break;
	case (long)SCO_SIG_HOLD:
		errx(1, "sco_sigaction: SIG_HOLD");
	default:
		sa.sa_handler = sco_sig_handler_glue;
		break;
	}
	sigfillset(&sa.sa_mask);	/* block everything */
#ifdef DEBUG
	sigdelset(&sa.sa_mask, SIGTRAP);
#endif
	sa.sa_flags = ssa->sa_flags & SCO_SA_NOCLDSTOP ? SA_NOCLDSTOP : 0;

	if (sig != SIGPWR_DUMMY &&
	    sigaction(sig, &sa, (struct sigaction *)0) == -1)
		return (-1);
	if (ossa)
		*ossa = sco_action[sig];
	sco_action[sig] = *ssa;
	if (*(int *)&ssa->sa_mask)
		sigset_in(&ssa->sa_mask, &sco_bsd_sa_mask[sig]);
	else
		sigemptyset(&sco_bsd_sa_mask[sig]);
	return (0);
}
