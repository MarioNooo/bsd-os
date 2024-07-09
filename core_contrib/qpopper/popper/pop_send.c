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
 *
 * Revisions:
 *
 * 08/13/01  [dts]
 *          - set hangup = TRUE in pop_write_now() and pop_write_flush()
 *            in the event of an I/O error.
 *
 * 04/22/01  [rcg]
 *          - pop_write_chunk() now correctly handles chunks longer
 *            than the output buffer size.
 *
 * 02/23/01  [RCG]
 *          - Debug octet checking only done if TRACE_MSG_BODY defined.
 *
 * 01/30/01  [RCG]
 *          - NOW CHECKING p->bUpdate_status_hdrs instead of NO_STATUS.
 *
 * 01/25/01  [rcg]
 *          - Implemented chunky_writes run-time option.
 *          - Added error checking when sending data to client.
 *          - Eliminated redundant calls on pop_write with no data.
 *          - Checking p->bNo_mime.
 *
 * 01/24/01  [rcg]
 *          - When DEBUG set, trace elapsed time and verify octet and
 *            line counts.
 *
 * 10/12/00  [rcg]
 *          - Fitted LGL's patches for tls support.
 *          - Added pop_write_fmt() to send formatted output to client.
 *
 * 08/23/00  [rcg]
 *          - Check for SIGPIP/SIGHUP more frequently.
 *
 * 06/10/00  [rcg]
 *          -  Add missing return value to pop_sendline.
 *          -  Don't omit body lines that look like X-UIDL or STATUS headers.
 *          -  Delete unused variables 'msg_lines' & 'uidl_sent'.
 *
 * 03/01/00  [rcg]
 *          -  Now calling msg_ptr to adjust for hidden msgs and check range.
 *
 * 02/10/00  [rcg]
 *          -  Don't trace message body unless TRACE_MSG_BODY defined.
 *
 * 01/18/00  [rcg]
 *          -  Put octet count in OK response to RETR to appease
 *             certain poorly-written clients.
 *
 * 12/22/98  [RG]
 *  3/20/98  [PY]
 * 
 */

#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#if HAVE_STRINGS_H
#  include <strings.h>
#endif /* HAVE_STRINGS_H */

#ifndef HAVE_BCOPY
#  define bcopy(src,dest,len)   (void) (memcpy(dest,src,len))
#  define bzero(dest,len)       (void) (memset(dest, (char)NULL, len))
#  define bcmp(b1,b2,n)         (void) (memcmp(b1,b2,n))
#endif /* HAVE_BCOPY */

#ifndef HAVE_INDEX
#  define index(s,c)            strchr(s,c)
#  define rindex(s,c)           strrchr(s,c)
#endif /* HAVE_INDEX */

#include "popper.h"
#include "pop_tls.h"
#include "mmangle/mime.h"
#include "mmangle/mangle.h"
#include "snprintf.c"


#ifdef    TRACE_MSG_BODY
#  define TRACING_BODY  1
#else
#  define TRACING_BODY  0
#endif /* TRACE_MSG_BODY */


static void pop_write_now   ( POP *p, char *buffer, int length );
static void pop_write_chunk ( POP *p, char *buffer, int length );


static long total_octets_sent = 0;


/* 
  Expects input one line at a time
  
  This skips some headers we shouldn't send, and inserts others
 */
static struct header_mucker {
  char   uidl_sent;
  char   in_hdr;
  void (*out_fn)(void *, char *, long);
  void **out_state;
  char  *uidl_str;
} header_mucker_global;

void *header_mucker_init(
  void (*out_fn)(void *, char *, long),
  void  *out_state,
  char  *uidl_str)
{
  header_mucker_global.out_fn    = out_fn;
  header_mucker_global.out_state = out_state;
  header_mucker_global.uidl_sent = FALSE;
  header_mucker_global.in_hdr    = TRUE;
  header_mucker_global.uidl_str  = uidl_str;
  return(&header_mucker_global);
}


