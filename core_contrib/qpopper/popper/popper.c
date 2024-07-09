/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 */
 
/*
 * QPOPPER.
 *
 * Revisions:
 *
 *  04/04/02 [rg]
 *           - getline() now clears out storage buffer when giving up
 *             after discarding bytes.  Fixes looping DOS attack seen
 *             on some systems.
 *
 *  06/05/01 [rg]
 *           - Added pop_noop() function to avoid "NOOP has null
 *             function" log message.
 *
 *  06/01/01 [rg]
 *           - Added patch by Carles Xavier Munyoz to fix erroneous
 *             scanning for \n in getline().
 *
 *  01/15/01 [rg]
 *           - Handle run-time options bShy, bDo_timing, bUpdate_on_abort.
 *
 *  12/21/00 [rg]
 *           - Handle tls_support instead of stls and alt_port.
 *
 *  10/20/00 [rg]
 *           - More tracing, error checking, etc. in new nw I/O code.
 *
 *  10/14/00 [rg]
 *           - Fitted LGL's TLS/SSL patches.
 *
 *  06/10/00 [rg]
 *           - Add return value to pop_exit and ring.
 *           - Added irix to HPUX voidstar define.
 *
 *  06/05/00 [rg]
 *           - Be less chatty in banner when SHY defined.
 *
 *  05/02/00 [rg]
 *           - Use both ferror() and errno for I/O errors on client
 *             input stream.
 *
 *  04/28/00 [rg]
 *           - Use ferror() on p->input instead of errno for I/O errors
 *             on client input stream.
 *
 *  03/07/00 [rg]
 *           - Added trace call when ready for user input.
 *           - Changed AUTH to SPEC_POP_AUTH.
 *
 *  12/03/99 [rg]
 *           - Added POP* parameter to tgets and myfgets.
 *           - Added logging for I/O errors and discarded input to myfgets.
 *           - Added errno to POP EOF -ERR message.
 *
 *  04/04/98 [py]
 *           Made pop_exit(), callstack.
 *
 */

#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <setjmp.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if HAVE_STRINGS_H
#  include <strings.h>
#endif

#if HAVE_UNISTD_H
#  include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if HAVE_SYS_UNISTD_H
#  include <sys/unistd.h>
#endif /* HAVE_SYS_UNISTD_H */

#ifdef SPEC_POP_AUTH
#  ifdef HAVE_SYS_SECURITY_H 
#    include <sys/security.h>
#  endif
#  ifdef HAVE_PROTO_H
#    include <prot.h>
#  endif
#endif /* SPEC_POP_AUTH */

#include "popper.h"
#include "misc.h"

#ifndef HAVE_STRERROR
  char * strerror();
#endif

extern  state_table *   pop_get_command();
volatile BOOL poptimeout = FALSE;
volatile BOOL hangup     = FALSE;

int catchSIGHUP ( SIGPARAM );

int     pop_timeout = POP_TIMEOUT;

#ifdef _DEBUG
  POP *global_debug_p = NULL;
#endif


/* 
 *  popper: Handle a Post Office Protocol version 3 session
 */
