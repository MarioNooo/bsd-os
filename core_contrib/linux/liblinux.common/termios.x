/*	BSDI termios.x,v 1.5 2000/12/08 04:21:37 donn Exp	*/

/*
 * Termios functions.
 * We emulate these directly, because the Linux kernel termios structure
 * is different from the Linux user termios structure, and
 * there's no good reason to transform the structures twice...
 */

%{
#include <sys/ioctl.h>
#include <fcntl.h>
#include <time.h>
%}

int tcgetattr(int fd, struct termios *t)
{
	int line;
	int r;

	if (__bsdi_error(r = __bsdi_syscall(SYS_ioctl, fd, TIOCGETA, t)))
		return (r);
	if (__bsdi_syscall(SYS_ioctl, fd, TIOCGETD, &line) == 0)
		t->c_cc_spare2 = ttydisc_t_out(line);
	return (0);
}

int tcsetattr(int fd, int how, const struct termios *ct)
{
	int line;
	int r;
	struct termios *t = (struct termios *)ct;

#ifdef LIBC5
	/*
	 * Linux doesn't support dsusp or status characters.
	 * Glibc's termios structure has crannies where we can stash them,
	 * but for libc5, we must copy them from our previous termios state.
	 */
	struct termios ot;

	if (__bsdi_syscall(SYS_ioctl, fd, TIOCGETA, &ot) == 0) {
		t->c_cc_VDSUSP = ot.c_cc_VDSUSP;
		t->c_cc_VSTATUS = ot.c_cc_VSTATUS;
	}
#endif

	r = __bsdi_syscall(SYS_ioctl, fd,
	    how == TCSADRAIN ? TIOCSETAW :
	    how == TCSAFLUSH ? TIOCSETAF :
	    TIOCSETA, t);
	if (__bsdi_error(r))
		return (r);
	line = ttydisc_t_in(t->c_cc_spare2);
	return (__bsdi_syscall(SYS_ioctl, fd, TIOCSETD, &line));
}

int tcdrain(int fd) { return (__bsdi_syscall(SYS_ioctl, fd, TIOCDRAIN, 0)); }

int tcflow(int fd, TCOOFF)
{
	return (__bsdi_syscall(SYS_ioctl, fd, TIOCSTOP, 0));
}
int tcflow(int fd, TCOON)
{
	return (__bsdi_syscall(SYS_ioctl, fd, TIOCSTART, 0));
}
int tcflow(int fd, tcflow_t op)
{
	struct termios t;
	int r;
	unsigned char c;

	if (op != TCIOFF && op != TCION)
		return (__bsdi_error_retval(EINVAL));
	if (__bsdi_error(r = __bsdi_syscall(SYS_ioctl, fd, TIOCGETA, &t)))
		return (r);
	c = op == TCIOFF ? t.c_cc_VSTOP : t.c_cc_VSTART;
	if (c != _POSIX_VDISABLE && 
	    __bsdi_error(r = __bsdi_syscall(SYS_write, fd, &c, sizeof (c))))
		return (r);
	return (0);
}

int tcflush(int fd, TCIFLUSH)
{
	int c = FREAD;
	return (__bsdi_syscall(SYS_ioctl, fd, TIOCFLUSH, &c));
}
int tcflush(int fd, TCOFLUSH)
{
	int c = FWRITE;
	return (__bsdi_syscall(SYS_ioctl, fd, TIOCFLUSH, &c));
}
int tcflush(int fd, TCIOFLUSH)
{
	int c = FWRITE|FWRITE;
	return (__bsdi_syscall(SYS_ioctl, fd, TIOCFLUSH, &c));
}
int tcflush(int fd, tcflush_t op) = EINVAL;

/* Stolen from the BSD routine.  */
int tcsendbreak(int fd, int len)
{
	struct timeval tv;
	int r;

	tv.tv_sec = 0;
	tv.tv_sec = 400000;
	if (__bsdi_error(r = __bsdi_syscall(SYS_ioctl, fd, TIOCSBRK, 0)))
		return (r);
	__bsdi_syscall(SYS_select, NULL, NULL, NULL, &tv);
	if (__bsdi_error(r = __bsdi_syscall(SYS_ioctl, fd, TIOCCBRK, 0)))
		return (r);
	return (0);
}
