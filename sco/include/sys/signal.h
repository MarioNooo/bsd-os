/*	BSDI signal.h,v 1.1 1995/07/10 18:25:48 donn Exp	*/

#ifndef _SCO_SIGNAL_H_
#define	_SCO_SIGNAL_H_

/* from iBCS2 p6-49 and from signal(5) in the SVr4.2 SFDR p356 */
#define	SIGHUP		1
#define	SIGINT		2
#define	SIGQUIT		3
#define	SIGILL		4
#define	SIGTRAP		5
#define	SIGABRT		6
#define	SIGIOT		SIGABRT
#define	SIGEMT		7
#define	SIGFPE		8
#define	SIGKILL		9
#define	SIGBUS		10
#define	SIGSEGV		11
#define	SIGSYS		12
#define	SIGPIPE		13
#define	SIGALRM		14
#define	SIGTERM		15
#define	SIGUSR1		16
#define	SIGUSR2		17
#define	SIGCLD		18
#define	SIGPWR		19
#define	SIGWINCH	20
#define	SIGURG		21
#define	SIGPOLL		22
#define	SIGSTOP		23
#define	SIGTSTP		24
#define	SIGCONT		25
#define	SIGTTIN		26
#define	SIGTTOU		27
#define	SIGVTALRM	28
#define	SIGPROF		29
#define	SIGXCPU		30
#define	SIGXFSZ		31

/* from iBCS2 p 6-50 */
#define	SIG_ERR		((void (*)())-1)
#define	SIG_DFL		((void (*)())0)
#define	SIG_IGN		((void (*)())1)
#define	SIG_HOLD	((void (*)())2)
#define	SA_NOCLDSTOP	1

#define	SIG_SETMASK	0
#define	SIG_BLOCK	1
#define	SIG_UNBLOCK	2

typedef long sigset_t;

struct sigaction {
	void		(*sa_handler)();
	sigset_t	sa_mask;
	int		sa_flags;
};

/* prototypes required by POSIX.1 */

#include <sys/cdefs.h>

int	kill __P((int, int));
int	raise __P((int));
void	(*signal __P((int, void (*) __P((int))))) __P((int));
int	sigaction __P((int, const struct sigaction *, struct sigaction *));
int	sigpending __P((sigset_t *));
int	sigprocmask __P((int, const sigset_t *, sigset_t *));
int	sigsuspend __P((const sigset_t *));

#define	sigaddset(set, signo)	(*(set) |= 1 << ((signo) - 1), 0)
#define	sigdelset(set, signo)	(*(set) &= ~(1 << ((signo) - 1)), 0)
#define	sigemptyset(set)	(*(set) = 0, 0)
#define	sigfillset(set)		(*(set) = ~(sigset_t)0, 0)
#define	sigismember(set, signo)	((*(set) & (1 << ((signo) - 1))) != 0)

#endif	/* _SCO_SIGNAL_H_ */