int
qpopper ( argc, argv )
int       argc;
char        **  argv;
{
    POP                 p;
    state_table     *   s;
    char                message [ MAXLINELEN ];
    pop_result          rslt = POP_FAILURE;
    char            *   tgetline();
    char            *   getline();
    
    /*
     * seed random with the current time to nearest second 
     */
    srandom ( (unsigned int) time ( (TIME_T *) 0 ) );

#if defined(POPSCO) || defined(SPEC_POP_AUTH)
#  ifdef HAVE_SET_AUTH_PARAMETERS
    (void) set_auth_parameters ( argc, argv );
#  endif /* HAVE_SET_AUTH_PARAMETERS */
#endif /* POPSCO or SPEC_POP_AUTH */

#ifdef AUX
    (void) set42sig();
#endif /* AUX */

/* 
 * Set umask for better security 
 */
#ifdef BINMAIL_IS_SETGID
    umask ( 0007 );        /* Trust the mail delivery group */
#else
    umask ( 0077 );        /* Trust no-one */
#endif

    if ( signal (SIGHUP,  VOIDSTAR catchSIGHUP ) == SIG_ERR ||
         signal (SIGPIPE, VOIDSTAR catchSIGHUP ) == SIG_ERR  ) {
        EXIT ( 1 ); 
    }


#ifdef _DEBUG
    global_debug_p = &p;
#endif

/*  
 * Start things rolling 
 */
    if ( pop_init ( &p, argc, argv ) != POP_SUCCESS )
        EXIT ( 1 );

    DEBUG_LOG1 ( &p, "before TLS; tls_support==%d",
                 p.tls_support );


    /*
     * Initialize the TLS/SSL context if any possibility of it being used.
     */
    if ( p.tls_support != QPOP_TLS_NONE ) {

        if ( p.tls_identity_file == NULL ) {
            p.tls_identity_file = strdup ( "/etc/mail/certs/identity.crt" );
            if ( p.tls_identity_file == NULL ) {
                pop_log ( &p, POP_PRIORITY, HERE, "Unable to allocate memory" );
                EXIT ( 1 );
            }
        }

        p.tls_context = pop_tls_init ( &p );
        if ( p.tls_context == NULL ) {
            pop_log ( &p, POP_PRIORITY, HERE, "Failed initializing TLS/SSL" );
            EXIT ( 1 );
        }
        DEBUG_LOG0 ( &p, "TLS Init" );
    } /* p.tls_support != QPOP_TLS_NONE */
    else
        DEBUG_LOG0 ( &p, "Skipped TLS Init" );

    /*
     * For alt port TLS the handshake is the first thing
     */
    if ( p.tls_support == QPOP_TLS_ALT_PORT ) {
        int nResult = pop_tls_handshake ( p.tls_context );
        if ( nResult != 0 ) {
            pop_log ( &p, POP_PRIORITY, HERE, 
                      "TLS/SSL Handshake failed: %d",
                      nResult);   
            EXIT ( 1 );
        } else {
            DEBUG_LOG1 ( &p, "(v%s) TLS OK", VERSION );
            p.tls_started = TRUE;
        }
        DEBUG_LOG0 ( &p, "TLS Done" );   
    }

/*  
 * Tell the user that we are listenting 
 */
    { /* local env */
    char                myname [ 128 ];

#ifdef _DEBUG
    strcpy ( p.myhost, "DEBUG" );
#endif /* _DEBUG */

#ifdef APOP
    sprintf ( p.md5str, "<%lu.%ld@%s>", 
              (long) getpid(), (long) time ( (TIME_T *) 0 ), p.myhost );
#else
    p.md5str[0] = '\0';
#endif /* APOP */

#ifdef _DEBUG
    strcpy ( myname, "local host" );
#else
    strcpy ( myname, p.myhost );
#endif /* _DEBUG */

DEBUG_LOG1 ( &p,"(v%s) Intro", VERSION );

    if ( p.bShy ) {
        /* 
         * say as little as possible.  According to RFC 1939 all we need is
         * "+OK" and the optional APOP string, but we'll be a little chatty
         * and add "ready".
         */
        pop_msg ( &p, POP_SUCCESS, HERE,
                  "ready  %s",
                  p.md5str );
    }
    else {
        pop_msg ( &p, POP_SUCCESS, HERE, 
                  "%s%.*s%s (version %s) at %s starting.  %s",
                  QPOP_NAME,
                  (strlen(BANNERSFX)>0 ? 1 : 0), " ",
                  BANNERSFX,
                  VERSION, 
                  myname,
                  p.md5str );
    }

    } /* local env */

    /*
     * Initalize input buffer
     */
    p.pcInEnd = p.pcInStart = p.pcInBuf;


/*
 *      State loop.  The POP server is always in a particular state in 
 *      which a specific suite of commands can be executed.  The following 
 *      loop reads a line from the client, gets the command, and processes 
 *      it in the current context (if allowed) or rejects it.  This continues 
 *      until the client quits or an error occurs. 
 */

    for ( p.CurrentState  = auth1;
          p.CurrentState != halt && p.CurrentState != error;
        ) { /* main state loop */
#ifdef    HAVE_SETPROCTITLE
        setproctitle ( "%s@%s [%s]: cmd read", p.user, p.client, p.ipaddr );
#endif /* HAVE_SETPROCTITLE */
        DEBUG_LOG3 ( &p, "Qpopper ready for input from %s at %s [%s]",
                     ( *p.user == '\0' ? "(null)" : p.user ),
                     p.client, p.ipaddr );

        if ( hangup ) {
            pop_exit ( &p, HANGUP );
        } 
        else if ( tgetline ( message, MAXLINELEN, &p, pop_timeout ) == NULL ) {
            pop_exit ( &p, (poptimeout) ? TIMEOUT :  ABORT );
        } 
        else if ( StackSize ( &(p.InProcess) ) ) { 
            int i;
            FP f;
            p.inp_buffer = message;
            for ( f = GetTop  ( &(p.InProcess), &i ); 
                  f; 
                  f = GetNext ( &(p.InProcess), &i ) 
                ) {
                  (*f) ( &p );
            }
        } 
        else { /* start new command */
            s = pop_get_command ( &p, message );
            if ( s != NULL )  {
                if ( s->function != NULL ) {

#ifdef    HAVE_SETPROCTITLE
                    int i;
                    char command [ 10 ];

                    for ( i = 0; s->command [ i ]; i++ )
                        command [ i ] = toupper ( s->command [ i ] );
                    command [ i ] = 0;
                    setproctitle ( "%s@%s [%s]: %s",
                                   p.user, p.client, p.ipaddr, command );
#endif /* SETPROCTITLE */

                    /* 
                     * honor a halt or error state from internal
                     * command processing
                     */
                    if ( p.CurrentState != halt && p.CurrentState != error ) {
                        rslt = ( *s->function ) ( &p );
                        p.CurrentState = s->result [ rslt ];
                        DEBUG_LOG3 ( &p, "%s returned %d; CurrentState now %s",
                                     s->command, rslt,
                                     get_state_name ( p.CurrentState ) );
                    } else {
                        DEBUG_LOG1 ( &p, "p.CurrentState remains %s",
                                     get_state_name ( p.CurrentState ) );
                    }
                } /* s->function != NULL */
                else {
                   /* 
                    * Treat a missing function as an error result
                    */
                    p.CurrentState = s->result [ POP_FAILURE ];
                    pop_msg ( &p, POP_SUCCESS, HERE, " " );
                    pop_log ( &p, POP_PRIORITY, HERE, "%s has null function",
                              s->command );
                }
            } /* s != NULL */
        } /* start new command */
    } /* main state loop */

    /*  
     * Say goodbye to the client 
     */
    pop_msg ( &p, POP_SUCCESS, HERE, "Pop server at %s signing off.",
#ifdef _DEBUG
              "local host"
#else
              p.myhost
#endif
            );

    /*
     * If we initialized any TLS stuff, clean it up
     */
    if ( p.tls_context != NULL )
        pop_tls_shutdown ( &p );

    /*  
     * Log the end of activity 
     */
    DEBUG_LOG4 ( &p, "(v%s) Ending request from \"%s\" at (%s) %s",
                 VERSION, p.user, p.client, p.ipaddr );

    /*
     * If requested, write timing record
     */
    if ( p.bDo_timing )
        pop_log ( &p, POP_INFO, HERE,
                  "(v%s) Timing for %s@%s (%s) auth=%lu init=%lu clean=%lu",
                  VERSION, p.user, p.client, 
                  ( p.CurrentState == error ? "error" : "normal" ),
                  p.login_time, p.init_time, p.clean_time );

    /*  
     * Stop logging 
     */
    closelog();

    return ( 0 );
}


