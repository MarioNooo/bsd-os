/*
 * Copyright (c) 2003 QUALCOMM Incorporated. All rights reserved.
 * See License.txt file for terms and conditions for modification and
 * redistribution.
 *
 * Revisions:
 *
 * 01/31/01  [rcg]
 *         -  No longer hiding messages deleted in prior aborted session
 *           when '--disable-update-abort' set.
 *
 * 01/30/01  [rcg]
 *         - Qmaillock now takes bNo_atomic_open and pSpool_dir parameters
 *           instead of using compile-time constants.
 *
 * 01/15/01  [rg]
 *         - Now using run-time options bKeep_temp_drop, bDo_timing,
 *           bUpdate_on_abort, bAuto_delete, bUpdate_status_hdrs.
 *
 * 10/02/00  [rg]
 *         - Using rename(2) instead of copying if p->fast_update set.
 *
 * 09/21/00  [rg]
 *         - Added calls on cache_unlink and cache_write.
 *         - Updating mlp as messages updated (for cache).
 *         - Updating p->spool_end and p->drop_size (for cache).
 *
 * 08/16/00  [rg]
 *         - Added error string to standard_error.
 *
 * 07/14/00  [rg]
 *         - Errors during spool copies/appends now always abort.
 *
 * 07/05/00  [rg]
 *         - Be more patient when locking spool.
 *
 * 06/10/00  [rg]
 *         - Fixed bug causing spool duplication in server mode when
 *           KEEP_TEMP_DROP defined.
 *         - Added more trace code to update path.
 *         - Removed -Z run-time switch.
 *         - Changed BBSIZE to BUFSIZE because BBSIZE is defined on IRIX.
 *         - AUTO_DELETE now checks if msg already deleted and updates
 *           bytes_deleted; also skips loop if msgs_deleted >= msg_count.
 *         - Deleted unused variables 'uidl_buf' & 'result'.
 *
 * 05/16/00  [rg]
 *         - We now check if we need to refresh the mail lock every
 *           p->check_lock_refresh messages.
 *         - When '-Z' run-time switch used, we revert to old (pre-3.x)
 *           locking.  This is dangerous and will likely be removed prior
 *           to release.
 * 04/21/00  [rg]
 *         - Always use our maillock(), even if system supplies one.
 *         - Rename our maillock() to Qmaillock().
 *
 * 03/09/00  [rg]
 *         - Fix incorrect use of save_errno / save_error.
 *
 * 02/16/00  [rg]
 *         - Now calling touchlock periodically.
 *
 * 12/06/99  [rg]
 *         - Added #include "misc.h" to make sure odds and ends are properly
 *           defined.
 *         - Fixed typo (SYS_MAILOCK instead of SYS_MAILLOCK).
 *
 * 11/22/99  [rg]
 *         - fixed HAVE_MAILLOCK to be HAVE_MAILLOCK_H
 *         - Changed MAILLOCK to be SYS_MAILLOCK
 *         - Removed mailunlock macro; added #include for our own maillock.h,
 *         - Made use of maillock() unconditional.
 *
 */


/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */


#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#if HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include <flock.h>

#if HAVE_STRINGS_H
#  include <strings.h>
#endif

#include <sys/stat.h>
#include <sys/file.h>

#include <time.h>

#include "popper.h"
#include "maillock.h"

#if !defined(L_XTND)
#  define L_XTND SEEK_END
#endif

#ifndef L_SET
#  define L_SET SEEK_SET
#endif

#if defined(HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include "misc.h"

extern int      errno;

#define         BUFSIZE          4096

static char standard_error[] =
    "Error updating primary drop.  Mail left in temporary maildrop [%s (%d)]";


void
unlink_temp_drop ( POP *p, char *filename, const char *fn, size_t ln )
{
    if ( p->bKeep_temp_drop ) {
        DEBUG_LOG3 ( p, "bKeep_temp_drop set [%s:%lu]; temp drop (%s) "
                        "not unlinked",
                     fn, (unsigned long) ln, filename );
    }
    else {
        /*
         * Added code in pop_dropcopy.c makes unlink ok now.
         * s-dorner@uiuc.edu, 12/91 
         */
        unlink ( filename );
        DEBUG_LOG3 ( p, "Unlinked [%s:%lu] temp drop (%s)", 
                     fn, (unsigned long) ln, filename );
    }
}


