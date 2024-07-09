/*
 * Copyright (c) 1993,1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_ttyops.c,v 2.1 1995/02/03 15:15:58 polk Exp
 */

/*
 * File operations for tty descriptors.
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "emulate.h"
#include "sco_ops.h"
#include "sco.h"
#include "sco_sig_state.h"

/*
 * File ops for terminal devices.
 */

extern struct fdops ttyops;

/*
 * We must keep track of whether O_NDELAY was specified
 * in the SCO open flags, since this flag acts differently under System V
 * when the file in question is a tty (argh).
 */
struct fdtty {
	struct fdbase t_base;
	enum { T_NORMAL, T_NDELAY } t_type;
};

/*
 * Much like reg_init()...
 */
static void
tty_init(f, filename, flags)
	int f;
	const char *filename;
	int flags;
{
	struct fdtty *tp;

	fd_register(f);
	if ((tp = malloc(sizeof (struct fdtty))) == 0)
		err(1, "can't initialize tty file");
	tp->t_base.f_ops = &ttyops;
	if (flags & X_O_NDELAY)
		tp->t_type = T_NDELAY;
	else
		tp->t_type = T_NORMAL;
	fdtab[f] = &tp->t_base;
}

ssize_t
tty_read(f, b, n)
	int f;
	void *b;
	size_t n;
{
	ssize_t r = commit_read(f, b, n);

	if (r == -1 && errno == EWOULDBLOCK &&
	    ((struct fdtty *)fdtab[f])->t_type == T_NDELAY)
		return (0);

	return (r);
}

ssize_t
tty_write(f, b, n)
	int f;
	const void *b;
	size_t n;
{
	ssize_t r = commit_write(f, b, n);

	if (r == -1 && errno == EWOULDBLOCK &&
	    ((struct fdtty *)fdtab[f])->t_type == T_NDELAY)
		return (0);

	return (r);
}

int
tty_dup(f)
	int f;
{
	int o = O_RDWR;
	int r;

	if (((struct fdtty *)fdtab[f])->t_type == T_NDELAY)
		o |= X_O_NDELAY;

	if ((r = dup(f)) == -1)
		return (-1);
	tty_init(r, "", o);
	return (r);
}

void
sgttyb_in(xssg, sg)
	const struct sgttyb *xssg;
	struct sgttyb *sg;
{

	errx(1, "gtty syscall conversion not yet implemented");
}

void
sgttyb_out(sg, xssg)
	const struct sgttyb *sg;
	struct sgttyb *xssg;
{

	errx(1, "stty syscall conversion not yet implemented");
}

/* counts and structures from iBCS2 p 6-60 */
#define	SCO_NCC			8
#define	SCO_NCCS		13

struct sco_termio {
	unsigned short	c_iflag;
	unsigned short	c_oflag;
	unsigned short	c_cflag;
	unsigned short	c_lflag;
	char		c_line;		/* line discipline; we ignore it */
	unsigned char	c_cc[SCO_NCC];
};

struct sco_termios {
	unsigned short	c_iflag;
	unsigned short	c_oflag;
	unsigned short	c_cflag;
	unsigned short	c_lflag;
	char		c_line;
	unsigned char	c_cc[SCO_NCCS];
	char		c_ispeed;
	char		c_ospeed;
};

/* termio ioctl()s from iBCS2 p6-68 */
#define	SCO_TCGETA		0x5401
#define	SCO_TCSETA		0x5402
#define	SCO_TCSETAW		0x5403
#define	SCO_TCSETAF		0x5404
#define	SCO_TCSBRK		0x5405
#define	SCO_TCXONC		0x5406
#define	SCO_TCFLSH		0x5407

#define	tc_in(f)		((f) - SCO_TCSETA)

/* termios ioctl()s from iBCS2 p6-69 */
#define	SCO_XCGETA		0x00586901
#define	SCO_TCSANOW		0x00586902
#define	SCO_TCSADRAIN		0x00586903
#define	SCO_TCSAFLUSH		0x00586904

#define	tcs_in(f)		(((f) & 0xff) - (SCO_TCSANOW & 0xff))

/* aliases */
#define	SCO_X_XCGETA		0x7801
#define	SCO_X_TCSANOW		0x7802
#define	SCO_X_TCSADRAIN		0x7803
#define	SCO_X_TCSAFLUSH		0x7804

#define	tcflush_in(f)		((f) + 1)
#define	tconoff_in(f)		((f) + 1)