jmp_buf env;


/*
 *  Our own getline.  One reason we don't use fgets because there was
 *  a comment here that fgets was broken on AIX.  Another reason is
 *  that we need to read out of the TLS stream at times.
 *
 *  This handles lines ending with \r\n or \n.  If lines are longer
 *  than the buffer passed in, whatever doesn't fit in the buffer passed in
 *  the input is discarded.
 */

char
*getline ( char *str, int size, POP *pPOP )
{
    char *p       = NULL;
    char *pEnd    = NULL;
    int   nRead   = 0;
    int   nRoom   = 0;
    int   nBufSz  = sizeof ( pPOP->pcInBuf );


    _DEBUG_LOG2 ( pPOP, "getline(%p,%d)", str, size );

    /*
     * See if there's a line in our input buffer
     */
    while ( TRUE ) {
        _DEBUG_LOG3 ( pPOP, "...reading; start=%d used=%d end=%d",
                      pPOP->nInBufStart, pPOP->nInBufUsed,
                      pPOP->pcInEnd - pPOP->pcInBuf );
        /*
         * Look for line in our buffer
         */
        for ( p = pPOP->pcInStart;
              p < pPOP->pcInEnd;
              p++ )
            if ( *p == '\n' )
                break;
        if ( p != pPOP->pcInEnd && *p == '\n' ) {
            /*
             * Got a line
             */
            if ( *(p-1) == '\r' )
                *(p-1) = '\0';
            else
                *p = '\0';
            p++;
            _DEBUG_LOG2 ( pPOP, "...found line (%d): '%.200s'",
                          p - pPOP->pcInStart, pPOP->pcInStart );
            bcopy ( pPOP->pcInStart,
                    str,
                    MIN ( size, p - pPOP->pcInStart ) );
            *(str + size - 1) = '\0';

            pPOP->pcInStart = p;
            if ( pPOP->pcInStart == pPOP->pcInEnd ) {
                _DEBUG_LOG1 ( pPOP, "...start==end (%d); resetting start & end",
                              pPOP->pcInStart - pPOP->pcInBuf );
                pPOP->pcInStart = pPOP->pcInEnd = pPOP->pcInBuf;
            }
            _DEBUG_LOG3 ( pPOP, "getline() returning %d: '%.*s'",
                          strlen(str), MIN(25, (int) strlen(str)), str );
            return ( str );
        } /* got a line */

        nRoom = pPOP->pcInBuf + nBufSz - pPOP->pcInEnd;
        _DEBUG_LOG2 ( pPOP, "...nRoom=%d; nBufSz=%d", nRoom, nBufSz );

        if ( nRoom < nBufSz / 2 && pPOP->pcInStart != pPOP->pcInBuf ) {
            int nToCopy = pPOP->pcInEnd - pPOP->pcInStart;
            _DEBUG_LOG2 ( pPOP, "...shifting %d bytes from %d to start",
                          nToCopy, pPOP->pcInStart - pPOP->pcInBuf );
            bcopy ( pPOP->pcInStart, pPOP->pcInBuf, nToCopy );
            pPOP->pcInEnd = pPOP->pcInBuf + nToCopy;
            pPOP->pcInStart = pPOP->pcInBuf;
            nRoom = nBufSz - nToCopy;
            _DEBUG_LOG2 ( pPOP, "...set end=%d nRoom=%d",
                          pPOP->pcInEnd - pPOP->pcInBuf, nRoom );
        } /* nRoom < nBufSz / 2 */
        else
        if ( nRoom == 0 ) {
            char  junk [ 1024 ] = "";
            int   len  = 0;
            int   disc = 0;
            int   keep = 0;
            char *q   = NULL;

            _DEBUG_LOG0 ( pPOP, "...buffer full; discarding bytes" );
            while ( TRUE ) {
                if ( pPOP->tls_started )
                    len = pop_tls_read ( pPOP->tls_context, junk, sizeof(junk) );
                else
                    len = read ( pPOP->input_fd, junk, sizeof(junk) );
                if ( len <= 0 ) {
                    _DEBUG_LOG3 ( pPOP, "...junk read returned %d; ERRNO: %s (%d)",
                                  len, STRERROR(errno), errno );
                    if ( errno == EAGAIN || errno == EINTR )
                        continue; /* There may still be some data */
                    else {
                        /*
                         * Clear out junk in buffer
                         */
                        pPOP->pcInEnd   = pPOP->pcInBuf;
                        pPOP->pcInStart = pPOP->pcInBuf;
                        nRoom = nBufSz;
                        _DEBUG_LOG2 ( pPOP, "...set end=%d nRoom=%d",
                                      pPOP->pcInEnd - pPOP->pcInBuf, nRoom );
                        *str = '\0'; /* assume EOF */
                        break;
                    }
                }
                q = strchr ( junk, '\n' );
                if ( q == NULL ) {
                    disc += len;
                    _DEBUG_LOG1 ( pPOP, "...read & discarded %d bytes", len );
                }
                else {
                    disc += ( q - junk );
                    keep  = len - ( q - junk );
                    _DEBUG_LOG2 ( pPOP, "...read & discarded %d bytes; retained %d",
                                  q - junk, keep );

                    /*
                     * Setup to return the junk before we started discarding
                     */
                    bcopy ( pPOP->pcInStart,
                            str,
                            MIN ( size, p - pPOP->pcInStart ) );
                    *(str + size - 1) = '\0';

                    /*
                     * Copy what we have after the '\n' to the start
                     * of our buffer
                     */
                    bcopy ( q, pPOP->pcInBuf, keep );
                    pPOP->pcInEnd = pPOP->pcInBuf + keep;
                    pPOP->pcInStart = pPOP->pcInBuf;
                    nRoom = nBufSz - keep;
                    _DEBUG_LOG2 ( pPOP, "...set end=%d nRoom=%d",
                                  pPOP->pcInEnd - pPOP->pcInBuf, nRoom );

                    pop_log ( pPOP, POP_NOTICE, HERE,
                              "Excessive input from %s %s (%s); discarded %d bytes",
                              pPOP->user, pPOP->client, pPOP->ipaddr, disc );
                    break;
                } /* found a '\n' */
            } /* loop and discard until we see a '\n' */

            _DEBUG_LOG2 ( pPOP, "getline() returning %d ('%c')",
                          strlen(str), *str );
            return ( str );
        } /* nRoom == 0 */

        if ( pPOP->tls_started )
            nRead = pop_tls_read ( pPOP->tls_context, pPOP->pcInEnd, nRoom );
        else
            nRead = read ( pPOP->input_fd,  pPOP->pcInEnd, nRoom );

        _DEBUG_LOG5 ( pPOP, "...read returned %d (nRoom was %d);%s using "
                            "tls; errno: %s (%d)",
                      nRead, nRoom,
                      ( pPOP->tls_started ? "" : " not" ),
                      ( nRead == -1 ? STRERROR ( errno ) : "" ),
                      ( nRead == -1 ? errno : 0 ) );

        if ( nRead > 0 )
            pPOP->pcInEnd += nRead;
        else
            break;
    } /* main loop */

    _DEBUG_LOG0 ( pPOP, "getline() returning NULL" );
    return ( NULL );
}



