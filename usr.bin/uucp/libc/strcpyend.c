/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Copy s2 into s1 and return address of null in s1.
**
**	RCSID strcpyend.c,v 2.1 1995/02/03 13:20:37 polk Exp
**
**	strcpyend.c,v
**	Revision 2.1  1995/02/03 13:20:37  polk
**	Update all revs to 2.1
**	
 * Revision 1.1.1.1  1992/09/28  20:08:50  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

char *
strcpyend(s1, s2)
	char *		s1;
	char *		s2;
{
	register char *	cp1 = s1;
	register char *	cp2 = s2;

	while ( *cp1++ = *cp2++ );

	return cp1 - 1;
}
