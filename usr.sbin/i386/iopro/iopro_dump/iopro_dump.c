/*	BSDI	iopro_dump.c,v 1.3 1995/10/24 21:37:07 cp Exp	*/
/*-
 *	Copyright (c) 1994 Robert J.Dunlop. All rights reserved.
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
 *	Chase Research IOPRO shared memory dump
 *
 *	SYNOPSIS: iopro_dump [-v|-h] [ card# [ unit# [ page# ]]]
 *
 *	Diagnostic dump of the shared memory contents. The results show the
 *	live info used by the external units rather than any internal values
 *	stored in the kernel. Comparision of these values with kernel stty
 *	values should help show up any problems.
 *
 *	iopro_dump.c,v 1.9 1995/07/28 08:59:54 rjd
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/fcntl.h>

/* This should be warning enough that this program is non-portable */
#include <i386/isa/aimreg.h>


char	*progname = "iopro_dump";		/* Name for messages */

int	inhex   = 0;				/* Dump hex for debug */
int	verbose = 0;				/* Display more items */

char	*fname = NULL;				/* The data file */
char	fname_buf[] = "/dev/iopro/c?u?d";
char	*fname_fmt = "/dev/iopro/c%1.1du%1.1dd";

int	ports_found = 0;			/* Number of ports found in the
						 * master page.
						 */

						/* Page type strings */
char	*pgtype_str[] = { "free","master","physical","printer"
			,"logical","protocol"};


/*
 *	Simple support macros
 */
#define usage()	{ \
		    fprintf ( stderr, \
	"usage: %s [-h|-v] [-f file] | [ card# [ unit# [ page# ]]]\n" \
		    	, progname ); \
		    exit ( 1 ); \
		}

#define	pf	printf

#define as_str(V,S) { \
			if ( verbose ) \
				pf ("(%d) ", (V)); \
			if ((V)<0||(V)>=sizeof((S))/sizeof(char *)) \
			    pf ("UNKNOWN"); \
			else \
			    pf ("%s", (S)[V]); \
		    }

#define as_strn(V,S)	{ as_str(V,S); pf ("\n"); }

/*
 *	Hex dump a block of memory
 */
#define BPL     16              /* Bytes per line */

void
hexdump ( buf, size, address )
	unsigned char	*buf;
	int		size;
	long		address;
{
	int	i;
	int	got;

	while ( size > 0 )
	{
	    got = min ( BPL, size );

	    /* Print the line */
	    pf ("%6lx", address );
	    for ( i = 0 ; i < got ; i++ )
		pf ( i % ( BPL / 2 ) ? " %02x":"   %02x", buf[ i ]);
	    for ( ; i < BPL ; i++ )
		pf ( i % ( BPL / 2 ) ? "   ":"     ");
	    pf ("  ");
	    for ( i = 0 ; i < got ; i++ )
	    {
		if ( buf[ i ] >= ' ' && buf[ i ] <= '~')
		    pf ("%c", buf[ i ]);
		else
		    pf (".");
	    }
	    pf ("\n");

	    /* Update pointers */
	    buf += got;
	    size -= got;
	    address += got;
	}
}


/*
 *	Dump the firmware master control page
 */
void
dump_master ( mp )
	struct master_page	*mp;
{
	static char *hwver_str[] = {
	    "discrete logic","split DCD/DSR","ASIC","IOLITE","IOLITE 4"
	};
	static char *hwcpu_str[] = {"NONE","80186"};
	static char *fwtype_str[] = {
	    "special","configuration","user diagnostic","production diag",
	    "standard comms"
	};
	static char *fwbeta_str[] = {"alpha","beta","gamma","full"};
	static char *fwconf_str[] = {
	    "fixed","dynamic","single protocol","multi protocol"
	};
	static char *stfirm_str[] = {
	    "stopped","running","configuration","running after error",
	    "stopped after fatal error"
	};