/* ----------------------------------------------------------------------
   This handles some header mucking we do, namely UIDL and content-length

   Args: header_mucker_state - our state, mainly saved UIDL and output Fns
         input               - input buffer
         len                 - len of input (not used)

   Returns: nothing

   Note that input MUST be a line at a time!
   --- */
static void header_mucker_input(
    void *header_mucker_state,
    char *input, 
    long len)
{
    struct header_mucker *hm_state = (struct header_mucker *)header_mucker_state;
    char uidl_buf[MAXMSGLINELEN];


    if ( *input == '\n' )
        hm_state->in_hdr = FALSE;

    if ( hm_state->in_hdr
         && ( strncasecmp ( input, "Content-Length:", 15 ) == 0 ||
              strncasecmp ( input, "X-UIDL:",          7 ) == 0
            )
       ) { /* Skip UIDLs */
        return;
    }

    /*
     * output UIDL if we're at end of header and it hasn't been output
     */
    if ( hm_state->uidl_sent == FALSE
         && ( *input == '\n' || 
              strncasecmp ( input, "Status:", 7 ) == 0
            )
       ) {
        sprintf ( uidl_buf, "X-UIDL: %s", hm_state->uidl_str );
        ( *(hm_state->out_fn) ) ( hm_state->out_state, uidl_buf, strlen(uidl_buf) );
        hm_state->uidl_sent = TRUE;
    }

  ( *(hm_state->out_fn) ) ( hm_state->out_state, input, len );
}



/* ----------------------------------------------------------------------
   This part of the pipeline does the TOP command and a few other things
  
  Input can be binary and buffered any which way
  
  This implements the top command, and converts UNIX line ends to
  network line ends, and stuffs the "." character.

  The precise line count for TOP is managed here. Once it has been
  reached the flag is set to signal the input to the to stop.
 */
struct topper topper_global;

static void *topper_init ( int            lines_to_output,
                           void          *out_state,
                           char           tp,
                           unsigned long  bl )
{
    topper_global.lines_to_output = lines_to_output;
    topper_global.out_state       = out_state;
    topper_global.in_header       = 1;
    topper_global.last_char       = '\0';
    topper_global.is_top          = tp;
    topper_global.body_lines      = bl;
    return ( &topper_global );
}


static void topper_input ( void *topper_state, char *input, long len )
{
    char          *i,
                  *i_end = input + len,
                  *buf;
    struct topper *t_state = (struct topper *) topper_state;
    char           dot     = '.';


    if ( input == NULL )
        return;

    buf = input;
    for ( i = input;
          i < i_end;
          t_state->last_char = *i, i++ ) {
        if ( *i == '.' && t_state->last_char == '\n' ) {
            pop_write ( (POP *) t_state->out_state,  buf, i-buf+1 );
            pop_write ( (POP *) t_state->out_state, &dot, 1       );
            buf = i+1;
        } else
        if ( *i == '\n' ) {
            pop_write ( (POP *) t_state->out_state, buf,    i-buf );
            pop_write ( (POP *) t_state->out_state, "\r\n", 2     );
            buf = i+1;
            if ( !t_state->in_header && t_state->is_top )
                t_state->lines_to_output--;
        } 
    }
    if ( i != buf )
        pop_write ( (POP *) t_state->out_state, buf, i-buf );
}



/* 
 *  send:   Send the header and a specified number of lines 
 *          from a mail message to a POP client.
 */

