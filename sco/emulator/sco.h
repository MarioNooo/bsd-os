/*
 * Copyright (c) 1993,1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco.h,v 2.3 1996/08/20 17:39:23 donn Exp
 */

/*
 * Declarations specific to SCO emulation.
 */

extern int bsd;

extern vm_offset_t sco_break_low;
extern vm_offset_t sco_break_high;

extern int *program_edx;
extern int *program_eflags;

extern char *curdir;
extern void set_curdir __P((char *));

#ifdef RUNSCO
vm_offset_t load_coff __P((int));
#endif
void init_standard_files __P((void));

struct pollfd;
struct scoutsname;
struct sigaction;

/*
 * SCO emulation support routines.
 */
void sco_exit __P((int));
int sco_open __P((const char *, int, mode_t));
pid_t sco_waitpid __P((int, int *, int));
int sco_creat __P((const char *, mode_t));
int sco_execv __P((const char *, char * const *));
int sco_chdir __P((const char *));
pid_t sco_fork __P((void));
int sco_mknod __P((const char *, mode_t, dev_t));
int sco_break __P((const char *));
pid_t sco_getpid __P((void));
int sco_mount __P((const char *, const char *, int));
int sco_umount __P((const char *));
uid_t sco_getuid __P((void));
int sco_stime __P((const time_t *));
int sco_ptrace __P((int, pid_t, caddr_t, int));
int sco_pgrpctl __P((int, int, pid_t, pid_t));
int sco_pipe __P((void));
int sco_plock __P((int));
int sco_protctl __P((int, int, int));
gid_t sco_getgid __P((void));
sig_t sco_signal __P((int, sig_t));
int sco_msg __P((int, int, int, int, int, int));
int sco_sysi86 __P((int, int, int, int));
int sco_shm __P((int, int, int, int));
int sco_sem __P((int, int, int, int, int));
int sco_uadmin __P((int, int, int));
int sco_execve __P((const char *, char * const *, char * const *));
int sco_ulimit __P((int, int));
int sco_sysfs __P((int, int, int));
int sco_poll __P((struct pollfd *, unsigned long, int));
int sco_sigaction __P((int, const struct sigaction *, struct sigaction *));
int sco_getgroups __P((int, gid_t *));
int sco_setgroups __P((int, const gid_t *));
int sco_uname __P((void *, int, int));
int scoinfo __P((struct scoutsname *));

pid_t bsd_vfork __P((void));
int bsd_break __P((const char *));


/*
 * Prototypes for data translation routines.
 *
 * Note that program types may not conform to our standard types.
 * Currently we just cast to a locally declared program type
 * in the conversion routine itself.
 */

struct flock;
struct sgttyb;
struct stat;
struct statfs;

dev_t dev_in __P((int));
int fcntl_in __P((int));
int fd_in __P((int));
char *filename_in __P((const char *));
void flock_in __P((const struct flock *, struct flock *));
void flock_out __P((const struct flock *, struct flock *));
/* gid_t gid_in __P((int)); */
void gidvec_in __P((const gid_t *, int *, int));
void gidvec_out __P((const int *, gid_t *, int));
mode_t mode_in __P((int));
/* off_t off_in __P((off_t)); */
int open_in __P((int));
int open_out __P((int));
int pathconf_in __P((int));
int procmask_in __P((int));
void sgttyb_in __P((const struct sgttyb *, struct sgttyb *));
void sgttyb_out __P((const struct sgttyb *, struct sgttyb *));
int signal_in __P((int));
void sigset_in __P((const sigset_t *, sigset_t *));
void sigset_out __P((const sigset_t *, sigset_t *));
void stat_out __P((const struct stat *, struct stat *));
void statfs_out __P((const struct statfs *, struct statfs *));
int sysconf_in __P((int));
/* uid_t uid_in __P((int)); */

int eisdir();
int enostr();
int enotdir();
int enotty();

/* miscellaneous support routines */
int fd_inherit __P((int));
void fd_register __P((int));
unsigned long transform_bits __P((const struct xbits *, unsigned long));

/*
 * Implementation of some data translation routines as macros.
 */

/* XXX */
#define	dev_in(d)	(d)

extern int const fcntl_in_map[], max_fcntl_in_map;
#define	fcntl_in(c) \
	((unsigned)(c) >= max_fcntl_in_map ? -1 : fcntl_in_map[c])

#define	fd_in(f) \
	(((unsigned)(f) >= max_fdtab || fdtab[f] == 0) && fd_inherit(f) == -1 ?\
	    errno = EBADF, -1 : (f))

#define	filename_out(f, x) \
	if ((vm_offset_t)*(f) >= emulate_break_low && \
	    (vm_offset_t)*(f) < emulate_break_high) \
	    	free(*(f));

#define	X_O_NDELAY	0x40000000	/* pass O_NDELAY in BSD flags */

extern const struct xbits open_in_xbits;
#define	open_in(o)	transform_bits(&open_in_xbits, (o));

extern const struct xbits open_out_xbits;
#define	open_out(o)	transform_bits(&open_out_xbits, (o));

extern int const pathconf_in_map[], max_pathconf_in_map;
#define	pathconf_in(c) \
	((unsigned)(c) >= max_pathconf_in_map ? -1 : pathconf_in_map[c])

extern int const procmask_in_map[], max_procmask_in_map;
#define	procmask_in(c) \
	((unsigned)(c) >= max_procmask_in_map ? -1 : procmask_in_map[c])

#define	SCO_SIG_MASK		0x1f00

extern int const sig_in_map[], max_sig_in_map;
#define	signal_in(c) \
	((unsigned)((c) & ~SCO_SIG_MASK) >= max_sig_in_map ? \
	    -1 : sig_in_map[(c) & ~SCO_SIG_MASK] | (c) & SCO_SIG_MASK)

extern const struct xbits sigset_in_xbits;
#define	sigset_in(ss, s) \
	((ss) ? *(int *)(s) = transform_bits(&sigset_in_xbits, *(int *)(ss)) \
	    : sigprocmask(SIG_BLOCK, 0, s))

extern const struct xbits sigset_out_xbits;
#define	sigset_out(s, ss) \
	((ss) && \
	    (*(int *)(ss) = transform_bits(&sigset_out_xbits, *(int *)(s))))

extern int const sysconf_in_map[], max_sysconf_in_map;
#define	sysconf_in(c) \
	((unsigned)(c) >= max_sysconf_in_map ? -1 : sysconf_in_map[c])