/*
 * Get a line of input with a timeout.  This part does the timeout
 */
char *
tgetline ( char *str, int size, POP *p, int timeout )
{
    int ring();


    (void) signal ( SIGALRM, VOIDSTAR ring );
    alarm ( timeout );
    if ( setjmp ( env ) ) {
        str = NULL;
        pop_log ( p, POP_NOTICE, HERE, "(v%s) Timeout (%d secs) during "
                                       "nw read from %s at %s (%s)",
                  VERSION, timeout, p->user, p->client, p->ipaddr );
    }
    else
        str = getline ( str, size, p );
    alarm  ( 0 );
    signal ( SIGALRM, SIG_DFL );
    return ( str );
}


int 
ring ( SIGPARAM )
{
    poptimeout = TRUE;
    longjmp ( env, 1 );
    return POP_FAILURE;
}


#ifdef STRNCASECMP
/*
 *  Perform a case-insensitive string comparision
 */
strncasecmp(str1,str2,len)
register char   *   str1;
register char   *   str2;
register int        len;
{
    register int    i;
    char            a,
                    b;

    for (i=len-1;i>=0;i--){
        a = str1[i];
        b = str2[i];
        if (isupper(a)) a = tolower(str1[i]);
        if (isupper(b)) b = tolower(str2[i]);
        if (a > b) return (1);
        if (a < b) return(-1);
    }
    return(0);
}
#endif