	if ( mp->hwschem == 1 )
	{
	    if ( verbose )
	    {
		pf ("hwtype:\t\t%02x\n", mp->hwtype );
		pf ("hwver:\t\t"); as_strn ( mp->hwver, hwver_str );
		pf ("hwrel:\t\t%d\n", mp->hwrel );
		pf ("hwcpu:\t\t"); as_strn ( mp->hwcpu, hwcpu_str );
	    }
	    pf ("hwndev:\t\t%d\t[# Physical ports]\n", mp->hwndev );
	    if ( verbose )
		pf ("hwppage:\t%d\n", mp->hwppage );
	    pf ("hwpmem:\t\t%dK\t[Private memory size]\n", mp->hwpmem );
	    pf ("hwsmem:\t\t%dK\t[Shared memory size]\n", mp->hwsmem );
	    pf ("hwclk:\t\t%dKHz\t[CPU clock]\n", mp->hwclk );
	    pf ("hwclk2:\t\t%dKHz\t[Baudrate clock]\n", mp->hwclk2 );
	    pf ("hwstr:\t\t\"%.16s\"\n", mp->hwstr );
	}
	else
	    pf ("Hardware scheme %d unknown\n", mp->hwschem );

        if ( mp->fwschem == 1 )
        {
            if ( verbose )
            {
		pf ("fwtype:\t\t"); as_strn ( mp->fwtype, fwtype_str );
		pf ("fwver:\t\t%d\n", mp->fwver );
		pf ("fwrel:\t\t%d\n", mp->fwrel );
		if ( isalpha ( mp->fwissue ))
		    pf ("fwissue:\t'%c'\n", mp->fwissue );
		else
		    pf ("fwissue:\t%d\n", mp->fwissue );
		pf ("fwbeta:\t\t"); as_strn ( mp->fwbeta, fwbeta_str );
		pf ("fwday:\t\t%d\n", mp->fwday );
		pf ("fwmonth:\t%d\n", mp->fwmonth );
		pf ("fwyear:\t\t%d\n", mp->fwyear );
	    }
	    else
	    {
	    	pf ("fwver:\t\t%d.%02d%c\t%2d/%2d/%d\n", mp->fwver, mp->fwrel
	    		, mp->fwissue, mp->fwday, mp->fwmonth, mp->fwyear );
	    }
	    pf ("fwndev:\t\t%d\t[# Logical channels]\n", mp->fwndev );
	    if ( verbose )
	    {
		pf ("fwlpage:\t%d\n", mp->fwlpage );
		pf ("fwconf:\t\t"); as_strn ( mp->fwconf, fwconf_str );
	    }
	    pf ("fwstr:\t\t\"%.16s\"\n", mp->fwstr );
	    if ( verbose )
		pf ("fwcpy:\t\t\"%.80s\"\n", mp->fwcpy );
        }
        else
            pf ("Firmware scheme %d unknown\n", mp->fwschem );

        if ( mp->stschem == 1 )
        {
            if ( verbose )
            {
		pf ("stfirm:\t\t"); as_strn ( mp->stfirm, stfirm_str );
		pf ("stmile:\t\t%d\n", mp->stmile );
		pf ("sterror:\t%d\n", mp->sterror );
		pf ("sterloc:\t%08lx\n", mp->sterloc );
	    }
	    pf ("stmem:\t\t%dK\t[Free memory]\n", mp->stmem );
	    if ( verbose )
		pf ("stidle:\t\t%d\n", mp->stidle );
	    pf ("stoutc:\t\t%ld\t[Output characters]\n", mp->stoutc );
	    pf ("stinc:\t\t%ld\t[Input characters]\n", mp->stinc );
	    if ( verbose )
		pf ("ststr:\t\t\"%.16s\"\n", mp->ststr );
        }
        else
            pf ("Status scheme %d unknown\n", mp->stschem );
}

/*
 *      Dump a channel control page
 */
