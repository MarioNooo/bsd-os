/*	BSDI ioctl.x,v 1.9 2002/12/10 23:08:45 polk Exp	*/

/*
 * Ioctl() function transforms.
 * This file is included by emulation-specific ioctl.x files,
 * so that we can use emulation-specific headers.
 */

%{
/* XXX for debugging */
#include <stdio.h>

#define NCC 8
struct termio {
        unsigned short c_iflag;         /* input mode flags */
        unsigned short c_oflag;         /* output mode flags */
        unsigned short c_cflag;         /* control mode flags */
        unsigned short c_lflag;         /* local mode flags */
        unsigned char c_line;           /* line discipline */
        unsigned char c_cc[NCC];        /* control characters */
};

#ifdef SOUND
/*
 * Some of the sound ioctls conflicit with the old-style cookies.
 * They need to determine if a file descriptor is referencing a sound
 * card.  The following routine uses fstat to determine this based on
 * this define of the device number for the sound devices.
 */
#define	SOUNDDEV_MAJOR	66

#include <sys/stat.h>

static int
is_sound(int fd)
{
	struct stat sb;

	if (__bsdi_syscall(SYS_fstat, fd, &sb) == 0 &&
		major(sb.st_rdev) == SOUNDDEV_MAJOR) {
		return (1);
	}
	return (0);
}
#endif
%}


/*
 * Tty-related ioctl() transformations.
 */

int ioctl(int fd, TIOCGETS, struct kernel_termios *tios) {
	return (__bsdi_syscall(SYS_ioctl, fd, TIOCGETA, tios));
}
int ioctl(int fd, TIOCSETS, const struct kernel_termios *ctios) {
	struct termios *tios = (struct termios *)ctios;
#ifdef LIBC5
	/*
	 * Linux doesn't support dsusp or status characters.
	 * Glibc's termios structure has crannies where we can stash them,
	 * but for libc5, we must copy them from our previous termios state.
	 */
	struct termios otios;
#endif

#ifdef SOUND
	/* Resolve conflict with sound ioctl */
	if (is_sound(fd))
		return (__bsdi_syscall(SYS_ioctl, fd, SNDCTL_TMR_START));
#endif

#ifdef	LIBC5
	if (__bsdi_syscall(SYS_ioctl, fd, TIOCGETA, &otios) == 0) {
		tios->c_cc_VDSUSP = otios.c_cc_VDSUSP;
		tios->c_cc_VSTATUS = otios.c_cc_VSTATUS;
	}
#endif
	return (__bsdi_syscall(SYS_ioctl, fd, TIOCSETA, tios));
}
int ioctl(int fd, TIOCSETAW, const struct kernel_termios *ctios) {
	struct termios *tios = (struct termios *)ctios;
#ifdef LIBC5
	struct termios otios;
#endif

#ifdef SOUND
	/* Resolve conflict with sound icotl */
	if (is_sound(fd))
		return (__bsdi_syscall(SYS_ioctl, fd, SNDCTL_TMR_STOP));
#endif


#ifdef	LIBC5
	if (__bsdi_syscall(SYS_ioctl, fd, TIOCGETA, &otios) == 0) {
		tios->c_cc_VDSUSP = otios.c_cc_VDSUSP;
		tios->c_cc_VSTATUS = otios.c_cc_VSTATUS;
	}
#endif
	return (__bsdi_syscall(SYS_ioctl, fd, TIOCSETAW, tios));
}
int ioctl(int fd, TIOCSETAF, const struct kernel_termios *ctios) {
	struct termios *tios = (struct termios *)ctios;
#ifdef LIBC5
	struct termios otios;
#endif

#ifdef SOUND
	/* Resolve conflict with sound ioctl */
	if (is_sound(fd))
		return (__bsdi_syscall(SYS_ioctl, fd, SNDCTL_TMR_CONTINUE));
#endif

#ifdef	LIBC5
	if (__bsdi_syscall(SYS_ioctl, fd, TIOCGETA, &otios) == 0) {
		tios->c_cc_VDSUSP = otios.c_cc_VDSUSP;
		tios->c_cc_VSTATUS = otios.c_cc_VSTATUS;
	}
#endif
	return (__bsdi_syscall(SYS_ioctl, fd, TIOCSETAF, tios));
}
int ioctl(int fd, TIOCGETA, void *v) {
	struct termio *termio = v;
	struct termios tios;
	struct linux_kernel_termios termios;
	int r;

	r = __bsdi_syscall(SYS_ioctl, fd, TIOCGETA, &tios);
	if (__bsdi_error(r))
		return (r);
	kernel_termios_out(&tios, &termios, sizeof(termios));

        termio->c_iflag = termios.c_iflag;
        termio->c_oflag = termios.c_oflag;
        termio->c_cflag = termios.c_cflag;
        termio->c_lflag = termios.c_lflag;
        termio->c_line  = termios.linux_c_line;
        memcpy(termio->c_cc, &termios.c_cc_VINTR, NCC);
	return(0);
}

