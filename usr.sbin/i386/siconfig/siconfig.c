/*	BSDI siconfig.c,v 2.2 1995/09/22 22:12:58 ewv Exp	*/

#ifndef lint
static char copyright1[] =  "@(#) (c) Specialix International, 1990,1992",
            copyright2[] =  "@(#) (a) Andy Rutter, 1993",
            rcsid[] = "Id: siconfig.c,v 1.3 1993/10/18 07:54:00 andy Exp";
#endif	/* not lint */
/*
 *	SLXOS configuration and debug interface
 *
 * Module:	siconfig.c
 * Target:	BSDI/386
 * Author:	Andy Rutter, andy@acronym.com
 *
 * Revision: 1.3
 * Date: 1993/10/18 07:54:00
 * State: Exp
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notices, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notices, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Andy Rutter of
 *	Advanced Methods and Tools Ltd. based on original information
 *	from Specialix International.
 * 4. Neither the name of Advanced Methods and Tools, nor Specialix
 *    International may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/tty.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vis.h>

#include <i386/isa/isavar.h>
#include <i386/isa/sireg.h>

int	getnum __P((const char *));
int	lvls2bits __P((char *));
int	onelevel __P((const char *));
int	opencontrol __P((void));
void	checkxprint __P((void));
void	dostat __P((void));
void	getxpstring __P((char *, char *));
void	onoff __P((int, char **, int, char *, char *, int));
void	prlevels __P((int));
void	prusage __P((int));
void	prxpstring __P((char *));
void	xpon_off __P((int, char **, int));

void	boot __P((int, char **));
void	cookmode __P((int, char **));
void	debug __P((int, char **));
void	hwflow __P((int, char **));
void	ixany __P((int, char **));
void	modem __P((int, char **));
void	mstate __P((int, char **));
void	nport __P((int, char **));
void	ppp __P((int, char **));
void	rxint __P((int, char **));
void	txint __P((int, char **));
void	txlowat __P((int, char **));
void	xpcps __P((int, char **));
void	xpoff __P((int, char **));
void	xpon __P((int, char **));

int	alldev;
int	ctlfd;
char	*xdevname;
struct si_tcsi tc;

struct lv {
	char	*lv_name;
	int 	lv_bit;
} lv[] = {
	"entry",	DBG_ENTRY,
	"open",		DBG_OPEN,
	"close",	DBG_CLOSE,
	"read",		DBG_READ,
	"write",	DBG_WRITE,
	"param",	DBG_PARAM,
	"select",	DBG_SELECT,
	"direct",	DBG_DIRECT,
	"intr",		DBG_INTR,
	"start",	DBG_START,
	"ioctl",	DBG_IOCTL,
	"xprint",	DBG_XPRINT,
	"fail",		DBG_FAIL,
	"autoboot",	DBG_AUTOBOOT,
	"download",	DBG_DOWNLOAD,
	"drain",	DBG_DRAIN,
	"poll",		DBG_POLL,
	0,		0
};

struct opt {
	char	*o_name;
	void	(*o_func) __P((int, char **));
} opt[] = {
	"debug",		debug,
	"boot",			boot,
	"rxint_throttle",	rxint,
	"int_throttle",		txint,
	"txlowat",		txlowat,
	"ixany",		ixany,
	"mstate",		mstate,
	"modem",		modem,
	"cookmode",		cookmode,
	"nport",		nport,
	"hwflow",		hwflow,
	"ppp",			ppp,
	"xpon",			xpon,
	"xpoff",		xpoff,
	"xpcps",		xpcps,
	0,			0,
};

struct stat_list {
	void	(*st_func) __P((int, char **));
} stat_list[] = {
	modem, mstate, ixany, hwflow, txlowat, ppp, cookmode,
	0
};

char *usage[] = {
#define	U_DEBUG		0
	"debug [[add|del|set debug_levels] | [off]]\n",
#define	U_BOOT		1
	"boot [bootfile]\n",
#define	U_TXINT		2
	"int_throttle [newvalue]\n",
#define	U_RXINT		3
	"rxint_throttle [newvalue]\n",
#define	U_BSPACE	4
	"txlowat [newvalue]\n",
#define	U_IXANY		5
	"ixany [on|off]\n",
#define	U_MSTATE	6
	"mstate\n",
#define	U_MODEM		7
	"modem [on|off]\n",
#define	U_COOKMODE	8
	"cookmode [on|off]\n",
#define	U_NPORT		9
	"nport\n",
#define	U_HWFLOW	10
	"hwflow [rts][cts]\n",
#define	U_PPP		11
	"ppp [on|off]\n",
#define	U_XPSTR		12
	"xpon|xpoff [string]\n",
#define	U_XPCPS		13
	"xpcps [chars_per_sec]\n",
	0
};
#define	U_MAX		14
#define	U_ALL		-1

int
main(argc, argv)
	int argc;
	char **argv;
{
	register struct opt *op;
	char *cp;
	void (*func) __P((int, char **));
	sidev_t dev;
	struct stat st;

	if (argc < 2)
		prusage(U_ALL);
	xdevname = argv[1];
	if (strcmp(xdevname, "-") == 0)
		alldev = 1;
	else {
		if (strchr(xdevname, '/') == NULL) {
			cp = malloc(sizeof(_PATH_DEV) + strlen(xdevname));
			if (cp == NULL)
				err(1, "malloc");
			(void)sprintf(cp, "/dev/%s", xdevname);
			xdevname = cp;
		}
		if (stat(xdevname, &st) < 0)
			err(1, "can't stat %s", xdevname);
		dev.sid_card = SI_CARD(st.st_rdev);
		dev.sid_port = SI_PORT(st.st_rdev);
		tc.tc_dev = dev;
	}
	ctlfd = opencontrol();
	if (argc == 2) {
		dostat();
		exit(0);
	}

	func = NULL;
	for (op = opt; op->o_name != NULL; op++) {
		if (strcmp(argv[2], op->o_name) == 0) {
			func = op->o_func;
			break;
		}
	}
	if (func == NULL)
		prusage(U_ALL);
	(*func)(argc - 3, argv + 3);
	exit(0);
}

int
opencontrol()
{
	int fd;

	if ((fd = open(CONTROLDEV, O_RDWR|O_NDELAY)) < 0)
		err(1, "open on %s", CONTROLDEV);
	return (fd);
}

/*
 * Print a usage message - this relies on U_DEBUG==0 and U_BOOT==1.
 * Don't print the DEBUG usage string unless explicity requested.
 */
