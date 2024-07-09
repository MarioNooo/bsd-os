/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Just in case setuid bit doesn't work for root,
**	fix it here.
**
**	RCSID Checkeuid.c,v 2.1 1995/02/03 13:17:34 polk Exp
**
**	Checkeuid.c,v
**	Revision 2.1  1995/02/03 13:17:34  polk
**	Update all revs to 2.1
**	
 * Revision 1.1.1.1  1992/09/28  20:08:39  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include	"global.h"



void
Checkeuid()
{
	GetNetUid();

	if ( E_uid == NetUid )
		return;

	(void)setegid(NetGid);

	if ( seteuid(NetUid) == SYSERROR )
		SysError(CouldNot, "seteuid", UUCPUSER);

	Trace((1, "Checkeuid() => u=%d g=%d", NetUid, NetGid));
}
