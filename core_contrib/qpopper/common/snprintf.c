/*
 * Copyright (c) 2003 QUALCOMM Incorporated. All rights reserved.
 * See License.txt file for terms and conditions for modification and
 * redistribution.
 *
 * snprintf.c: 
 *
 * Revisions: 
 *
 *  03/11/03  [rcg]
 *            - Buffer now always returned null-terminated.
 *            - Return value now always -1 when we run out of room.
 *
 *  01/05/01  [rcg]
 *            - Added Qstrlen, as a safe strlen.
 *
 *  11/14/00  [rcg]
 *            - Recognize '%p'
 *
 *  06/20/00  [rcg]
 *            - width and limit now size_t; separate bWidth and bLimit
 *              used to know if width or limit is in use.
 *
 *  06/09/00  [rcg]
 *            - Correct blank padding for minimum-width '%s'.
 *
 *  04/21/00  [rcg]
 *            -  Changed snprintf to Qsnprintf, vsnprintf to Qvsnprintf.
 *               added Qsprintf.  Code now unconditionally included.
 *            -  Fixed copy_buf() to use strlen of source string instead
 *               of value of first character.
 *
 *  04/07/00  [rcg]
 *            -  Account for sprintf not returning length of added chars
 *               on some systems.
 *
 *  01/27/00  [rcg]
 *            -  Changed snprintf to vsnprintf, added snprint as jacket.
 *
 *  01/20/00  [rcg]
 *            -  Guarded with #ifndef HAVE_SNPRINTF
 *
 */

#include <sys/types.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "snprintf.h"
#include "string_util.h" /* for BOOL, TRUE, and FALSE */



/*
 * A (hopefully full enough) snprintf implementation built over sprintf.
 * Based on modified version of snprint from one extracted from K&R 'C'.
 * Reference : Unix specification version 2. Copyright 1997 Open Group.
 * null terminated string.
 *
 * Caveats :
 *    - Limited support for format modifiers.
 *    - No nth argument specification %n$.
 *    - The known formats "sdcfxXgGeEou%".
 *    - Assuming int as 4 bytes, double as 8 bytes. No support for
 *      long double.
 *
 * Parameters:
 *     s:       buffer to print to.
 *     n:       max number of bytes to print.
 *     format:  the format string.
 *
 * Returned : On success the number of bytes printed to buffer.
 *            On failure -1.
 */



enum { BUFSIZE = 1024 };
enum { NO_SIZE = 2147483647 };

#ifndef MIN
#  define MIN(A,B) ( ( (A) < (B) ) ? (A) : (B) )
#endif

#ifndef MAX
#  define MAX(A,B) ( ( (A) > (B) ) ? (A) : (B) )
#endif



/*
 * Copy chars and update pointer and remaining space count.
 *
 * Parameters:
 *    ppDest: pointer to char * for destination.
 *    pSrc:   pointer to source string.
 *    len:    maximum chars to copy.
 *    left:   pointer to remaining space count.
 *
 * Results:
 *    String 'pSrc' is appended to *'ppDest'; 'ppDest' is updated to
 *    point at terminating null; 'left' is decremented by
 *    number of characters copied.
 *
 *    Return value is number of characters copied.
 */
static int 
copy_buf ( char **ppDest, char *pSrc, size_t len, size_t *left )
{
    len = Qstrlen ( pSrc, len );
    strncpy ( *ppDest, pSrc, len );    
    *left   -= len;
    *ppDest += len;
    return     len;
}


enum STATES_TAG 
{
    IN_FORM,    /* Copying format string chars */
    IN_CONV,    /* In a conversion specification */
    IN_LEN      /* In a maximum length field */
};

typedef enum STATES_TAG STATES;