void
prusage(strn)
	int strn;
{
	char **cp;

	if (strn == U_ALL) {
		fprintf(stderr, "Usage: siconfig - %s", usage[1]);
		fprintf(stderr, "       siconfig devname %s", usage[2]);
		for (cp = &usage[3]; *cp != NULL; cp++)
			fprintf(stderr, "%24s%s", "", *cp);
	} else if (strn == U_BOOT)
		fprintf(stderr, "Usage: siconfig - %s", usage[U_BOOT]);
	else if (strn >= 0 && strn <= U_MAX)
		fprintf(stderr, "Usage: siconfig devname %s", usage[strn]);
	else
		fprintf(stderr, "siconfig: usage ???\n");
	exit(1);
}

/* print port status */
void
dostat()
{
	register struct stat_list *stp;
	size_t len;
	char *leadin;
	char sepch;
	char *av[1];
	struct si_tcsi savetc;

	leadin = alldev ? "ALL" : xdevname;
	sepch = ':';
	len = strlen(leadin);
	savetc = tc;
	for (stp = stat_list; stp->st_func != NULL; stp++) {
		printf("%*s%c ", len, leadin, sepch);
		leadin = "";
		sepch = ' ';
		av[0] = NULL;
		tc = savetc;
		(*stp->st_func)(-1, av);
	}
}

void
boot(ac, av)
	int ac;
	char **av;
{
	register int n, f;
	struct si_loadcode load;
	extern unsigned char si_download[];
	extern int si_dsize;

	if (ac > 1 || alldev == 0)
		prusage(U_BOOT);
	load.sd_offset = 0;
	if (ac > 0) {
		if ((f = open(av[0], O_RDONLY, 0)) < 0)
			err(1, "can't open download file %s", av[0]);
		while ((n = read(f, load.sd_bytes, sizeof load.sd_bytes)) > 0) {
			load.sd_nbytes = n;
			if (ioctl(ctlfd, TCSIDOWNLOAD, (char *)&load))
				err(1, "TCSIDOWNLOAD");
			load.sd_offset += n;
		}
		if (load.sd_nbytes < 0)
			err(1, "error reading download file %s", av[0]);
		(void)close(f);
	} else {
		f = si_dsize;
		while (f > 0) {
			n = MIN(f, sizeof load.sd_bytes);
			bcopy(&si_download[load.sd_offset], load.sd_bytes, n);
			load.sd_nbytes = n;
			if (ioctl(ctlfd, TCSIDOWNLOAD, (char *)&load))
				err(1, "TCSIDOWNLOAD");
			load.sd_offset += n;
			f -= n;
		}
	}
	if (ioctl(ctlfd, TCSIBOOT, (char *)&tc))
		err(1, "TCSIBOOT");
	exit(tc.tc_int == 0 ? 1 : 0);
}