int
pop_send(p)
POP     *   p;
{
    MsgInfoList         *   mp;         /*  Pointer to message info list */
    int                     top_lines; 
    register int            msg_num;
    char                    buffer[MAXMSGLINELEN];
    void                   *hm_state, *t_state;
    MimeParsePtr            mimeParse = NULL;
    ManglerStateType        manglerState;
    BOOL                    is_top = FALSE;  /* Set to TRUE if "top" */
    int                     do_mangle; /* Contain the param number which contain
                                          mangle command */
    long                    bufLen = 0;
    char                   *pLine  = NULL;
    time_t                  timer       = time(0);
    double                  sand        = 0;
    int                     check_lines = 0;
    int                     check_body  = 0;
    int                     check_bytes = 0;
    long                    check_total = total_octets_sent;


    /*  
     * Convert the first parameter into an integer; it's the mesage num 
     */
    msg_num = atoi ( p->pop_parm [ 1 ] );

    /*  
     * Get a pointer to the message in the message list 
     */
    mp = msg_ptr ( p, msg_num );
    if ( mp == NULL )
        return ( pop_msg ( p, POP_FAILURE, HERE, 
                           "Message %d does not exist.", msg_num ) );

    /*  
     * Is the message flagged for deletion? 
     */
    if ( mp->del_flag )
        return ( pop_msg ( p, POP_FAILURE, HERE,
                              "Message %d has been deleted.", msg_num ) );

    DEBUG_LOG2 ( p, "(message %d body contains %d lines)", 
                 msg_num, mp->body_lines );

    /* 
     * Parse arg for top command if it exists
     *
     * Also parse for "x-mangle" / "mangle" extension      
     */
    top_lines = mp->body_lines - 1; /* 'cuz body_lines include newline separating
                                       headers and message body */
    if ( strcmp ( p->pop_command, "top" ) == 0 ) {
        /*  
         * TOP command -- Convert the second parameter into an integer 
         */
        top_lines = atoi ( p->pop_parm[2] );
        if ( p->parm_count > 2 )
            pop_lower ( p->pop_parm[3] );
        do_mangle = ( p->parm_count > 2 && 
                      ( ( !strncmp ( p->pop_parm[3], "mangle",   6 ) ) ||
                        ( !strncmp ( p->pop_parm[3], "x-mangle", 8 ) ) 
                      ) 
                    )
                  ? 3 : 0;
        is_top = TRUE;
    } /* top */
    else { /* retr */
        /* 
         * NO_STATUS does not dirty the mailspool if a status is changed 
         */

        /*  
         * Assume that a RETR (retrieve) command was issued 
         */
        if ( p->bUpdate_status_hdrs && mp->retr_flag != TRUE )
            p->dirty = 1;

        /*  
         * Flag the message as retreived 
         */
        mp->retr_flag = TRUE;
        if ( p->parm_count > 1 )
            pop_lower ( p->pop_parm[2] );

        do_mangle = ( p->parm_count > 1 && 
                      ( !strncmp ( p->pop_parm[2], "mangle",   6 ) ||
                        !strncmp ( p->pop_parm[2], "x-mangle", 8 )  ) 
                    ) ? 2 : 0;
    } /* retr */

    /*
     * Handle "-no-mime" user.  If this command didn't specify mangle
     * but the user name ends in "-no-mime", we pretend mangle was used
     * with default parameters (TEXT=
     */
    if ( do_mangle == 0 && p->bNo_mime ) {
        do_mangle = 3;
        p->pop_parm[3] = "X-MANGLE(TEXT=PLAIN)";
    }

    /* 
     * Set up the output pipeline
     *
     * Last thing is topper, to do top, newline stuff and . escaping 
     */
    t_state = topper_init (
                            top_lines,
                            (void *) p,   /* Passed to pop_write_x */
                            is_top,
                            mp->body_lines - 1
                            );

    /* 
     * Set up the header mucker and possibly the Mime mangler 
     */
    if ( do_mangle ) {
        memset ( (void *)&manglerState, 0, sizeof(ManglerStateType) );
        manglerState.outFnConstructor 
            = manglerState.outFn
            = manglerState.outFnDestructor
            = topper_input;
        manglerState.outFnState        = t_state;
        manglerState.lastWasGoodHeader = 0;
        /* 
         * Initialize the MimeTypeInfo with the parameters supplied 
         */
        if ( FillMangleInfo ( p->pop_parm[do_mangle], 
                              top_lines == 0,
                              &manglerState.rqInfo ) == -1 ) {
            return ( pop_msg ( p, POP_FAILURE, HERE, 
                               "Syntax error in Mangle" ) );
        }
        mimeParse = MimeInit ( MangleMapper, &manglerState, p->drop );
        hm_state = header_mucker_init ( (void(*) (void *, char *, long)) MimeInput,
                                        (void *) mimeParse,
                                        mp->uidl_str );
        DEBUG_LOG0 ( p, "Mangling; initialized header_mucker: MimeInput, mimeParse" );
    } else {
        hm_state = header_mucker_init ( topper_input, t_state, mp->uidl_str );
        DEBUG_LOG0 ( p, "Not mangling; initialized header_mucker: topper_input, t_state" );
    }

    /*  
     * Position to the start of the message 
     */
    (void) fseek ( p->drop, mp->offset, 0 );

    /*  
     * Skip the first line (the sendmail "From" or MMDF line) 
     */
    (void) fgets ( buffer, MAXMSGLINELEN, p->drop );

    /* Some poorly-written clients (reported to include Netscape Messenger)
     * expect the octet count to be in the OK response to RETR, which is of
     * course a clear violation of RFC 1939.  But, in the spirit of bad
     * money driving out good, we accommodate such behavior.
     */
    if ( do_mangle || is_top )
        pop_msg ( p, POP_SUCCESS, HERE, "Message follows" );
    else
        pop_msg ( p, POP_SUCCESS, HERE, "%lu octets", mp->length );
    
    /*  
     * Stuff the message down the pipeline 
     */

    /* 
     * Send the headers 
     */
    pLine = fgets ( buffer, MAXMSGLINELEN, p->drop );
    while ( hangup == FALSE && pLine != NULL ) {
        bufLen = strlen ( pLine );
        if ( TRACING_BODY ) {
            if ( strncasecmp ( pLine, "Content-Length:", 15 ) != 0 )
                check_bytes += bufLen + 1; /* add one for CR */
            check_lines++;
        }
        DEBUG_LOG4 ( p, "HDR (len=%ld; hangup=%d; errno=%d) *** %.50s",
                     bufLen, hangup, errno, buffer );
        if ( strcmp(buffer,"\n") == 0 ) {
            header_mucker_input ( hm_state, buffer, bufLen );
            break;
        }
        header_mucker_input ( hm_state, buffer, bufLen );
        pLine = fgets ( buffer, MAXMSGLINELEN, p->drop );
    } /* while loop */

    /* 
     * Send the body 
     */
    ((struct topper *)t_state)->in_header = 0;
    if ( is_top ) {
        pLine = fgets ( buffer, MAXMSGLINELEN, p->drop );
        while ( hangup == FALSE                           &&
                pLine != NULL                             && 
               ((struct topper*)t_state)->lines_to_output && 
               MSG_LINES-- ) {
        bufLen = strlen ( pLine );
        if ( DEBUGGING ) {
            check_bytes += bufLen + 1; /* add one for CR */
            check_lines++;
            check_body++;
        }
        if ( TRACING_BODY ) {
            DEBUG_LOG6 ( p, "BDY (hangup=%d; errno=%d) *** %ld(%d)[%li] %.256s", 
                         hangup, errno, MSG_LINES, top_lines, bufLen, buffer );
        }
        header_mucker_input ( hm_state, buffer, bufLen );
        pLine = fgets ( buffer, MAXMSGLINELEN, p->drop );
        } /* while loop */
    } /* is_top */
    else { /* not TOP */
        pLine = fgets ( buffer, MAXMSGLINELEN, p->drop );
        while ( hangup == FALSE && pLine != NULL && MSG_LINES-- ) {
            bufLen = strlen ( pLine );
            if ( DEBUGGING ) {
                check_bytes += bufLen + 1; /* add one for CR */
                check_lines++;
                check_body++;
            }
            if ( TRACING_BODY ) {
            DEBUG_LOG3 ( p, "BDY %ld[%li] %.256s", 
                         MSG_LINES, bufLen, buffer );
            }
            header_mucker_input ( hm_state, buffer, bufLen );
            pLine = fgets ( buffer, MAXMSGLINELEN, p->drop );
        } /* while loop */
    } /* not TOP */

    if ( do_mangle ) {
        FreeMangleInfo ( &manglerState.rqInfo );
        /* 
         * We want to force the MimeFinish to be sent out, if TOP Command 
         */
        MimeFinish ( mimeParse );
    }
    if ( hangup )
        return ( pop_msg ( p, POP_FAILURE, HERE, "SIGHUP or SIGPIPE flagged" ) );

    /* 
     * Make sure message ends with a CRLF 
     */
    if ( topper_global.last_char != '\n' ){
        DEBUG_LOG0 ( p,"*** adding trailing CRLF" );
        pop_write ( p, "\r\n", 2 );
    }

    pop_write_line  ( p, "." );
    pop_write_flush ( p );
    DEBUG_LOG0   ( p, "---- sent termination sequence ----" );

    if ( TRACING_BODY && DEBUGGING ) {
        int expected_lines = mp->lines + 1; /* for X-UIDL */

        check_total = total_octets_sent - check_total;
        sand = difftime ( time(0), timer );
        DEBUG_LOG3 ( p, "#elapsed time: %f seconds for message %d (%ld total octets)",
                     sand, mp->number, check_total );

        check_body++; /* separator counts as body */
        check_lines++; /* don't forget X-UIDL */
        check_bytes += strlen("X-UIDL: ") + strlen(mp->uidl_str) + 1;

        if ( expected_lines != check_lines )
            pop_log ( p, POP_NOTICE, HERE,
                      "Incorrect line count (expected %d; found %d) msg %d in %s",
                      expected_lines, check_lines, mp->number, p->drop_name );
        if ( mp->body_lines != check_body )
            pop_log ( p, POP_NOTICE, HERE,
                      "Incorrect body lines (expected %d; found %d) msg %d in %s",
                      mp->body_lines, check_body, mp->number, p->drop_name );
        if ( mp->length != check_bytes )
            pop_log ( p, POP_NOTICE, HERE,
                      "Incorrect octet count (expected %ld; sent %d) msg %d in %s",
                      mp->length, check_bytes, mp->number, p->drop_name );
    }

    return ( POP_SUCCESS );
}





