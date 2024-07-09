/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	`uux' - program to execute commands on remote.
**
**	RCSID main.c,v 2.1 1995/02/03 13:22:42 polk Exp
**
**	main.c,v
**	Revision 2.1  1995/02/03 13:22:42  polk
**	Update all revs to 2.1
**	
 * Revision 1.1.1.1  1992/09/28  20:09:02  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	SYSEXITS

#include	"global.h"



void
main(
	int	argc,
	char *	argv[]
)
{
	uux_args(argc, argv, NULLSTR);

	Checkeuid();

	do_uux();

	finish(EX_OK);
}
