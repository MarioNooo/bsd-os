/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Encode a number into a string using ValidChars => VC_SHIFT bits/char.
**
**	RCSID EncodeNum.c,v 2.1 1995/02/03 13:17:50 polk Exp
**
**	EncodeNum.c,v
**	Revision 2.1  1995/02/03 13:17:50  polk
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



int
EncodeNum(name, number, length)
	char *		name;
	register Ulong	number;
	register int	length;
{
	register char *	p;

	if ( length == 0 )
		return length;

	p = name + length;

	for ( ;; )
	{
		*--p = ValidChars[number % VC_size];

		if ( p <= name )
			break;

		number /= VC_size;
	}

	return length;
}
