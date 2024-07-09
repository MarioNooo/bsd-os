/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Copyright (c) 2003 QUALCOMM Incorporated. All rights reserved.
 * See License.txt file for terms and conditions for modification and
 * redistribution.
 *
 * Revisions: 
 *
 * 10/13/00  [rcg]
 *           - Fitted LGL's changes for TLS.
 *
 * 04/24/00  [rcg]
 *           - Remove STDC_HEADERS stuff.
 *
 * 04/21/00  [rcg]
 *           - Use Qsprintf, Qvsnprintf.
 *
 * 03/10/00  [rcg]
 *           - Fix message logged when -ERR response sent.
 *
 * 01/27/00  [rcg]
 *           - Replaced snprintf() with vsnprintf() for whiter whites and
 *             brighter colors.
 *           - Added file name and line number parameters, for passing
 *             through to pop_log.
 *           - Replaced sprintf with snprintf.
 *
 * 01/20/00  [rcg]
 *
 */



#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#if HAVE_STRINGS_H
#  include <strings.h>
#endif

#include "popper.h"
#include "snprintf.h"

/* 
 *  msg:    Send a formatted line to the POP client
 */

int
pop_msg ( POP         *p,         /* it all comes back to this */
          pop_result   status,    /* success or failure message? */
          const char  *fn,        /* file name whence called */
          size_t       ln,        /* line number whence called */
          const char  *format,    /* format string for message */
          ... )                   /* parameters for message */
{
    va_list             ap;
    register char   *   mp;
    char                message [ MAXLINELEN ];
    size_t              iLeft;
    int                 iChunk;


    va_start ( ap, format );

    /*  
     * Point to the message buffer and set iLeft to the amount of
     * room we have.  Reserve space for the CRLF and null at the end.
     */
    mp = message;
    iLeft = sizeof(message) - 3;

    /*  
     * Format the POP status code at the beginning of the message 
     */
    if ( status == POP_SUCCESS )
        iChunk = Qsprintf ( mp, "%s ", POP_OK );
    else
        iChunk = Qsprintf ( mp, "%s ", POP_ERR );

    /*  
     * Point past the POP status indicator in the message message 
     */
    mp    += iChunk;
    iLeft -= iChunk;

    /*  
     * Append the message (formatted, if necessary) 
     */
    if ( format != NULL ) {
        iChunk = Qvsnprintf ( mp, iLeft, format, ap );
        
        if ( iChunk == -1 ) {
            /* 
             * We blew out the format buffer.
             */
            pop_log ( p, POP_NOTICE, fn, ln, 
                      "Buffer size exceeded formatting message: %.4s %s",
                      message, format );
        }
    } /* format != NULL */
    
    va_end ( ap );
    
    /*  
     * Log the message if debugging is turned on 
     */
    if ( DEBUGGING && p->debug && status == POP_SUCCESS )
        pop_log ( p, POP_DEBUG, fn, ln, "%s", message );

    /*  
     * Log the message if a failure occurred 
     */
    if ( status != POP_SUCCESS ) {
#ifdef _DEBUG
        pop_log ( p, POP_NOTICE, fn, ln, "%s at localhost: %s", 
                  ( p->user ? p->user : "(null)" ), message );
#else
        pop_log ( p, POP_NOTICE, fn, ln, "%s at %s (%s): %s",
                  ( *p->user != '\0' ? p->user : "(null)" ),
                  p->client, p->ipaddr, message );
#endif
    }

    /*  
     * Append the <CR><LF> (we reserved space)
     */
    strcat ( message, "\r\n" );
        
    /*  
     * Send the message to the client 
     */
    pop_write  ( p, message, strlen(message) );
    pop_write_flush ( p );

    /* 
     * Return the success or failure code passed in to us 
     */
    return ( status );
}