void
dump_channel ( cp )
        struct channel_page     *cp;
{
	long	div;

	static char *phtype_str[] = {
	    "UNKNOWN","Serial DTE","Serial DCE","Parallel","Bidir Parallel"
	};
	static char *phbsel_str[] = {
	    "actual rate","table selected","IBM divisor","IBM divisor *16",
	    "CD180 divisor"
	};
	static char *cvlevel_str[] = {
	    "raw","flow control","simple translation","complex translation",
	    "special protocols"
	};
	static char *fcotflg_str[] = {
	    "no output sw flow ctl","xon/xoff output flow ctl",
	    "output flow ctl restarts on any char","line/ack output flow ctl",
	    "output rate throttle"
	};

        if ( cp->phschem == 1 )
        {
	    pf ("phtype:\t\t");
	    if ( verbose )
	    {
		if ( cp->phtype & PHTYPE_LOGICAL )
		    pf ("logical type ");
		else
		    pf ("physical type ");
	    }
	    as_strn ( cp->phtype & PHTYPE_MASK, phtype_str );

	    if ( verbose )
	    {
		pf ("phinln:\t\t");
		if ( cp->phinln & PHINLN_CTS ) pf ("CTS ");
		if ( cp->phinln & PHINLN_DSR ) pf ("DSR ");
		if ( cp->phinln & PHINLN_RI ) pf ("RI ");
		if ( cp->phinln & PHINLN_DCD ) pf ("DCD ");
		if ( cp->phinln & PHINLN_RX ) pf ("RX");
		pf ("\t[inputs monitored]\n");

		pf ("phinact:\t");
		if ( cp->phinact & PHINACT_TX_NEEDS_CTS ) pf ("cts_oflow ");
		if ( cp->phinact & PHINACT_TX_NEEDS_DSR ) pf ("dsr_oflow ");
		if ( cp->phinact & PHINACT_TX_NEEDS_DCD ) pf ("dcd_oflow ");
		if ( cp->phinact & PHINACT_RX_NEEDS_DCD )
						pf ("dcd_needed_for_rx");
		pf ("\n");

		pf ("phoutln:\tRTS ");
		switch ( cp->phoutln & PHOUTLN_RTS_MASK )
		{
		case PHOUTLN_RTS_OFF:	pf ("OFF");		break;
		case PHOUTLN_RTS_ON:	pf ("ON");		break;
		case PHOUTLN_RTS_FLOW:	pf ("input flow");	break;
		case PHOUTLN_RTS_DATA:	pf ("data to send");	break;
		}
		if ( cp->phoutln & PHOUTLN_RTS_NEEDS_DSR ) pf (", needs DSR");
		if ( cp->phoutln & PHOUTLN_RTS_NEEDS_DCD ) pf (", needs DCD");
		pf ("\n");

		pf ("phoutln:\tDTR ");
		switch ( cp->phoutln & PHOUTLN_DTR_MASK )
		{
		case PHOUTLN_DTR_OFF:	pf ("OFF");		break;
		case PHOUTLN_DTR_ON:	pf ("ON");		break;
		case PHOUTLN_DTR_FLOW:	pf ("input flow");	break;
		default:		pf ("ILLEGAL");		break;
		}
		if ( cp->phoutln & PHOUTLN_DTR_NEEDS_DSR ) pf (", needs DSR");
		if ( cp->phoutln & PHOUTLN_DTR_NEEDS_DCD ) pf (", needs DCD");
		pf ("\n");
	    }

	    pf ("phfrmt:\t\t");
	    switch ( cp->phfrmt & PHFRMT_8BIT )
	    {
	    case PHFRMT_5BIT:		pf ("5 bit");		break;
	    case PHFRMT_6BIT:		pf ("6 bit");		break;
	    case PHFRMT_7BIT:		pf ("7 bit");		break;
	    case PHFRMT_8BIT:		pf ("8 bit");		break;
	    }
	    switch ( cp->phfrmt & PHFRMT_CLR_PAR )
	    {
	    case PHFRMT_ODD_PAR:	pf (", odd parity");	break;
	    case PHFRMT_EVEN_PAR:	pf (", even parity");	break;
	    case PHFRMT_SET_PAR:	pf (", set parity");	break;
	    case PHFRMT_CLR_PAR:	pf (", clear parity");	break;
	    case PHFRMT_NO_PAR:
	    default:			pf (", no parity");	break;
	    }
	    if ( cp->phfrmt & PHFRMT_2STOP ) pf (", 2 stop");
	    if ( cp->phfrmt & PHFRMT_BREAK ) pf (", FORCED BREAK");
	    pf ("\n");

	    if ( verbose )
	    {
		pf ("phbsel:\t\t"); as_strn ( cp->phbsel, phbsel_str );
	    }
	    switch ( cp->phbsel )
	    {
	    case PHBSEL_ACTUAL:		div = 256L;	break;
	    case PHBSEL_TABLE:		div = 1L;	break;
	    case PHBSEL_IBM_DIV:	div = 115200L;	break;
	    case PHBSEL_IBM16_DIV:	div = 1843200L;	break;
	    case PHBSEL_CD180_DIV:	div = 614400L;	break;
	    }
	    pf ("phbaud:\t\t%ld\n", cp->phbaud / div );
	    if ( verbose )
		pf ("phibaud:\t%ld\n", cp->phibaud / div );
        }
        else
            pf ("Physical scheme %d unknown\n", cp->phschem );

