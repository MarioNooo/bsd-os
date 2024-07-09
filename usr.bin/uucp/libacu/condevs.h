/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Include control for devices.
**
**	RCSID condevs.h,v 2.1 1995/02/03 13:19:40 polk Exp
**
**	condevs.h,v
**	Revision 2.1  1995/02/03 13:19:40  polk
**	Update all revs to 2.1
**	
 * Revision 1.1.1.1  1992/09/28  20:08:48  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

#define	ERRNO
#define	FILES
#define	FILE_CONTROL
#define	SETJMP
#define	SIGNALS
#define	TERMIOCTL

#include	"global.h"
#include	"../uucico/cico.h"
#include	"../uucico/devices.h"