/* winsz ioctl()s from iBCS2 p6-68 */
#define	SCO_TIOCSWINSZ		0x5467
#define	SCO_TIOCGWINSZ		0x5468

#define	SCO_TIOCSPGRP		0x5476
#define	SCO_TIOCGPGRP		0x5477

/* VMIN/VTIME overlap VEOF/VEOL in iBCS2 (sigh); see p 6-61 */
static int const cc_map[] = {
	VINTR,
	VQUIT,
	VERASE,
	VKILL,
	VEOF,
	VEOL,
	VEOL2,
	-1,
};

#define	SCO_VMIN	4
#define	SCO_VTIME	5

/* sco_termios settings */
#define	SCO_VSUSP	10
#define	SCO_VSTART	11
#define	SCO_VSTOP	12

/* for convenience */
#define	TBITS(xb, sb, i, o) \
	(transform_bits((xb), (i)) | ((o) & (sb)))

/* iBCS2 p 6-63 */
#define	IFLAG_COMMON_BITS \
    (IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK|ISTRIP|INLCR|IGNCR|ICRNL|IXANY|IMAXBEL)
#define	SCO_IXON		0x0400
#define	SCO_IXOFF		0x1000

#define	IFLAG_SAVE_BITS		(~(IFLAG_COMMON_BITS|IXON|IXOFF))

const unsigned long iflag_in_bits[] = {
	SCO_IXON,	IXON,
	SCO_IXOFF,	IXOFF,
	0
};
const unsigned long iflag_out_bits[] = {
	IXON,		SCO_IXON,
	IXOFF,		SCO_IXOFF,
	0
};

const struct xbits iflag_in_xbits = { IFLAG_COMMON_BITS, iflag_in_bits };
const struct xbits iflag_out_xbits = { IFLAG_COMMON_BITS, iflag_out_bits };
#define	iflag_in(i, o)	TBITS(&iflag_in_xbits, IFLAG_SAVE_BITS, (i), (o))
#define	iflag_out(i)	transform_bits(&iflag_out_xbits, (i))

/* iBCS2 p 6-64 */
#define	SCO_ONLCR		0x0004
#define	SCO_OXTABS		0x1800

#define	OFLAG_SAVE_BITS		(~(OPOST|ONLCR|OXTABS))

const unsigned long oflag_in_bits[] = {
	SCO_ONLCR,	ONLCR,
	SCO_OXTABS,	OXTABS,
	0
};
const unsigned long oflag_out_bits[] = {
	ONLCR,		SCO_ONLCR,
	OXTABS,		SCO_OXTABS,
	0
};

const struct xbits oflag_in_xbits = { OPOST, oflag_in_bits };
const struct xbits oflag_out_xbits = { OPOST, oflag_out_bits };
#define	oflag_in(i, o)	TBITS(&oflag_in_xbits, OFLAG_SAVE_BITS, (i), (o))
#define	oflag_out(i)	transform_bits(&oflag_out_xbits, (i))

/* iBCS2 p 6-66 */
#define	SCO_CBAUD	0x000f
#define	SCO_CSIZE	0x0030
#define	SCO_CS5		0x0000
#define	SCO_CS6		0x0010
#define	SCO_CS7		0x0020
#define	SCO_CS8		0x0030
#define	SCO_CSTOPB	0x0040
#define	SCO_CREAD	0x0080
#define	SCO_PARENB	0x0100
#define	SCO_PARODD	0x0200
#define	SCO_HUPCL	0x0400
#define	SCO_CLOCAL	0x0800

#define	CFLAG_SAVE_BITS \
    (~(CSIZE|CSTOPB|CREAD|PARENB|PARODD|HUPCL|CLOCAL))

const unsigned long cflag_in_bits[] = { 
	SCO_CSTOPB,	CSTOPB,
	SCO_CREAD,	CREAD,
	SCO_PARENB,	PARENB,
	SCO_PARODD,	PARODD,
	SCO_HUPCL,	HUPCL,
	SCO_CLOCAL,	CLOCAL,
	0
};
const unsigned long cflag_out_bits[] = {
	CSTOPB,		SCO_CSTOPB,
	CREAD,		SCO_CREAD,
	PARENB,		SCO_PARENB,
	PARODD,		SCO_PARODD,
	HUPCL,		SCO_HUPCL,
	CLOCAL,		SCO_CLOCAL,
	0
};

const struct xbits cflag_in_xbits = { 0, cflag_in_bits };
const struct xbits cflag_out_xbits = { 0, cflag_out_bits };