        if ( cp->cvschem == 1 )
        {
	    if ( verbose )
	    {
		pf ("cvlevel:\t"); as_strn ( cp->cvlevel, cvlevel_str );
		pf ("fcinflg:\t");
		if ( cp->fcinflg & FCINFLG_ENABLE )
		    pf ("input sw flow ctl enabled");
		else
		    pf ("no input sw flow ctl");
		if ( cp->fcinflg & FCINFLG_CTL_IS_FC ) pf (", ctl_is_fc");
		if ( cp->fcinflg & FCINFLG_ROBUST_XON ) pf (", robust XON");
		if ( cp->fcinflg & FCINFLG_ROBUST_XOFF ) pf (", robust XOFF");
		pf ("\n");

		pf ("fcotflg:\t"); as_strn ( cp->fcotflg, fcotflg_str );

		pf ("fctxon:\t\t%d\n", cp->fctxon );
		pf ("fctxoff:\t%d\n", cp->fctxoff );
		pf ("fcrxon:\t\t%d\n", cp->fcrxon );
		pf ("fcrxoff:\t%d\n", cp->fcrxoff );

		pf ("fchighw:\t%d\n", cp->fchighw );
		pf ("fcloww:\t\t%d\n", cp->fcloww );
		pf ("fcstime:\t%d\n", cp->fcstime );
	    }

	    pf ("otflags:\t");
	    if ( cp->otflags & OTFLAGS_LFCOL0 ) pf ("LFcol0 ");
	    if ( cp->otflags & OTFLAGS_NOCRCOL0	) pf ("noCRcol0 ");
	    if ( cp->otflags & OTFLAGS_TABEXPAND ) pf ("OXTABS ");
	    if ( cp->otflags & OTFLAGS_LF_TO_CRLF ) pf ("ONLCR ");
	    if ( cp->otflags & OTFLAGS_CR_TO_LF	) pf ("CRLF");
	    pf ("\n");

	    if ( verbose )
	    	pf ("itstrip:\t%x\n", cp->itstrip );

	    pf ("itflags:\t");
	    if ( cp->itflags & ITFLAGS_NOERRSTRIP ) pf ("NoErrStrip ");
	    if ( cp->itflags & ITFLAGS_CR_TO_LF	) pf ("ICRNL ");
	    if ( cp->itflags & ITFLAGS_IGNORE_CR ) pf ("IGNCR ");
	    if ( cp->itflags & ITFLAGS_LF_TO_CR ) pf ("INLCR ");
	    if ( cp->itflags & ITFLAGS_IGNORE_ALL ) pf ("IgnoreInput");
	    pf ("\n");
        }
        else
            pf ("Data conversion scheme %d unknown\n", cp->cvschem );

