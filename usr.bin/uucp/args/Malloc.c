/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Guaranteed "malloc".
**
**	RCSID Malloc.c,v 2.1 1995/02/03 13:16:37 polk Exp
**
**	Malloc.c,v
**	Revision 2.1  1995/02/03 13:16:37  polk
**	Update all revs to 2.1
**	
 * Revision 1.1.1.1  1992/09/28  20:08:32  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include	"global.h"


char *
Malloc(size)
	int		size;
{
	register char *	cp;

	if ( (cp = malloc((unsigned)size)) == NULLSTR )
	{
		perror("no memory for malloc");
		abort();
	}

	return cp;
}