static int
cflag_in(c, o)
	int c, o;
{
	int r = transform_bits(&cflag_in_xbits, c);

	r |= o & CFLAG_SAVE_BITS;

	switch (c & SCO_CSIZE) {
	case SCO_CS5:	r |= CS5; break;
	case SCO_CS6:	r |= CS6; break;
	case SCO_CS7:	r |= CS7; break;
	case SCO_CS8:	r |= CS8; break;
	}
	return (r);
}

static int
cflag_out(c)
	int c;
{
	int r = transform_bits(&cflag_out_xbits, c);

	switch (c & CSIZE) {
	case CS5:	r |= SCO_CS5; break;
	case CS6:	r |= SCO_CS6; break;
	case CS7:	r |= SCO_CS7; break;
	case CS8:	r |= SCO_CS8; break;
	}
	return (r);
}

static int const speed_map[] = {
	0, 50, 75, 110, 134, 150, 200, 300, 600, 1200,
	1800, 2400, 4800, 9600, 19200, 38400,
};
#define	sco_tio_cfgetspeed(t)	speed_map[(t)->c_cflag & SCO_CBAUD]
#define	sco_tios_cfgetispeed(t)	speed_map[(t)->c_ispeed & SCO_CBAUD]
#define	sco_tios_cfgetospeed(t)	speed_map[(t)->c_ospeed & SCO_CBAUD]

/* iBCS2 p 6-67 -- no bits in common */
#define	SCO_ISIG		0x0001
#define	SCO_ICANON		0x0002
#define	SCO_ECHO		0x0008
#define SCO_ECHOE		0x0010
#define	SCO_ECHOK		0x0020
#define	SCO_ECHONL		0x0040
#define	SCO_NOFLSH		0x0080
#define	SCO_IEXTEN		0x0100
#define	SCO_TOSTOP		0x0200

#define	LFLAG_SAVE_BITS \
    (~(ISIG|ICANON|ECHO|ECHOE|ECHOK|ECHONL|NOFLSH|IEXTEN|TOSTOP))

const unsigned long lflag_in_bits[] = {
	SCO_ISIG,	ISIG,
	SCO_ICANON,	ICANON,
	SCO_ECHO,	ECHO,
	SCO_ECHOE,	ECHOE,
	SCO_ECHOK,	ECHOK,
	SCO_ECHONL,	ECHONL,
	SCO_NOFLSH,	NOFLSH,
	SCO_IEXTEN,	IEXTEN,
	SCO_TOSTOP,	TOSTOP,
	0
};
const unsigned long lflag_out_bits[] = {
	ISIG,		SCO_ISIG,
	ICANON,		SCO_ICANON,
	ECHO,		SCO_ECHO,
	ECHOE,		SCO_ECHOE,
	ECHOK,		SCO_ECHOK,
	ECHONL,		SCO_ECHONL,
	NOFLSH,		SCO_NOFLSH,
	IEXTEN,		SCO_IEXTEN,
	TOSTOP,		SCO_TOSTOP,
	0
};

const struct xbits lflag_in_xbits = { 0, lflag_in_bits };
const struct xbits lflag_out_xbits = { 0, lflag_out_bits };
#define	lflag_in(i, o)	TBITS(&lflag_in_xbits, LFLAG_SAVE_BITS, (i), (o))
#define	lflag_out(i)	transform_bits(&lflag_out_xbits, (i))

static void
sco_tio_cfsetspeed(t, speed)
	struct sco_termio *t;
	speed_t speed;
{
	int i;

	for (i = 0; i < sizeof (speed_map) / sizeof (int) - 1; ++i)
		if (speed_map[i + 1] > speed)
			break;
	t->c_cflag |= i;
}

static void
sco_tios_cfsetispeed(t, speed)
	struct sco_termios *t;
	speed_t speed;
{

	sco_tio_cfsetspeed((struct sco_termio *)t, speed);
	t->c_ispeed = t->c_cflag & SCO_CBAUD;
}

static void
sco_tios_cfsetospeed(t, speed)
	struct sco_termios *t;
	speed_t speed;
{

	sco_tio_cfsetspeed((struct sco_termio *)t, speed);
	t->c_ospeed = t->c_cflag & SCO_CBAUD;
}

/* assumes termios was initialized with tcgetattr() */
static void
termio_in(stio, btios)
	struct sco_termio *stio;
	struct termios *btios;
{
	int i;

