/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Return date/time in form "mm/dd-hh:mm".
**
**	RCSID UUCP_time.c,v 2.2 1998/09/22 05:39:26 torek Exp
**
**	UUCP_time.c,v
**	Revision 2.2  1998/09/22 05:39:26  torek
**	The usual "time_t vs long" (remove bogus casts)
**	
 * Revision 2.1  1995/02/03  13:19:12  polk
 * Update all revs to 2.1
 *
 * Revision 1.1.1.1  1992/09/28  20:08:42  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	TIME

#include	"global.h"



char *
UUCPDateTimeStr(ts, string)
	Time_t			ts;
	char *			string;	/* Pointer to buffer of length UUCPDATETIMESTRLEN */
{
	register struct tm *	tmp;

	tmp = localtime(&ts);

	(void)sprintf
	(
		string,
		"%02d/%02d-%02d:%02d",
		tmp->tm_mon+1, tmp->tm_mday,
		tmp->tm_hour, tmp->tm_min
	);

	return string;
}
