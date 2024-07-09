
/*
 * Copyright (c) 2003 QUALCOMM Incorporated. All rights reserved.
 * See License.txt file for terms and conditions for modification and
 * redistribution.
 *
 * Revisions:
 *
 * 03/09/01  [rcg]
 *         - Now including continuation of UIDL headers in UIDL hash.
 *         - Don't include random component of extra headers in UIDL
 *           hash if old-style-uid is set.
 *
 * 03/05/01  [rcg]
 *         - Open temp drop with group access, so fast-update won't
 *           lose it.
 *
 * 01/31/01  [rcg]
 *         - Preserve original group ID, for use in pop_bull.
 *
 * 01/30/01 [ rcg]
 *         - Qmaillock now takes bNo_atomic_open and pSpool_dir parameters
 *           instead of using compile-time constants.
 *         - Now using p->pCfg_spool_dir insted of POP_MAILDIR.
 *
 * 01/15/01  [rcg]
 *         - New run-time settings: bDo_timing, bCheck_old_spool_loc,
 *           bCheck_hash_dir, hash_spool, bUpdate_status_hdrs, bHome_dir_mail,
 *           bOld_style_uid, bUW_kluge, bKeep_temp_drop, bulldb_max_tries.
 *
 * 10/04/00  [rcg]
 *         - Allow Content-Length: header to be CONTLEN_FUZZ greater than
 *           reality.
 *
 * 09/21/00  [rcg]
 *         - Accounting for uidl_str no longer dynamically allocated.
 *
 * 08/17/00  [rcg]
 *         - Adjusted bulldb open algorithm to use usleep() if available
 *           and thus wait for very short periods, with randomness.
 *         - Added support for bulldb-nonfatal option, to allow session to
 *           proceed without bulletins.
 *
 * 07/05/00  [rcg]
 *         - Print maillock error string.
 *         - Be a little more patient locking spool.
 *
 * 06/27/00  [rcg]
 *         - Fix UID generation when OLD_STYLE_UIDL set to match 2.53.
 *
 * 06/16/00  [rcg]
 *         - Don't check for old spools in old location unless HASH_SPOOL
 *           or HOMEDIRMAIL set, and DONT_CHECK_OLD_SPOOL not set.
 *         - Deleted some unused variables.
 *
 * 06/08/00  [rcg]
 *         - UIDs now deterministic when NO_STATUS set.
 *         - Removed switch for old-style locking.
 *         - Added switch (OLD_STYLE_UIDL) to encode UIDLs in old
 *           (pre-3.x) style.
 *
 * 06/05/00  [rcg]
 *         - Applied patches from Jeffrey C. Honig at BSDI.
 *
 * 05/16/00  [rg]
 *         - We now check if we need to refresh the mail lock every
 *           p->check_lock_refresh messages.
 *         - When '-Z' run-time switch used, we revert to old (pre-3.x)
 *           locking.  This is dangerous and will likely be removed prior
 *           to release.
 *
 * 04/28/00  [rg]
 *         - Avoid being fooled by a spoofed 'From ' line that follows
 *           a line which overflowed our input buffer.  [Based on patch
 *           by '3APA3A'].
 *         - Fixed some cases in which an empty .user.pop file could be
 *           left behind.
 *
 * 04/21/00  [rg]
 *         - Always use our maillock(), even if system supplies one.
 *         - Rename our maillock() to Qmaillock().
 *
 * 03/14/00  [rg]
 *         - Minor syntactical tweaks.
 *
 * 02/16/00  [rg]
 *         - Now calling touchlock periodically.
 *         - Added "or check for corrupted mail drop" to infamous
 *           'change recognition mode' message.
 *         - Check for and hide UW Folder Internal Data message if
 *           CHECK_UW_KLUDGE defined.
 *
 * 02/04/00  [rg]
 *         - Minor changes to try and appease HP C compiler.
 *
 * 02/02/00  [rg]
 *         - Return extended error response [SYS/TEMP] or [SYS/PERM]
 *           for various system errors.
 *         - Improved error message when maillock fails.
 *         - No longer reporting "Unable to process From lines" when
 *           running in normal (not server) mode and previous session 
 *           aborted due to quota exceeded or disk full.
 *         - If we are unable to copy spool to temp drop in normal (not
 *           server) mode due to disk full or over quota, and temp drop
 *           was empty or did not previously exist, we now properly zero
 *           temp drop instead of setting it to a random size.
 *
 * 01/26/00  [rg]
 *         - Fixed typo in UIDL processing.
 *
 * 01/25/00  [rg]
 *         - Added more trace code to UID processing.
 *
 * 01/07/00  [rg]
 *         - Added STRERROR
 *         - Create hashed spool directory if it doesn't exist
 *         - Check for old .pop file in old location first.
 *
 * 12/30/99  [rg]
 *         - Added retry loop if bulletin db in use.
 *
 * 12/07/99  [rg]
 *         - Added misc.h to be sure odd things that are missing on some
 *           systems are properly defined.
 *         - Fixed typo (SYS_MAILOCK instead of SYS_MAILLOCK)./
 *
 * 11/24/99  [rg]
 *         - Moved genpath() to its own file: genpath.c (and genpath.h).
 *         - Fixed HAVE_MAILLOCK to be HAVE_MAILLOCK_H
 *         - Changed MAILLOCK to be SYS_MAILLOCK
 *         - Removed MAILUNLOCK macro; added #include for our own maillock.h.
 *         - Made use of maillock() unconditional.
 *
 *  6/22/98  [py]
 *         - Added kludge for NULLs in message content (suggested by 
 *           Mordechai T.Abzug).
 *           Uidl to contain From Envelope in non-mmdf, and length check.
 *
 *  3/18/98  [py]
 *         - Reviewed the code, added DEBUG macros, corrected the 
 *           resources that are not closed.
 */

/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */


/*
 * When adding each line length into the size of the message and the maildrop,
 * increase the character count by one for the <cr> added when sending the
 * message to the mail client.  All lines sent to the client are terminated
 * with a <cr><lf>.
 */


#include <sys/types.h>
#include <errno.h>
#include <sys/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <flock.h>

#include "config.h"
#include "md5.h"

#if HAVE_UNISTD_H
#  include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if HAVE_SYS_UNISTD_H
#  include <sys/unistd.h>
#endif /* HAVE_SYS_UNISTD_H */

#ifndef HAVE_INDEX
#  define   index(s,c)      strchr(s,c)
#  define   rindex(s,c)     strrchr(s,c)
#endif /* HAVE_INDEX */

#if HAVE_STRINGS_H
#  include <strings.h>
#endif

#include <sys/stat.h>
#include <sys/file.h>
#include <pwd.h>

#include <time.h>

#include "genpath.h"
#include "popper.h"
#include "misc.h"
#include "maillock.h"
#include "string_util.h"


/*
 * RFC 1939 permits UIDs to be between 1 and 70 characters, but
 * we only recognize our own, so we can be more restrictive.
 */
#define MIN_UIDL_LENGTH 12     /* Ignore totally bogus UIDLs */
#define MAX_UIDL_LENGTH ( (DIG_SIZE * 2) + 1 )

#ifndef   CONTLEN_FUZZ
#  define CONTLEN_FUZZ 25
#endif /* CONTLEN_FUZZ */

/* This macro comes from Mark Crispin's c-client code */

/* Validate line
 * Accepts: pointer to candidate string to validate as a From header
 *      return pointer to end of date/time field
 *      return pointer to offset from t of time (hours of ``mmm dd hh:mm'')
 *      return pointer to offset from t of time zone (if non-zero)
 * Returns: t,ti,zn set if valid From string, else ti is NIL
 */

#define VALID(s,x,ti,zn) {                      \
  ti = 0;                               \
  if ((*s == 'F') && (s[1] == 'r') && (s[2] == 'o') && (s[3] == 'm') && \
      (s[4] == ' ')) {                          \
    for (x = s + 5; *x && *x != '\n'; x++);             \
    if (x) {                                \
      if (x - s >= 41) {                        \
    for (zn = -1; x[zn] != ' '; zn--);              \
    if ((x[zn-1] == 'm') && (x[zn-2] == 'o') && (x[zn-3] == 'r') && \
        (x[zn-4] == 'f') && (x[zn-5] == ' ') && (x[zn-6] == 'e') && \
        (x[zn-7] == 't') && (x[zn-8] == 'o') && (x[zn-9] == 'm') && \
        (x[zn-10] == 'e') && (x[zn-11] == 'r') && (x[zn-12] == ' '))\
      x += zn - 12;                         \
      }                                 \
      if (x - s >= 27) {                        \
    if (x[-5] == ' ') {                     \
      if (x[-8] == ':') zn = 0,ti = -5;             \
      else if (x[-9] == ' ') ti = zn = -9;              \
      else if ((x[-11] == ' ') && ((x[-10]=='+') || (x[-10]=='-'))) \
        ti = zn = -11;                      \
    }                               \
    else if (x[-4] == ' ') {                    \
      if (x[-9] == ' ') zn = -4,ti = -9;                \
    }                               \
    else if (x[-6] == ' ') {                    \
      if ((x[-11] == ' ') && ((x[-5] == '+') || (x[-5] == '-')))    \
        zn = -6,ti = -11;                       \
    }                               \
    if (ti && !((x[ti - 3] == ':') &&               \
            (x[ti -= ((x[ti - 6] == ':') ? 9 : 6)] == ' ') &&   \
            (x[ti - 3] == ' ') && (x[ti - 7] == ' ') &&     \
            (x[ti - 11] == ' '))) ti = 0;           \
      }                                 \
    }                                   \
  }                                 \
}