/*
 * debug
 * debug [[set|add|del debug_lvls] | [off]]
 */
void
debug(ac, av)
	int ac;
	char **av;
{
	int level;

	if (ac > 2)
		prusage(U_DEBUG);
	if (alldev) {
		if (ioctl(ctlfd, TCSIGDBG_ALL, &tc.tc_dbglvl) < 0)
			err(1, "TCSIGDBG_ALL on %s", xdevname);
	} else {
		if (ioctl(ctlfd, TCSIGDBG_LEVEL, &tc) < 0)
			err(1, "TCSIGDBG_LEVEL on %s", xdevname);
	}

	switch (ac) {

	case 0:
		printf("%s: debug levels -", xdevname);
		prlevels(tc.tc_dbglvl);
		return;

	case 1:
		if (strcmp(av[0], "off") == 0) {
			tc.tc_dbglvl = 0;
			break;
		}
		prusage(U_DEBUG);
		/* NOTREACHED */

	case 2:
		level = lvls2bits(av[1]);
		if (strcmp(av[0], "add") == 0)
			tc.tc_dbglvl |= level;
		else if (strcmp(av[0], "del") == 0)
			tc.tc_dbglvl &= ~level;
		else if (strcmp(av[0], "set") == 0)
			tc.tc_dbglvl = level;
		else
			prusage(U_DEBUG);
	}
	if (alldev) {
		if (ioctl(ctlfd, TCSISDBG_ALL, &tc.tc_dbglvl) < 0)
			err(1, "TCSISDBG_ALL on %s", xdevname);
	} else {
		if (ioctl(ctlfd, TCSISDBG_LEVEL, &tc) < 0)
			err(1, "TCSISDBG_LEVEL on %s", xdevname);
	}
}

void
rxint(ac, av)
	int ac;
	char **av;
{
	tc.tc_port = 0;
	switch (ac) {
	case 0:
		printf("%s: ", xdevname);
		/* FALLTHROUGH */
	case -1:
		if (ioctl(ctlfd, TCSIGRXIT, &tc) < 0)
			err(1, "TCSIGRXIT");
		printf("RX interrupt throttle: %d\n", tc.tc_int);
		break;
	case 1:
		tc.tc_int = getnum(av[0]);
		if (tc.tc_int == 0)
			tc.tc_int = 1;
		if (ioctl(ctlfd, TCSIRXIT, &tc) < 0)
			err(1, "TCSIRXIT on %s at %d", xdevname,
			    tc.tc_int);
		break;
	default:
		prusage(U_RXINT);
	}
}

void
txint(ac, av)
	int ac;
	char **av;
{

	tc.tc_port = 0;
	switch (ac) {
	case 0:
		printf("%s: ", xdevname);
		/* FALLTHROUGH */
	case -1:
		if (ioctl(ctlfd, TCSIGIT, &tc) < 0)
			err(1, "TCSIGIT");
		printf("aggregate interrupt throttle: %d\n", tc.tc_int);
		break;
	case 1:
		tc.tc_int = getnum(av[0]);
		if (ioctl(ctlfd, TCSIIT, &tc) < 0)
			err(1, "TCSIIT on %s at %d", xdevname, tc.tc_int);
		break;
	default:
		prusage(U_TXINT);
	}
}

void
txlowat(ac, av)
	int ac;
	char **av;
{
	int i;

	tc.tc_port = 0;
	switch (ac) {
	case 0:
		printf("%s: ", xdevname);
		/* FALLTHROUGH */
	case -1:
		if (ioctl(ctlfd, TCSIGMIN, &tc) < 0)
			err(1, "TCSIGMIN");
		printf("TX buffer low water mark - %d bytes\n",
		    SLXOS_BUFFERSIZE - tc.tc_int);
		break;
	case 1:
		i = getnum(av[0]);
		if (i < MIN_TXWATER || i > MAX_TXWATER) {
			printf(
	"requested tx low water mark '%d' outside sensible range %d - %d\n",
			    i, MIN_TXWATER, MAX_TXWATER);
			return;
		}
		tc.tc_int = i;
		if (ioctl(ctlfd, TCSIMIN, &tc) < 0)
			err(1, "TCSIMIN on %s at %d", xdevname, tc.tc_int);
		break;
	default:
		prusage(U_BSPACE);
	}
}

