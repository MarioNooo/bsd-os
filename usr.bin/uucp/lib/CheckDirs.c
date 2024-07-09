/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Create all directories in ``name'' if errno == ENOENT.
**
**	RCSID CheckDirs.c,v 2.1 1995/02/03 13:17:30 polk Exp
**
**	CheckDirs.c,v
**	Revision 2.1  1995/02/03 13:17:30  polk
**	Update all revs to 2.1
**	
 * Revision 1.1.1.1  1992/09/28  20:08:39  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ERRNO
#define	FILE_CONTROL

#include	"global.h"


int	LogDirLen;



bool
CheckDirs(name)
	register char *	name;
{
	Trace((2, "CheckDirs(%s)", name));

	if ( errno != ENOENT )
		return false;

	while ( name[0] == '/' )
	{
		if ( strncmp(name, SPOOLDIR, SpoolDirLen) == STREQUAL )
			break;

		if ( LogDirLen == 0 )
			LogDirLen = strlen(LOGDIR);

		if ( strncmp(name, LOGDIR, LogDirLen) == STREQUAL )
			break;

		return false;
	}

	GetNetUid();

	return MkDirs(name, NetUid, NetGid);
}
