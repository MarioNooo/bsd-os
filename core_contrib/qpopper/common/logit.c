/* 
 * Copyright (c) 2003 QUALCOMM Incorporated. All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 */
 
 
/*
 * Revisions:
 *
 * 02/09/01  [rg]
 *         - Include year.
 *
 * 08/19/00  [rg]
 *         - Added vlogit.
 *
 * 08/18/00  [rg]
 *         - Omit name of day of week and year from time string.  Show
 *           miliseconds (if system has clock_gettime).
 *
 * 06/30/00  [rg]
 *         - Made sure buffer is null-terminated after calling strncat.
 *
 * 04/21/00  [rg]
 *         - Now using Qvsnprintf et al.
 *         - Removed STDC_HEADERS/k&R junk.
 *
 * 01/27/00  [rg]
 *         - Replaced snprintf() with vsnprintf() for whiter whites
 *           and brighter colors.
 *
 * 01/25/00  [rg]
 *         - Replaced sprintf() with snprintf().
 *         - Log error message if buffer size exceeded.
 *         - Added filename and line number params; this info is now
 *           added to message.
 *
 * 12/22/99  [rg]
 *         - Changed __STDC__ to STDC_HEADERS, because sometimes
 *           __STDC__ is only defined if the compiler is invoked
 *           with flags which disable vendor extensions and force
 *           pure ANSI compliance; the intent of its use here is
 *           to check for modern compilers.
 *
 *  9/16/98  [py]
 *         - Edited to be independent of pop.
 *
 *  3/23/98  [PY]
 *         - Added the __STDC__ stuff.
 *
 */

#include "config.h"

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

#if HAVE_UNISTD_H
#  include <unistd.h>
#endif

#if HAVE_SYS_UNISTD_H
#  include <sys/unistd.h>
#endif

#if HAVE_STRINGS_H
#  include <strings.h>
#endif

#ifndef   HAVE_GETTIMEOFDAY
#  define gettimeofday(a,b) 65535 /* non-zero  means error */
#endif /* HAVE_GETTIMEOFDAY */

#include <sys/stat.h>
#include <sys/file.h>

#include "popper.h"
#include "snprintf.h"
#include "logit.h"


#include <stdlib.h>
#include <stdarg.h>



/*---------------------------------------------------
 *  logit:    Make a log entry
 *
 * Parameters:
 *    str:    FILE * for output, or NULL (to use syslog).
 *    loglev: The priority to use with syslog.
 *    fn:     File name whence called (__FILE__).
 *    ln:     Line number whence called (__LINE__).
 *    format: Format string.
 *    ...:    Optional parameters, as specified in format string.
 *
 * Returns:
 *    stat.
 */

static char msgbuf      [ MAXLINELEN * 2 ];


int
logit ( FILE        *str,       /* STREAM to write to, or NULL for syslog */
        log_level    loglev,    /* log level */
        const char  *fn,        /* file name whence called */
        size_t       ln,        /* line number whence called */
        const char  *format,    /* format string to log */
        ... )                   /* parameters for format string */
{
    va_list          ap;
    int              rslt = 0;


    va_start ( ap, format );
    rslt = vlogit ( str, loglev, fn, ln, format, ap );
    va_end ( ap );
    return rslt;
}


int
vlogit ( FILE        *str,       /* STREAM to write to, or NULL for syslog */
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
    iLeft  = sizeof ( msgbuf ) -3; /* allow for CRLF NULL */
    iChunk = Qvsnprintf ( msgbuf, iLeft, format, ap );

    /*
     * Append file name and line number.
     */
    if ( DEBUGGING && fn != NULL ) {
        char whence [ 512 ];
        int  len;
        
        iLeft -= ( iChunk >= 0 ? iChunk : strlen(msgbuf) );
        len    = Qsprintf ( whence, " [%s:%d]", fn, ln );
        strncat ( msgbuf, whence, iLeft );
        msgbuf [ sizeof(msgbuf) -1 ] = '\0'; /* just to make sure */
        iLeft -= len;
    }

    /* 
     * If the stream is given send output to it. otherwise to syslog. 
     */
    if ( str != NULL ) {
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
        fprintf ( str, "%s.%03ld %.4s [%ld] %s\n", 
                  pDate, lMsec, pYear, (long) getpid(), msgbuf );
        fprintf ( str, "%s.%03ld %.4s \n", pDate, lMsec, pYear );
        fflush  ( str );
    }
    else {
        syslog ( loglev, "%s", msgbuf) ;
    }
    
    if ( iChunk == -1 ) {
        /* 
         * We blew out the format buffer.
         */
        if ( str ) {
            fprintf ( str, "%s [%ld] Buffer size exceeded logging msg: %s\n", 
                      date_time, (long) getpid(), format );
            fprintf ( str, "%s \n", date_time );
            fflush  ( str );
        }
        else {
            syslog ( LOG_NOTICE, "Buffer size exceeded logging msg: %s", format );
        }
    } /* iChunk == -1 */

    return ( loglev );
}