/* You are not expected to understand this macro, but read the next page if
 * you are not faint of heart.
 *
 * Known formats to the VALID macro are:
 *      From user Wed Dec  2 05:53 1992
 * BSD      From user Wed Dec  2 05:53:22 1992
 * SysV     From user Wed Dec  2 05:53 PST 1992
 * rn       From user Wed Dec  2 05:53:22 PST 1992
 *      From user Wed Dec  2 05:53 -0700 1992
 *      From user Wed Dec  2 05:53:22 -0700 1992
 *      From user Wed Dec  2 05:53 1992 PST
 *      From user Wed Dec  2 05:53:22 1992 PST
 *      From user Wed Dec  2 05:53 1992 -0700
 * Solaris  From user Wed Dec  2 05:53:22 1992 -0700
 *
 * Plus all of the above with `` remote from xxx'' after it. Thank you very
 * much, smail and Solaris, for making my life considerably more complicated.
 */


/*
 * Prototypes
 */
       int     isfromline(char *cp);
static char   *base80(char *output, unsigned char *digest);
static char   *hexit(char *output, unsigned char *digest);
static char   *encode_uid_hash(char *output, unsigned char *digest, BOOL bOld_style);
static int     init_dropinfo(POP *p, char *fname, FILE *fdesc, time_t time_locked);
static int     do_drop_copy(POP *p, int mfd, time_t time_locked);
       int     pop_dropcopy(POP *p, struct passwd *pwp);
static char   *mfgets(char *s, int size, FILE *stream);
static void    doze(POP *p, int atmpt);


/*
 * Globals
 */
int newline  = 1;   /* set to 1 when preceding line was only a newline */
int isOflow  = 0;   /* set to 1 when current line exceeds input buffer */
int wasOflow = 0;   /* set to 1 when previous line exceeded input buffer */

/*
 *  0 for not a from line
 *  1 for is a from line
 */

/* Valid UUCP From lines:
 *
 *  From Address [tty] date
 *  From [tty] date
 *
 *  "From [tty] date" is probably valid but I'm too lazy at this
 *  time to add the code.
 *
 */
int
isfromline ( cp )
char        *cp;
{
    int ti, zn;
    char *x;

    /* If we overflow out input buffer, we can't trust an immediately
       following "\nFrom " line as a valid message separator. */
    if ( isOflow ) {
        isOflow  = 0;
        wasOflow = 1;
        return ( 0 );
    }
    if ( wasOflow ) {
        wasOflow = 0;
        return ( 0 );
    }

    /* If the previous line was not a newline then just return */
    /* 'From ' message separators are preceeded by a newline */ 
    if ( *cp == '\n' ) {
        newline = 1;
        return ( 0 );
    } else if ( !newline ) {
        return ( 0 );
    } else
        newline = 0;

    VALID ( cp, x, ti, zn );
    return ( ti != 0 );
}


/*
 * Base 80 encoding of 16byte MD5 hash. This is about the smallest encoding
 * possible for the MD5 hash using the chars allowed for UIDLs. 
 * (93 ASCII values are available and 19^93 < 2^128 < 20^93)
 * This implementation isn't exact about MSBs and LSB, but it is unique..
 */

static char *
base80 ( char *output, unsigned char *digest )
{
  unsigned long v, j;
  int i, k;

  for ( i = 0; i < 16; ) {
    v = 0L;
    do {
      v = (v << 4) + digest[i++];
    } while ( i%4 );
    for ( k = 0; k < 5; k++ ) {
      j = v % 80;
      v = (v - j)/80;
      *output = 0x21 + j;
      /* 
       * Avoid '.' altgother to avoid having to stuff it 
       */
      if ( *output == '.' )
        *output = '~';
      output++;
    }
  }
  *output = '\0';
  return ( output );
}


/*
 * Old-style (pre-3.x) encoding of MD5 digests for UID strings was simple hex
 */
static char *
hexit ( char *output, unsigned char *digest )
{
    int   i;


    for ( i = 0; 
          i < DIG_SIZE; 
          i++, output += 2 ) {
        sprintf ( output, "%02x", digest[i] );
    }

    *output = '\0';
    return ( output );
}


static char *
encode_uid_hash ( char *output, unsigned char *digest, BOOL bOld_style )
{
    if ( bOld_style )
        return hexit  ( output, digest );
    else
        return base80 ( output, digest );
}


/* 
 * Build the Msg_info_list from the temp_drop in case the previous session
 * failed to truncate. init_dropinfo and do_drop_copy differ in that
 * do_drop_copy copies all the messages from the source file to the 
 * destination file while it generates the Msg_info_list.
 */