int 
catchSIGHUP ( SIGPARAM )
{
    hangup = TRUE ;

    /* This should not be a problem on BSD systems */
    signal ( SIGHUP,   VOIDSTAR catchSIGHUP );
    signal ( SIGPIPE,  VOIDSTAR catchSIGHUP );
    return 0;
}


#ifndef HAVE_STRERROR
char *
strerror(e)
        int e;
{
    if(e < sys_nerr)
        return(sys_errlist[e]);
    else
        return("unknown error");
}
#endif

#ifdef POPSCO
/*
 * Ftruncate() for non-BSD systems.
 *
 * This module gives the basic functionality for ftruncate() which
 * truncates the given file descriptor to the given length.
 * ftruncate() is a Berkeley system call, but does not exist in any
 * form on many other versions of UNIX such as SysV. Note that Xenix
 * has chsize() which changes the size of a given file descriptor,
 * so that is used if M_XENIX is defined.
 *
 * Since there is not a known way to support this under generic SysV,
 * there is no code generated for those systems.
 *
 * SPECIAL NOTE: On Xenix, using this call in the BSD library
 * will REQUIRE the use of -lx for the extended library since chsize()
 * is not in the standard C library.
 *
 * By Marc Frajola, 3/27/87
 */

#include <fcntl.h>

ftruncate(fd,length)
    int fd;                     /* File descriptor to truncate */
    off_t length;               /* Length to truncate file to */
{
    int status;                 /* Status returned from truncate proc */

    status = chsize(fd,length);
/*
    status = -1;
    NON-XENIX SYSTEMS CURRENTLY NOT SUPPORTED
*/

    return(status);
}
#endif

