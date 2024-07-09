/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	System error/warning on ErrorFd.
**
**	SysWarn returns true if should try again.
**
**	RCSID SysWarn.c,v 2.2 1996/06/11 19:27:09 vixie Exp
**
**	SysWarn.c,v
**	Revision 2.2  1996/06/11 19:27:09  vixie
**	2.2
**	
**	Revision 2.1  1995/02/03 13:19:08  polk
**	Update all revs to 2.1
**
 * Revision 1.3  1995/01/08  07:48:19  donn
 * Some changes from Paul Vixie.
 *
 * Revision 1.1  1994/12/28  06:07:59  vixie
 * ckp
 *
 * Revision 1.2  1993/02/28  15:28:48  pace
 * Add recent uunet changes
 *
 * Revision 1.1.1.1  1992/09/28  20:08:37  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ERRNO
#define	NO_VA_FUNC_DECLS
#define	SETJMP
#define	STDIO
#define	SYSEXITS

#include	"global.h"

#define		MAXZERO		3
static int	ZeroErrnoCount;	/* Count incidences of errno==0 */
static char	SysErrStr[]	= english("system error");
static char	SysWrnStr[]	= english("system warning");

#define	Fprintf	(void)fprintf
#define	Fflush	(void)fflush


#ifndef __bsdi__
extern char *	sys_errlist[];
extern int	sys_nerr;

/*
**	Return string representing <errno>.
*/

const char *
strerror(en)
	int		en;
{
	static char	errs[6+10+1];

	if ( en < sys_nerr && en > 0 )
		return sys_errlist[en];

	(void)sprintf(errs, english("Error %d"), en);
	return errs;
}
#endif


/*
**	Write error record to ErrorFd, and ErrorLog.
**
**	Return true if recoverable resource error, else false.
*/
static char	errs[]	= ": %s";

static bool
sys_common(type, en, vp)
	char *		type;
	int		en;
	va_list		vp;
{
	bool		val;

	if ( (val = SysRetry(en)) == true && en == 0 )
		return true;

	MesgV(type, vp);

	Fprintf(ErrorFd, errs, strerror(en));
	(void)fputc('\n', ErrorFd);
	Fflush(ErrorFd);

#	ifdef	USE_SYSLOG
	if ( en == ENOMEM )
		syslog(LOG_ERR, "No memory");
	else
#	endif	/* USE_SYSLOG */

	return val;
}



/*
**	Report system error, terminate if non-recoverable, else return.
*/

void
SysError(va_alist)
	va_dcl
{
	va_list		vp;
	int		en;

	if ( (en = errno) == EINTR )
		return;

	va_start(vp);

	FreeStr(&ErrString);
	if ( en != ENOMEM )
		ErrString = newvprintf(vp);
	va_end(vp);

	va_start(vp);
	if ( sys_common(SysErrStr, en, vp) )
	{
		va_end(vp);
		errno = en;
		return;
	}
	va_end(vp);

	va_start(vp);
	if ( ErrorLogV_1(SysErrStr, vp) )
		ErrorLogN(errs, strerror(en));
	va_end(vp);
	va_start(vp);
	if ( ErrorLogV_2(SysErrStr, vp) )
		ErrorLogN(errs, strerror(en));
	va_end(vp);

	if ( ErrFlag != ert_exit )
		finish(EX_OSERR);

	exit(EX_OSERR);
}



/*
**	Test if error is retry-able, and if so, sleep, return true.
*/

bool
SysRetry(
	int	en
)
{
	switch ( errno = en )
	{
	default:
		ZeroErrnoCount = 0;
		return false;

	case 0:
		if ( ++ZeroErrnoCount > MAXZERO )
			return false;
		(void)sleep(10);
		break;

	case EINTR:
		ZeroErrnoCount = 0;
		return true;

	case ENFILE:
	case ENOSPC:
	case EAGAIN:

#	ifdef	EALLOCFAIL
	case EALLOCFAIL:
#	endif	/* EALLOCFAIL */

#	ifdef	ENOBUFS
	case ENOBUFS:
#	endif	/* ENOBUFS */

		ZeroErrnoCount = 0;
		(void)sleep(20);
		break;
	}

	errno = en;
	return true;
}



/*
**	Report system error, return true if recoverable, else false.
*/

bool
SysWarn(va_alist)
	va_dcl
{
	va_list		vp;
	int		en;
	bool		val;

	if ( (en = errno) == EINTR )
		return true;

	va_start(vp);
	val = sys_common(SysWrnStr, en, vp);
	va_end(vp);

	va_start(vp);
	if ( ErrorLogV_1(SysWrnStr, vp) )
		ErrorLogN(errs, strerror(en));
	va_end(vp);
	va_start(vp);
	if ( ErrorLogV_2(SysWrnStr, vp) )
		ErrorLogN(errs, strerror(en));
	va_end(vp);

	errno = en;

	return val;
}