        if ( cp->mdschem == 1 )
        {
	    if ( verbose )
	    {
		pf ("mdflags:\t");
		if ( cp->mdflags & MDFLAGS_IMMED ) pf ("Immediate ");
		if ( cp->mdflags & MDFLAGS_DRAIN ) pf ("WhenDrained ");
		if ( cp->mdflags & MDFLAGS_REPORT ) pf ("ReportCompletion ");
		if ( cp->mdflags & MDFLAGS_GUARD ) pf ("Changing");
		pf ("\n");
	    }
        }
        else
            pf ("Mode change scheme %d unknown\n", cp->mdschem );

        if ( cp->lnschem == 1 )
        {
	    pf ("lnstate:\t");
	    if ( cp->lnstate & LNSTATE_OVERRUN ) pf ("OE ");
	    if ( cp->lnstate & LNSTATE_PARITY ) pf ("PE ");
	    if ( cp->lnstate & LNSTATE_FRAME ) pf ("FE ");
	    if ( cp->lnstate & LNSTATE_BREAK ) pf ("BRK ");
	    if ( ! ( cp->lnstate & LNSTATE_NO_CTS )) pf ("CTS ");
	    if ( ! ( cp->lnstate & LNSTATE_NO_DSR )) pf ("DSR ");
	    if ( cp->lnstate & LNSTATE_RI ) pf ("RI ");
	    if ( ! ( cp->lnstate & LNSTATE_NO_DCD )) pf ("DCD");
	    pf ("\t[modem status]\n");

	    if ( verbose )
	    {
		pf ("lndelta:\t");
		if ( cp->lndelta & LNSTATE_OVERRUN ) pf ("OE ");
		if ( cp->lndelta & LNSTATE_PARITY ) pf ("PE ");
		if ( cp->lndelta & LNSTATE_FRAME ) pf ("FE ");
		if ( cp->lndelta & LNSTATE_BREAK ) pf ("BRK ");
		if ( cp->lndelta & LNSTATE_NO_CTS ) pf ("CTS ");
		if ( cp->lndelta & LNSTATE_NO_DSR ) pf ("DSR ");
		if ( cp->lndelta & LNSTATE_RI ) pf ("RI ");
		if ( cp->lndelta & LNSTATE_NO_DCD ) pf ("DCD");
		pf ("\n");

		pf ("lnintr:\t\t");
		if ( cp->lnintr & LNSTATE_OVERRUN ) pf ("OE ");
		if ( cp->lnintr & LNSTATE_PARITY ) pf ("PE ");
		if ( cp->lnintr & LNSTATE_FRAME ) pf ("FE ");
		if ( cp->lnintr & LNSTATE_BREAK ) pf ("BRK ");
		if ( cp->lnintr & LNSTATE_NO_CTS ) pf ("CTS ");
		if ( cp->lnintr & LNSTATE_NO_DSR ) pf ("DSR ");
		if ( cp->lnintr & LNSTATE_RI ) pf ("RI ");
		if ( cp->lnintr & LNSTATE_NO_DCD ) pf ("DCD");
		pf ("\n");
            }


	    pf ("olstate:\t");
	    if ( cp->olstate & OLSTATE_DTR ) pf ("DTR ");
	    if ( cp->olstate & OLSTATE_RTS ) pf ("RTS");
	    pf ("\t[output status]\n");

	    if ( verbose )
	    {
		pf ("oldelta:\t");
		if ( cp->oldelta & OLSTATE_DTR ) pf ("DTR ");
		if ( cp->oldelta & OLSTATE_RTS ) pf ("RTS");
		pf ("\n");

		pf ("olintr:\t\t");
		if ( cp->olintr & OLSTATE_DTR ) pf ("DTR ");
		if ( cp->olintr & OLSTATE_RTS ) pf ("RTS");
		pf ("\n");
            }


        }
        else
            pf ("Line status scheme %d unknown\n", cp->lnschem );

