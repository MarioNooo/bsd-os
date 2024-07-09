/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Return pointer to newly allocated string containing 's'.
**
**	RCSID newstr.c,v 2.1 1995/02/03 13:20:28 polk Exp
**
**	newstr.c,v
**	Revision 2.1  1995/02/03 13:20:28  polk
**	Update all revs to 2.1
**	
 * Revision 1.1.1.1  1992/09/28  20:08:49  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

#include	"global.h"

#if	0
char		EmptyStr[]	= "";
#endif	/* 0 */

#ifndef	NULLSTR
#define	NULLSTR	(char *)0
#endif	/* NULLSTR */

char *
newstr(s)
	char *	s;
{
	if ( s == NULLSTR )
		s = EmptyStr;

	return strcpy(Malloc(strlen(s)+1), s);
}
