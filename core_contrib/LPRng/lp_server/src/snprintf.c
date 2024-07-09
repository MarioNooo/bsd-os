/**************************************************************************
 * LPRng IFHP Filter
 * Copyright 1994-1997 Patrick Powell, San Diego, CA <papowell@sdsu.edu>
 *
 * Based on the CTI printer filters.
 *  See COPYRIGHT for details.
 *
 * $Id: snprintf.c,v 1.1 1997/05/30 18:53:55 papowell Exp $
 */

#include "portable.h"

static char *const _id = "$Id: snprintf.c,v 1.1 1997/05/30 18:53:55 papowell Exp $";
static void dopr();
static char *end;

/**************************************************************
 * Original:
 * Patrick Powell Tue Apr 11 09:48:21 PDT 1995
 * A bombproof version of doprnt (dopr) included.
 * Sigh.  This sort of thing is always nasty do deal with.  Note that
 * the version here does not include floating point...
 *
 * plp_snprintf() is used instead of sprintf() as it does limit checks
 * for string length.  This covers a nasty loophole.
 *
 * The other functions are there to prevent NULL pointers from
 * causing nast effects.
 **************************************************************/
int show_ctrl;

int vplp_snprintf(str, count, fmt, args)
	char *str;
	size_t count;
	const char *fmt;
	va_list args;
{
	str[0] = 0;
	end = str+count-1;
	dopr( str, fmt, args );
	if( count>0 ){
		end[0] = 0;
	}
	return(strlen(str));
}

/* VARARGS3 */
#ifdef HAVE_STDARGS
int plp_snprintf (char *str,size_t count,const char *fmt,...)
#else
int plp_snprintf (va_alist) va_dcl
#endif
{
#ifndef HAVE_STDARGS
    char *str;
	size_t count;
    char *fmt;
#endif
    VA_LOCAL_DECL

    VA_START (fmt);
    VA_SHIFT (str, char *);
    VA_SHIFT (count, size_t );
    VA_SHIFT (fmt, char *);
    (void) vplp_snprintf ( str, count, fmt, ap);
    VA_END;
	return( strlen( str ) );
}

/*
 * dopr(): poor man's version of doprintf
 */

static void fmtstr(  char *value, int ljust, int len, int zpad );
static void fmtnum(  long value, int base, int dosign,
	int ljust, int len, int zpad );
static void dostr( char * );
static char *output;
static void dopr_outch( int c );

static void dopr( buffer, format, args )
	char *buffer;
	char *format;
	va_list args;
{
	int ch;
	long value;
	int longflag = 0;
	char *strvalue;
	int ljust;
	int len;
	int zpad;

	output = buffer;
	while( (ch = *format++) ){
		switch( ch ){
		case '%':
			ljust = len = zpad = 0;
		nextch: 
			ch = *format++;
			switch( ch ){
			case 0:
				dostr( "**end of format**" );
				return;
			case '-': ljust = 1; goto nextch;
			case '0': /* set zero padding if len not set */
				if(len==0) zpad = '0';
			case '1': case '2': case '3':
			case '4': case '5': case '6':
			case '7': case '8': case '9':
				len = len*10 + ch - '0';
				goto nextch;
			case 'l': longflag = 1; goto nextch;
			case 'u': case 'U':
				/*fmtnum(value,base,dosign,ljust,len,zpad) */
				if( longflag ){
					value = va_arg( args, long );
				} else {
					value = va_arg( args, int );
				}
				fmtnum( value, 10,0, ljust, len, zpad ); break;
			case 'o': case 'O':
				/*fmtnum(value,base,dosign,ljust,len,zpad) */
				if( longflag ){
					value = va_arg( args, long );
				} else {
					value = va_arg( args, int );
				}
				fmtnum( value, 8,0, ljust, len, zpad ); break;
			case 'd': case 'D':
				if( longflag ){
					value = va_arg( args, long );
				} else {
					value = va_arg( args, int );
				}
				fmtnum( value, 10,1, ljust, len, zpad ); break;
			case 'x':
				if( longflag ){
					value = va_arg( args, long );
				} else {
					value = va_arg( args, int );
				}
				fmtnum( value, 16,0, ljust, len, zpad ); break;
			case 'X':
				if( longflag ){
					value = va_arg( args, long );
				} else {
					value = va_arg( args, int );
				}
				fmtnum( value,-16,0, ljust, len, zpad ); break;
			case 's':
				strvalue = va_arg( args, char *);
				fmtstr( strvalue,ljust,len,zpad ); break;
			case 'c':
				ch = va_arg( args, int );
				dopr_outch( ch ); break;
			case '%': dopr_outch( ch ); continue;
			default:
				dostr(  "???????" );
			}
			longflag = 0;
			break;
		default:
			dopr_outch( ch );
			break;
		}
	}
	*output = 0;
}

static void
fmtstr(  value, ljust, len, zpad )
	char *value;
	int ljust, len, zpad;
{
	int padlen, strlen;	/* amount to pad */

	if( value == 0 ){
		value = "<NULL>";
	}
	for( strlen = 0; value[strlen]; ++ strlen ); /* strlen */
	padlen = len - strlen;
	if( padlen < 0 ) padlen = 0;
	if( ljust ) padlen = -padlen;
	while( padlen > 0 ) {
		dopr_outch( ' ' );
		--padlen;
	}
	dostr( value );
	while( padlen < 0 ) {
		dopr_outch( ' ' );
		++padlen;
	}
}

static void
fmtnum(  value, base, dosign, ljust, len, zpad )
	long value;
	int base, dosign, ljust, len, zpad;
{
	int signvalue = 0;
	unsigned long uvalue;
	char convert[20];
	int place = 0;
	int padlen = 0;	/* amount to pad */
	int caps = 0;

	/* DEBUGP(("value 0x%x, base %d, dosign %d, ljust %d, len %d, zpad %d\n",
		value, base, dosign, ljust, len, zpad )); */
	uvalue = value;
	if( dosign ){
		if( value < 0 ) {
			signvalue = '-';
			uvalue = -value; 
		}
	}
	if( base < 0 ){
		caps = 1;
		base = -base;
	}
	do{
		convert[place++] =
			(caps? "0123456789ABCDEF":"0123456789abcdef")
			 [uvalue % (unsigned)base  ];
		uvalue = (uvalue / (unsigned)base );
	}while(uvalue);
	convert[place] = 0;
	padlen = len - place;
	if( padlen < 0 ) padlen = 0;
	if( ljust ) padlen = -padlen;
	/* DEBUGP(( "str '%s', place %d, sign %c, padlen %d\n",
		convert,place,signvalue,padlen)); */
	if( zpad && padlen > 0 ){
		if( signvalue ){
			dopr_outch( signvalue );
			--padlen;
			signvalue = 0;
		}
		while( padlen > 0 ){
			dopr_outch( zpad );
			--padlen;
		}
	}
	while( padlen > 0 ) {
		dopr_outch( ' ' );
		--padlen;
	}
	if( signvalue ) dopr_outch( signvalue );
	while( place > 0 ) dopr_outch( convert[--place] );
	while( padlen < 0 ){
		dopr_outch( ' ' );
		++padlen;
	}
}

static void dostr( str )
	char *str;
{
	while(*str) dopr_outch(*str++);
}

static void dopr_outch( c )
	int c;
{
	if( show_ctrl ){
		c = c & 0xFF;
		if( iscntrl(c) && c != '\n' && c != '\t' ){
			if( end == 0 || output < end ){
				*output++ = '^';
			}
			c = (0x1F & c) | '@'; 
		}
	}
	if( end == 0 || output < end ){
		*output++ = c;
	}
}