void
ixany(ac, av)
	int ac;
	char **av;
{
	onoff(ac, av, TCSIIXANY, "TCSIIXANY", "ixany mode", U_IXANY);
}

void
modem(ac, av)
	int ac;
	char **av;
{
	onoff(ac, av, TCSIMODEM, "TCSIMODEM", "modem mode", U_MODEM);
}

void
cookmode(ac, av)
	int ac;
	char **av;
{
	onoff(ac, av, TCSICOOKMODE, "TCSICOOKMODE", "cookmode", U_COOKMODE);
}

void
ppp(ac, av)
	int ac;
	char **av;
{
	onoff(ac, av, TCSIPPP, "TCSIPPP", "PPP mode", U_PPP);
}

void
onoff(ac, av, cmd, cmdstr, prstr, usage)
	int ac;
	char **av;
	int cmd;
	char *cmdstr, *prstr;
	int usage;
{
	if (ac > 1)
		prusage(usage);
	if (ac == 1) {
		if (strcmp(av[0], "on") == 0)
			tc.tc_int = 1;
		else if (strcmp(av[0], "off") == 0)
			tc.tc_int = 0;
		else
			prusage(usage);
	} else
		tc.tc_int = -1;
	if (ioctl(ctlfd, cmd, &tc) < 0)
		err(1, "%s on %s", cmdstr, xdevname);
	switch (ac) {
	case 0:
		printf("%s: ", xdevname);
		/* FALLTHROUGH */
	case -1:
		printf("%s - %s\n", prstr, tc.tc_int ? "on" : "off");
		/* FALLTHROUGH */

	/* default = return */
	}
}

void
mstate(ac, av)
	int ac;
	char **av;
{
	switch (ac) {
	case 0:
		printf("%s: ", xdevname);
		/* FALLTHROUGH */
	case -1:
		break;
	default:
		prusage(U_MSTATE);
	}
	if (ioctl(ctlfd, TCSISTATE, &tc) < 0)
		err(1, "TCSISTATE on %s", xdevname);
	printf("modem bits state - (0x%x)", tc.tc_int);
	if (tc.tc_int & IP_DCD)
		printf(" DCD");
	if (tc.tc_int & IP_DTR)
		printf(" DTR");
	if (tc.tc_int & IP_RTS)
		printf(" RTS");
	printf("\n");
}

void
nport(ac, av)
	int ac;
	char **av;
{
	int ports;

	if (ac != 0)
		prusage(U_NPORT);
	if (ioctl(ctlfd, TCSIPORTS, &ports) < 0)
		err(1, "TCSIPORTS on %s", xdevname);
	printf("SLXOS: total of %d ports\n", ports);
}

void
hwflow(ac, av)
	int ac;
	char **av;
{
	if (ac > 1)
		prusage(U_HWFLOW);
	if (ac <= 0)
		tc.tc_int = -1;
	else {
		tc.tc_int = 0;
		if (strstr(av[0], "rts") != 0 || strstr(av[0], "RTS") != 0)
			tc.tc_int = SPF_RTSIFLOW;
		if (strstr(av[0], "cts") != 0 || strstr(av[0], "CTS") != 0)
			tc.tc_int |= SPF_CTSOFLOW;
	}
	if (ioctl(ctlfd, TCSIFLOW, &tc) < 0)
		err(1, "TCSIFLOW on %s", xdevname);
	switch (ac) {
	case 0:
		printf("%s: ", xdevname);
		/* FALLTHROUGH */
	case -1:
		printf("H/W flow -");
		if (tc.tc_int == 0)
			printf(" (none set)");
		else {
			if (tc.tc_int & SPF_CTSOFLOW)
				printf(" CTSOFLOW");
			if (tc.tc_int & SPF_RTSIFLOW)
				printf(" RTSIFLOW");
		}
		printf("\n");
	}
}

void
xpon(ac, av)
	int ac;
	char **av;
{
	xpon_off(ac, av, 1);
}

void
xpoff(ac, av)
	int ac;
	char **av;
{
	xpon_off(ac, av, 0);
}

