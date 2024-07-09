/*
 * Copyright (c) 1993,1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_uname.c,v 2.2 1995/07/10 18:39:52 donn Exp
 */

#include <sys/param.h>
#include <sys/utsname.h>
#include <err.h>
#include <string.h>
#include "emulate.h"
#include "sco.h"

/*
 * Support for uname() and ustat() emulation.
 */

/* from iBCS2 p 6-83 */

#define SCO_UTS_SIZE	9

struct sco_utsname {
	char	sysname[SCO_UTS_SIZE];
	char	nodename[SCO_UTS_SIZE];
	char	release[SCO_UTS_SIZE];
	char	version[SCO_UTS_SIZE];
	char	machine[SCO_UTS_SIZE];
};

/* from the __scoinfo(S) man page */
/* yes, the name is very awkward */

struct scoutsname {
	char	sysname[SCO_UTS_SIZE];
	char	nodename[SCO_UTS_SIZE];
	char	release[16];
	char	kernelid[20];
	char	machine[SCO_UTS_SIZE];
	char	bustype[9];
	char	sysserial[10];
	unsigned short
		sysorigin;
	unsigned short
		sysoem;
	char	numuser[9];
	unsigned short
		numcpu;
};

/*
 * from SVr4 API p 247;
 * I don't know if this is right,
 * but ustat.h is missing from iBCS2 and the SVr4 ABI...
 */
struct sco_ustat {
	long	f_tfree;
	u_short	f_tinode;
	char	f_fname[6];
	char	f_fpack[6];
};

/* iBCS2 p 3-36 */
#define	SCO_UNAME	0
#define	SCO_USTAT	2

/* fake our uname parameters to make us look like SCO 3.2v4.2 */
#define	SCO_SYSNAME	1

void
utsname_out(u, xsu)
	const struct utsname *u;
	struct utsname *xsu;
{
	struct sco_utsname *su = (struct sco_utsname *)xsu;
	char *p;

	bzero(su, sizeof *su);

#define	copy(c) \
	strncpy(su->c, u->c, MIN(sizeof (su->c) - 1, sizeof (u->c)))
#ifndef SCO_SYSNAME
	copy(sysname);
#endif
	copy(nodename);
	if (p = memchr(su->nodename, '.', sizeof su->nodename))
		bzero(p, sizeof su->nodename - (p - su->nodename));
#ifndef SCO_SYSNAME
	copy(release);
	copy(version);
#else
	bcopy(su->nodename, su->sysname, sizeof su->sysname);
	strncpy(su->release, "3.2", sizeof su->release);
	strncpy(su->version, "2", sizeof su->version);
#endif
	copy(machine);
}

int
sco_ustat(dev, buf)
	int dev;
	struct sco_ustat *buf;
{

	errx(1, "unsupported ustat system call");
}

int
sco_uname(buf, dev, cookie)
	void *buf;
	int dev, cookie;
{
	struct utsname u;

	if (cookie == SCO_USTAT)
		return (sco_ustat(dev, (struct sco_ustat *)buf));
	if (uname(&u) == -1)
		return (-1);
	utsname_out(&u, (struct sco_utsname *)buf);
	return (0);
}

int
scoinfo(su)
	struct scoutsname *su;
{
	struct utsname un;
	struct utsname *u = &un;
	char *p;

	if (uname(u) == -1)
		return (-1);
#ifndef SCO_SYSNAME
	copy(sysname);
#endif
	copy(nodename);
	if (p = memchr(su->nodename, '.', sizeof su->nodename))
		bzero(p, sizeof su->nodename - (p - su->nodename));
#ifndef SCO_SYSNAME
	copy(release);
#else
	bcopy(su->nodename, su->sysname, sizeof su->sysname);
	strncpy(su->release, "3.2v4.2", sizeof su->release);
#endif
	/* make something up; some data from uname -X on a real SCO system */
	strncpy(su->kernelid, "94/09/12", sizeof su->kernelid);
#ifndef SCO_SYSNAME
	copy(machine);
#else
	strncpy(su->machine, "i80486", sizeof su->machine);
#endif
	strncpy(su->bustype, "ISA", sizeof su->bustype);
	strncpy(su->sysserial, "123456789", sizeof su->sysserial);
	su->sysorigin = 1;
	su->sysoem = 0;
	strncpy(su->numuser, "999-user", sizeof su->numuser);
	su->numcpu = 1;
	return (0);
}