        if ( cp->daschem == 1 )
        {
	    if ( verbose )
	    {
		pf ("txflags:\t");
		if ( cp->txflags & TXFLAGS_FLUSH ) pf ("flush ");
		if ( cp->txflags & TXFLAGS_RESUME ) pf ("resume ");
		if ( cp->txflags & TXFLAGS_SUSPEND ) pf ("suspend ");
		pf ("\n");
		pf ("txinp:\t\t%d\n", cp->txinp );
		pf ("txoutp:\t\t%d\tchars in buffer %d\n", cp->txoutp
	    				, ( cp->txinp - cp->txoutp ) & 0xFF );
		if ( cp->txwater == TXWATER_DRAIN )
		    pf ("txwater:\tDRAIN\n");
		else
		    pf ("txwater:\t%d\n", cp->txwater );
		pf ("txnewop:\t%d\n", cp->txnewop );

		pf ("rxflags:\t");
		if ( cp->rxflags & RXFLAGS_FLUSH ) pf ("flush");
		pf ("\n");
		pf ("rxtime:\t\t%d\n", cp->rxtime );
		pf ("rxinp:\t\t%d\n", cp->rxinp );
		pf ("rxoutp:\t\t%d\tchars in buffer %d\n", cp->rxoutp
	    				, ( cp->rxinp - cp->rxoutp ) & 0xFF );
		pf ("rxwater:\t%d\n", cp->rxwater );
		pf ("rxnewop:\t%d\n", cp->rxnewop );
            }
            else
            {
		pf ("txbuf:\t\t%d\t[chars in Tx buffer]\n"
	    				, ( cp->txinp - cp->txoutp ) & 0xFF );
		pf ("rxbuf:\t\t%d\t[chars in Rx buffer]\n"
	    				, ( cp->rxinp - cp->rxoutp ) & 0xFF );
            }
        }
        else
            pf ("Data buffer scheme %d unknown\n", cp->daschem );
}


/*
 *	Get and dump a dual port memory page
 */
