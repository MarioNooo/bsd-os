/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Report fatal error on ErrorFd and abort().
**
**	RCSID Fatal.c,v 2.2 1996/06/11 19:27:00 vixie Exp
**
**	Fatal.c,v
**	Revision 2.2  1996/06/11 19:27:00  vixie
**	2.2
**	
**	Revision 2.1  1995/02/03 13:18:05  polk
**	Update all revs to 2.1
**
 * Revision 1.1.1.1  1992/09/28  20:08:41  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	LOCKING
#define	NO_VA_FUNC_DECLS
#define	STDIO

#include	"global.h"



void
Fatal(va_alist)
	va_dcl
{
	register va_list vp;
	static char	type[]	= english("INTERNAL error");

	va_start(vp);
	FreeStr(&ErrString);
	ErrString = newvprintf(vp);
	va_end(vp);

	va_start(vp);
	MesgV(type, vp);
	va_end(vp);

	putc('\n', ErrorFd);
	(void)fflush(ErrorFd);

	va_start(vp);
	if ( ErrorLogV_1(type, vp) )
		ErrorLogN(NULLSTR);
	va_end(vp);
	va_start(vp);
	if ( ErrorLogV_2(type, vp) )
		ErrorLogN(NULLSTR);
	va_end(vp);

	(void)abort();
}