	btios->c_iflag = iflag_in(stio->c_iflag, btios->c_iflag);
	btios->c_oflag = oflag_in(stio->c_oflag, btios->c_oflag);
	btios->c_cflag = cflag_in(stio->c_cflag, btios->c_cflag);
	cfsetspeed(btios, sco_tio_cfgetspeed(stio));
	btios->c_lflag = lflag_in(stio->c_lflag, btios->c_lflag);

	if (stio->c_iflag & ICANON) {
		for (i = 0; i < SCO_NCC; ++i)
			if (cc_map[i] >= 0)
				btios->c_cc[cc_map[i]] = stio->c_cc[i];
	} else {
#if 0
		warnx("termio_in: setting VMIN(%d)/VTIME(%d)",
		    stio->c_cc[SCO_VMIN], stio->c_cc[SCO_VTIME]);
#endif
		btios->c_cc[VMIN] = stio->c_cc[SCO_VMIN];
		btios->c_cc[VTIME] = stio->c_cc[SCO_VTIME];
	}
}

static void
termio_out(btios, stio)
	struct termios *btios;
	struct sco_termio *stio;
{
	int i;

	/* EFAULT? */
	bzero(stio, sizeof (*stio));
	stio->c_iflag = iflag_out(btios->c_iflag);
	stio->c_oflag = oflag_out(btios->c_oflag);
	stio->c_cflag = cflag_out(btios->c_cflag);
	sco_tio_cfsetspeed(stio, cfgetospeed(btios));
	stio->c_lflag = lflag_out(btios->c_lflag);

	if (btios->c_iflag & ICANON) {
		for (i = 0; i < SCO_NCC; ++i)
			if (cc_map[i] >= 0)
				stio->c_cc[i] = btios->c_cc[cc_map[i]];
	} else {
		stio->c_cc[SCO_VMIN] = btios->c_cc[VMIN];
		stio->c_cc[SCO_VTIME] = btios->c_cc[VTIME];
	}
}

static void
termios_in(stios, btios)
	struct sco_termios *stios;
	struct termios *btios;
{

	termio_in((struct sco_termio *)stios, btios);
	btios->c_cc[VSUSP] = stios->c_cc[SCO_VSUSP];
	btios->c_cc[VSTART] = stios->c_cc[SCO_VSTART];
	btios->c_cc[VSTOP] = stios->c_cc[SCO_VSTOP];
	cfsetispeed(btios, sco_tios_cfgetispeed(stios));
	cfsetospeed(btios, sco_tios_cfgetospeed(stios));
}

static void
termios_out(btios, stios)
	struct termios *btios;
	struct sco_termios *stios;
{

	termio_out(btios, (struct sco_termio *)stios);
	stios->c_cc[SCO_VSUSP] = btios->c_cc[VSUSP];
	stios->c_cc[SCO_VSTART] = btios->c_cc[VSTART];
	stios->c_cc[SCO_VSTOP] = btios->c_cc[VSTOP];
	sco_tios_cfsetispeed(stios, cfgetispeed(btios));
	sco_tios_cfsetospeed(stios, cfgetospeed(btios));
}

/* XXX: these routines know about innards of BSD termios */
static int
commit_tcsadrain(f, tp)
	int f;
	struct termios *tp;
{

	return (commit_ioctl(f, TIOCSETAW, (int)tp));
}

static int
commit_tcdrain(f, tp)
	int f;
	struct termios *tp;
{

	return (commit_ioctl(f, TIOCDRAIN, 0));
}

