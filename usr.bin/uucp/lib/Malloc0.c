/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Clear guaranteed memory.
**
**	RCSID Malloc0.c,v 2.1 1995/02/03 13:18:38 polk Exp
**
**	Malloc0.c,v
**	Revision 2.1  1995/02/03 13:18:38  polk
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
Malloc0(size)
	int		size;
{
	return strclr(Malloc(size), size);
}