void
dump_page ( file, page )
	FILE	*file;
	int	page;
{
	union {			/* Three formats of memory */
	    struct master_page  mp;
	    struct channel_page cp;
	    Ubyte pp[ PGSIZE ];
	} buf;

	/* Get the data */
	fseek ( file, page * PGSIZE, SEEK_SET );
	if ( fread ( &buf, sizeof ( buf ), 1, file ) != 1 )
	{
	    fprintf ( stderr,"%s: Cannot read data at offset %d\n", progname
	    						, page * PGSIZE );
	    return;
	}

	/* Display some common info */
	pf ("Page %d Type ", buf.mp.pgnum );
	as_str ( buf.mp.pgtype, pgtype_str );
	pf (" ID = \"%.16s\"\n\n", buf.mp.pgstr );

	/* Sanity checks */
	if ( buf.mp.pgnum != page )
	{
	    pf ("WARNING: Page number %d does not match expected %d\n"
							, buf.mp.pgnum, page );
	}
	if ( buf.mp.pgnum == PGNUM_MASTER )
	{
	    if ( buf.mp.pgtype != PGTYPE_FIRM )
		pf ("WARNING: Page was expected to be of type MASTER\n");
	    else
	    {
	    	/* XXX Side effect I am not proud of.
	    	 * Store the number of ports for the default display.
		 */
	        ports_found = buf.mp.hwndev;
	    }
	}

	/* If the user wants it in hex give it to him */
	if ( inhex )
	{
	    hexdump ( &buf.pp, PGSIZE, PGSIZE * page );
	    return;
	}

	/* Display page type dependent info */
	switch ( buf.mp.pgtype )
	{
	case PGTYPE_FIRM:
	    dump_master ( &buf.mp );
	    break;

	case PGTYPE_PHYS:
	case PGTYPE_PRINT:
	case PGTYPE_LOGI:
	    dump_channel ( &buf.cp );
	    break;

	case PGTYPE_FREE:
	case PGTYPE_PROTO:
	default:
	    hexdump ( &buf.pp, PGSIZE, PGSIZE * page );
	    break;
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
	int	card = 0;
	int	unit = 0;
	int	page = -1;
	FILE	*file;
	char	*pp;
	int	i;

	/* Set program name for messages */
	if (( progname = strrchr ( argv[0],'/')) != NULL )
	    ++progname;
	else
	    progname = argv[0];

	/* Check for flags */
	while ( argc > 1 && argv[1][0] == '-')
	{
	    pp = &argv[1][0];
	    while ( *++pp != '\0')
	    {
		switch ( *pp )
		{
		case 'f':
		    if ( argc > 2 )
		    {
			fname = argv[2];
			argc--;
			argv++;
		    }
		    else
			usage ();
		    break;

		case 'h':
		    inhex = 1;
		    break;

		case 'v':
		    verbose = 1;
		    break;

		default:
		    usage ();
		}
	    }
	    argc--;
	    argv++;
	}

	/* Get and check remaining arguments */
	if ( argc > 4
		|| ( argc > 3 && sscanf ( argv[3],"%i", &page ) != 1 )
		|| ( argc > 2 && sscanf ( argv[2],"%i", &unit ) != 1 )
		|| ( argc > 1 && sscanf ( argv[1],"%i", &card ) != 1 )
		|| ( fname != NULL && argc > 1 ))
	{
	    usage ();
	}
	if ( page >= NPAGES )
	{
	    fprintf ( stderr,"%s: Page number should be in range 0..%d\n"
						, progname, NPAGES - 1 );
	    exit ( 1 );
	}
	if ( unit < 0 || unit >= MAX_NUNITS )
	{
	    fprintf ( stderr,"%s: Unit number should be in range 0..%d\n"
						, progname, MAX_NUNITS - 1 );
	    exit ( 1 );
	}
	if ( card < 0 || card >= MAX_NCARDS )
	{
	    fprintf ( stderr,"%s: Card number should be in range 0..%d\n"
						, progname, MAX_NCARDS - 1 );
	    exit ( 1 );
	}

	/* Create the device name if we havn't been given an alternate */
	if ( fname == NULL )
	{
	    sprintf ( fname_buf, fname_fmt, card, unit );
	    fname = fname_buf;
	}
	/* Open the data file */
	if (( file = fopen ( fname,"r")) == NULL )
	{
	    fprintf ( stderr,"%s: cannot access \"%s\"\n", progname, fname );

	    /* Expand on reason for failure */
	    switch ( errno )
	    {
	    case ENOENT:
		fprintf ( stderr,"Unit # out of driver range\n");
		break;

	    case ENXIO:
		fprintf ( stderr,"Card not detected at system boot\n");
		break;

	    case EIO:
		fprintf ( stderr,"Unit not powered\n");
		break;

	    case EACCES:
		fprintf ( stderr,"You need to be root to access this info\n");
		break;

	    default:
	    	fprintf ( stderr,"Errno=%d\n", errno );
	    	break;
	    }
	    exit ( 1 );
	}

	/* Do the deed */
	if ( page == -1 )
	{
	    /* -1 is a special case that dumps master and all channel pages */
	    dump_page ( file, PGNUM_MASTER );
	    for ( i = 1 ; i <= ports_found ; i++ )
	    {
	    	pf ("\n");
	    	dump_page ( file, PGNUM_MASTER + i );
	    }
	}
	else
	    dump_page ( file, page );

	/* All done */
	exit ( 0 );
}
