/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Unix-to-Unix Copy Program.
**
**	(Or at least, the one that starts the ball rolling.)
**
**	RCSID main.c,v 2.1 1995/02/03 13:22:03 polk Exp
**
**	main.c,v
**	Revision 2.1  1995/02/03 13:22:03  polk
**	Update all revs to 2.1
**	
 * Revision 1.1.1.1  1992/09/28  20:08:57  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	SYSEXITS

#include	"global.h"



int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	uucp_args(argc, argv, NULLSTR);

	Checkeuid();

	do_uucp();

	finish(EX_OK);
}
