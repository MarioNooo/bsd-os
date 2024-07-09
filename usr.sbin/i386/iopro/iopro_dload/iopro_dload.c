/*      BSDI iopro_dload.c,v 1.3 1995/10/24 21:37:02 cp Exp	*/
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
 *	Chase Research IOPRO unit downloader
 *
 *	SYNOPSIS: iopro_dload file device
 *
 *	Performs the downloading of an IOPRO external unit and starts the
 *	processor.
 *
 *	iopro_dload.c,v 1.5 1995/07/28 08:59:51 rjd Exp
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <i386/isa/aimioctl.h>

/* One day I will find a compiler that handles "enum Bool { FALSE, TRUE }"
 * nicely and don't blow up on "if ( bool1 && bool2 )".
 * In the mean time this will have to do.
 */
typedef int		Bool;
#if ! defined ( TRUE )
#   define TRUE		(1==1)
#   define FALSE	(!TRUE)
#endif


char	*progname = "iopro_dload";		/* Name for messages */
int	dlfile;					/* File to download */
int	dldev;					/* Device to download to */

/*
 *	Download file structure
 *
 *	It's kept "simple stupid" for the benefit of DOMESTOS programs.
 *	The file consists of 1K blocks the first being a header and the
 *	remainder data. The header contains a two byte magic number followed
 *	by a 256 byte array of flags, each flag corresponding to a 1K page
 *	of the host visible address space in the external unit. If a flag is
 *	0 the page is uninitialised and should be skipped, if 1 then it should
 *	be initialised from the next block in the file, if 2 then it should
 *	be zeroed.
 */
#define DL_PAGE		1024
#define DL_NPAGES	256
#define	DL_MAGIC	0x1237		/* Low byte first storage */

#define DL_NOINIT	0
#define DL_INIT		1
#define DL_ZERO		2

char	dl_flags[ DL_NPAGES ];


/*
 *	Simple support macros
 */
#define usage()	{ \
		    fprintf ( stderr,"usage: %s file device\n", progname ); \
		    exit ( 1 ); \
		}

/*
 *	Open the download file and read the header
 */
void
open_dlfile ( name )
	char *name;
{
	unsigned short magic;


	if (( dlfile = open ( name, O_RDONLY )) < 0 )
	{
	    fprintf ( stderr,"%s: cannot open \"%s\" for reading\n", progname
	    							, name );
	    perror ( progname );
	    exit ( 1 );
	}

	/* Read magic number and flags */
	if ( read ( dlfile, &magic, sizeof( magic )) != sizeof ( magic )
				|| read ( dlfile, dl_flags, sizeof( dl_flags ))
				!= sizeof ( dl_flags ))
	{
	    fprintf ( stderr,"%s: cannot read \"%s\" header\n", progname, name);
	    perror ( progname );
	    exit ( 1 );
	}

	if ( magic != DL_MAGIC )
	{
	    fprintf ( stderr,"%s: bad magic in \"%s\"\n", progname, name);
	    exit ( 1 );
	}
}

/*
 *	Download the file ( or check )
 */
void
do_dload ( check )
	Bool	check;
{
	int	page;
	off_t	last_loc = -1;
	char	dbuf[ DL_PAGE ];
	Bool	buf_is_zero = FALSE;
	char	rbuf[ DL_PAGE ];
	int	flag;

	/* Position for first data block */ 
	if ( lseek ( dlfile, (off_t)DL_PAGE, SEEK_SET ) < 0 )
	{
	    fprintf ( stderr,"%s: cannot seek to data in download file\n"
								, progname );
	    perror ( progname );
	    exit ( 1 );
	}

	for ( page = 0 ; page < DL_NPAGES ; ++page )
	{
	    /* get data */
	    switch ( flag = dl_flags[ page ])
	    {
	    case DL_NOINIT:
		/* Nothing to do for this page */
		continue;

	    case DL_INIT:
		if ( read ( dlfile, dbuf, DL_PAGE ) != DL_PAGE )
		{
		    fprintf ( stderr,"%s: cannot read data for page %d\n"
							, progname, page );
		    perror ( progname );
		    exit ( 1 );
		}
		buf_is_zero = FALSE;
		break;

	    case DL_ZERO:
		if ( ! buf_is_zero )
		{
		    bzero ( dbuf, DL_PAGE );
		    buf_is_zero = TRUE;
		}
		break;

	    default:
		fprintf ( stderr,"%s: bad flag %d for page %d\n", progname
								, flag, page );
		exit ( 1 );
	    }

	    /* Seek to correct position */
	    if ( last_loc != page * DL_PAGE )
	    {
		if ( lseek ( dldev, page * DL_PAGE, SEEK_SET ) < 0 )
		{
		    fprintf ( stderr
		    		,"%s: cannot seek to page %d of download dev\n"
				, progname );
		    perror ( progname );
		    exit ( 1 );
		}
		last_loc = page * DL_PAGE;
	    }

	    if ( check )
	    {
		if ( read ( dldev, rbuf, DL_PAGE ) != DL_PAGE )
		{
		    fprintf ( stderr,"%s: cannot read back page %d\n"
							, progname, page );
		    perror ( progname );
		    exit ( 1 );
		}

		if ( memcmp ( dbuf, rbuf, DL_PAGE ))
		{
		    fprintf ( stderr,"%s: page %d not read back correctly\n"
							, progname, page );
		    exit ( 1 );
		}
	    }
	    else
	    {
		if ( write ( dldev, dbuf, DL_PAGE ) != DL_PAGE )
		{
		    fprintf ( stderr,"%s: cannot write page %d\n"
							, progname, page );
		    perror ( progname );
		    exit ( 1 );
		}
	    }

	    last_loc += DL_PAGE;
	}
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
	/* Set program name for messages */
	if (( progname = strrchr ( argv[0],'/')) != NULL )
	    ++progname;
	else
	    progname = argv[0];

	/* Check arguments */
	if ( argc != 3 )
	    usage ();

	/* Open the files */
	open_dlfile ( argv[1] );

	if (( dldev = open ( argv[2], O_RDWR )) < 0 )
	{
	    fprintf ( stderr,"%s: cannot access \"%s\" ", progname, argv[2] );
	    switch ( errno )
	    {
	    case ENOENT:
		fprintf ( stderr,"unit not configured in /dev\n");
		break;

	    case ENXIO:
		fprintf ( stderr,"unit not present\n");
		break;

	    case EIO:
		fprintf ( stderr,"unit not powered\n");
		break;

	    default:
		fprintf ( stderr,"errno = %d\n", errno );
		break;
	    }
	    exit ( 1 );
	}

	/* Do the deed */
	iopro_reset ( dldev );
	do_dload ( FALSE );
	do_dload ( TRUE );
	iopro_release ( dldev );

	/* All done */
	/* Remember to wait two seconds before poking the configuration
	 * interface
	 */
	exit ( 0 );
}