static int 
init_dropinfo ( POP *p, char *fname, FILE *fdesc, time_t time_locked )
{
    MsgInfoList    *mp;                     /* Pointer to message info list */
    int             msg_num;                /* Current message number */
    int             visible_msg_num;        /* Current visible message number */
    int             nchar;
    BOOL            bInHeader;
    BOOL            bInContHeader;
    int             uidl_found;
    int             expecting_trailer;
    int             content_length, content_nchar, cont_len;
    char            buffer[MAXLINELEN];     /* Read buffer */
    char            frombuf[MAXLINELEN];    /* From Envelope of each
                                             * message is saved, for
                                             * adding it to UIDL
                                             * computation */
    MD5_CTX         mdContext;
    unsigned char   digest[16];
    long            unique_num = time_locked;
    int             uw_hint    = 0;
    BOOL            bNewUIDs   = p->bUpdate_status_hdrs == TRUE &&
                                 p->bOld_style_uid      == FALSE;

    DEBUG_LOG1 ( p, "DROPINFO Checking file %s", fname );


#ifdef NULLKLUDGE
    {
    /* 
     * Eat NULLs at the beginning of the mailspool 
     */
    char tempchar;
    while ( ( tempchar = getc ( fdesc ) ) == 0 );
    ungetc ( tempchar, fdesc );
    }
#endif

    if ( mfgets ( buffer, MAXMSGLINELEN, fdesc ) == NULL ) {
        DEBUG_LOG1 ( p, "Drop file empty intially : %s", fname );
        return ( POP_SUCCESS );    
    }

    /*
     * Is this an MMDF file? 
     */
    if ( *buffer == MMDF_SEP_CHAR ) {
        p->mmdf_separator = (char *)strdup(buffer);
    } else if ( !isfromline(buffer) ) {
        DEBUG_LOG5 ( p, "newline=%d; isOflow=%d; wasOflow=%d; buffer(%u):  %.256s", 
                     newline, isOflow, wasOflow, strlen(buffer), buffer );
        return pop_msg ( p, POP_FAILURE, HERE,
                         "[SYS/PERM] Unable to process From lines "
                         "(envelopes) in %s; change recognition "
                         "mode or check for corrupted mail drop.",
                         fname );
    }

    newline = 1;
    rewind ( fdesc );

    bInHeader         = FALSE;
    bInContHeader     = FALSE;
    msg_num           = 0;
    visible_msg_num   = 0;
    uidl_found        = 0;
    expecting_trailer = 0;
    content_length    = 0;
    content_nchar     = 0;
    cont_len          = 0;
    p->msg_count      = ALLOC_MSGS;


    if ( DEBUGGING && time_locked == 0 )
        pop_log ( p, POP_NOTICE, HERE, "init_dropinfo called with time_locked == 0" );

#ifdef NULLKLUDGE
    {
    /* 
     * Eat NULLs at the beginning of the mailspool 
     */
    char tempchar;
    while ( ( tempchar = getc ( fdesc ) ) == 0 );
    ungetc ( tempchar, fdesc );
    }
#endif

    DEBUG_LOG1 ( p, "Building the Message Information list from %s", fname );

    for ( mp = p->mlp - 1; mfgets ( buffer, MAXMSGLINELEN, fdesc ); ) {
        nchar = strlen(buffer);
        
        /*
         * It is very important to refresh the mail lock on an
         * interval between one and three minutes.  To avoid
         * calling time(0) too often, we check every x msgs.
         */
        if ( msg_num % p->check_lock_refresh == 0 &&
             time(0) - time_locked >= LOCK_REFRESH_INTERVAL ) {
            Qtouchlock ( HERE );
            time_locked = time(0);
        }

        if ( ( content_nchar >= content_length ||
               content_length - content_nchar <= CONTLEN_FUZZ )
            && ( p->mmdf_separator ? !strcmp(p->mmdf_separator, buffer)
                                   : isfromline(buffer) ) ) {
            /* 
             * ---- A message boundary ----- 
             *
             * ---- Tidy up previous message (if there was one) ---- 
             */

            if ( expecting_trailer ) {
                /* 
                 * Skip over the MMDF trailer 
                 */
                expecting_trailer = 0;
                continue;
            }

            if ( !p->mmdf_separator ) {
                strlcpy(frombuf, buffer, sizeof(frombuf)); /* Preserve From envelope */
            }
            
            /* 
             * ------ Get ready for the next message ----- 
             */
            MD5Init   ( &mdContext );
            MD5Update ( &mdContext, (unsigned char *)buffer,
                        strlen(buffer) );

            if ( bNewUIDs ) {
                /* 
                 * Include a unique number in case everything else is not.
                 *
                 * We don't do this unless bUpdate_status_hdrs is set, because
                 * in that case the UIDs need to be deterministic so they stay
                 * the same when recalculated each session.
                 */
                unique_num++;
                MD5Update ( &mdContext, (unsigned char *)&unique_num, 
                            sizeof(long) );
            }

            if ( bInHeader == FALSE ) {
                if ( ++msg_num > p->msg_count ) {
                    p->mlp = (MsgInfoList *) realloc ( p->mlp,
                                                       (p->msg_count += ALLOC_MSGS) 
                                                       * sizeof(MsgInfoList) );
                    if ( p->mlp == NULL ) {
                        p->msg_count = 0;
                        return pop_msg ( p, POP_FAILURE, HERE,
                                         "[SYS/TEMP] Can't build message list "
                                         "for '%s': Out of memory",
                                         p->user );
                    }
                    mp = p->mlp + msg_num - 2;
                }

                if ( msg_num != 1 ) {
                    DEBUG_LOG5 ( p, "Msg %d uidl '%s' at offset %ld is %ld "
                                    "octets long and has %u lines.",
                                 mp->number, mp->uidl_str, mp->offset, 
                                 mp->length, mp->lines );
                }
                else
                    DEBUG_LOG0 ( p, "Found top of first message" );
                ++mp;

            } 
            else { /* still in header */
                pop_log ( p, POP_PRIORITY, HERE,
                          "Msg %d corrupted; ignoring previous header"
                          " information.",
                          mp->number );
            }
            mp->number     = msg_num;
            mp->length     = 0;
            mp->lines      = 0;
            mp->body_lines = 0;
            mp->offset     = ftell(fdesc) - nchar;
            mp->del_flag   = FALSE;
            mp->hide_flag  = FALSE;
            mp->retr_flag  = FALSE;
            mp->orig_retr_state = FALSE;
            mp->uidl_str[0]= '\n';
            mp->uidl_str[1]= '\0';
            DEBUG_LOG2 ( p, "Msg %d being added to list %p", mp->number, p->mlp );
            bInHeader      = TRUE;
            bInContHeader  = FALSE;
            uidl_found     = 0;
            content_nchar  = 0;
            content_length = 0;
            cont_len       = 0;
            if ( p->mmdf_separator )
                expecting_trailer = 1;

            continue;   /* Don't count message separator in total size */
        }
        
        if ( bInHeader ) {
            if ( bInContHeader && ( *buffer != ' ' || *buffer != '\t' ) )
                bInContHeader  = FALSE;
            if ( *buffer == '\n' ) { /* End of headers */
                bInHeader      = FALSE;
                bInContHeader  = FALSE;
                content_length = cont_len;
                mp->body_lines = 1;  /* Count newline as the first body line */
                if ( mp->hide_flag == 0 )
                    mp->visible_num = ++visible_msg_num;
                
                if ( !uidl_found ) {
                    char    *cp;

                if ( p->bOld_style_uid == FALSE )
                    MD5Update ( &mdContext, (unsigned char *)frombuf, 
                                strlen(frombuf) );

                    MD5Final ( digest, &mdContext );
                    cp = mp->uidl_str;
                    cp = encode_uid_hash ( cp, digest, p->bOld_style_uid );
                    *cp++ = '\n';
                    *cp   = '\0';
                    
                    DEBUG_LOG2 ( p, "UID not found; generated UID(%d): %s",
                                    strlen(mp->uidl_str), mp->uidl_str );

                    mp->length   += strlen("X-UIDL: ") + strlen(mp->uidl_str) +1;
                    p->drop_size += strlen("X-UIDL: ") + strlen(mp->uidl_str) +1;

                    /* New UIDs do not dirty the mailspool unless
                     * bUpdate_status_hdrs is set.  They are just recalculated
                     * each time Qpopper is run.
                     */
                    if ( p->bUpdate_status_hdrs )
                        p->dirty = TRUE;
                } /* !uidl_found */

            } else if ( CONTENT_LENGTH && 
                        !strncmp(buffer, "Content-Length:", 15) ) {
                cont_len = atoi(buffer + 15);
                MD5Update ( &mdContext, (unsigned char *)buffer, strlen(buffer) );
                mp->lines++;
                continue;   /* not part of the message size */
            } else if ( !uidl_found && 
                            ( !strncasecmp ( "Received:",   buffer,  9 ) ||
                              !strncasecmp ( "Date:",       buffer,  5 ) ||
                              !strncasecmp ( "Message-Id:", buffer, 11 ) ||
                              !strncasecmp ( "Subject:",    buffer,  8 ) ||
                              ( bInContHeader && bNewUIDs )
                            )
                      ) {
                MD5Update ( &mdContext, (unsigned char *)buffer,
                            strlen(buffer) );
            } else if ( !strncasecmp("X-UIDL:", buffer, 7) ) {
                if ( !uidl_found ) {
                    char *cp;
                    int len;

                    /* 
                     * Skip over header string 
                     */
                    cp = index ( buffer, ':' );
                    if ( cp != NULL ) {
                        cp++;
                        while ( *cp && ( *cp == ' ' || *cp == '\t') ) 
                            cp++;
                    } else
                        cp = "";

                    len = strlen ( cp );
                    if ( len >= MIN_UIDL_LENGTH && 
                         len <= MAX_UIDL_LENGTH ) {
                        uidl_found++;
                        strlcpy ( mp->uidl_str, cp, sizeof(mp->uidl_str) );
                        mp->length   += nchar + 1;
                        p->drop_size += nchar + 1;
                        DEBUG_LOG1 ( p, "Found UIDL header: %s", buffer );
                    }
                    else
                        DEBUG_LOG4 ( p, "Ignoring UIDL header: %s "
                                        "(len=%d; min=%d; max=%d)",
                                     buffer, len, MIN_UIDL_LENGTH, 
                                     MAX_UIDL_LENGTH );
                } /* !uidl_found */
            
                mp->lines++;
                continue;  /* Do not include this value in the message size */
            } else if ( (strncasecmp(buffer, "Status:", 7) == 0) ) {
                if ( index(buffer, 'R') != NULL ) {
                    mp->retr_flag = TRUE;
                    mp->orig_retr_state = TRUE;
                }
            }
            if ( p->bUW_kluge && msg_num == 1 ) {
                if ( strncasecmp ( buffer, "Subject: ", 9  ) == 0   &&
                      ( strstr ( buffer, "DO NOT DELETE"             ) != NULL ||
                        strstr ( buffer, "DON'T DELETE THIS MESSAGE" ) != NULL ||
                        strstr ( buffer, "FOLDER INTERNAL DATA"      ) != NULL  
                      )
                   ) {
                    uw_hint++;
                    DEBUG_LOG2 ( p, "uw_hint now %i; UW_KLUDGE matched '%.64s'",
                                 uw_hint, buffer );
                } else
                if ( strncasecmp ( buffer, "X-IMAP: ", 8 ) == 0 ) {
                    uw_hint++;
                    DEBUG_LOG2 ( p, "uw_hint now %i; UW_KLUDGE matched '%.64s'",
                                 uw_hint, buffer );
                } else
                if ( strncasecmp ( buffer, "From: Mail System Internal Data", 31 ) == 0 ) {
                    uw_hint++;
                    DEBUG_LOG2 ( p, "uw_hint now %i; UW_KLUDGE matched '%.64s'",
                                 uw_hint, buffer );
                }
                if ( uw_hint == 2 ) {
                    mp->hide_flag = 1;
                    p->first_msg_hidden = 1;
                    DEBUG_LOG0 ( p, "UW_KLUDGE check matched 2 conditions; msg hidden" );
                }
            } /* UW_KLUDGE && msg_num == 1 */
        } /* bInHeader */
        else { /* not bInHeader */
            content_nchar += nchar;
            mp->body_lines++;
        }
           
        mp->length   += nchar + 1;
        p->drop_size += nchar + 1;
        mp->lines++;
    } /* for loop */

    p->msg_count = msg_num;
    p->visible_msg_count = visible_msg_num;

    DEBUG_LOG1 ( p, "init_dropinfo exiting; newline=%d", newline );
    return ( POP_SUCCESS );
}