int
tty_ioctl(f, c, arg)
        int f, c, arg;
{
	struct termios t;

	switch (c) {
	case SCO_TCSETAW:
	case SCO_TCSADRAIN:
	case SCO_TCSBRK:
		break;
	default:
		sig_state = SIG_POSTPONE;
	}

        switch (c) {

	case SCO_TCGETA:
		if (tcgetattr(f, &t) == -1)
			return (-1);
		termio_out(&t, (struct sco_termio *)arg);
		break;

	case SCO_TCSETAW:
		if (tcgetattr(f, &t) == -1)
			return (-1);
		termio_in((struct sco_termio *)arg, &t);
		if (commit_tcsadrain(f, &t) == -1)
			return (-1);
		break;

	case SCO_TCSETA:
	case SCO_TCSETAF:
		if (tcgetattr(f, &t) == -1)
			return (-1);
		termio_in((struct sco_termio *)arg, &t);
		if (tcsetattr(f, tc_in(c), &t) == -1)
			return (-1);
		break;

	case SCO_TCSBRK:
		if (commit_tcdrain(f) == -1)
			return (-1);
		if (arg && tcsendbreak(f, 0) == -1)
			return (-1);
		break;

	case SCO_TCXONC:
		if (tcflow(f, tconoff_in(arg)) == -1)
			return (-1);
		break;

	case SCO_TCFLSH:
		if (tcflush(f, tcflush_in(arg)) == -1)
			return (-1);
		break;

	case SCO_XCGETA:
	case SCO_X_XCGETA:
		if (tcgetattr(f, &t) == -1)
			return (-1);
		termios_out(&t, (struct sco_termios *)arg);
		break;

	case SCO_TCSADRAIN:
	case SCO_X_TCSADRAIN:
		if (tcgetattr(f, &t) == -1)
			return (-1);
		termios_in((struct sco_termios *)arg, &t);
		if (commit_tcsadrain(f, &t) == -1)
			return (-1);
		break;

	case SCO_TCSANOW:
	case SCO_TCSAFLUSH:
	case SCO_X_TCSANOW:
	case SCO_X_TCSAFLUSH:
		if (tcgetattr(f, &t) == -1)
			return (-1);
		termios_in((struct sco_termios *)arg, &t);
		if (tcsetattr(f, tcs_in(c), &t) == -1)
			return (-1);
		break;

	case SCO_TIOCGWINSZ:
		if (ioctl(f, TIOCGWINSZ, (struct winsize *)arg) == -1)
			return (-1);
		break;

	case SCO_TIOCSWINSZ:
		if (ioctl(f, TIOCSWINSZ, (struct winsize *)arg) == -1)
			return (-1);
		break;

	case SCO_TIOCGPGRP:
		if (ioctl(f, TIOCGPGRP, (pid_t *)arg) == -1)
			return (-1);
		break;

	case SCO_TIOCSPGRP:
		if (ioctl(f, TIOCSPGRP, (pid_t *)arg) == -1)
			return (-1);
		break;

	default:
		/* warnx("unsupported ioctl command (%#x)", c); */
		errno = EINVAL;
		return (-1);
        }

	return (0);
}

int
tty_fcntl(f, c, arg)
	int f, c, arg;
{
	struct flock fl;
	int a = arg;
	int r;

	if (c == F_SETLKW) {
		flock_in((struct flock *)arg, &fl);
		return (commit_fcntl(f, c, (int)&fl));
	}

	sig_state = SIG_POSTPONE;

	switch (c) {

	case F_GETLK:
	case F_SETLK:
		flock_in((struct flock *)arg, &fl);
		a = (int)&fl;
		break;

	case F_SETFL:
		a = open_in(arg);
		if (a & X_O_NDELAY) {
			a = (a &~ X_O_NDELAY) | O_NONBLOCK;
			((struct fdtty *)fdtab[f])->t_type = T_NDELAY;
		} else
			((struct fdtty *)fdtab[f])->t_type = T_NORMAL;
		break;

	/* case F_CHKFL? */
	}

	if ((r = fcntl(f, c, a)) == -1)
		return (-1);

	switch (c) {

	case F_DUPFD:
		a = O_RDWR;
		if (((struct fdtty *)fdtab[r])->t_type == T_NDELAY)
			a |= X_O_NDELAY;
		tty_init(r, "", a);
		break;

	case F_GETLK:
		flock_out(&fl, (struct flock *)arg);
		break;

	case F_GETFL:
		if (((struct fdtty *)fdtab[f])->t_type == T_NDELAY)
			r |= X_O_NDELAY;
		r = open_out(r);
		break;
	}

	return (r);
}

extern int reg_close __P((int));
extern int reg_dup __P((int));
extern int reg_fcntl __P((int, int, int));

extern int stty __P((int, const struct sgttyb *));
extern int gtty __P((int, struct sgttyb *));

struct fdops ttyops = {
	0,
	0,
	reg_close,
	0,
	tty_dup,
	0,
	0,
	0,
	0,
	0,
	tty_fcntl,
	0,
	fpathconf,
	fstat,
	/* XXX handle statfs buffer length? */
	(int (*) __P((int, struct statfs *, int, int)))fstatfs,
	0,
	0,
	enotdir,
	0,
	enostr,
	0,
	0,
	0,
	gtty,
	tty_init,
	tty_ioctl,
	0,
	lseek,
	0,
	0,
	enostr,
	tty_read,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	stty,
	tty_write,
	0,
};
