#	BSDI sco_transform2.x,v 2.2 1995/07/10 18:38:44 donn Exp
void	 nosys(void);	/* 1 = xlocking */
void	 nosys(void);	/* 2 = creatsem */
void	 nosys(void);	/* 3 = opensem */
void	 nosys(void);	/* 4 = sigsem */
void	 nosys(void);	/* 5 = waitsem */
void	 nosys(void);	/* 6 = nbwaitsem */
void	 nosys(void);	/* 7 = rdchk */
void	 nosys(void);
void	 nosys(void);
int	 ftruncate(fd_t, sco_off_t);	/* 10 = chsize */
int	 ftime(struct timeb *);
long	 nap(long);
void	 nosys(void);	/* 13 = sdget */
void	 nosys(void);	/* 14 = sdfree */
void	 nosys(void);	/* 15 = sdenter */
void	 nosys(void);	/* 16 = sdleave */
void	 nosys(void);	/* 17 = sdgetv */
void	 nosys(void);	/* 18 = sdwaitv */
void	 nosys(void);
void	 nosys(void);
void	 nosys(void);	/* 21 = xnet_sys */
void	 nosys(void);
void	 nosys(void);
void	 nosys(void);
void	 nosys(void);
void	 nosys(void);
void	 nosys(void);
void	 nosys(void);
void	 nosys(void);
void	 nosys(void);
void	 nosys(void);
void	 nosys(void);	/* 32 = proctl */
void	 nosys(void);	/* 33 = execseg */
void	 nosys(void);	/* 34 = unexecseg */
void	 nosys(void);
int	 select(int, fd_set *, fd_set *, fd_set *, struct timeval *) blocking;
int	 eaccess(filename_t, int);
void	 nosys(void);	/* 38 = paccess */
int	 sigaction(signal_t, const struct sigaction *, struct sigaction *);
int	 sigprocmask(procmask_t, const sigset_t *, sigset_t *) blocking;
int	 sigpending(sigset_t *);
int	 sigsuspend(const sigset_t *) blocking;
int	 getgroups(int, gid_t *);
int	 setgroups(int, const gid_t *);
long	 sysconf(sysconf_t);
long	 pathconf(filename_t, pathconf_t);
long	 fpathconf(fd_t, pathconf_t);
int	 rename(filename_t, filename_t);
void	 nosys(void);	/* 49 = security */
int	 scoinfo(struct scoutsname *);
void	 nosys(void);
void	 nosys(void);
void	 nosys(void);
void	 nosys(void);
int	 getitimer(int, struct itimerval *);
int	 setitimer(int, const struct itimerval *, struct itimerval *);
void	 nosys(void);
void	 nosys(void);	/* 58 = setreuid */
void	 nosys(void);	/* 59 = setregid */
