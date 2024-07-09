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
 *	Wait for Chase Research IOPRO port to become available
 *
 *	SYNOPSIS: iopro_wait [-v] [device]
 *
 *	Program is necessary because some demons get in a tizzy if they see
 *	device temporarily unavailable before the external unit has been
 *	downloaded and the port is made available.
 *
 *	iopro_wait.c,v 1.2 1995/07/28 08:59:57 rjd Exp
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>


char	*progname = "iopro_wait";		/* Name for messages */
char	*portname = "/dev/tty4a";		/* Device to open */


/* One day I will find a compiler that handles "enum Bool { FALSE, TRUE }"
 * nicely and don't blow up on "if ( bool1 && bool2 )".
 * In the mean time this will have to do.
 */
typedef int		Bool;
#if ! defined ( TRUE )
#   define TRUE		(1==1)
#   define FALSE	(!TRUE)
#endif


/*
 *	Simple support macros
 */
#define usage()	{ \
		    fprintf ( stderr,"usage: %s [-v] [device]\n", progname ); \
		    exit ( 1 ); \
		}


void
main ( argc, argv )
	int	argc;
	char	*argv[];
{
	int	fd;
	Bool	verbose = FALSE;
	Bool	firsttime = TRUE;
	char	*cardname = NULL;

	/* Set program name for messages */
	if (( progname = strrchr ( argv[0],'/')) != NULL )
	    ++progname;
	else
	    progname = argv[0];

	/* Check arguments */
	if ( argc >= 2 && strcmp ( argv[1],"-v") == 0 )
	{
	    verbose = TRUE;
	    --argc;
	    ++argv;
	}
	if ( argc > 2 )
	    usage ();
	if ( argc == 2 )
	    portname = argv[1];

	/* Try to guess the name of the interface cards based on the device
	 * name. Assumes that the device names follow Chase Research
	 * conventions ie. {tty|prn}{unit character}{port character}.
	 */
	/* First check for BSDI convention name {tty}{card}{unit}{port} */
	switch ( portname[ strlen ( portname ) - 3 ])
	{
	    case 'E':
		cardname = "/dev/iopro/c0";
		break;

	    case 'F':
		cardname = "/dev/iopro/c1";
		break;

	    case 'G':
		cardname = "/dev/iopro/c2";
		break;

	    case 'H':
		cardname = "/dev/iopro/c3";
		break;

	    default:

		/* That failed so try old Chase naming  */
		switch ( portname[ strlen ( portname ) - 2 ])
		{
		    case '4': case '5': case '6': case '7':
		    case '8': case '9': case 'A': case 'B':
			cardname = "/dev/iopro/c0";
			break;

		    case 'C': case 'D': case 'E': case 'F':
		    case 'G': case 'H': case 'I': case 'J':
			cardname = "/dev/iopro/c1";
			break;

		    case 'K': case 'L': case 'M': case 'N':
		    case 'O': case 'P': case 'Q': case 'R':
			cardname = "/dev/iopro/c2";
			break;

		    case 'S': case 'T': case 'U': case 'V':
		    case 'W': case 'X': case 'Y': case 'Z':
			cardname = "/dev/iopro/c2";
			break;
		}
	}

	/* Quick trick to prepend /dev to names if required */
	chdir ("/dev");

	/* Wait for the device to be assigned */

	while (( fd = open ( portname, O_RDWR | O_NONBLOCK )) < 0 )
	{
	    switch ( errno )
	    {
	    case EAGAIN:
		if ( verbose && firsttime )
		{
		    fprintf ( stderr,"%s: %s is not yet assigned. waiting...\n"
							, progname, portname );
		}

		/* Wait of the interface card to report power if we have worked
		 * out a name for it.
		 */
	        if ( cardname != NULL )
	        {
	            if (( fd = open ( cardname, O_RDONLY )) >= 0 )
	            {
	            	if ( verbose && firsttime )
	            	{
			    fprintf ( stderr,"%s: Card %s is powered\n"
							, progname, cardname );
			}
			close ( fd );
		    }
		    else
		    {
		    	/* We are silent about errors here as me may have
		    	 * worked out the wrong controlling card name, or may
		    	 * not be privledged enough to access it.
		    	 */
		    	cardname = NULL;
		    }
	        }

		/* The external units are powered so now we busy wait for the
		 * deamon to configure them and make the port assignements.
		 */
		/* This busy wait really gets up my nose but I can't think of
		 * a way to do it without adding special cases into the driver.
		 */
		sleep ( 1 );
		firsttime = FALSE;
		break;

	    default:
		fprintf ( stderr,"%s: open of %s failed. errno = %d\n"
						, progname, portname, errno );
		perror ( progname );
		exit ( 1 );
	    }
	}

	if ( verbose )
	{
	    printf ("%s: non-blocking open succeeded. %s is assigned\n"
	    					, progname, portname );
	}

	/* All done */
	exit ( 0 );
}
