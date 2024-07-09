/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Find "match" in "string".
**	Return pointer to start of match, or NULLSTR.
**
**	`QuickSearch' algorithm.
**	[D.M.Sunday, CACM Aug '90, Vol.3 No.8 pp.132-142]
**
**	RCSID strstr.c,v 2.1 1995/02/03 13:20:51 polk Exp
**
**	strstr.c,v
**	Revision 2.1  1995/02/03 13:20:51  polk
**	Update all revs to 2.1
**	
 * Revision 1.1  1995/01/08  07:48:37  donn
 * Some changes from Paul Vixie.
 *
 * Revision 1.1  1994/12/28  06:19:26  vixie
 * Initial revision
 *
 * Revision 1.2  1993/12/19  23:05:32  donn
 * Make it match the prototype.
 *
 * Revision 1.1.1.1  1992/09/28  20:08:50  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include	"global.h"

#define	ASIZE	040
#define	AMASK	(ASIZE-1)



char *
strstr(
	register const char *	string,
	register const char *	match
)
{
	register int	i;
	register int	j;
	register int *	a;
	register char *	m;
	register char *	s;
	int		b[ASIZE];

	if ( string == NULLSTR || match == NULLSTR )
		return NULLSTR;

	Trace((5, "strstr(%.22s, %.22s)", string, match));

	for ( m = match, s = string ; *m++ != '\0' ; )
		if ( *s++ == '\0' )
			return NULLSTR;

	if ( (i = m - match) == 1 )
		if ( string[0] == '\0' )
			return string;
		else
			return NULLSTR;

	/** `i' is length of `match' +1 **/

	for ( a = b, j = ASIZE ; --j >= 0 ; )
		*a++ = i;

	for ( a = b, m = match ; --i > 0 ; )
		a[(*m++)&AMASK] = i;

	j = m - match;

	/** `j' is length of `match' **/

	for ( ;; )
	{
		for ( m = match, s = string ; i = *m++ ; )
			if ( i != *s++ )
				break;

		if ( i == '\0' )
		{
			Trace((4, "strstr(%.22s, %.22s) MATCH", string, match));
			return string;
		}

		s = string + j;

		i = a[(*s)&AMASK];

		for ( string += i ; --i >= 0 ; )
			if ( *s++ == '\0' )
				return NULLSTR;
	}
}
