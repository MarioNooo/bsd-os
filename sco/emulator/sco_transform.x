#	BSDI sco_transform.x,v 2.3 1995/10/20 14:48:29 donn Exp
void	 exit(int);
pid_t	 fork(void);
ssize_t	 read(fd_t, void *, size_t) blocking;
ssize_t	 write(fd_t, const void *, size_t) blocking;
fd_t	 open(filename_t, open_t, mode_t) blocking;
int	 close(fd_t) blocking;
pid_t	 waitpid(int, int *, int) blocking;
fd_t	 creat(filename_t, mode_t);
int	 link(filename_t, filename_t);
int	 unlink(filename_t);
int	 execv(filename_t, char * const *);
int	 chdir(filename_t);
time_t	 time(time_t *);
int	 mknod(filename_t, mode_t, dev_t);
int	 chmod(filename_t, mode_t);
int	 chown(filename_t, uid_t, gid_t);
int	 brk(const char *);
int	 stat(filename_t, struct stat *);
sco_off_t lseek(fd_t, sco_off_t, int);
pid_t	 getpid(void);				/* + getppid() */
int	 sco_mount(filename_t, filename_t, int);
int	 umount(filename_t);
int	 setuid(uid_t);
uid_t	 getuid(void);				/* + geteuid() */
int	 stime(const time_t *);
int	 ptrace(int, pid_t, caddr_t, int);
unsigned alarm(unsigned);
int	 fstat(fd_t, struct stat *);
int	 pause(void) blocking;
int	 utime(filename_t, const struct utimbuf *);
int	 stty(fd_t, const struct sgttyb *);
int	 gtty(fd_t, struct sgttyb *);
int	 access(filename_t, int);
int	 nice(int);
int	 statfs(filename_t, struct statfs *, int, int);
void	 sync(void);
int	 kill(pid_t, signal_t);
int	 fstatfs(fd_t, struct statfs *, int, int);
int	 pgrpctl(int, int, pid_t, pid_t);
void	 nosys(void);
int	 dup(fd_t);				/* dup2() => fcntl() */
fd_t	 pipe(void);				/* internal version */
clock_t	 times(struct tms *);
int	 profil(char *, int, int, int);
int	 plock(int);
int	 setgid(gid_t);
gid_t	 getgid(void);				/* + getegid() */
sig_t	 signal(signal_t, sig_t);
int	 msgop(int, int, int, int, int, int);
int	 sysi86(int, int, int, int);
int	 acct(filename_t);
int	 shmop(int, int, int, int);
int	 semop(int, int, int, int, int);
int	 ioctl(fd_t, unsigned long, int) blocking;
int	 uadmin(int, int, int);
void	 nosys(void);
int	 uname(void *, int, int);
void	 nosys(void);
int	 execve(filename_t, char * const *, char * const *);
mode_t	 umask(mode_t);
int	 chroot(filename_t);
int	 fcntl(fd_t, fcntl_t, int) blocking;
long	 ulimit(int, int);
void	 nosys(void);
void	 nosys(void);
void	 nosys(void);
void	 nosys(void);
void	 nosys(void);
void	 nosys(void);
void	 nosys(void);				/* 70 = advfs */
void	 nosys(void);				/* 71 = unadvfs */
void	 nosys(void);				/* 72 = rmount */
void	 nosys(void);				/* 73 = rumount */
void	 nosys(void);				/* 74 = rfstart */
void	 nosys(void);
void	 nosys(void);				/* 76 = rdebug */
void	 nosys(void);				/* 77 = rfstop */
void	 nosys(void);				/* 78 = rfsys */
int	 rmdir(filename_t);
int	 mkdir(filename_t, mode_t);
int	 getdents(fd_t, struct dirent *, size_t);
void	 nosys(void);				/* 82 = libattach */
void	 nosys(void);				/* 83 = libdetach */
int	 sysfs(int, int, int);
int	 getmsg(fd_t, struct strbuf *, struct strbuf *, int *) blocking;
int	 putmsg(fd_t, const struct strbuf *, const struct strbuf *, int) blocking;
int	 poll(struct pollfd *, unsigned long, int) blocking;
void	 nosys(void);
int	 protctl(int, int, int);
int	 symlink(filename_t, filename_t);
int	 lstat(filename_t, struct stat *);
int	 readlink(filename_t, char *, int);