/* 
 *  Used to be dropinfo:   Extract information about the POP maildrop and store 
 *  it for use by the other POP routines.
 *
 *  Now, the copy and info collection are done at the same time.
 *
 *  Parameters:
 *     p:         Pointer to POP structure.
 *     mfd:       File descriptor for spool file (p->drop_name is name).
 */
static int
do_drop_copy ( POP *p, int mfd, time_t time_locked ) 
{
    char            buffer  [ MAXLINELEN ];      /*  Read buffer */
    char            frombuf [ MAXLINELEN ];      /*  To save From Envelope */
    MsgInfoList    *mp;                          /*  Pointer to message 
                                                     info list */
    int             nchar;                       /*  Bytes written/read */
    BOOL            bInHeader;                   /*  Header section flag */
    BOOL            bInContHeader;               /*  Continuation hader flag */
    int             uidl_found;                  /*  UIDL header flag */
    int             msg_num;                     /*  Current msg number */
    int             visible_msg_num;             /*  Current visible msg num */
    int             expecting_trailer;
    int             content_length, content_nchar, cont_len;
    MD5_CTX         mdContext;
    unsigned char   digest[16];
    FILE           *mail_drop;                   /*  Streams for fids */
    long            unique_num = time_locked;
    int             uw_hint    = 0;
    BOOL            bNewUIDs   = p->bUpdate_status_hdrs == TRUE &&
                                 p->bOld_style_uid      == FALSE;


    DEBUG_LOG1 ( p, "DROPCOPY: Reading the mail drop (p->msg_count = %i).",
                 p->msg_count );

    if ( DEBUGGING && time_locked == 0 )
        pop_log ( p, POP_NOTICE, HERE, "do_drop_copy called with time_locked == 0" );

    /*  
     * Acquire a stream pointer for the maildrop 
     */
    mail_drop = fdopen ( mfd, "r" );
    if ( mail_drop == NULL ) {
        return pop_msg ( p, POP_FAILURE, HERE,
                         "[SYS/TEMP] Cannot assign stream for %s: %s (%d)",
                         p->drop_name, STRERROR(errno), errno );
    }

    rewind ( mail_drop );
    newline = 1;  /* first line of file counts as having new line before it */

#ifdef NULLKLUDGE
    {/* 
      * Eat NULLs at the beginning of the mailspool 
      */
    char tempchar;
    
    while ( ( tempchar = getc ( p->drop ) ) == 0 );
    ungetc ( tempchar, p->drop );
    }
#endif

    if ( mfgets ( buffer, MAXMSGLINELEN, mail_drop ) == NULL ) {
        return ( POP_SUCCESS );
    }

    /* 
     * Is this an MMDF file? 
     */
    if ( *buffer == MMDF_SEP_CHAR ) {
        if ( !p->mmdf_separator )
            p->mmdf_separator = (char *)strdup(buffer);
    } else if ( !isfromline ( buffer ) ) {
        DEBUG_LOG5 ( p, "newline=%d; isOflow=%d; wasOflow=%d; buffer(%u):  %.256s", 
                     newline, isOflow, wasOflow, strlen(buffer), buffer );
        return pop_msg ( p, POP_FAILURE, HERE,
                            "[SYS/PERM] Unable to process From lines "
                            "(envelopes), change recognition modes or "
                            "check for corrupted mail drop." );
    }

    newline = 1;  /* first line of file counts as having new line before it */
    rewind ( mail_drop );

    /*  
     * Scan the file, loading the message information list with 
     * information about each message 
     */

    bInHeader         = FALSE;
    bInContHeader     = FALSE;
    uidl_found        = 0;
    expecting_trailer = 0;
    msg_num           = p->msg_count;
    visible_msg_num   = p->visible_msg_count;
    content_length    = 0;
    content_nchar     = 0;
    cont_len          = 0;
    p->msg_count      = (((p->msg_count - 1) / ALLOC_MSGS) + 1) * ALLOC_MSGS;

#ifdef NULLKLUDGE
    {/* Eat NULLs at the beginning of the mailspool if any.*/
    char tempchar;
    
    while ( ( tempchar = getc ( p->drop ) ) == 0 );
    ungetc  ( tempchar, p->drop );
    }
#endif /* NULLKLUDGE */

    for ( mp = p->mlp + msg_num - 1; 
          mfgets ( buffer, MAXMSGLINELEN, mail_drop ); 
        ) {
        nchar = strlen ( buffer );
        
        /*
         * It is very important to refresh the mail lock on an
         * interval between one and three minutes.  To avoid
         * calling time(0) too often, we check every x msgs.
         */
        if ( msg_num % p->check_lock_refresh == 0 &&
             time(0) - time_locked >= LOCK_REFRESH_INTERVAL ) {
            Qtouchlock ( HERE );
            time_locked = time(0);
        }

        if ( fputs ( buffer, p->drop)  == EOF ) {
            if ( errno == EDQUOT )
                return pop_msg ( p, POP_FAILURE, HERE,
                                 "[SYS/TEMP] Unable to copy mail spool file, "
                                 "quota exceeded (%d)",
                                  errno );
            return pop_msg ( p, POP_FAILURE, HERE,
                             "[SYS/TEMP] Unable to copy mail spool file to "
                             "temp pop dropbox %s: %s (%d)",
                             p->temp_drop, STRERROR(errno), errno );
        }
    
        /* 
         * End of previous message 
         */
        if ( ( content_nchar >= content_length ||
               content_length - content_nchar <= CONTLEN_FUZZ )
            && ( p->mmdf_separator ? !strcmp(p->mmdf_separator, buffer)
                                   : isfromline(buffer) ) 
           ) {
            /*
             * === Matched separator, tie off last message, start new one ===
             */


            if ( expecting_trailer ) {
                expecting_trailer = 0;
                continue;
            }

            if ( !p->mmdf_separator ) {
                strlcpy ( frombuf, buffer, sizeof(frombuf) );
            }

            /*
             * === Starting new message ===
             */
            MD5Init   ( &mdContext );
            MD5Update ( &mdContext, (unsigned char *)buffer, strlen(buffer) );

            if ( bNewUIDs ) {
                /* 
                 * Include a unique number in case everything else is not.
                 *
                 * We don't do this unless bUpdate_status_hdrs is set, because
                 * then the UIDs have to be deterministic so they stay the
                 * same when recalculated each session.
                 */
                unique_num++;
                MD5Update ( &mdContext, (unsigned char *)&unique_num, 
                            sizeof(long) );
            }

            if ( bInHeader == FALSE ) {
                if ( ++msg_num > p->msg_count ) {
                    p->mlp = (MsgInfoList *) realloc ( p->mlp,
                                                       (p->msg_count+=ALLOC_MSGS)
                                                       * sizeof(MsgInfoList) );
                    if ( p->mlp == NULL ) {
                        p->msg_count = 0;
                        return pop_msg ( p, POP_FAILURE, HERE,
                                        "[SYS/TEMP] Can't build message list "
                                        "for '%s': Out of memory",
                                        p->user );
                    }
                    mp = p->mlp + msg_num - 2;
                } /* ++msg_num > p->msg_count */
            if ( msg_num != 1 )
                DEBUG_LOG5 (p, "Msg %d uidl '%s' at offset %ld is %ld octets long"
                               " and has %u lines.",
                            mp->number, mp->uidl_str, mp->offset, mp->length,
                            mp->lines );
            ++mp;

            } else { /* still in header */
                pop_log ( p, POP_PRIORITY, HERE,
                          "Msg %d corrupted, ignoring previous header information.",
                          mp->number );
            }

            mp->number     = msg_num;
            mp->length     = 0;
            mp->lines      = 0;
            mp->body_lines = 0;
            mp->offset     = ftell(p->drop) - nchar;
            mp->del_flag   = FALSE;
            mp->hide_flag  = FALSE;
            mp->retr_flag  = FALSE;
            mp->orig_retr_state = FALSE;
            mp->uidl_str[0] = '\n';
            mp->uidl_str[1] = '\0';

            DEBUG_LOG1 ( p, "Msg %d being added to list.", mp->number );
            bInHeader      = TRUE;
            content_length = 0;
            content_nchar  = 0;
            cont_len       = 0;
            uidl_found     = 0;
            if ( p->mmdf_separator )
                expecting_trailer = 1;

            continue;   /* Do not include From separator in message size */
            } /* tie off last message, start new one */

        /* 
         * ====== Handle header and body of message here ==== 
         */
        if ( bInHeader ) {
            /* 
             * ===== Handle the header of a message ==== 
             */
            if ( bInContHeader && ( *buffer != ' ' || *buffer != '\t' ) )
                bInContHeader  = FALSE;

            if ( *buffer == '\n' ) {
                /* 
                 * === Encountered transition to body ====
                 */
                bInHeader      = FALSE;
                bInContHeader  = FALSE;
                mp->body_lines = 1;
                content_length = cont_len;
                if ( mp->hide_flag == 0 )
                    mp->visible_num = ++visible_msg_num;

                if ( !uidl_found ) {
                    char *cp = mp->uidl_str;

                if ( p->bOld_style_uid == FALSE )
                    MD5Update ( &mdContext, (unsigned char *)frombuf,
                                strlen(frombuf) );

                    MD5Final  ( digest, &mdContext );
                    cp = encode_uid_hash ( cp, digest, p->bOld_style_uid );
                    *cp++ = '\n';
                    *cp   = '\0';
                    DEBUG_LOG3 ( p, "UID not found; generated UID(%d): %s [%d]",
                                    strlen(mp->uidl_str), mp->uidl_str, __LINE__ );

                    mp->length   += strlen("X-UIDL: ") + strlen(mp->uidl_str) +1;
                    p->drop_size += strlen("X-UIDL: ") + strlen(mp->uidl_str) +1;

                    /* New UIDs do not dirty the mailspool unless
                     * bUpdate_status_hdrs is set.  They are just recalculated
                     * each time Qpopper is run. 
                     */
                    if ( p->bUpdate_status_hdrs )
                        p->dirty = TRUE;
                } /* !uidl_found */

            } else 
            if ( CONTENT_LENGTH && 
                 !strncmp ( buffer, "Content-Length:", 15 ) ) {
                /*=== 
                 * Handle content_length if we do that sort of thing ===
                 */
                cont_len = atoi ( buffer + 15 );
                MD5Update ( &mdContext, (unsigned char *)buffer, strlen(buffer) );
                mp->lines++;
                continue;  /* Not included in message size */

            } else 
            if ( !uidl_found && 
                            ( !strncasecmp ( "Received:",   buffer,  9 ) ||
                              !strncasecmp ( "Date:",       buffer,  5 ) ||
                              !strncasecmp ( "Message-Id:", buffer, 11 ) ||
                              !strncasecmp ( "Subject:",    buffer,  8 ) ||
                              ( bInContHeader && bNewUIDs )
                            )
                      ) {
                /* 
                 * === Include desired headers in UIDL hash ===
                 */
                MD5Update ( &mdContext, (unsigned char *)buffer, 
                            strlen(buffer) );
            } else if ( !strncasecmp ( "X-UIDL:", buffer, 7 ) ) {
                /* 
                 * ==== Do the UIDL header ==== 
                 */
                if ( !uidl_found ) {
                    char *cp;
                    int len;

                    /* 
                     * Skip over header 
                     */
                    cp = index ( buffer, ':' );
                    if ( cp != NULL ) {
                        cp++;
                        while ( *cp && ( *cp == ' ' || *cp == '\t' ) ) 
                            cp++;
                    } else
                        cp = "";

                    len = strlen ( cp );
                    if ( len > MIN_UIDL_LENGTH && 
                         len < MAX_UIDL_LENGTH ) {
                        uidl_found++;
                        strlcpy ( mp->uidl_str, cp, sizeof(mp->uidl_str) );
                        mp->length += nchar + 1;
                        p->drop_size += nchar + 1;
                        DEBUG_LOG1 ( p, "Found UIDL header: %s", buffer );
                    }
                    else
                        DEBUG_LOG4 ( p, "Ignoring UIDL header: %s "
                                        "(len=%d; min=%d; max=%d)",
                                     buffer, len, MIN_UIDL_LENGTH, 
                                     MAX_UIDL_LENGTH );
                } /* !uidl_found */
                mp->lines++;
                continue;  /* Do not include this value in the message size */
            } 
            else if ( !strncasecmp ( buffer, "Status:", 7 ) ) {
                /* 
                 * === Do the status header === 
                 */
                if ( index(buffer, 'R') != NULL ) {
                    mp->retr_flag = TRUE;
                    mp->orig_retr_state = TRUE;
                }
            }
            if ( p->bUW_kluge && msg_num == 1 ) {
                if ( strncasecmp ( buffer, "Subject: ", 9  ) == 0   &&
                      ( strstr ( buffer, "DO NOT DELETE"             ) != NULL ||
                        strstr ( buffer, "DON'T DELETE THIS MESSAGE" ) != NULL ||
                        strstr ( buffer, "FOLDER INTERNAL DATA"      ) != NULL  
                      )
                   ) {
                    uw_hint++;
                    DEBUG_LOG2 ( p, "uw_hint now %i; UW_KLUDGE matched '%.64s'",
                                 uw_hint, buffer );
                } else
                if ( strncasecmp ( buffer, "X-IMAP: ", 8 ) == 0 ) {
                    uw_hint++;
                    DEBUG_LOG2 ( p, "uw_hint now %i; UW_KLUDGE matched '%.64s'",
                                 uw_hint, buffer );
                } else
                if ( strncasecmp ( buffer, "From: Mail System Internal Data", 31 ) == 0 ) {
                    uw_hint++;
                    DEBUG_LOG2 ( p, "uw_hint now %i; UW_KLUDGE matched '%.64s'",
                                 uw_hint, buffer );
                }
                if ( uw_hint == 2 ) {
                    mp->hide_flag = 1;
                    p->first_msg_hidden = 1;
                    DEBUG_LOG0 ( p, "UW_KLUDGE check matched 2 conditions; msg hidden" );
                }
            } /* UW_KLUDGE && msg_num == 1 */
        } /* bInHeader */
        else {
            /* 
             * ==== Handle the body of a message (not much to do) === 
             */
            content_nchar += nchar;
            mp->body_lines++;
        }

        mp->length   += nchar + 1;
        p->drop_size += nchar + 1;
        mp->lines++;
    } /* for loop */

    p->msg_count = msg_num;
    p->visible_msg_count = visible_msg_num;

    if ( DEBUGGING && p->debug && msg_num > 0 ) {
        register int    i;
        
        for ( i = 0, mp = p->mlp; i < p->msg_count; i++, mp++ )
            pop_log ( p, POP_DEBUG, HERE,
                      "Msg %d (%d) uidl '%s' at offset %ld is %ld octets long"
                      "%.*s and has %u lines.",
                      mp->number, mp->visible_num, mp->uidl_str, 
                      mp->offset, mp->length,
                      (mp->hide_flag ? 9 : 0), " (hidden)",
                      mp->lines );
    }

    if ( fflush ( p->drop ) == EOF )
        return pop_msg ( p, POP_FAILURE, HERE, 
                         "[SYS/TEMP] Flush of temp pop dropbox %s failed: %s (%d)",
                         p->temp_drop, STRERROR(errno), errno );

    return ( POP_SUCCESS );
}

