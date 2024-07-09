/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Provide `_sobuf' if missing.
**
**	RCSID sobuf.c,v 2.1 1995/02/03 13:20:34 polk Exp
**
**	sobuf.c,v
**	Revision 2.1  1995/02/03 13:20:34  polk
**	Update all revs to 2.1
**	
 * Revision 1.1.1.1  1992/09/28  20:08:50  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

#define	STDIO
#include	"global.h"

#if	SYSV < 4
char	_sobuf[BUFSIZ];
#else	/* SYSV < 4 */
char	_dummy_sobuf;
#endif	/* SYSV < 4 */