void
truncate_file ( POP *p, int fd, const char *id, const char *fn, size_t ln )
{
    int rslt = 0;


    rslt = ftruncate ( fd, (OFF_T) 0 );
    if ( rslt == 0 ) {
        DEBUG_LOG4 ( p, "Truncated [%s:%lu] %s (%d)", 
                     fn, (unsigned long) ln, id, fd );
    }
    else {
        int e = errno;

        pop_log ( p, POP_PRIORITY, HERE,
                  "Unable to truncate [%s:%lu] %s: %s (%d) ",
                  fn, (long unsigned) ln,
                  id, strerror(e), e );
    }
}




/*---------------------------------------------------------------------
  Flow:
  =====

Normal mode:
    1.  If all msgs deleted, truncate, unlink, & close temp drop, and exit.
    2.  Copy any newly-arrived msgs from spool to end of temp drop.
    3.  truncate spool (at offset 0).
    4.  Copy (& update state/uidl) non-deleted msgs from temp drop to spool.
    5.  Copy new msgs (copied to temp drop in 2) from temp drop to spool.
    6.  Truncate temp drop (at offset 0).
    7.  Unlink temp drop.
    8.  Exit.


Server mode:
    1.  If spool not dirty, unlink temp drop and exit.
    2.  If all msgs deleted and no new mail arrived, truncate spool & unlink
        temp drop, and exit.
    3.  Switch md and mfd from spool to temp drop.
    4.  Copy (& update state/uidl) non-deleted msgs from spool to temp drop.
    5.  Copy any newly-arrived msgs from spool to end of temp drop.
    6.  Truncate spool (at offset 0).
    7.  If p->fast_update, try to rename temp drop to spool.
    8.  If p->fast_update not set, or rename failed, copy temp drop to spool.
    9.  Truncate temp drop (at offset 0).
   10.  Unlink temp drop.
   11.  Exit.


Restore (non-update):
    1.  If server mode, unlink temp drop and exit.
    2.  Copy any newly-arrived msgs from spool to end of temp drop.
    3.  Truncate (at offset 0).
    4.  Copy temp drop to spool.
    5.  Truncate temp drop (at offset 0).
    6.  Unlink temp drop.
    7.  Exit.


File descriptors:
-----------------
md:     Normal mode:    File descriptor for spool.
        Server mode:    Changes to file descriptor for temp drop.
mdf:    Normal mode:    FILE * (stream) for spool.
        Server mode:    Changes to FILE * for temp drop.


  ---------------------------------------------------------------------*/




/* 
 *  updt:   Apply changes to a user's POP maildrop
 */

