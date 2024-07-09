/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Yet another date/time format.
**
**	RCSID DateTimeStr.c,v 2.2 1999/05/24 19:41:31 polk Exp
**
**	DateTimeStr.c,v
**	Revision 2.2  1999/05/24 19:41:31  polk
**	Y2K fixes
**	
**	Revision 2.1  1995/02/03 13:17:42  polk
**	Update all revs to 2.1
**	
 * Revision 1.1.1.1  1992/09/28  20:08:41  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

#include	"global.h"
#include	"debug.h"

#include	<time.h>



/*
**	Return date in form "yy/mm/dd hh:mm:ss".
*/

char *
DateTimeStr(date, string)
	Time_t			date;
	char *			string;	/* Pointer to buffer of length DATETIMESTRLEN */
{
	register struct tm *	tmp;

	tmp = localtime((long *)&date);

	(void)sprintf
	(
		string,
		"%d/%02d/%02d %02d:%02d:%02d",
		tmp->tm_year+1900, tmp->tm_mon+1, tmp->tm_mday,
		tmp->tm_hour, tmp->tm_min, tmp->tm_sec
	);

	return string;
}