int 
Qvsnprintf ( char *s, size_t n, const char *format, va_list ap )
{
    char    *sval   = NULL;
    int     ival    = 0;
    double  dval    = (double) 0;
    char    *p      = (char *) format, /* Pointer into format string */
             msgBuf [ BUFSIZE ],       /* Temporary store for printing
                                        *     ints, floats etc using sprintf */
             frmToken [ 64 ],          /* Temporary store for conversion
                                        *     tokens. */
            *f      = NULL;            /* Pointer into frmToken */
    STATES   nState = IN_FORM; 
    size_t   nSize  = n-1;             /* Maximum chars we'll handle */
    size_t   width  = 0;               /* If we have a width minimum */
    BOOL     bWidth = FALSE;           /* TRUE if width specified */
    size_t   limit  = 0;               /* If we have a length limiter */
    BOOL     bLimit = FALSE;           /* TRUE if limit specified */
    size_t   s_len  = 0;               /* length of string parameter */


    if ( s == NULL || n == 0 || format == NULL ) 
        return -1;
    
    for ( ; nSize > 0 && *p != '\0'; p++ ) {
        if ( nState == IN_FORM ) {
            if ( *p != '%' ) {
                /*
                 * Still not in a conversion spec; just copy the char
                 */
                *s++ = *p;
                nSize--;
            }
            else {
                /*
                 * Start of a conversion spec
                 */
                nState =  IN_CONV;
                width  = 0;
                bWidth = FALSE;
                limit  = 0;
                bLimit = FALSE;
                f      = frmToken;
                *f++   = *p;
            }
        } /* nState == IN_FORM */
        else { /* We're still inside a conversion spec */
            switch ( *p ) {

            case 'c':                   /* C converts char args to int */
            case 'd':
            case 'i':
            case 'o':
            case 'u':
            case 'x':
            case 'X':
                /*
                 * Ensure width or max length, if specified, does not exceed
                 * available space.
                 */
                if ( bWidth || bLimit )
                    if ( MAX ( width,   limit ) >
                         MIN ( BUFSIZE, nSize )
                       ) {
                        *s = '\0';
                        return -1;
                }

                *f++ = *p;
                *f   = '\0';
                ival = va_arg ( ap, int );
                sprintf  ( msgBuf, frmToken, ival );
                copy_buf ( &s, msgBuf, nSize, &nSize );
                nState = IN_FORM;
                break;

            case 'f':                   /* C converts float args to double */
            case 'e':
            case 'E':
            case 'g':
            case 'G':
                /*
                 * Ensure width or max length, if specified, does not exceed
                 * available space.
                 */
                if ( bWidth || bLimit )
                    if ( MAX ( width,   limit ) >
                         MIN ( BUFSIZE, nSize )
                       ) {
                        *s = '\0';
                        return -1;
                }

                *f++ = *p;
                *f   = '\0';
                dval = va_arg ( ap, double );
                sprintf  ( msgBuf, frmToken, dval );
                copy_buf ( &s, msgBuf, nSize, &nSize );
                nState = IN_FORM;
                break;

            case 'p':
                /*
                 * Ensure width or max length, if specified, does not exceed
                 * available space.
                 */
                if ( bWidth || bLimit )
                    if ( MAX ( width,   limit ) >
                         MIN ( BUFSIZE, nSize )
                       ) {
                        *s = '\0';
                        return -1;
                }

                *f++ = *p;
                *f   = '\0';
                sval = va_arg ( ap, char * );
                sprintf  ( msgBuf, frmToken, sval );
                copy_buf ( &s, msgBuf, nSize, &nSize );
                nState = IN_FORM;
                break;

            case 's':
                /*
                 * Ensure width or max length, if specified, does not exceed
                 * available space.
                 */
                if ( bWidth || bLimit )
                    if ( MAX ( width,   limit ) >
                         MIN ( BUFSIZE, nSize )
                       ) {
                        *s = '\0';
                        return -1;
                }

                /* 
                 * Get the string pointer.  If NULL, or if the maximum length
                 * is zero, we're done with this conversion.
                 */
                sval = va_arg ( ap, char * );
                if ( sval == NULL || ( bLimit && limit == 0 ) ) {
                    nState = IN_FORM;
                    break;
                }
                
                s_len = Qstrlen ( sval, nSize );
                
                /*
                 * Make sure we have room if limit is unspecified.
                 */
                if ( bLimit == FALSE && s_len >= nSize ) {
                        *s = '\0';
                        return -1;
                }
                
                /* 
                 * Handle minimum width
                 */
                if ( bWidth && width > s_len ) {
                    long pad = width - s_len; /* padding needed */
                    
                    for ( ; pad > 0; pad-- )
                        copy_buf ( &s, " ", nSize, &nSize );
                }
                
                /*
                 * Handle maximum length
                 */
                if ( bLimit && limit < s_len )
                    copy_buf ( &s, sval, limit, &nSize );
                else
                    copy_buf ( &s, sval, nSize, &nSize );
                
                nState = IN_FORM;
                break;
            
            case '%':
                *s++ = *p;
                nSize--;
                nState = IN_FORM;
                break;
            
            case '*': 
                /*
                 * Variable min width or max length specification.
                 */
                if ( nState == IN_LEN && bLimit == FALSE ) {
                    bLimit = TRUE;
                    limit  = va_arg ( ap, int );
                    sprintf ( f, "%lu", (unsigned long) limit );
                    f += strlen ( f ); /* on some systems sprintf doesn't
                                          return length of added chars */
                }
                else
                if ( nState == IN_FORM && bWidth == FALSE ) {
                    bWidth = TRUE;
                    width  = va_arg ( ap, int );
                    sprintf ( f, "%lu", (unsigned long) width );
                    f += strlen ( f ); /* on some systems sprintf doesn't
                                          return length of added chars */
                }
                else {
                    *s = '\0';
                    return -1;
                }
                break;
            
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                *f++ = *p;
                *f   = '\0';

                /*
                 * Width or max length specification.  If the
                 * appropriate BOOLEAN is FALSE, this is the start;
                 * otherwise it is a subsequent digit and we just
                 * skip over it.
                 */
                if ( nState == IN_LEN && bLimit == FALSE ) {
                    bLimit = TRUE;
                    limit  = atoi ( p );
                }
                else
                if ( nState == IN_FORM && bWidth == FALSE ) {
                    bWidth = TRUE;
                    width  = atoi ( p );
                }
                break;
             
            case '.':
                /*
                 * Max length follows
                 */
                *f++ = *p;
                *f   = '\0';
                nState = IN_LEN;
                break;
            
            default:
                *f++ = *p;
                *f   = '\0';
                break;
            } /* switch */
        } /* We're still inside a format */
    } /* for loop */
    
    /*
     * If we haven't run out of room, copy last character of
     * format string (which is usually the terminating null).
     * If we have run out of room, just make sure the buffer
     * is null-terminated.
     */
    if ( nSize > 0 ) 
        *s++ = *p;
    else
        *s = '\0';

    /*
     * Return chars put in buffer (or -1 if we ran out of room)
     */
    if ( nSize == 0 && *p != '\0' )
        return -1;
    else
        return ( (n-1) - nSize );
}


int 
Qsnprintf ( char *s, size_t n, const char *format, ... )
{
    int      rslt;
    va_list  ap;                       /* Pointer into stack to extract
                                        *     parameters */
    va_start ( ap, format );
    rslt = Qvsnprintf ( s, n, format, ap );
    va_end   ( ap );
    return rslt;
}


int 
Qsprintf ( char *s, const char *format, ... )
{
    int      rslt;
    va_list  ap;                       /* Pointer into stack to extract
                                        *     parameters */
    va_start ( ap, format );
    rslt = Qvsnprintf ( s, NO_SIZE, format, ap );
    va_end   ( ap );
    return rslt;
}


/*
 * This is a safe strlen().  It stops when it hits a null or
 * when the maximum size has been reached.  This way you don't
 * run off the end of the buffer looking for a null that might
 * not be there.
 */
long Qstrlen ( const char *s, long m )
{
    long len = 0;


    if ( s == NULL )
        return 0;

    while ( m-- > 0 && *s++ != '\0' )
        len++;

    return len;
}