int pop_updt (p)
POP     *   p;
{
    FILE                *   md = NULL;              /*  Stream pointer for 
                                                        the user's maildrop */
    int                     mfd = -1;               /*  File descriptor for
                                                        above */
    char                    buffer[BUFSIZE];        /*  Read buffer */

    MsgInfoList         *   mp = NULL;              /*  Pointer to message 
                                                        info list */
    register int            msg_num;                /*  Current message 
                                                        counter */
    register int            status_written;         /*  Status header field 
                                                        written */
    int                     nchar  = 0;             /*  Bytes read/written */

    long                    offset = 0;             /*  New mail offset */

    int                     save_errno = 0;         /*  Save the error value we
                                                        are trying to print. */
    int                     rslt = 0;               /*  temp use when calling */
    struct stat             mybuf;                  /*  For fstat() */
    time_t                  time_locked = 0;
    int                     retrycnt    = 0;        /*  For patient locking */
    long                    new_end     = 0;        /*  New end of spool */
    time_t                  my_timer    = time(0);  /*  For timing */


    if ( p->bAuto_delete && p->bUpdate_on_abort == FALSE )
    {
        /* 
           bAuto_delete causes messages which have been RETRd by the client to be
           unconditionally and automatically marked deleted.  This prevents clients
           from leaving mail on the server.  

           CAUTION: Be sure users are informed of this before enabling it, to 
           prevent lost mail.

           Note: bUpdate_on_abort must be FALSE; otherwise users may lose mail
           if they try and RETR it but the connection closes for any reason, the 
           client aborts, etc.
        */
        if ( p->msgs_deleted < p->msg_count )
        { /* there are msgs available for deletion */
            DEBUG_LOG0 ( p, "Auto-deleting messages which have been RETRd..." );
            for ( msg_num = 0; msg_num < p->msg_count; ++msg_num )
            { /* msg loop */
                mp = &p->mlp [ msg_num ];
                if ( mp->retr_flag && !mp->del_flag )
                {
                    delete_msg ( p, mp );
                    DEBUG_LOG1 ( p, ">>> msg %i automatically flagged as DELETED <<<", 
                                 msg_num );
                }
        
            } /* msg loop */
        } /* there are msgs available for deletion */
    } /* bAuto_delete && bUpdate_on_abort == FALSE */

    DEBUG_LOG0 ( p, "Performing maildrop update..." );
    DEBUG_LOG0 ( p, "Checking to see if all messages were deleted" );

    if ( p->bStats ) {
        pop_log ( p, POP_PRIORITY, HERE, "Stats: %s %d %ld %d %ld %s %s",
                  p->user, p->msgs_deleted, p->bytes_deleted,
                  p->msg_count - p->msgs_deleted,
                  p->drop_size - p->bytes_deleted,
                  p->client, p->ipaddr );
    }

    if ( p->server_mode && p->dirty == FALSE ) {
        /*
         * Write out cache file
         */
        rslt = cache_write ( p, fileno(p->drop), p->mlp );
        if ( rslt == POP_FAILURE )
            cache_unlink ( p );

        unlink_temp_drop ( p, p->temp_drop, HERE );
        DEBUG_LOG0 ( p, "Running in server mode; spool not "
                        "changed; deleting temp drop" );
    if ( p->bDo_timing )
        p->clean_time = time(0) - my_timer;

        return ( POP_SUCCESS );
    } /* server mode and spool not dirty */

    /*
     * Delete the cache file, just to cover all paths.
     */
    cache_unlink ( p );

    if ( p->server_mode == FALSE && p->msgs_deleted == p->msg_count ) {
        /*
         * In non-server-mode the Qpopper process is the sole user of
         * .user.pop so truncate and unlink is not a problem.
         *
         * Truncate before close, to avoid race condition.  
         */
        truncate_file ( p, fileno ( p->drop ), "temp drop", HERE );
        DEBUG_LOG1 ( p, "Non-server mode and all msgs deleted; "
                        "truncated temp drop (%d)", fileno(p->drop) );
        unlink_temp_drop ( p,  p->temp_drop, HERE );
        fclose ( p->drop );

    if ( p->bDo_timing )
        p->clean_time = time(0) - my_timer;

        return ( POP_SUCCESS );
    } /* non-server mode and all msgs deleted */

    /* 
     * Always use Qmaillock()
     */

    rslt = Qmaillock ( p->drop_name,
                       4,
                       p->bNo_atomic_open,
                       p->pCfg_spool_dir,
                       p->trace,
                       HERE,
                       p->debug );
    if ( rslt != 0 )
        return ( pop_msg ( p, POP_FAILURE, HERE, 
                           "maillock error '%s' (%d) on '%s': %s (%d)", 
                           Qlockerr(rslt), rslt, p->temp_drop, 
                           STRERROR(errno), errno ) );
    time_locked = time(0);
    
    if ( p->server_mode == FALSE ) {
        DEBUG_LOG1 ( p, "(Normal mode) opening spool \"%s\"", 
                     p->drop_name );
        /*  
         * Open the user's real maildrop 
         */
        if ( ( mfd = open   ( p->drop_name, O_RDWR | O_CREAT, 0660 ) ) == -1 ||
             ( md  = fdopen ( mfd, "r+" ) ) == NULL ) {
            Qmailunlock ( HERE );
            return pop_msg ( p, POP_FAILURE, HERE, standard_error, 
                             STRERROR(errno), errno );
        }
        
        DEBUG_LOG2 ( p, "(normal mode) mfd=%d: %s", mfd, p->drop_name );
    } 
    else { /* server mode */
        mfd = fileno ( p->drop );
        DEBUG_LOG2 ( p, "(Server mode) accessing spool; mfd=%d: %s", 
                     mfd, p->drop_name );
    }

    /*  
     * Lock the user's real mail drop 
     */
    while ( flock ( mfd, LOCK_EX ) == -1 && retrycnt++ < 4 )
        sleep ( retrycnt * 5 );
    if ( retrycnt == 4 ) {
        fclose ( (p->server_mode) ? p->drop : md );
        Qmailunlock ( HERE );
        return pop_msg ( p, POP_FAILURE, HERE, "flock: '%s': %s (%d)", 
                         p->drop_name,
                         STRERROR(errno), errno );
    }

    if ( p->server_mode == FALSE )  {
        /* 
         * Go to the right places 
         */
        fseek ( p->drop, 0, SEEK_END ); 
        offset = ftell ( p->drop ); 
        DEBUG_LOG1 ( p, "Drop has %lu octets", offset );
        
        Qtouchlock ( HERE );

        /*  
         * Append any messages that may have arrived during the session 
         * to the temporary maildrop 
         */
        DEBUG_LOG2 ( p, "Copying new mail from spool (%d) to temp drop (%d)",
                     mfd, fileno(p->drop) );

        while ( ( nchar = read ( mfd, buffer, BUFSIZE ) ) > 0 ) {
            if ( nchar != write ( fileno ( p->drop ), buffer, nchar ) ) {
                nchar = -1; /* write failed */
                break ;
            }
        }
        if ( nchar ) {
            save_errno = errno;
            fclose ( md );
            ftruncate ( fileno ( p->drop ), (OFF_T)offset );
            fclose ( p->drop );
            Qmailunlock ( HERE );
            return pop_msg ( p, POP_FAILURE, HERE, 
                             "Error copying new messages: '%s' : %s (%d)",
                             p->drop_name, STRERROR(save_errno), save_errno );
        }

        fflush ( md );
        rewind ( md );
        truncate_file ( p, mfd, "spool", HERE );
        lseek ( mfd, (OFF_T)0, L_SET );
        DEBUG_LOG1 ( p, "Non-server mode; copied any new msgs to temp "
                        "drop; truncated spool (%d)", mfd );

        /* 
         * Synch stdio and the kernel for the POP drop 
         */
        rewind ( p->drop );
        lseek  ( fileno ( p->drop ), (OFF_T)0, L_SET );

        /*
         *  Transfer messages not flagged for deletion from the temporary 
         *  maildrop to the new maildrop 
         */
        DEBUG_LOG2 ( p, "Creating new maildrop \"%s\" from \"%s\"",
                     p->drop_name, p->temp_drop );

    } /* not server mode */
    else { /* SERVER_MODE */
        fstat ( mfd, &mybuf );
        if ( p->msgs_deleted == p->msg_count &&
             mybuf.st_size   == p->spool_end ) {
            /* 
             * Truncate before close, to avoid race condition.  
             */
            truncate_file ( p, mfd, "spool", HERE );
            DEBUG_LOG1 ( p, "Server mode; all msgs deleted and spool size "
                            "unchanged; truncated spool (%d)",
                            mfd );
            unlink_temp_drop ( p,  p->temp_drop, HERE );
            cache_unlink ( p );
            flock  ( mfd, LOCK_UN );
            fclose ( p->drop );
            fclose ( p->hold );
            Qmailunlock ( HERE );

            if ( p->bDo_timing )
                p->clean_time = time(0) - my_timer;

            return ( POP_SUCCESS );
        } /* all msgs deleted; no new ones arrived */

        md  = p->hold;   /* Really the temp drop */
        mfd = fileno ( md );
        DEBUG_LOG1 ( p, "server mode: set md/mfd to temp drop (p->hold): %d",
                     mfd );
    } /* server mode */

    if ( p->server_mode == FALSE
         || ( p->msgs_deleted != p->msg_count ) ) {
        DEBUG_LOG7 ( p, "Server mode=%i; %i out of %i msgs deleted; "
                        "copying msgs from %s (%d) to %s (%d)", 
                     p->server_mode, p->msgs_deleted, p->msg_count,
                     p->server_mode ? "spool" : "temp drop",
                     fileno(p->drop),
                     p->server_mode ? "temp drop" : "spool",
                     mfd );

        p->drop_size = 0; /* Recalculate */

        for ( msg_num = 0; msg_num < p->msg_count; ++msg_num ) {

            int  inheader   = 1;
            int  body_lines = 0;
            int  lines      = 0;
            long length     = 0;
            long offset     = 0;

            /*  
             * Get a pointer to the message information list 
             */
            mp = &p->mlp [ msg_num ];
            
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

            if ( mp->del_flag ) {
                DEBUG_LOG1 ( p, "Message %d flagged for deletion.", mp->number );
                continue;
            }

            fseek ( p->drop, mp->offset, SEEK_SET );
            
            offset = ftell ( md );

            /* 
             * Put the From line separator 
             * (Does not count in line count or length)
             */
            fgets ( buffer, MAXMSGLINELEN, p->drop );
            if ( fputs ( buffer, md ) == EOF )
                break;

            sprintf ( buffer, "X-UIDL: %s", mp->uidl_str );
            if ( fputs ( buffer, md ) == EOF )
                break;
            length += strlen ( buffer ) + 1; /* for CRLF */
            lines  ++;

            for ( status_written = 0, inheader = 1;
                  fgets ( buffer, MAXMSGLINELEN, p->drop );
                ) {

                if ( inheader ) { /* Header */

                    /*  
                     * A blank line signals the end of the header. 
                     */
                    if ( *buffer == '\n' ) {
                        if ( p->bUpdate_status_hdrs 
                             && status_written == FALSE ) {
                            if ( mp->retr_flag ) {
                                sprintf ( buffer, "Status: RO\n\n" );
                            } else {
                                sprintf ( buffer, "Status: U\n\n" );
                            }
                        } /* bUpdate_status_hdrs && status_written == FALSE */

                        inheader       = 0;
                        body_lines     = 1;
                        status_written = 0;

                    } else if ( !strncasecmp ( buffer, "X-UIDL:", 7 ) ) {
                        continue;       /* Skip all existing UIDL lines */

                    /*
                     * Content-Length header counts as a line, but not 
                     * as length.
                     */
                    } else if ( CONTENT_LENGTH && 
                                !strncmp ( buffer, "Content-Length:", 15 ) ) {
                        length -= strlen ( buffer ) + 1; /* for CRLF */

                    } else if ( !strncasecmp ( buffer, "Status:", 7 ) ) {

                        /*  
                         * Update the message status 
                         */
                        if ( mp->retr_flag )
                            sprintf ( buffer, "Status: RO\n" );
                        status_written++;
                    }
                    /*  
                     * Save another header line 
                     */
                    if ( fputs ( buffer, md ) == EOF )
                        break;
                    length += strlen ( buffer ) + 1; /* for CRLF */
                    lines  ++;

                    } else { /* Body */ 
                        if ( ++body_lines > mp->body_lines ) {
                            body_lines--;
                            break;
                        }
                        if ( fputs ( buffer, md ) == EOF )
                            break;
                        length += strlen ( buffer ) + 1; /* for CRLF */
                        lines  ++;
                    }
            }

            if ( ferror ( md ) ) {
                break;
            }

            if ( p->mmdf_separator ) {
                fputs ( p->mmdf_separator, md );
            }
            
            DEBUG_LOG9 ( p, "Copied message %d: offset %ld (was %ld); "
                            "length %ld (was %ld); lines %d (was %d); "
                            "body_lines %d (was %d)",
                         mp->number, offset, mp->offset, length, mp->length,
                         lines,  mp->lines,  body_lines, mp->body_lines );

            mp->offset      = offset;
            mp->length      = length;
            mp->lines       = lines;
            mp->body_lines  = mp->body_lines;
            p->drop_size   += length;
        } /* msg loop */

    /* flush and check for errors now!  The new mail will be writen
     * without stdio,  since we need not separate messages 
     */

        if ( ferror ( md ) ) {
            save_errno = errno;

            truncate_file ( p, mfd, "spool", HERE );
            fclose ( md );
            Qmailunlock ( HERE );
            fclose ( p->drop );
            if ( save_errno == EDQUOT )
                return pop_msg ( p, POP_FAILURE, HERE,
                                 "Overquota copying messages to spool."
                                 "  Temp drop unchanged (%d)",
                                 save_errno );
            else
                return pop_msg ( p, POP_FAILURE, HERE, 
                                 standard_error, STRERROR(save_errno),
                                 save_errno );
        }

        fflush ( md );
        if ( ferror ( md ) ) {
            int save_error = errno;

            truncate_file ( p, mfd, "spool", HERE );
            close ( mfd );
            Qmailunlock ( HERE );
            fclose ( p->drop );

            if ( save_error == EDQUOT )
                return pop_msg ( p, POP_FAILURE, HERE,
                                 "Overquota copying messages to Mailspool. "
                                 "Temp drop unchanged (%d)",
                                 save_error );
            else
                return pop_msg ( p, POP_FAILURE, HERE, 
                                 standard_error, STRERROR ( save_error ),
                                 save_error );
        }
        
        /*
         * Update the spool EOF (we may have increased it by writing X-UIDL
         * and/or Status headers) and we want it to be correct in the cache
         * file.
         */
        new_end = ftell ( md );

    } /* p->msgs_deleted != p->msg_count */

    if ( p->server_mode ) {
        DEBUG_LOG2 ( p, "(server mode; p->drop (%d) is really spool; "
                        "mfd (%d) is temp drop)",
                     fileno(p->drop), mfd );

        if ( mybuf.st_size > p->spool_end ) {
            /* 
             * Go to the right places 
             */
            lseek ( fileno ( p->drop ), (OFF_T) p->spool_end, L_SET ); 
            
            DEBUG_LOG1 ( p, "%lu octets were added to spool during session",
                         (long unsigned) mybuf.st_size - p->spool_end );

            Qtouchlock ( HERE );
            
            /*  
             * Append any messages that may have arrived during the session 
             * to the temporary maildrop 
             */
            while ( ( nchar = read ( fileno ( p->drop ), buffer, BUFSIZE ) ) > 0 )
                if ( nchar != write ( mfd, buffer, nchar ) ) {
                    nchar = -1;
                    break ;
                }
                
            DEBUG_LOG3 ( p, "%scopying new msgs from spool (%d) to "
                            "temp drop (%d)", 
                            nchar == 0 ? "" : "ERROR ",
                            fileno(p->drop), mfd );

            if ( nchar != 0 ) {
                truncate_file ( p, mfd, "temp drop", HERE );
                close ( mfd );
                Qmailunlock ( HERE );

                if ( errno == EDQUOT ) {
                    return pop_msg ( p, POP_FAILURE, HERE,
                                     "Overquota appending messages from "
                                     "mailspool to temporary drop (%d)",
                                     errno );
                } else
                    return pop_msg ( p, POP_FAILURE, HERE,
                                     "Error appending messages from mailspool "
                                     "to temporary drop: %s (%d)",
                                      STRERROR(errno), errno );
            }
        } /* mybuf.st_size > p->spool_end */

        rewind ( p->drop );
        truncate_file ( p, fileno(p->drop ), "spool", HERE );
        lseek ( fileno ( p->drop ), (OFF_T)0, L_SET );

        lseek ( mfd, (OFF_T)0, L_SET );

        Qtouchlock ( HERE );

        /*
         * Now the spool is empty and the temp drop contains the updated
         * complete spool.  If p->fast_update is set, try using rename(2)
         * to move the temp drop to the spool.  If this fails (because 
         * they are on different volumes, for example), or if the option
         * isn't set, copy the temp drop to the spool, then truncate and
         * unlink the temp spool.
         */
        if ( p->bFast_update ) {
            nchar = rename ( p->temp_drop, p->drop_name );
            if ( nchar == 0 ) {
                DEBUG_LOG2 ( p, "Moved %s to %s",
                             p->temp_drop, p->drop_name );
                close  ( mfd );
                fclose ( md );
                mfd = open ( p->temp_drop, O_RDWR | O_CREAT, 0600 );
                if ( mfd >= 0 ) {
                    DEBUG_LOG1 ( p, "Reopened mfd as temp drop (%d)", mfd );
                    md = fdopen ( mfd, "r+" );
                    if ( md == NULL )
                        pop_log ( p, POP_NOTICE, HERE,
                                  "Unable to reopen md from mfd (%d): %s (%d)",
                                  mfd, STRERROR(errno), errno );
                }
                else
                    pop_log ( p, POP_NOTICE, HERE,
                              "Unable to open temporary maildrop "
                              "'%s': %s (%d)",
                              p->temp_drop, STRERROR(errno), errno );
            }
            else {
                pop_log ( p, POP_NOTICE, HERE,
                          "Unable to move %s to %s: %s (%d)",
                          p->temp_drop, p->drop_name,
                          STRERROR(errno), errno );
            }
        } /* p->fast_update */
        else
            DEBUG_LOG0 ( p, "(p->fast_update not set)" );

        if ( p->bFast_update == FALSE || nchar == -1 ) {
            while ( ( nchar = read ( mfd, buffer, BUFSIZE ) ) > 0 )
                if ( nchar != write ( fileno ( p->drop ), buffer, nchar ) ) {
                    nchar = -1;
                    break ;
                }

            DEBUG_LOG3 ( p, "%scopying temp drop (%d) to spool (%d)",
                         nchar == 0 ? "" : "ERROR ",
                         mfd, fileno(p->drop) );

            if ( nchar != 0 ) {
                truncate_file ( p, fileno(p->drop), "spool", HERE );
                fclose ( p->drop );
                Qmailunlock ( HERE );
                fclose ( md );
                if ( errno == EDQUOT ) {
                    return pop_msg ( p, POP_FAILURE, HERE,
                                     "Overquota copying messages back to "
                                     "mailspool (%d)",
                                     errno );
                } else
                    return pop_msg ( p, POP_FAILURE, HERE,
                                     "Error copying messages from temp "
                                     "drop back to mailspool: %s (%d)",
                                     STRERROR(errno), errno );
            } /* failed to write */
        } /* p->fast_update == FALSE || nchar == -1 */

        /*
         * Update the spool end to messages we wrote (not counting any new
         * ones that arrived during the session) for the cache file.
         */
        DEBUG_LOG2 ( p, "Updated p->spool_end from %ld to %ld (for cache)",
                     p->spool_end, new_end );
        p->spool_end = new_end;

        /*
         * Write out cache file
         */
        rslt = cache_write ( p, fileno(p->drop), p->mlp );
        if ( rslt == POP_FAILURE )
            cache_unlink ( p );

        truncate_file ( p, mfd, "temp drop", HERE );
        unlink_temp_drop ( p, p->temp_drop, HERE );
        fclose ( md );
        fclose ( p->drop );
        Qmailunlock ( HERE );
    } /* server mode */
    else {
        DEBUG_LOG2 ( p, "non-server mode; copying any new msgs back from"
                        "temp drop (%d) to spool (%d)",
                     fileno(p->drop), mfd );
        /* 
         * Go to start of new mail if any 
         */
        lseek ( fileno(p->drop), (OFF_T)offset, L_SET );

        Qtouchlock ( HERE );
        
        /* 
         * Copy over any new mail that arrived while processing the pop drop 
         */
        while ( (nchar = read ( fileno(p->drop), buffer, BUFSIZE ) ) > 0 )
            if ( nchar != write ( mfd, buffer, nchar ) ) {
                nchar = -1;
                break ;
            }
        if ( nchar != 0 ) {
            int save_error = errno;

            ftruncate ( mfd, (OFF_T)0 );
            fclose ( md );
            fclose ( p->drop );
            Qmailunlock ( HERE );

            if ( save_error == EDQUOT )
                return pop_msg ( p, POP_FAILURE, HERE,
                                 "Overquota copying messages to Mailspool. "
                                 "Temp drop unchanged (%d)",
                                 save_error );
            else
                return pop_msg ( p, POP_FAILURE, HERE, 
                                 standard_error, STRERROR ( save_error ),
                                 save_error );
        }

        /*  
         * Close the maildrop and empty temporary maildrop 
         */
        fclose ( md );
        ftruncate ( fileno(p->drop), (OFF_T)0 );
        DEBUG_LOG1 ( p, "truncated temp drop (%d)", fileno(p->drop) );
        unlink_temp_drop ( p,  p->temp_drop, HERE );
        fclose ( p->drop );
        Qmailunlock ( HERE );

    } /* non-server mode */

    if ( p->bDo_timing )
        p->clean_time = time(0) - my_timer;

    return ( pop_quit ( p ) );
}