/* 
 *  dropcopy:   Create table of contents for spool.  Also copies spool into
 *              temp drop if not in server mode.
 */


/*---------------------------------------------------------------------
 * Explanation of spool and drop variables:
 *    p->drop_name:     Name of spool.
 *    p->temp_drop:     Name of temp drop.
 *    p->drop:          Normal mode: FILE * (stream) for temp drop (dfd).
 *                      Server mode: FILE * (stream) for spool (mfd).
 *    p->hold:          Normal mode: NULL.
 *                      Server mode: FILE * for temp drop (saved p->drop).
 *    mfd:              File descriptor for spool.
 *    dfd:              File descriptor for temp drop.
 *---------------------------------------------------------------------*/

int
pop_dropcopy ( p, pwp )
POP        *   p;
struct passwd   * pwp;
{
    int                     mfd;                    /*  File descriptor for 
                                                        the user's maildrop */
    int                     dfd;                    /*  File descriptor for 
                                                        the SERVER maildrop */
    OFF_T                   offset = 0;             /*  Old/New boundary */
    int                     nchar;                  /*  Bytes written/read */
    struct stat             mybuf;                  /*  For fstat() */
    int                     rslt;
    time_t                  time_locked = 0;
    time_t                  my_timer    = 0;


    if ( p->bDo_timing )
        my_timer = time(0);

    if ( genpath ( p, p->drop_name, sizeof(p->drop_name), GNPH_SPOOL ) < 0 )
        return pop_msg ( p, POP_FAILURE, HERE, 
                         "[SYS/TEMP] Unable to get spool name" );

