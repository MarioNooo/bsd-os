/*	BSDI iopro_debug.c,v 1.3 1995/10/24 21:36:57 cp Exp	*/
/*-
 *	Copyright (c) 1993 Robert J.Dunlop. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided the following conditions are met:
 *   1.	Redistributions in source form must retain the above copyright, this
 *	list of conditions and the following disclaimers.
 *   2. Redistributions in binary form must reproduce all items from condition
 *	1 in the accompanying documentation and/or other materials.
 *   3. Neither the names of Chase Research PLC and it's subsidiary companies,
 *	nor that of Robert Dunlop may be used to endorse, advertise or promote
 *	products derived from this software without specific written permission
 *	from the party or parties concerned.
 *   4.	Adaptation of this software to operating systems other than BSD/OS
 *	requires the prior written permission of Chase Research PLC.
 *   5.	Adaptation of this software for operation with communications hardware
 *	other than the Chase IOPRO and IOLITE series requires the prior written
 *	permission of Chase Research PLC.
 */

/*
 *	Chase Research IOPRO debug level controler
 *
 *	SYNOPSIS: iopro_debug level
 *
 *	iopro_debug.c,v 1.3 1995/07/28 08:59:50 rjd Exp
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <i386/isa/aimioctl.h>

char	*progname = "iopro_debug";		/* Name for messages */

char	*ctlfile = "/dev/iopro/all";		/* The control file name */


/*
 *	Simple support macros
 */
#define usage()	{ \
		    fprintf ( stderr,"usage: %s level\n", progname ); \
		    exit ( 1 ); \
		}


/*
 *	The root of all evil
 *	====================
 */
void
main ( argc, argv )
	int	argc;
	char	*argv[];
{
	int level;
	int fd;

	/* Set program name for messages */
	if (( progname = strrchr ( argv[0],'/')) != NULL )
	    ++progname;
	else
	    progname = argv[0];

	/* Check arguments */
	if ( argc != 2 || sscanf ( argv[1],"%i", &level ) != 1 )
	    usage ();

	/* Open the control file */
	if (( fd = open ( ctlfile, O_WRONLY )) < 0 )
	{
	    fprintf ( stderr,"%s: cannot access \"%s\"\n", progname, ctlfile );
	    exit ( 1 );
	}

	/* Do the deed */
	ioctl ( fd, IOPRO_DEBUG, &level );

	/* All done */
	exit ( 0 );
}
