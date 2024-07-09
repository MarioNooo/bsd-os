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
**	RCSID Malloc.c,v 2.1 1995/02/03 13:18:36 polk Exp
**
**	Malloc.c,v
**	Revision 2.1  1995/02/03 13:18:36  polk
**	Update all revs to 2.1
**	
 * Revision 1.1.1.1  1992/09/28  20:08:40  trent
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

	while ( (cp = malloc((unsigned)size)) == NULLSTR )
		SysError("no mem for malloc(%d)", size);

	TraceT(8, (8, "malloc(%d) => %#lx", size, (long)cp));

	return cp;
}

#ifdef	free
#undef	free

void
Free(str)
	char *	str;
{
	TraceT(8, (8, "\tfree(%#lx)", (long)str));
	free(str);
}
#endif	/* free */