    if ( ( p->hash_spool > 0 || p->pHome_dir_mail != NULL )
         && p->bCheck_old_spool_loc ) {
        /*  
         *  Get name of temporary maildrop into which to copy the updated 
         *  maildrop. 
         *
         *  Older versions of qpopper ignored HASH_SPOOL and HOMEDIRMAIL
         *  when building the path for this file, so we first check if
         *  a file exists using the old rules.  If not, or if it is empty,
         *  we build it where it goes.
         *  
         */
        if ( genpath ( p,
                       p->temp_drop,
                       sizeof(p->temp_drop),
                       GNPH_OLDPOP ) < 0 )
            return pop_msg ( p, POP_FAILURE, HERE,
                             "[SYS/TEMP] Unable to get temp drop name" );
        
        if ( stat ( p->temp_drop, &mybuf ) == -1 || mybuf.st_size <= 0 ) {
            if ( genpath ( p, 
                           p->temp_drop, 
                           sizeof(p->temp_drop),
                           GNPH_POP ) < 0 )
                return pop_msg ( p, POP_FAILURE, HERE,
                                 "[SYS/TEMP] Unable to get temp drop name" );
        }
    }
    else {
        /*
         * not using hash_spool or Home_dir_mail; or bCheck_old_spool_loc not set
         */
        if ( genpath ( p, 
                       p->temp_drop, 
                       sizeof(p->temp_drop),
                       GNPH_POP ) < 0 )
            return pop_msg ( p, POP_FAILURE, HERE,
                             "[SYS/TEMP] Unable to get temp drop name" );
    }

    DEBUG_LOG1 ( p, "Temporary maildrop name: '%s'", p->temp_drop );

#ifdef BULLDB
    if ( p->bulldir != NULL ) {
        char bull_db [ MAXLINELEN ];
        int  tries     = 0;
        BOOL try_again = TRUE;

#  ifdef BULLDBDIR
        sprintf ( bull_db, "%s/bulldb", BULLDBDIR  );
#  else /* BULLDBDIR not defined, use bull dir */
        sprintf ( bull_db, "%s/bulldb", p->bulldir );
#  endif /* BULLDBDIR */

        p->bull_db = NULL;
    
        while ( try_again ) {
#  ifdef GDBM
            p->bull_db = gdbm_open ( bull_db, 512, GDBM_WRCREAT, 0600, 0 );
#  else /* not GDBM */
            p->bull_db = dbm_open  ( bull_db, O_RDWR | O_CREAT,  0600    );
#  endif /* GDBM */
            if ( p->bull_db == NULL && errno == EAGAIN
                                    && tries++ <= p->bulldb_max_tries ) {
                DEBUG_LOG4 ( p, "Bulletin database '%s' unavailable on "
                                "try #%d of %d (user %s); sleeping",
                            bull_db, tries, p->bulldb_max_tries, p->user );
                doze ( p, tries );   /* Sleep between retries */
            } else {
                try_again = FALSE;
            }
        } /* while loop */

        if ( p->bull_db == NULL ) {
            pop_log (p, POP_PRIORITY, HERE, "gdbm_open failed: %s (%d)",
                        sys_errlist[errno], errno );
            if ( p->bulldb_nonfatal == FALSE )
                return ( pop_msg ( p, POP_FAILURE, HERE, 
                                   "[SYS/TEMP] Unable to open Bulletin "
                                   "database; contact your administrator" ) );
            else
                DEBUG_LOG2 ( p, "Unable to open bulldb '%s' for user '%s' "
                                "but bulldb_nonfatal set",
                             bull_db, p->user );
        } else {
            DEBUG_LOG2 ( p, "Opened Bulletin database '%s' (checking user '%s')",
                        bull_db, p->user );
        }
        
    } /* if (p->bulldir) */
#endif /* BULLDB */

    if ( p->hash_spool > 0 && p->bCheck_hash_dir ) {
        /* 
         * Make sure the path to the spool file exists 
         */
         char    buffer [ MAXLINELEN ];
         mode_t  my_umask;
         mode_t  spool_mode;
         mode_t  temp_mode = 0x1C0;
         uid_t   spool_owner;
         gid_t   spool_group;
         char   *ptr;
         
        /*
         * Get the path name.
         */
        if ( genpath ( p, 
                       buffer, 
                       sizeof(buffer),
                       GNPH_PATH ) < 0 )
            return ( pop_msg ( p, POP_FAILURE, HERE,
                               "[SYS/TEMP] Unable to get spool path name" ) );

        /*
         * Get the mode of the parent directory.
         */
        if ( stat ( p->pCfg_spool_dir, &mybuf ) != 0 )
            return pop_msg ( p, POP_FAILURE, HERE, 
                             "[SYS/TEMP] Unable to access spool directory "
                             "%s: %s (%d)",
                             p->pCfg_spool_dir, STRERROR(errno), errno );
        /*
         * We create subdirectories with same mode and owner/group as parent.
         */
        spool_mode  = mybuf.st_mode;
        spool_owner = mybuf.st_uid;
        spool_group = mybuf.st_gid;
        DEBUG_LOG4 ( p, "Spool directory %s has mode %04o; owner %d; group %d",
                     p->pCfg_spool_dir, (unsigned int) spool_mode,
                     (int) spool_owner, (int) spool_group );

        /*
         * Zero umask during this operation, so we can create the new
         * directories with the correct modes.
         */
        my_umask = umask ( 0000 );
        DEBUG_LOG4 ( p, "umask was %04o; now %04o (my real UID=%d; eUID=%d)", 
                     (unsigned int) my_umask, 0000, (int) getuid(),
                     (int) geteuid() );

       /*
        * In theory, we could just create the new directories with mkdir(2)
        * or mknod(2), and pass in the desired mode.  But this causes
        * problems: we can't set the sticky bit this way on Solaris, and 
        * sometimes the call fails on Linux.  So we first create the
        * directories with a temporary mode, then call chmnod(2).
        */

        if ( p->hash_spool == 2 ) {
            /*
             * This method uses two-level hashed spool directories -- create
             * first one first.
             */
            for ( ptr = buffer + ( strlen(buffer) -1 );
                  ptr > buffer && *ptr != '/';
                  ptr-- );
            if ( *ptr == '/' ) {
                *ptr = '\0';
                if ( stat ( buffer, &mybuf ) == -1 && errno == ENOENT ) {
                    /* The directory doesn't exit -- create it */
                    if ( mkdir ( buffer, temp_mode ) == -1 )
                        return pop_msg ( p, POP_FAILURE, HERE,
                                         "[SYS/TEMP] Unable to create spool directory "
                                         "%s (%04o): %s (%d)",
                                         buffer, (int) temp_mode, STRERROR(errno), errno );
                    if ( chmod ( buffer, spool_mode ) == -1 )
                        return pop_msg ( p, POP_FAILURE, HERE,
                                        "[SYS/TEMP] Unable to chmod of %s to %04o: "
                                        "%s (%d)",
                                        buffer, (int) spool_mode, STRERROR(errno), errno );
                    if ( chown ( buffer, spool_owner, spool_group ) == -1 )
                        return pop_msg ( p, POP_FAILURE, HERE,
                                        "[SYS/TEMP] Unable to set owner/group of spool "
                                        "directory %s to %d/%d: %s (%d)",
                                        buffer, (int) spool_owner, (int) spool_group, 
                                        STRERROR(errno), errno );

                    if ( DEBUGGING ) {
                        if ( stat ( buffer, &mybuf ) != 0 )
                            return pop_msg ( p, POP_FAILURE, HERE, 
                                            "[SYS/TEMP] Unable to access newly-created "
                                            "dir %s: %s (%d)",
                                            buffer, STRERROR(errno), errno );
                        DEBUG_LOG4 ( p, "Created %s; mode=%04o; owner=%d; group=%d",
                                     buffer, (unsigned int) mybuf.st_mode,
                                     (int) mybuf.st_uid, (int) mybuf.st_gid );
                    } /* DEBUGGING */

                } /* directory doesn't exit */
                *ptr = '/';
            } else {
                DEBUG_LOG1 ( p, "HASH_SPOOL=2 but unable to find first dir in '%s'",
                             buffer );
            }
        } /* HASH_SPOOL == 2 */
        
        if ( stat ( buffer, &mybuf ) == -1 && errno == ENOENT ) {
            /* The directory doesn't exit -- create it */
            DEBUG_LOG2 ( p, "HASH_SPOOL; Creating spool path: %s (mode %04o)",
                         buffer, (unsigned int) temp_mode );
            if ( mkdir ( buffer, temp_mode ) == -1 )
                return pop_msg ( p, POP_FAILURE, HERE,
                                 "[SYS/TEMP] Unable to create spool directory "
                                 "%s (%04o): %s (%d)",
                                 buffer, (unsigned int) temp_mode,
                                 STRERROR(errno), errno );
            if ( chmod ( buffer, spool_mode ) == -1 )
                return pop_msg ( p, POP_FAILURE, HERE,
                                 "[SYS/TEMP] Unable to chmod of %s to %04o: %s (%d)",
                                 buffer, (unsigned int) spool_mode,
                                 STRERROR(errno), errno );
            if ( chown ( buffer, spool_owner, spool_group ) == -1 )
                return pop_msg ( p, POP_FAILURE, HERE,
                                 "[SYS/TEMP] Unable to set owner/group of spool "
                                 "directory %s to %d/%d: %s (%d)",
                                 buffer, (int) spool_owner, (int) spool_group, 
                                 STRERROR(errno), errno );

            if ( DEBUGGING ) {
                if ( stat ( buffer, &mybuf ) != 0 )
                    return pop_msg ( p, POP_FAILURE, HERE, 
                                     "[SYS/TEMP] Unable to access newly-created "
                                     "dir %s: %s (%d)",
                                     buffer, STRERROR(errno), errno );
                DEBUG_LOG4 ( p, "Created %s; mode=%04o; owner=%d; group=%d",
                             buffer, (unsigned int) mybuf.st_mode,
                             (int) mybuf.st_uid, (int) mybuf.st_gid );
            } /* DEBUGGING */

        } /* directory doesn't exit */
        umask ( my_umask );
        DEBUG_LOG1 ( p, "umask now %04o", (unsigned int) my_umask );
    } /* p->hash_spool > 0 && bCheck_hash_dir */


