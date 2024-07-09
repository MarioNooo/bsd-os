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
 *  04/28/01 [rg]
 *           - Added fix for XTND XMIT (sent in by Jacques Distler and
 *    		   others).
 *
 *  02/02/01 [rg]
 *           - Accounted for tgetline now stripping CRLF from input.
 *           - Now using p->log_facility.
 *
 *  10/13/00 [rg]
 *           - Added LGL's patch accounting for p->input_fd.
 *           - Unlinking temp file now conditional on _DEBUG instead
 *             of _DEBUG.
 *
 *  06/11/00 [rg]
 *           - Applied patch by Clifton Royston to translate network EOL
 *             (CRLF) to local EOL ('\n').
 *           - Now only initial ".\n' is recognized as end-of-msg (instead
 *             of any line which starts with a "." and ends with a ".").
 *
 *  03/10/00 [rg]
 *           - Additional change suggested by Jacques Distler for NEXT systems.
 *
 *  02/02/00 [rg]
 *           - Made changes suggested by Jacques Distler for NEXT systems.
 *
 *  01/14/00 [rg]
 *           - Used new genpath() call to get POP_TMPXMIT path.
 *
 *  12/17/99 [rg]
 *           - Made sure all functions return a value, to appease some 
 *             compilers.
 *
 *  06/24/98 [py]
 *           - Added a deprecated switch (-i) to sendmail to ignore 
 *             dots. (Need to find another method.)
 *
 *  04/04/98 [py]
 *           -Made callstack and xmit_recv.
 *
 */

#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <string.h>

#if HAVE_STRINGS_H
#  include <strings.h>
#endif /* HAVE_STRINGS_H */

#ifdef POPSCO
#  define __SCO_WAIT3__
#  include <fcntl.h>
#endif

#if HAVE_SYS_STAT_H
#  include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#  include <fcntl.h>
#endif

#if HAVE_UNISTD_H
#  include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if HAVE_SYS_UNISTD_H
#  include <sys/unistd.h>
#endif /* HAVE_SYS_UNISTD_H */

#include "popper.h"
#include "genpath.h"

/*
 *  xmit:   POP XTND function to receive a message from 
 *          a client and send it in the mail
 */

FILE     *tmp;                       /* File descriptor for temporary file */
char      temp_xmit [ MAXDROPLEN ];  /* Name of the temporary filedrop */


int
pop_xmit (p)
POP    *  p;
{
    int   tfn;                    

    if ( p->xmitting ) 
        return pop_xmit_recv ( p, p->inp_buffer );

    /*  
     * Create a temporary file into which to copy the user's message 
     */

    if ( genpath ( p, temp_xmit, sizeof(temp_xmit), GNPH_XMT ) < 0 )
        return ( pop_msg ( p, POP_FAILURE, HERE, 
                           "Unable to get temporary message file name" ) );

    DEBUG_LOG1 ( p, "Creating temporary file for sending mail message \"%s\"",
                 temp_xmit);
    tfn = mkstemp ( temp_xmit );
    if ( ( tfn == -1 ) ||
         ( ( tmp = fdopen ( tfn, "w+" ) ) == NULL)  ) {    /* failure, bail out */
        return pop_msg ( p, POP_FAILURE, HERE,
                         "Unable to create temporary message file \"%s\": "
                         "%s (%d)",
                         temp_xmit, STRERROR(errno), errno );
    }

    /*  
     * Receive the message 
     */
    DEBUG_LOG1 ( p, "Receiving mail message in \"%s\"", temp_xmit );
    p->xmitting = TRUE;
    Push ( &(p->InProcess), (FP)pop_xmit );
    return pop_msg ( p, POP_SUCCESS, HERE, "Start sending message." );
}