/* This is used outside this file, but not by anything in this file  */

/*
 *  sendline:   Send a line of a multi-line response to a client.
 */
int
pop_sendline ( p, buffer )
POP         *   p;
char        *   buffer;
{
    char        *   bp;

    /*  
     * Byte stuff lines that begin with the termination octet 
     */
    if ( *buffer == POP_TERMINATE ) 
        pop_write ( p, POP_TERMINATE_S, 1 );

    /*  
     * Terminate the string at a <NL> if one exists in the buffer
     */
    bp = index ( buffer, NEWLINE );
    if ( bp != NULL ) 
        *bp = '\0';

    /*  
     * Send the line to the client 
     */
    pop_write_line ( p, buffer );

    return POP_SUCCESS;
}



/*
 *  Write a null-terminated string to client with proper line ending. 
 *  Does NOT dot-stuff, so this is for commands, not messages.
 */
void
pop_write_line ( p, buffer )
POP  *p;
char *buffer;
{
    pop_write ( p, buffer, strlen(buffer) );
    pop_write ( p, "\r\n", 2 );
}




/*
 *  write a buffer to the client (may be pooled with later writes)
 */
void
pop_write ( POP *p, char *buffer, int length )
{
#ifdef    _DEBUG
    DEBUG_LOG4 ( p, "#chunky_writes=%d; tls_started=%d; %s %d bytes",
                 (int) p->chunky_writes, p->tls_started,
                 ( p->chunky_writes == ChunkyAlways
                   || ( p->chunky_writes == ChunkyTLS
                        && p->tls_started ) ) ? "buffering" : "sending",
                 length );
#endif /* _DEBUG */

    if ( p->chunky_writes == ChunkyAlways
         || ( p->chunky_writes == ChunkyTLS && p->tls_started ) )
        pop_write_chunk ( p, buffer, length );
    else
        pop_write_now   ( p, buffer, length );
}




