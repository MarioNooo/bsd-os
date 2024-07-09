/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Concatenate strings into allocated memory.
**	Variable number of args terminated by a NULLSTR.
**
**	RCSID concat.c,v 2.1 1995/02/03 13:20:20 polk Exp
**
**	concat.c,v
**	Revision 2.1  1995/02/03 13:20:20  polk
**	Update all revs to 2.1
**	
 * Revision 1.1.1.1  1992/09/28  20:08:49  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

#define	NO_VA_FUNC_DECLS

#include	"global.h"

char *
concat(va_alist)
	va_dcl
{
	register va_list vp;
	register char *	ap;
	register char *	cp;
	register int	size	= 0;
	char *		string;

	va_start(vp);
	while ( (ap = va_arg(vp, char *)) != NULLSTR )
		size += strlen(ap);
	va_end(vp);

	string = cp = Malloc(size+1);

	va_start(vp);
	while ( (ap = va_arg(vp, char *)) != NULLSTR )
		cp = strcpyend(cp, ap);
	va_end(vp);

	return string;
}