int
pop_xmit_recv ( p, buffer )
POP  *p;
char *buffer;
{

    /* 
     * Data written to a file should follow UNIX or local EOL format ('\n').
     * sendmail and some MTAs will ignore superfluous CRs, but not all will.
     */

    /*  
     * Look for initial period 
     */
    DEBUG_LOG2 ( p, "Receiving (%d): \"%.128s\"", strlen(buffer), buffer );
    if ( buffer[0] == '.' ) {
        /*  
         * Exit on end of message 
         */
        if ( buffer[1] == '\0' ) {
            DEBUG_LOG0 ( p, "Received end of msg" );
            fclose ( tmp );
            p->xmitting = FALSE;
            Pop ( &(p->InProcess) );
            pop_xmit_exec ( p );
        }

        /* 
         * sendmail will not remove escaped dot 
         */
        else if ( buffer[1] == '.' ) {
            fputs ( &buffer[1], tmp );
            fputc ( '\n', tmp );
        }
        else {
            fputs ( buffer, tmp );
            fputc ( '\n', tmp );
            DEBUG_LOG4 ( p, "Client sent unescaped leading dot: %s at %s (%s): %.60s",
            			 p->user, p->client, p->ipaddr, buffer );
        }
    } /* buffer[0] == '.' */

    else {
        fputs ( buffer, tmp );
        fputc ( '\n', tmp );
    }
    return 0;
}


int
pop_xmit_exec ( p )
POP *p;
{

#ifdef NEXT
    int                    *status;
#else
    int                     status;
#endif /* NEXT */

    PID_T                   id, pid;
    
    DEBUG_LOG1 ( p, "Forking for \"%s\"",  p->pMail_command );
    DEBUG_LOG1 ( p, "Reading from \"%s\"", temp_xmit );
    pop_log ( p, POP_DEBUG, HERE, "Pop transmit from \"%s\" on \"%s\"", 
              p->user, p->client );
    /*  
     * Send the message 
     */
    switch ( pid = fork() ) {
        case 0:
            /*  
             * Open the log file 
             */
            (void) closelog();
#ifdef SYSLOG42
            (void) openlog ( p->myname, 0 );
#else
            (void) openlog ( p->myname, LOG_PID, p->log_facility );
#endif
            pop_log ( p, POP_DEBUG, HERE, "Pop transmit from \"%s\" on \"%s\"",
                      p->user, p->client );
            (void) close  ( p->input_fd  );
            (void) fclose ( p->output    );
            (void) close  ( 0            );   /* Make sure open return stdin.
                                               * to be used by MAIL_COMMAND
                                               */
            if ( open ( temp_xmit, O_RDONLY, 0 ) < 0 ) {
                DEBUG_LOG3 ( p, "Unable to open \"%s\": %s (%d)",
                             temp_xmit, STRERROR(errno), errno );
                (void) _exit ( 1 );
            }
            (void) execl ( p->pMail_command, "send-mail", "-t", "-i", "-oem", NULLCP );
            (void) _exit ( 1 );

        case -1:
#ifndef   _DEBUG
                (void) unlink ( temp_xmit );
#endif /* _DEBUG */
            return pop_msg ( p, POP_FAILURE, HERE, "Unable to execute \"%s\": %s (%d)",
                             p->pMail_command, STRERROR(errno), errno );
        default:

            DEBUG_LOG1 ( p, "waiting on forked process (%u)",
                         (unsigned) pid );
#ifdef NEXT
            while ( ( id = wait(status) ) >= 0 && id != pid );
#else
            id = waitpid ( pid, &status, 0 );
#endif

#ifndef   _DEBUG
                (void) unlink ( temp_xmit );
#endif /* _DEBUG */

#ifdef NEXT
            if ( !WIFEXITED (*status) )
#else
            if ( (!WIFEXITED (status) ) || ( WEXITSTATUS (status) != 0) )
#endif
                return pop_msg ( p, POP_FAILURE, HERE, 
                                 "Unable to send message" );

            return pop_msg ( p, POP_SUCCESS, HERE,
                             "Message sent successfully" );
    }
}


int
pop_xmit_clean ( POP *p )
{
    (void) p;
    fclose ( tmp );

#ifndef _DEBUG
    unlink ( temp_xmit );
#endif /* _DEBUG */

    return 0;
}