#ifdef NEED_FTRUNCATE
/* ftruncate emulations that work on some System V's.
   This file is in the public domain.  */

#include "config.h"

#include <sys/types.h>
#include <fcntl.h>

#ifdef F_CHSIZE

int
ftruncate (fd, length)
     int fd;
     off_t length;
{
  return fcntl (fd, F_CHSIZE, length);
}

#else /* not F_CHSIZE */
#ifdef F_FREESP

/* By William Kucharski <kucharsk@netcom.com>.  */

#include <sys/stat.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

int
ftruncate (fd, length)
     int fd;
     off_t length;
{
  struct flock fl;
  struct stat filebuf;

  if (fstat (fd, &filebuf) < 0)
    return -1;

  if (filebuf.st_size < length)
    {
      /* Extend file length. */
      if (lseek (fd, (length - 1), SEEK_SET) < 0)
        return -1;

      /* Write a "0" byte. */
      if (write (fd, "", 1) != 1)
        return -1;
    }
  else
    {

      /* Truncate length. */

      fl.l_whence = 0;
      fl.l_len = 0;
      fl.l_start = length;
      fl.l_type = F_WRLCK;      /* write lock on file space */

      /* This relies on the *undocumented* F_FREESP argument to fcntl,
         which truncates the file so that it ends at the position
         indicated by fl.l_start.  Will minor miracles never cease?  */

      if (fcntl (fd, F_FREESP, &fl) < 0)
        return -1;
    }

  return 0;
}

#else /* not F_CHSIZE nor F_FREESP */
#ifdef HAVE_CHSIZE

int
ftruncate (fd, length)
     int fd;
     off_t length;
{
  return chsize (fd, length);
}

#else /* not F_CHSIZE nor F_FREESP nor HAVE_CHSIZE */

#include <errno.h>
#ifndef errno
extern int errno;
#endif

int
ftruncate (fd, length)
     int fd;
     off_t length;
{
  errno = EIO;
  return -1;
}

#endif /* not HAVE_CHSIZE */
#endif /* not F_FREESP */
#endif /* not F_CHSIZE */
#endif /* NEED_FTRUNCATE */


int
pop_exit ( p, e )
    POP         *p;
    EXIT_REASON e;
{
    switch ( e ) {
    case HANGUP :
        pop_msg ( p, POP_FAILURE, HERE, "POP hangup from %s", p->myhost );
        break;
    case TIMEOUT :
        DEBUG_LOG1 ( p, "Timed out %d", pop_timeout );
        pop_msg ( p, POP_FAILURE, HERE, "POP timeout from %s", p->myhost ); 
        break;
    case ABORT:
        pop_msg ( p, POP_FAILURE, HERE, "POP EOF or I/O Error" );
        break;
    } /* switch(e) */

    if ( p->xmitting ) 
        pop_xmit_clean ( p );

    if ( p->bUpdate_on_abort ) {
        if ( (p->CurrentState != auth1) && (p->CurrentState != auth2)
                                        && !pop_updt(p) )
            pop_msg ( p, POP_FAILURE, HERE, "POP mailbox update for %s failed!",
                      p->user );
    }
    else {
        if ( (p->CurrentState != auth1) && (p->CurrentState != auth2)
                                        && !pop_restore(p) )
            pop_msg ( p, POP_FAILURE, HERE, "POP mailbox restoration for %s failed!",
                      p->user );
    }

    if ( p->pw.pw_dir != NULL ) {
        free ( p->pw.pw_dir );
    }

    p->CurrentState = error;
    return POP_FAILURE;
}

int
pop_noop ( POP *p )
{
    return pop_msg ( p, POP_SUCCESS, HERE, "no big woop" );
}


char *
get_state_name ( state curState )
{
    static char buf [ 31 ] = "";

    switch ( curState )
    {
        case auth1:
            strcpy ( buf, "auth1" );
            break;
        case auth2:
            strcpy ( buf, "auth2" );
            break;
        case trans:
            strcpy ( buf, "trans" );
            break;
        case update:
            strcpy ( buf, "update" );
            break;
        case halt:
            strcpy ( buf, "halt" );
            break;
        case error:
            strcpy ( buf, "error" );
            break;
        default:
            sprintf ( buf, "** %d **", curState );
    }
    
    return buf;
}

