/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_prot.c,v 2.1 1995/10/20 14:48:30 donn Exp
 */

#include <sys/param.h>
#include <errno.h>
#include <pwd.h>
#include <unistd.h>

#include "emulate.h"
#include "sco.h"

#define	SCO_STOPIO	0
#define	SCO_GETLUID	1
#define	SCO_SETLUID	2
#define	SCO_GETPRIV	3
#define	SCO_SETPRIV	4

int
sco_stopio(s)
	const char *s;
{

	/* not quite the same, but good enough? */
	return (revoke(s));
}

int
sco_getluid()
{
	char *name;
	struct passwd *p;

	if ((name = getlogin()) == NULL || (p = getpwnam(name)) == NULL) {
		errno = EPERM;
		return (-1);
	}

	return (p->pw_uid);
}

int
sco_setluid(uid)
	unsigned short uid;
{
	struct passwd *p;

	if (getlogin()) {
		errno = EPERM;
		return (-1);
	}
	if ((p = getpwuid(uid)) == NULL) {
		errno = EINVAL;
		return (-1);
	}

	return (setlogin(p->pw_name));
}

/* the SETOWNER bit is off; should it be off? */
unsigned long privileges = 0x7f;

int
sco_getpriv(type, p)
	int type;
	unsigned long *p;
{

	if (type != 1) {
		errno = EINVAL;
		return (-1);
	}

	*p = privileges;
	return (sizeof (privileges));
}

int
sco_setpriv(type, p)
	int type;
	const unsigned long *p;
{

	if (type != 1) {
		errno = EINVAL;
		return (-1);
	}
	if ((*p | privileges) != privileges && geteuid()) {
		errno = EPERM;
		return (-1);
	}

	/* we don't actually implement any privileges */

	return (0);
}

int
sco_protctl(cookie, a, b)
	int cookie, a, b;
{

	switch (cookie) {
	case SCO_STOPIO:
		return (sco_stopio((char *)a));
	case SCO_GETLUID:
		return (sco_getluid());
	case SCO_SETLUID:
		return (sco_setluid());
	case SCO_GETPRIV:
		return (sco_getpriv(a, (unsigned long *)b));
	case SCO_SETPRIV:
		return (sco_setpriv(a, (unsigned long *)b));
	}
	errno = EINVAL;
	return (-1);
}