    /*
     * Here we work to make sure the user doesn't cause us to remove or
     * write over existing files by limiting how much work we do while
     * running as root.
     */

    p->orig_group = pwp->pw_gid; /* save original group */

#ifdef BINMAIL_IS_SETGID
#  if BINMAIL_IS_SETGID > 1
   pwp->pw_gid = (gid_t)BINMAIL_IS_SETGID;
#  else
   if ( !stat(p->pCfg_spool_dir, &mybuf) )
       pwp->pw_gid = mybuf.st_gid;
#  endif /* BINMAIL_IS_SETGID > 1 */
#endif /* BINMAIL_IS_SETGID */

    /* 
     * Now we run as the user. 
     */
#if defined(__bsdi__) && _BSDI_VERSION >= 199608
    (void) setsid();
    (void) setusercontext ( p->class, pwp, pwp->pw_uid,
                            LOGIN_SETALL & ~(LOGIN_SETPATH|LOGIN_SETUMASK ) );
#else /* everything except BSDI after 1996 */
    (void) setgid ( (GID_T) pwp->pw_gid );
    (void) setgroups ( 1, (GID_T *) &pwp->pw_gid ); /* Set the supplementary groups list */
    (void) setuid ( (UID_T) pwp->pw_uid );
#endif /* __bsdi__) && _BSDI_VERSION >= 199608 */

    DEBUG_LOG4 ( p, "uid = %lu, gid = %lu, euid = %lu, egid = %lu", 
                 (long unsigned) getuid(), 
                 (long unsigned) getgid(),
                 (long unsigned) geteuid(),
                 (long unsigned) getegid() );

    dfd = open ( p->temp_drop, O_RDWR | O_CREAT, 0660 );
    if ( dfd == -1 ) {
        pop_log ( p, POP_PRIORITY, HERE,
                  "[SYS/TEMP] Unable to open temporary maildrop "
                  "'%s': %s (%d)",
                  p->temp_drop, STRERROR(errno), errno );
        return pop_msg ( p, POP_FAILURE, HERE,
                         "[SYS/TEMP] Failed to create %s with uid %lu, "
                         "gid %lu.  Change permissions.",
                          p->temp_drop,
                          (long unsigned) pwp->pw_uid, 
                          (long unsigned) pwp->pw_gid );
    }

    DEBUG_LOG2 ( p, "Opened temp drop %s (%d)", p->temp_drop, dfd );

    fstat ( dfd, &mybuf );
    if ( mybuf.st_uid != pwp->pw_uid ) {
        close  ( dfd );
        return ( pop_msg ( p, POP_FAILURE, HERE, 
                           "[SYS/PERM] Temporary drop %s not owned by %s.",
                           p->temp_drop, p->user) );
    }
#ifdef NEXT
    if ( mybuf.st_mode & (0x7) )
#else /* not NEXT */
    if ( mybuf.st_mode & (S_IWOTH | S_IROTH) )
#endif /* NEXT */
    {
        close  ( dfd );
        return ( pop_msg ( p, POP_FAILURE, HERE,
                           "[SYS/PERM] Your temporary file %s is accessible "
                           "by others.  This must be fixed",
                           p->temp_drop ) );
    }
    /* 
     * Make sure the mailspool is not a hard link 
     */
    if ( mybuf.st_nlink != 1 ) {
        close  ( dfd );
        return ( pop_msg ( p, POP_FAILURE, HERE,
                           "[SYS/PERM] Your temporary file appears to have "
                           "more than one link." ) );
    }

    /* 
     * If the temporary popdrop is not empty, revert to regular mode. 
     */
    if ( mybuf.st_size != 0 )
        p->server_mode = 0;

    /* 
     * Lock the temporary maildrop.  Locking is just to check for single
     * pop session per user.
     */
    if ( flock ( dfd, LOCK_EX | LOCK_NB ) == -1 ) {
        switch ( errno ) {
            case EWOULDBLOCK:
                close ( dfd );
                return pop_msg ( p, POP_FAILURE, HERE,
                                "[IN-USE] %s lock busy!  Is another session "
                                "active? (%d)",
                                p->temp_drop, errno );
            default:
                close ( dfd );
                return pop_msg ( p, POP_FAILURE, HERE, 
                                "[SYS/TEMP] flock: '%s': %s (%d)",
                                p->temp_drop, STRERROR(errno), errno );
        }
    }
    
    if ( p->bKeep_temp_drop == FALSE ) {
        /* check for race conditions involving unlink.  See pop_updt.c */
        /* s-dorner@uiuc.edu, 12/91 */
        struct stat byName, byFd;

        if ( stat ( p->temp_drop, &byName ) || fstat ( dfd, &byFd )
                                            || byName.st_ino != mybuf.st_ino )
        {
            flock ( dfd, LOCK_UN );
            close ( dfd );
            return pop_msg ( p, POP_FAILURE, HERE,
                             "[IN-USE] Maildrop being unlocked; try again.");
        }
    } /* bKeep_temp_drop == FALSE */

    /*  
     * Acquire a stream pointer for the temporary maildrop 
     */
    p->drop = fdopen ( dfd, "r+" );
    if ( p->drop == NULL ) {
        flock ( dfd, LOCK_UN );
        close ( dfd );
        return pop_msg ( p, POP_FAILURE, HERE, 
                         "[SYS/TEMP] Cannot assign stream for %s: %s (%d)",
                         p->temp_drop, STRERROR(errno), errno );
    }

    DEBUG_LOG1 ( p, "Set p->drop to stream for %d", dfd );

    /*
     * Allocate memory for message information structures and this is
     * not deleted since a failure, for some reason in this function
     * would result in process death. 
     */
    p->mlp = (MsgInfoList *) calloc ( (unsigned) ALLOC_MSGS, sizeof(MsgInfoList) );
    if ( p->mlp == NULL ){
        flock ( dfd, LOCK_UN );
        close ( dfd );
        return pop_msg ( p, POP_FAILURE, HERE,
                         "[SYS/TEMP] Can't allocate memory for message list." );
    }

    p->msg_count = 0;
    p->visible_msg_count = 0;
    p->drop_size = 0;

    if ( mybuf.st_size != 0 ) { /* Mostly this is for regular mode. */
        DEBUG_LOG2 ( p, "Temp drop %s not empty (%u octets)", 
                     p->temp_drop, (unsigned) mybuf.st_size );
        if ( init_dropinfo ( p, p->temp_drop, p->drop, time(0) ) != POP_SUCCESS ) {
            /* Occurs on temp_drop corruption */
            flock ( dfd, LOCK_UN );
            close ( dfd );
            return ( POP_FAILURE );
        }
    /* 
     * Sync with stdio; should be at end anyway 
     */
    fseek ( p->drop, 0L, SEEK_END );
    offset = ftell ( p->drop );
    DEBUG_LOG1 ( p, "Temp drop has %u octets of old data", (unsigned) offset );
    } /* mybuf.st_size != 0 */

    /* 
     * Always use Qmaillock().
     */

    DEBUG_LOG0 ( p, "Getting mail lock" );
    rslt = Qmaillock ( p->drop_name,
                      2,
                      p->bNo_atomic_open,
                      p->pCfg_spool_dir,
                      p->trace,
                      HERE,
                      p->debug );
    if ( rslt != 0 ) {
        flock ( dfd, LOCK_UN );
        close ( dfd );
        return pop_msg ( p, POP_FAILURE, HERE, 
                         "[SYS/TEMP] maillock error '%s' (%d) on '%s': %s (%d)", 
                         Qlockerr(rslt), rslt, p->drop_name, 
                         strerror(errno), errno );
    }
    time_locked = time(0);

