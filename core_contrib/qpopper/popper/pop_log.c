/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */


/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 *
 * Revisions:
 *
 * 02/09/01  [rg]
 *         - Include year.
 *
 * 10/29/00  [rg]
 *         - Added pop_logv to accept va_list.
 *
 * 08/18/00  [rg]
 *         - Omit name of day of week and year from time string.  Show
 *           miliseconds (if system has clock_gettime).
 *
 *  06/30/00 [RG]
 *         - Made sure buffer is null-terminated after calling strncat.
 *
 *  04/21/00 [RG]
 *         - Now using Qvsnprintf, Qsprintf, et al.
 *         - Removed STDC_HEADERS and K&R junk.
 *
 *  01/27/00 [RG]
 *         - Replaced snprintf() with vsnprintf() for whiter whites and
 *           brighter colors.
 *
 *  01/25/00 [RG]
 *         - Replaced sprintf() with snprintf().
 *         - Log error message if buffer size exceeded.
 *         - Replace __STDC__ with STDC_HEADERS.
 *         - Added filename and line number params; this info is now
 *           added to message.
 *
 *  03/23/98 [PY]
 *         - Added the __STDC__ stuff.
 *
 */

#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

#if HAVE_STRINGS_H
#  include <strings.h>
#endif

#if HAVE_UNISTD_H
#  include <unistd.h>
#endif

#if HAVE_SYS_UNISTD_H
#  include <sys/unistd.h>
#endif

#ifndef   HAVE_GETTIMEOFDAY
#  define gettimeofday(a,b) 65535 /* non-zero  means error */
#endif /* HAVE_GETTIMEOFDAY */

#include "popper.h"
#include "snprintf.h"

/* 
 *  log:    Make a log entry
 */

static char msgbuf      [ MAXLINELEN * 2 ];


int
pop_logv ( POP         *p,         /* the source of all things */
           log_level    loglev,    /* log level */
           const char  *fn,        /* file name whence called */
           size_t       ln,        /* line number whence called */
           const char  *format,    /* format string to log */
           va_list      ap )       /* parameters for format string */
{
    char            *date_time = NULL;
    struct timeval   tval;
    size_t           iLeft     = 0;
    int              iChunk    = 0;
    int              iDateLen  = 0;
    long             lMsec     = 0;
    char            *pDate     = NULL;
    char            *pYear     = NULL;


    /* 
     * Get the information into msgbuf 
     */
    iLeft  = sizeof ( msgbuf ) -3; /* Allow for CRLF NULL at end */
    iChunk = Qvsnprintf ( msgbuf, iLeft, format, ap );

    /*
     * Append file name and line number.
     */
    if ( DEBUGGING && fn != NULL ) {
        char whence [ 512 ];
        int  len;
        
        iLeft -= ( iChunk >= 0 ? iChunk : strlen(msgbuf) );
        len    = Qsprintf ( whence, " [%s:%lu]", fn, (long unsigned) ln );
        strncat ( msgbuf, whence, iLeft );
        msgbuf [ sizeof(msgbuf) -1 ] = '\0'; /* just to make sure */
        iLeft -= len;
    }

    /* 
     * Depending on the status decide where to send the information 
     */
    if ( p->debug && p->trace ) {
        if ( gettimeofday ( &tval, NULL ) != 0 ) {
            tval.tv_sec  = time ( 0 ); /* get it the old-fashioned way */
            tval.tv_usec = 0;
        }
        date_time = ctime ( (time_t *) &tval.tv_sec );
        pDate     = date_time + 4; /* skip day of week name */
        iDateLen  = strlen ( pDate ); /* length including year */
        pYear     = pDate + ( iDateLen - 5 ); /* point to start of year */
        date_time [ iDateLen - 2 ] = '\0'; /* cut off the year */
        lMsec     = (tval.tv_usec + 500) / 1000; /* convert useconds to milliseconds */
        fprintf ( p->trace, "%s.%03ld %.4s [%ld] %s\n", 
                  pDate, lMsec, pYear, (long) getpid(), msgbuf );
        fprintf ( p->trace, "%s.%03ld %.4s \n", pDate, lMsec, pYear );
        fflush  ( p->trace );
    }
    else {
        syslog ( (int) loglev, "%s", msgbuf );
    }
    
    if ( iChunk == -1 ) {
    /* 
     * We blew out the format buffer.
     */
        if ( p->debug && p->trace ) {
            fprintf ( p->trace, "%s [%ld] Buffer size exceeded logging msg: %s\n", 
                      date_time, (long) getpid(), format );
            fprintf ( p->trace, "%s \n", date_time );
            fflush  ( p->trace );
        }
        else {
            syslog ( LOG_NOTICE, "Buffer size exceeded logging msg: %s", format );
        }
    } /* iChunk == -1 */

    return ( loglev );
}


int
pop_log ( POP         *p,         /* the source of all things */
          log_level    loglev,    /* log level */
          const char  *fn,        /* file name whence called */
          size_t       ln,        /* line number whence called */
          const char  *format,    /* format string to log */
          ... )                   /* parameters for format string */
{
    va_list          ap;
    int              ret;


    va_start ( ap, format );
    ret = pop_logv ( p, loglev, fn, ln, format, ap );
    va_end ( ap );
    return ret;
}


/*
 * Handle log-login function.  In the p->pLog_login string, an occurance
   of '%0' is replaced with the Qpopper version, '%1' with the user name,
   '%2' with the user's host name, and '%3' with the IP address.
 */
void
do_log_login ( POP *p )
{
    char *ptrs [ 5 ];
    char *str  = NULL;
    char *q    = NULL;
    int   inx  = 0;
    BOOL  bFmt = FALSE;


    str = strdup ( p->pLog_login );
    if ( str == NULL )
        return;

    for ( q = str; *q != '\0'; q++ ) {
        if ( *q == '%' )
            bFmt = TRUE;
        else if ( bFmt ) {
            switch ( *q ) {
                case '0':
                    ptrs [ inx++ ] = VERSION;
                    break;
                case '1':
                    ptrs [ inx++ ] = p->user;
                    break;
                case '2':
                    ptrs [ inx++ ] = p->client;
                    break;
                case '3':
                    ptrs [ inx++ ] = p->ipaddr;
                    break;
                default:
                    pop_log ( p, POP_NOTICE, HERE,
                              "\"%%%c\" is invalid in a log-login "
                              "string: \"%.128s\"",
                              *q, p->pLog_login );
                    free ( str );
                    return;
            } /* switch */

            bFmt = FALSE;
            *q = 's';
            if ( inx > 4 ) {
                pop_log ( p, POP_NOTICE, HERE,
                             "Log-login string contains too many "
                             "substitutions: \"%.128s\"",
                              p->pLog_login );
                free ( str );
                return;
            }
        } /* bFmt && isdigit ( *q ) */
    } /* for loop */

    pop_log ( p, POP_PRIORITY, HERE, str,
              ptrs [ 0 ], ptrs [ 1 ], ptrs [ 2 ], ptrs [ 3 ] );
    free ( str );
}

