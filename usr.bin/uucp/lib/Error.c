/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Error handlers.
**
**	RCSID Error.c,v 2.2 1996/06/11 19:26:59 vixie Exp
**
**	Error.c,v
**	Revision 2.2  1996/06/11 19:26:59  vixie
**	2.2
**	
**	Revision 2.1  1995/02/03 13:17:52  polk
**	Update all revs to 2.1
**
 * Revision 1.1.1.1  1992/09/28  20:08:38  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

#define	ERRNO
#define	FILE_CONTROL
#define	NO_VA_FUNC_DECLS
#define	SETJMP
#define	STDIO
#define	SYSEXITS

#include	"global.h"



jmp_buf		ErrBuf;
ERC_t		ErrFlag;
bool		ErrIsatty;
char *		ErrString;
int		ErrVal;
bool		IsattyDone;

#define	Fprintf	(void)fprintf
#define	Fflush	(void)fflush



/*
**	Hard error, message on ErrorFd, and recover.
*/

void
Error(va_alist)
	va_dcl
{
	register va_list vp;
	int		err;
	static char	type[]	= english("error");

	va_start(vp);
	FreeStr(&ErrString);
	ErrString = newvprintf(vp);
	va_end(vp);

	va_start(vp);
	MesgV(type, vp);
	va_end(vp);

	putc('\n', ErrorFd);
	Fflush(ErrorFd);

	va_start(vp);
	if ( ErrorLogV_1(type, vp) )
		ErrorLogN(NULLSTR);
	va_end(vp);
	va_start(vp);
	if ( ErrorLogV_2(type, vp) )
		ErrorLogN(NULLSTR);
	va_end(vp);

	errno = 0;

	if ( (err = ErrVal) == 0 )
		err = EX_SOFTWARE;

	switch ( ErrFlag )
	{
	case ert_here:
		longjmp(ErrBuf, err);
	case ert_finish:
		finish(err);
	case ert_exit:
		exit(err);
	}
}



/*
**	Set pointer to fd of error file, check if error file is a ``tty''.
*/

bool
ErrorTty(fdp)
	int *	fdp;
{
	if ( !IsattyDone )
	{
		ErrIsatty = (bool)isatty(fileno(ErrorFd));
		IsattyDone = true;
	}

	if ( fdp != (int *)0 )
		*fdp = fileno(ErrorFd);

	return ErrIsatty;
}



/*
**	Reset error file knowledge (eg: after fork()).
*/

void
ResetErrTty()
{
	IsattyDone = false;
}



/*
**	Prevent diversions of stderr into new files.
*/

void
StdError()
{
	ErrIsatty = true;
	IsattyDone = true;
}
