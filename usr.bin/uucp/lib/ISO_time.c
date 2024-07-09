/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Return date/time in form "yymmddhhmmss.pppp".
**
**	RCSID ISO_time.c,v 2.3 1999/05/24 19:41:31 polk Exp
**
**	ISO_time.c,v
**	Revision 2.3  1999/05/24 19:41:31  polk
**	Y2K fixes
**	
**	Revision 2.2  1998/09/22 05:39:25  torek
**	The usual "time_t vs long" (remove bogus casts)
**	
 * Revision 2.1  1995/02/03  13:18:23  polk
 * Update all revs to 2.1
 *
 * Revision 1.1.1.1  1992/09/28  20:08:38  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	TIME

#include	"global.h"



char *
ISODateTimeStr(
	Time_t			secs,
	Time_t			usecs,
	char *			string	/* Pointer to buffer of length ISODATETIMESTRLEN */
)
{
	register struct tm *	tmp;

	tmp = localtime(&secs);

	(void)sprintf
	(
		string,
		"%d%02d%02d%02d%02d%02d.%04.0f",
		tmp->tm_year+1900, tmp->tm_mon+1, tmp->tm_mday,
		tmp->tm_hour, tmp->tm_min, tmp->tm_sec,
		(float)usecs/1000.0
	);

	return string;
}