/* 
 *  Write formatted text to the client.
 */

static char msgbuf      [ MAXLINELEN * 2 ];

void
pop_write_fmt ( POP         *p,         /* the source of all things */
                const char  *format,    /* format string to log */
                ... )                   /* parameters for format string */
{
    va_list          ap;
    size_t           iLeft     = 0;
    int              iChunk    = 0;


    /* 
     * Get the information into msgbuf 
     */
    va_start ( ap, format );

    iLeft  = sizeof ( msgbuf ) -3; /* Allow for CRLF NULL at end */
    iChunk = Qvsnprintf ( msgbuf, iLeft, format, ap );

    va_end ( ap );

    if ( iChunk == -1 ) {
        /* 
         * We blew out the format buffer.
         */
        pop_log ( p, POP_PRIORITY, HERE,
                  "Buffer size exceeded formatting msg: %s", 
                  format );
        iChunk = strlen ( msgbuf ); /* send what we have */
    } /* iChunk == -1 */

    /*
     * Now send it.
     */
    pop_write ( p, msgbuf, iChunk );
    return;
}




/*
 *  Flush the output that might be buffered to client
 */
void
pop_write_flush ( POP *p )
{
    int rslt = 0;


    if ( p->nOutBufUsed > 0 ) {
        pop_write_now ( p, p->pcOutBuf, p->nOutBufUsed );
        p->nOutBufUsed = 0;
    }

    if ( p->tls_started ) {
        rslt = pop_tls_flush ( p->tls_context );
    } else {
        rslt = fflush ( p->output );
    }

    if ( rslt == EOF ) {
        if ( p->tls_started )
            pop_log ( p, POP_NOTICE, HERE, "Error flushing data to client" );
        else {
            int e = ferror ( p->output );
            pop_log ( p, POP_NOTICE, HERE,
                      "I/O error flushing output to client %s at %s [%s]: "
                      "%s (%d)",
                      p->user, p->client, p->ipaddr, STRERROR(e), e );
        }
        hangup = TRUE;
    } /* flush failed */
#ifdef    _DEBUG
    else
        DEBUG_LOG0 ( p, "#flushed output to client" );
#endif /* _DEBUG */
}