    /*  Open the user's maildrop; if this fails,  no harm in assuming empty */
    /* <todo> Mail drop has to be created if one doesn't exist for server_mode.
     * Because p->drop is used for bulletins. Right now it reverts back to 
     * regular mode if drop box open fails. </todo>
     */
    mfd = open ( p->drop_name, O_RDWR );
    if ( mfd > 0 ) {
        /* 
         * Lock the maildrop 
         */
        if ( flock ( mfd, LOCK_EX ) == -1 ) {
            flock ( dfd, LOCK_UN );
            close ( dfd );
            close ( mfd );
            Qmailunlock ( HERE );
            return pop_msg ( p, POP_FAILURE, HERE, 
                             "[SYS/TEMP] flock: '%s': %s (%d)", 
                             p->drop_name, STRERROR(errno), errno );
        }

        DEBUG_LOG2 ( p, "Opened spool %s (%d)", p->drop_name, mfd );

        if ( !p->server_mode ) {
            /*
             * New routine to copy and get dropbox info all at the same time 
             */
            nchar = do_drop_copy ( p, mfd, time_locked );

            if ( nchar != POP_SUCCESS ) {
                /* 
                 * pop_dropcopy has integrated the info gathering pass into
                 * the copy pass so now the information of the dropfile is
                 * inconsistent if there is an error.  Now we exit instead
                 * of trying to continue with what is available.
                 */
                DEBUG_LOG1 ( p, "do_drop_copy returned error; truncating "
                                "temp drop to %u octets", (unsigned) offset );
                ftruncate ( dfd, offset );
                goto bailout;
            } 
            else { /* do_drop_copy worked */
                /* 
                 * Mail transferred!  Zero the mail drop NOW,  that we
                 * do not have to do gymnastics to figure out what's new
                 * and what is old later
                 */
                ftruncate ( mfd, 0 );
                DEBUG_LOG0 ( p, "Mail copied to temp drop; zeroing spool" );
            }

            /* 
             * Unlock and close the actual mail drop 
             */
            flock ( mfd, LOCK_UN );
            close ( mfd );
        } /* not p->server_mode */
        else { /* SERVER_MODE */
            /* 
             * Save the temporary drop FILE and fid values
             */
            p->hold = p->drop;
            p->drop = fdopen ( mfd, "r+" );
            if ( p->drop == NULL ) {
                pop_msg ( p, POP_FAILURE, HERE,
                          "[SYS/TEMP] Cannot assign stream for %s: %s (%d)",
                          p->drop_name, STRERROR(errno), errno );
                goto bailout;
            }

            DEBUG_LOG2 ( p, "Server mode: set p->hold to temp drop (%d) "
                            "and p->drop to stream for spool (%d)",
                         fileno(p->hold), mfd );

            /*
             * If we have a usable cache file, that saves us from having to
             * process the spool.
             */
            rslt = cache_read ( p, fileno(p->drop), &p->mlp );
            if ( rslt == POP_FAILURE ) {
                rslt = init_dropinfo ( p, p->drop_name, p->drop, time_locked );
                if ( rslt != POP_SUCCESS )
                    goto bailout;
            /*
             * If the cache file is smaller than the spool, we need to
             * process the new messages.
             */
            }
        }
    } /* opened maildrop */
    else {
        /* 
         * Revert to regular operation when there are no mails in
         * users mail box. This is for the sake of any bulletins, 
         * which uses p->drop (temp_drop) and to allow pop_updt to
         * copy back to original mail box.
         */
        p->server_mode = 0;
        DEBUG_LOG3 ( p, "Unable to open maildrop %s: %s (%d)", 
                     p->drop_name, (errno < sys_nerr) ? sys_errlist[errno] : "", errno );
        cache_unlink ( p );
    }

    if ( p->bulldir    != NULL
#ifdef BULLDB
         && p->bull_db != NULL
#endif /* BULLDB */
       ) {
        /* 
         * Recalculate offset 
         */
        fseek ( p->drop, 0L, SEEK_END );
        offset = ftell ( p->drop );
        DEBUG_LOG1 ( p, "Recalculated offset before bulletin processing: %u",
                     (unsigned) offset );
        /* 
         * In Server_mode, p->drop can be null (no mails); 
         * pop_bull requires a drop handle. </bug> 
         */
        if ( pop_bull ( p, pwp ) != POP_SUCCESS ) {
            /* 
             * Return pop drop back to what it was before the bulletins 
             */
            ftruncate ( p->server_mode ? ( (mfd == -1) ? dfd : mfd ) 
                                       : dfd, 
                        offset );
            DEBUG_LOG3 ( p, "pop_bull reported error; truncating; "
                            "server_mode=%i; offset=%u; fd=%s",
                         p->server_mode, (unsigned) offset, 
                         p->server_mode ? ( (mfd == -1) ? "dfd" : "mfd" ) 
                                        : "dfd" );
        }
    }
#ifdef BULLDB
    if ( p->bull_db != NULL ) {

#  ifdef GDBM
        gdbm_close ( p->bull_db );
#  else /* not GDBM */
        dbm_close ( p->bull_db );
#  endif /* GDBM */

    DEBUG_LOG0 ( p, "Closed Bulletin database" );
    }
#endif /* BULLDB */

    fseek ( p->drop, 0L, SEEK_END );
    p->spool_end = ftell ( p->drop );
    DEBUG_LOG3 ( p, "Temp drop contains %u (%u visible) messages in %lu octets", 
                 p->msg_count, p->visible_msg_count, p->spool_end );

    if ( DEBUGGING && p->debug && p->msg_count > 0 ) {
        register int    i;
        MsgInfoList    *mp; 
                           
        for ( i = 0, mp = p->mlp; i < p->msg_count; i++, mp++ )
            pop_log ( p, POP_DEBUG, HERE,
                      "Msg %d (%d) uidl '%s' at offset %ld is %ld octets "
                      "long%.*s and has %d lines.",
                      mp->number, mp->visible_num, 
                      mp->uidl_str, mp->offset, mp->length,
                      (mp->hide_flag ? 9 : 0), " (hidden)",
                      mp->lines );
    }

    Qmailunlock ( HERE );

    if ( p->server_mode )
        flock ( mfd, LOCK_UN );

    if ( p->bDo_timing )
        p->init_time = time(0) - my_timer;

    return ( POP_SUCCESS );

bailout:
    DEBUG_LOG0 ( p, "--- bailing out ---" );
    Qmailunlock ( HERE );

    if ( p->bKeep_temp_drop == FALSE ) {
        if ( fstat ( dfd, &mybuf ) == 0 && mybuf.st_size == 0 ) {
            unlink ( p->temp_drop ); /* didn't get to use it */
            DEBUG_LOG1 ( p, "Removed empty temp drop: %s", 
                         p->temp_drop );
        }
    } /* bKeep_temp_drop == FALSE */

    flock ( mfd, LOCK_UN );
    flock ( dfd, LOCK_UN );
    close ( mfd );
    close ( dfd );

#ifdef BULLDB
    if ( p->bull_db != NULL ) {
#  ifdef GDBM
        gdbm_close ( p->bull_db );
#  else /* not GDBM */
        dbm_close  ( p->bull_db );
#  endif /* GDBM */
    }
#endif /* BULLDB */
    return ( POP_FAILURE );
}

/*
 * This function emulates fgets() except that it replaces the NULLs with
 * SPACEs; includes the '\n', and checks for overflows.
 */

static char *
mfgets ( char *s, int size, FILE *stream )
{
    int c;
    char *p = s;
#ifdef _DEBUG
    extern POP *global_debug_p;
#endif

    if ( size <= 0 ) {
        return NULL;
    }
    isOflow = 1; /* assume the worst */
    while ( --size && ((c = getc(stream)) != EOF) ) {
        if ( (*p = (char)c) == '\0' ) 
            *p = ' ';
        if ( *p++ == '\n' ) {
            isOflow = 0; /* we didn't overflow after all */
            break;
        }
    }
    if ( p == s ) {
        isOflow = 0;
        return NULL;
    }
    *p = '\0';
#ifdef _DEBUG
    DEBUG_LOG4 ( global_debug_p, "#mfgets: isOflow=%d wasOflow=%d s=(%d) %.128s",
               isOflow, wasOflow, strlen(s), s );
#endif
    return s;
}

/*
 * Suspends execution for a small amount of time.
 */
void doze ( POP *p, int atmpt )
{
#define MAX_USEC        500000 /* try not to sleep more than about .5 seconds */

#ifdef HAVE_USLEEP
    static BOOL inited = FALSE;
    int         r;
    long        t;

    /*
     * We use rand() because it's reportedly faster, and we're not as
     * concerned with randomness here.  (We could use lower bits of
     * tv_usec from gettimeofday here and it would probably be better.)
     */
    if ( inited == FALSE ) {
        inited = TRUE;
        srand ( time ( 0 ) );
    }
    
    r = rand();
    t = ( r % MAX_USEC ) + atmpt;
    DEBUG_LOG1 ( p, "...dozing for %ld milliseconds", (t + 500) / 1000 );
    usleep ( t );

#else /* don't have USLEEP */
    int t = ( atmpt == 0 ? 2 : atmpt + 1 ) / 2;

    DEBUG_LOG1 ( p, "...dozing for %d seconds", t );
    sleep ( t );

#endif /* HAVE_USLEEP */
}

