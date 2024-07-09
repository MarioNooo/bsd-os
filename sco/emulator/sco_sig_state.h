/*	BSDI sco_sig_state.h,v 2.1 1995/02/03 15:15:22 polk Exp	*/

#define	SIG_POSTPONE	0
#define	SIG_COMMIT	1
#define	SIG_UNWIND	2

#ifndef LOCORE
extern int sig_state;

struct pollfd;
struct rusage;
struct sockaddr;
struct strbuf;
struct timeval;

int	 commit_accept __P((int, struct sockaddr *, int *));
int	 commit_close __P((int));
int	 commit_connect __P((int, const struct sockaddr *, int));
int	 commit_fcntl __P((int, int, int));
int	 commit_getmsg __P((int, struct strbuf *, struct strbuf *, int *));
int	 commit_ioctl __P((int, unsigned long, int));
int	 commit_open __P((const char *, int, mode_t));
int	 commit_pause __P((void));
int	 commit_poll __P((struct pollfd *, unsigned long, int));
int	 commit_putmsg
	    __P((int, const struct strbuf *, const struct strbuf *, int));
ssize_t	 commit_read __P((int, void *, size_t));
int	 commit_recvfrom __P((int, void *, int, int, struct sockaddr *, int *));
int	 commit_select
	    __P((int, fd_set *, fd_set *, fd_set *, struct timeval *));
int	 commit_sigprocmask __P((int, const sigset_t *, sigset_t *));
int	 commit_sigsuspend __P((const sigset_t *));
pid_t	 commit_wait4 __P((pid_t, int *, int, struct rusage *));
ssize_t	 commit_write __P((int, const void *, size_t));
#endif