int pop_restore ( POP *p )
{
  FILE   *md            = NULL;
  int     mfd           = -1;
  char    buffer [ BUFSIZE ];
  int     nchar         = 0;
  long    offset        = 0;
  int     save_errno    = 0;
  int     rslt          = 0;
  int     retrycnt      = 0;
  time_t  my_timer      = time(0);

    DEBUG_LOG0 ( p, "Performing maildrop restoration..." );

    /*
     * Undelete any deleted messages so cache is correct.
     */
    if ( p->msgs_deleted > 0 ) {
        MsgInfoList    *mp     = NULL;
        int             msgx   = 0;

        for ( msgx = 0; msgx < p->msg_count; ++msgx ) {
                mp = &p->mlp [ msgx ];
                if ( mp->del_flag )
                    undelete_msg ( p, mp );
        } /* msg loop */
    } /* msgs_deleted > 0 */

    /*
     * In server mode we don't need to do much...
     */
    if ( p->server_mode ) {
        unlink_temp_drop ( p,  p->temp_drop, HERE );

        if ( p->bDo_timing )
            p->clean_time = time(0) - my_timer;

        /*
         * Write out cache file
         */
        rslt = cache_write ( p, fileno(p->drop), p->mlp );
        if ( rslt == POP_FAILURE )
            cache_unlink ( p );

        return ( POP_SUCCESS );
    } /* server_mode */

    DEBUG_LOG1 ( p, "Opening mail drop \"%s\"", p->drop_name );

    rslt = Qmaillock ( p->drop_name,
                       4,
                       p->bNo_atomic_open,
                       p->pCfg_spool_dir,
                       p->trace,
                       HERE,
                       p->debug );
    if ( rslt != 0 )
        return ( pop_msg ( p, POP_FAILURE, HERE, 
                           "maillock error '%s' (%d) on '%s': %s (%d)", 
                           Qlockerr(rslt), rslt, p->drop_name, 
                           STRERROR(errno), errno ) );

    if ( ( mfd = open   ( p->drop_name, O_RDWR | O_CREAT, 0660 ) ) == -1 ||
         ( md  = fdopen ( mfd, "r+" ) ) == NULL ) {
        Qmailunlock ( HERE );
        return pop_msg ( p, POP_FAILURE, HERE, standard_error, 
                         STRERROR ( errno ), errno );
    }

    while ( flock ( mfd, LOCK_EX ) == -1 && retrycnt++ < 4 )
        sleep ( retrycnt * 5 );
    if ( retrycnt == 4 ) {
        fclose ( md );
        Qmailunlock ( HERE );
        return pop_msg ( p, POP_FAILURE, HERE, "flock: '%s': %s (%d)", 
                         p->drop_name, STRERROR(errno), errno );
    }

    fseek ( p->drop, 0, SEEK_END );
    offset = ftell ( p->drop );

    /*
     * Append any messages that may have arrived during the session 
     * to the temporary mail drop 
     */
    while ( ( nchar = read ( mfd, buffer, BUFSIZE ) ) > 0 )
        if ( nchar != write ( fileno(p->drop), buffer, nchar ) ) {
            nchar = -1;
            break;
        }

    DEBUG_LOG3 ( p, "%scopying new msgs from spool (%d) to temp drop (%d)",
                 nchar == 0 ? "" : "ERROR ",
                 mfd, fileno(p->drop) );

    if ( nchar != 0) {
        save_errno = errno;
        fclose ( md );
        ftruncate ( fileno(p->drop), (OFF_T)offset );
        DEBUG_LOG2 ( p, "ERROR; truncated temp drop (%d) to %lu",
                     fileno(p->drop), offset );
        fclose ( p->drop );
        Qmailunlock ( HERE );

        if ( save_errno == EDQUOT ) {
            return pop_msg ( p, POP_FAILURE, HERE, 
                             "Overquota: appending messages "
                             "to temporary drop (%d)",
                             save_errno );
        }
        else
            return pop_msg ( p, POP_FAILURE, HERE,
                             "Error appending messages from mailspool "
                             "to temporary drop: %s (%d)",
                             STRERROR(save_errno), save_errno );
    }

    fflush ( md );
    rewind ( md );
    truncate_file ( p, mfd, "spool", HERE );
    lseek ( mfd, (OFF_T)0, L_SET );

    rewind ( p->drop );
    lseek ( fileno(p->drop), (OFF_T)0, L_SET );
    
    while ( ( nchar = read ( fileno(p->drop), buffer, BUFSIZE ) ) > 0 )
         if ( nchar != write ( mfd, buffer, nchar ) ) {
             nchar = -1;
             break;
         }

     DEBUG_LOG3 ( p, "%scopying from temp drop (%d) to spool (%d)",
              nchar == 0 ? "" : "ERROR ",
              fileno(p->drop), mfd );

    if ( nchar != 0 ) {
        save_errno = errno;
        truncate_file ( p, mfd, "spool", HERE );
        fclose ( md );
        fclose ( p->drop );
        Qmailunlock ( HERE );

        if ( save_errno == EDQUOT )
            return pop_msg ( p, POP_FAILURE, HERE,
                             "Overquota copying messages to Mailspool. "
                             "Temp drop unchanged (%d)",
                             save_errno );
        else
            return pop_msg ( p, POP_FAILURE, HERE, standard_error,
                             STRERROR ( errno ), errno );
    }

    fclose ( md );

    cache_unlink ( p );

    truncate_file ( p, fileno(p->drop), "temp drop", HERE );
    unlink_temp_drop ( p,  p->temp_drop, HERE );
    fclose ( p->drop );
    Qmailunlock ( HERE );

    if ( p->bDo_timing )
        p->clean_time = time(0) - my_timer;

    return pop_quit ( p );
}








