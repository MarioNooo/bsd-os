/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Allocate new string with given length and copy arg into it.
**
**	RCSID newnstr.c,v 2.1 1995/02/03 13:20:25 polk Exp
**
**	newnstr.c,v
**	Revision 2.1  1995/02/03 13:20:25  polk
**	Update all revs to 2.1
**	
 * Revision 1.1.1.1  1992/09/28  20:08:49  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include	"global.h"

char *
newnstr(s, n)
	char *		s;
	int		n;
{
	register char *	cp;

	if ( s == NULLSTR )
	{
		s = EmptyStr;
		n = 0;
	}

	cp = strncpy(Malloc(n+1), s, n);

	cp[n] = '\0';

	return cp;
}