/* [string] */
void
xpon_off(ac, av, on)
	int ac;
	char **av;
	int on;
{
	if (ac < 0 || ac > 1)
		prusage(U_XPSTR);
	checkxprint();
	switch (ac) {
	case 0:
		printf("%s ", xdevname);
		/* FALLTHROUGH */
	case -1:
		if (ioctl(ctlfd, on ? TCSIGXPON : TCSIGXPOFF, &tc) < 0)
			err(1, "%s on %s", on ? "TCSIGXPON" : "TCSIGXPOFF",
			    xdevname);
		printf("%s - ", on ? "xpon" : "xpoff");
		prxpstring(tc.tc_string);
		break;
	default:
		getxpstring(av[0], tc.tc_string);
		if (ioctl(ctlfd, on ? TCSIXPON : TCSIXPOFF, &tc) < 0)
			err(1, "%s on %s", on ? "TCSIXPON" : "TCSIXPOFF",
			    xdevname);
	}
}

void
prxpstring(str)
	char *str;
{
	char *space;

	if ((space = malloc(strlen(str) * 4 + 1)) == NULL)
		err(1, "malloc");
	(void)strvis(space, str, VIS_WHITE | VIS_CSTYLE);
	printf("\"%s\"\n", space);
	free(space);
}

/* translate the C syntax string in `str' into byte in `buf' */
void
getxpstring(str, buf)
	char *str, *buf;
{
	char *space;

	if ((space = malloc(strlen(str) + 1)) == NULL)
		err(1, "malloc");
	if (strunvis(space, str) < 0)
		errx(1, "unknown or invalid escape sequence in \"%s\"\n", str);
	if (strlen(space) > XP_MAXSTR)
		errx(1, "specified string is too long\n");
	strcpy(buf, space);
	free(space);
}

void
xpcps(ac, av)
	int ac;
	char **av;
{
	checkxprint();
	switch (ac) {
	case 0:
		printf("%s: ", xdevname);
		/* FALLTHROUGH */
	case -1:
		if (ioctl(ctlfd, TCSIGXPCPS, &tc) < 0)
			err(1, "TCSIGXPCPS on %s", xdevname);
		printf("xpcps -  %d\n", tc.tc_int);
		break;
	case 1:
		tc.tc_int = getnum(av[0]);
		if (ioctl(ctlfd, TCSIXPCPS, &tc) < 0)
			err(1, "TCSIXPCPS on %s", xdevname);
		break;
	default:
		prusage(U_XPCPS);
	}
}

void
checkxprint()
{

	/* try grabbing the XPCPS value */
	if (ioctl(ctlfd, TCSIGXPCPS, &tc) < 0)
		err(1, "TCSIGXPCPS on %s", xdevname);
	if (tc.tc_int < 0) {
		printf("XPRINT driver not configured\n");
		exit(1);
	}
}

/*
 * Convert a string consisting of tokens separated by white space, commas
 * or `|' into a bitfield - flag any unrecognised tokens.  Note that this
 * clobbers the input string.
 */
int
lvls2bits(str)
	char *str;
{
	register int i, bits, err;
	register char *tok;

	bits = err = 0;
	while ((tok = strsep(&str, ",| \t")) != NULL) {
		if (*tok == '\0')
			continue;
		if (strcmp(tok, "all") == 0)	/* should use strcasecmp? */
			bits = ~0;
		else if ((i = onelevel(tok)) != 0)
			bits |= i;
		else {
			warnx("unknown token '%s'", tok);
			err = 1;
		}
	}
	if (err)
		exit(1);
	return (bits);
}

int
onelevel(tok)
	const char *tok;
{
	register struct lv *lvp;

	for (lvp = lv; lvp->lv_name != NULL; lvp++)
		if (strcasecmp(lvp->lv_name, tok) == 0)
			return (lvp->lv_bit);
	return (0);
}

int
getnum(str)
	const char *str;
{
	register long l;
	char *ep;

	errno = 0;
	if (!isdigit(*str) || (l = strtol(str, &ep, 10)) < 0 || *ep != 0 ||
	    l >= INT_MAX || (l == LONG_MAX && errno != 0))
		errx(1, "%s is not a number", str);
	return (l);
}

void
prlevels(x)
	register int x;
{
	register struct lv *lvp;

	switch (x) {
	case 0:
		printf(" (none)\n");
		break;
	case ~0:
		printf(" all\n");
		break;
	default:
		for (lvp = lv; lvp->lv_name != NULL; lvp++)
			if (x & lvp->lv_bit)
				printf(" %s", lvp->lv_name);
		printf("\n");
	}
}