int ioctl(int fd, TIOCSETA, void *v) {
	struct termio *termio = v;
	struct termios tios;
	struct linux_kernel_termios termios;

#define SET_LOW_BITS(x,y)       (*(unsigned short *)(&x) = (y))
        SET_LOW_BITS(termios.c_iflag, termio->c_iflag);
        SET_LOW_BITS(termios.c_oflag, termio->c_oflag);
        SET_LOW_BITS(termios.c_cflag, termio->c_cflag);
        SET_LOW_BITS(termios.c_lflag, termio->c_lflag);
#undef SET_LOW_BITS
        termios.linux_c_line  = termio->c_line;
        memcpy(&termios.c_cc_VINTR, termio->c_cc, NCC);

	kernel_termios_in(&termios, &tios, sizeof(termios));
	return (__bsdi_syscall(SYS_ioctl, fd, TIOCSETA, &tios));
}

int ioctl(int fd, TIOCSETSW, void *v) {
	struct termio *termio = v;
	struct termios tios;
	struct linux_kernel_termios termios;

#define SET_LOW_BITS(x,y)       (*(unsigned short *)(&x) = (y))
        SET_LOW_BITS(termios.c_iflag, termio->c_iflag);
        SET_LOW_BITS(termios.c_oflag, termio->c_oflag);
        SET_LOW_BITS(termios.c_cflag, termio->c_cflag);
        SET_LOW_BITS(termios.c_lflag, termio->c_lflag);
#undef SET_LOW_BITS
        termios.linux_c_line  = termio->c_line;
        memcpy(&termios.c_cc_VINTR, termio->c_cc, NCC);

	kernel_termios_in(&termios, &tios, sizeof(termios));
	return (__bsdi_syscall(SYS_ioctl, fd, TIOCSETAW, &tios));
}

int ioctl(int fd, TIOCSETSF, void *v) {
	struct termio *termio = v;
	struct termios tios;
	struct linux_kernel_termios termios;

#define SET_LOW_BITS(x,y)       (*(unsigned short *)(&x) = (y))
        SET_LOW_BITS(termios.c_iflag, termio->c_iflag);
        SET_LOW_BITS(termios.c_oflag, termio->c_oflag);
        SET_LOW_BITS(termios.c_cflag, termio->c_cflag);
        SET_LOW_BITS(termios.c_lflag, termio->c_lflag);
#undef SET_LOW_BITS
        termios.linux_c_line  = termio->c_line;
        memcpy(&termios.c_cc_VINTR, termio->c_cc, NCC);

	kernel_termios_in(&termios, &tios, sizeof(termios));
	return (__bsdi_syscall(SYS_ioctl, fd, TIOCSETAF, &tios));
}


/*
 * Socket-related ioctl() transformations.
 */

int ioctl(int fd, SIOCGIFCONF, volatile struct ifconf *conf);
int ioctl(int fd, SIOCGIFFLAGS, volatile struct ifflags *flags);
int ioctl(int fd, SIOCSIFFLAGS, const struct ifflags *flags);
int ioctl(int fd, SIOCGIFADDR, volatile struct ifraddr *addr);
int ioctl(int fd, SIOCSIFADDR, const struct ifraddr *addr);
int ioctl(int fd, SIOCGIFDSTADDR, volatile struct ifraddr *addr);
int ioctl(int fd, SIOCSIFDSTADDR, const struct ifraddr *addr);
int ioctl(int fd, SIOCGIFBRDADDR, volatile struct ifraddr *addr);
int ioctl(int fd, SIOCSIFBRDADDR, const struct ifraddr *addr);
int ioctl(int fd, SIOCGIFNETMASK, volatile struct ifraddr *addr);
int ioctl(int fd, SIOCSIFNETMASK, const struct ifraddr *addr);
int ioctl(int fd, SIOCADDMULTI, const struct ifraddr *addr);
int ioctl(int fd, SIOCDELMULTI, const struct ifraddr *addr);
int ioctl(int fd, SIOCDIFADDR, const struct ifraddr *addr);

/*
 * The generic ioctl() call -- sweeps up the odds and ends.
 */
int ioctl(int fd, ioctl_t cmd, void *arg);