/*
 * send data to client without extra buffering
 */
static void
pop_write_now ( POP *p, char *buffer, int length )
{
    int rslt = 0;


    if ( p->tls_started ) {
        rslt = pop_tls_write ( p->tls_context, buffer, length );
    } else {
        rslt = fwrite ( buffer, 1, length,  p->output );
    }

    if ( rslt == length ) {
        total_octets_sent += rslt;
#ifdef    _DEBUG
        DEBUG_LOG2 ( p, "#wrote %d (out of %d) octets to client",
                     rslt, length );
#endif /* _DEBUG */
        }
    else { /* write failed */
        if ( p->tls_started )
            pop_log ( p, POP_NOTICE, HERE, "Error writing to client" );
        else {
            int e = ferror ( p->output );
            pop_log ( p, POP_NOTICE, HERE,
                    "I/O error writing to client %s at %s [%s]: %s (%d)",
                    p->user, p->client, p->ipaddr, STRERROR(e), e );
        }
        hangup = TRUE;
    } /* write failed */
}




/*
 *  Add a buffer to the chunk of data to be sent to the client.
 *  Data gets sent when chunk fills up, or pop-write_flush is
 *  called.
 */
static void
pop_write_chunk ( POP *p, char *buffer, int length )
{
    int nCopied = 0;
    int nToCopy = 0;


#ifdef    _DEBUG
    DEBUG_LOG3 ( p, "#adding %d bytes to %d (out of %d max) currently buffered",
                 length, p->nOutBufUsed, OUT_BUF_SIZE );
#endif /* _DEBUG */

    while ( length > 0 ) {
       if ( p->nOutBufUsed + length > OUT_BUF_SIZE ) {
           /*  new stuff won't fit in output buffer, flush old stuff */
           pop_write_flush ( p );
       }

       nToCopy = OUT_BUF_SIZE - p->nOutBufUsed;
       if ( length < nToCopy )
            nToCopy = length;

       bcopy ( buffer + nCopied,
               &( p->pcOutBuf [ p->nOutBufUsed ] ),
               nToCopy );
       
       p->nOutBufUsed += nToCopy;
       nCopied        += nToCopy;
       length         -= nToCopy;
    } /* nCopied < length */
}




