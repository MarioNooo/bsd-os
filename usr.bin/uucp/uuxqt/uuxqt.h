/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Header for uux common routines.
**
**	RCSID uuxqt.h,v 2.1 1995/02/03 13:22:58 polk Exp
**
**	uuxqt.h,v
**	Revision 2.1  1995/02/03 13:22:58  polk
**	Update all revs to 2.1
**	
 * Revision 1.1.1.1  1992/09/28  20:09:04  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:48:05  piers
 * Initial revision
 *
*/

/*
**	Parameters set from arguments.
*/

extern char *	XqtNode;		/* Nodename of command */

/*
**	Miscellaneous definitions.
*/

/*
**	Common subroutines.
*/

extern void	do_uuxqt();
extern void	uuxqt_args(int, char **, char*);
