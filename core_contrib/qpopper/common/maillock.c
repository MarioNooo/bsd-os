/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 * 
 * Description: 
 *   maillock() and mailunlock() try to emulate the SysV functions
 *   to create the user.lock file.
 *
 *   call touchlock() to update the lock.
 *
 * Caveats :
 *   1.  mailunlock() will only remove the lockfile created from previous
 *       maillock().  
 *   2.  maillock() called for different users without intermediate calls
 *       to mailunlock() leaves the initial locks in place.
 *   3.  Process termination does not clean the .lock files.
 * 
 * Revisions:
 *
 *   04/06/01 [rcg]
 *        - Fixed case where lock is usurped in error.  Thanks to Michael
 *          Smith for finding this.
 *
 *   01/30/01 [rcg]
 *        - Qmaillock now takes bNo_atomic_open and pSpool_dir parameters
 *          instead of using compile-time constants.
 *
 *   01/19/01 [rcg]
 *        - Modified to pass drop name instead of user name (based on
 *          patch by Fergal Daly).
 *
 *   07/17/00 [rcg]
 *        - Allow lock to proceed even if user is over quota.
 *
 *   06/16/00 [rcg]
 *        - Fixed crash on NeXT.
 *        - Fixed result code, added error check.
 *
 *   06/07/00 [rcg]
 *        - Added file and line from whence called to trace records.
 *        - Added Qmailerr to return lock error string.
 *
 *   04/21/00 [rcg]
 *        - Replaced sprintf with Qsprintf.
 *        - Give up on system maillock.h or maillock -- we might be running on
 *          older OS with stuff missing.
 *        - Rename maillock() and mailunlock() to Qmaillock() and Qmailunlock().
 *
 *   03/08/00 [rcg]
 *        - Made trace calls depend on DEBUGGING as well as bDebugging
 *
 *   02/01/00 [rcg]
 *        - Delete redundant mailunlock trace call.
 *
 *   01/31/00 [rcg]
 *        - Remove .lock file if write fails.
 *
 *   01/27/00 [rcg]
 *        - Updated logit calls to pass HERE.
 *
 *   12/06/99 [rcg]
 *        - Replaced DEBUGGING with bDebugging.
 *
 *   12/02/99 [rg]
 *        - Replaced #ifdef DEBUG with if ( DEBUGGING ) for more
 *          readable code.
 *        - Reduce logging when DEBUG not set.
 *         
 *   11/28/99 [rg]
 *        - Added MAXHOSTNAMELEN define.
 *
 *   11/24/99 [rg]
 *        - Added calls to new genpath().
 *        - Added extra #includes.
 *        - Added logit calls for errors.
 *        - Changed pszLock to szLock.
 *
 *   11/22/99 [rg]
 *        - Guarded with #ifndef SYS_MAILLOCK
 *
 *   09/02/98 [py]
 *        - Created.
 */


#include "config.h"

#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <flock.h>
#include <netdb.h>
#include <utime.h>
#include <string.h>
#include <time.h>

#if HAVE_UNISTD_H
#  include <unistd.h>
#endif

#if HAVE_SYS_UNISTD_H
#  include <sys/unistd.h>
#endif

#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

#include "maillock.h"
#include "genpath.h"
#include "logit.h"
#include "snprintf.h"
#include "popper.h"
#include "string_util.h"

extern int gethostname();

/* *** MACROS *** */

/* 5 minutes */
#define LOCK_LIFE      300

#define SLEEP_INTERVAL 5

#ifndef FILENAME_MAX
#  define FILENAME_MAX  1024
#endif

#ifndef MAXPATHLEN
#  define MAXPATHLEN   FILENAME_MAX
#endif

#define BUF_SIZE       (MAXHOSTNAMELEN + 32)

/* *** PRIVATE GLOBALS *** */
static char szLock[MAXPATHLEN] = { 0 };   /* Current lock path is saved */
static void *gfTrace;                     /* For debug logging          */
static int   gbDebugging;                 /* If debug logging is wanted */

/* *** FUNCTIONS *** */


/*
 * Function:    touch_file
 *
 * Description: Updates access and modification times for a file to now.
 *
 * Parameters:  filenm:     name of file to be touched.
 *              fTrace:     file ID for trace info, or NULL to use syslog.
 *
 * Returns:     0 on success, -1 on error.
 */
int
touch_file ( char *filenm, void *fTrace )
{
    int     rslt;

#ifdef NEXT
    time_t timep[2];

    timep[0] = timep[1] = time(0);
    rslt = utime ( filenm, timep ); /* utime with null dies on NeXT */
#else /* not NEXT */
    rslt = utime ( filenm, NULL );
#endif /* NEXT */

    if ( rslt != 0 ) {
        logit ( fTrace, POP_PRIORITY, HERE,
                "utime for %s returned %s (%d)",
                filenm, strerror(errno), errno );
    }

    return rslt;
}


/*
 * Function:    maillock
 *
 * Descrption:  Creates .lock file in the spool directory for the mailbox of the
 *              user.
 *              Every maillock should be paired with mailunlock before a
 *              subsequent call to maillock() is made.
 *
 * Parameters:  drop_name:       Name of file to be locked (spool name).
 *              retrycnt:        Retry Count. Between retries there is a
 *                                  sleep of 5 seconds times the retrycount.
 *              bNo_atomic_open: TRUE if open() isn't automic.
 *              pSpool_dir:      Spool directory.
 *              fTrace:          FILE * for trace info, or NULL to use syslog.
 *              fn:              File name whence called.
 *              ln:              Line number whence called.
 *              bDebugging:      TRUE if debugging is desired.
 *
 * Returns:     L_SUCCESS on success. 
 *
 * Caveats:     Clock drift on Shared FS not accounted.
 */

int 
Qmaillock ( char         *drop_name,
            int           retrycnt,
            BOOL          bNo_atomic_open,
            char         *pSpool_dir,
            void         *fTrace,
            const char   *fn,
            size_t        ln,
            int           bDebugging )
{
    int           fd           = -1;
    struct stat   byNameStat;
    int           attemptcnt   = 0;
    time_t        curTime;
    int           nRet         = L_ERROR;
    int           nBufSize     = sizeof(szLock);
    int           nLen1        = 0;
    int           nLen2        = 0;
    char         *pTmp         = NULL;

    /*
     * Save trace file pointer and bDebugging flag for use in 
     * touchlock and mailunlock.
     */
    gfTrace     = fTrace;
    gbDebugging = bDebugging;

    /*
     * Append ".lock" to spool name.
     */
    nLen1 = strlcpy ( szLock, drop_name, nBufSize );
    nLen2 = strlcat ( szLock, ".lock",   nBufSize );

    /*
     * See if we ran out of room with the drop_name, ".lock" and a NULL.
     */
    if ( nLen1 > nBufSize || nLen2 > nBufSize )
    {
        logit ( fTrace, POP_PRIORITY, HERE,
                "insufficient room to generate lock file for %s", drop_name );
        return L_ERROR;
    }

    /*
     * Verify if lockfile is a symbolic link, or a hard link.
     *
     * Even though the link wouldn't be followed with O_CREAT | O_EXCL
     * we better do it.
     */
    if ( lstat ( szLock, &byNameStat ) == -1 ) {
        if ( errno != ENOENT ) {
            logit ( fTrace, POP_PRIORITY, HERE,
                    "error on lstat for file %s: %s (%d)", 
                    szLock, strerror(errno), errno );
            return L_ERROR;
        }
    }
    else if ( ( byNameStat.st_mode & S_IFMT ) == S_IFLNK  || 
                byNameStat.st_nlink            > 1         ){
        logit ( fTrace, POP_PRIORITY, HERE,
                "file type of %s is S_IFLNK or more than 1 link", 
                szLock );
        return L_ERROR;
    }

    while ( nRet != L_SUCCESS && attemptcnt < retrycnt ) {
        if ( attemptcnt != 0 ) 
            sleep ( attemptcnt * SLEEP_INTERVAL );  /* Sleep between retries */

        if ( bNo_atomic_open ) {
            while ( fd == -1 ) {
                /* 
                 * Create a temporary file and link it to lock file 
                 */
                pTmp = tempnam ( pSpool_dir, "POP" );
                if ( pTmp != NULL ) {
                    fd = open ( pTmp, O_CREAT | O_EXCL, 0600 );
                }
                if ( fd == -1 ) {
                    if ( DEBUGGING && bDebugging ) {
                        logit ( fTrace, POP_DEBUG, HERE,
                                "open() for temp file %s returned %s (%d)",
                                pTmp, strerror(errno), errno );
                    }
                    if ( errno == EEXIST ) {
                        if ( DEBUGGING && bDebugging ) {
                            logit ( fTrace, POP_DEBUG, HERE,
                                    "temp file %s exists", pTmp );
                        }
                        if ( pTmp != NULL )
                            free ( pTmp );
                        pTmp = NULL;
                        continue;
                        }
                    else {
                        logit ( fTrace, POP_PRIORITY, HERE,
                                "Unable to create temp file (for lock) %s: %s (%d)",
                                pTmp, strerror(errno), errno );
                        nRet = L_ERROR;
                        if ( pTmp != NULL )
                            free ( pTmp );
                        pTmp = NULL;
                        break;
                    } /* unexpected error */
                } /* open failed */
            } /* while loop */

            if ( pTmp != NULL && link ( pTmp, szLock ) == -1 ) {
                if ( errno == EEXIST ) {
                    curTime = time(0);
                    if ( stat ( szLock, &byNameStat ) != -1 && 
                         curTime - (time_t)(byNameStat.st_mtime) > LOCK_LIFE ) {
                        unlink ( szLock );                   /* Remove the old lock */
                        if ( link ( pTmp, szLock ) == -1 ) { /* Place a new lock */
                            nRet = L_TMPLOCK;
                            logit ( fTrace, POP_PRIORITY, HERE,
                                    "Unable to create link from temp file %s to "
                                    "new lock file %s: %s (%d)",
                                    pTmp, szLock, strerror(errno), errno );
                        } /* link failed */
                        attemptcnt = retrycnt;              /* Break out */
                    } /* lock present and unexpired */
                    else
                        attemptcnt++;                       /* Try again */
                } /* EEXIST error */
                else {
                    if ( DEBUGGING && bDebugging ) {
                        logit ( fTrace, POP_DEBUG, HERE,
                                "Unable to create link from temp file %s to "
                                "lock file %s: %s (%d)",
                                pTmp, szLock, strerror(errno), errno );
                    }
                    nRet =  L_ERROR;
                    attemptcnt = retrycnt;
                } /* unexpected error */
            } /* link or tempnam failed */
            else {
                if ( DEBUGGING && bDebugging ) {
                    logit ( fTrace, POP_DEBUG, HERE,
                            "successfully linked temp file %s to lock file %s",
                            pTmp, szLock );
                }
                nRet = L_SUCCESS;
                if ( pTmp != NULL ) {
                    unlink ( pTmp );
                    free   ( pTmp );
                    pTmp = NULL;
                }
            }

            if ( nRet != L_SUCCESS && fd != -1 ) {
                if ( pTmp != NULL )
                    unlink ( pTmp );
            }

            if ( pTmp != NULL ) {
                free ( pTmp );
                pTmp = NULL;
            }
            fd = -1;
        } /* bNo_atomic_open == TRUE */
        else { 
            /* 
             * The following is supposed to be atomic -- even on NFS (but
             * we don't believe it).
             */
            fd = open ( szLock, O_WRONLY | O_CREAT | O_EXCL, 0600 );
            if ( fd == -1 ) {
                if ( DEBUGGING && bDebugging ) {
                    logit ( fTrace, POP_DEBUG, HERE,
                            "open() for %s returned %s (%d)",
                            szLock, strerror(errno), errno );
                }
                
                switch ( errno ) {

                case EACCES:
                    /* 
                     * Keep trying for the lock life 
                     */
                    curTime = time(0);
                    nRet = stat ( szLock, &byNameStat );
                    if ( nRet == -1 ) {
                       logit ( fTrace, POP_PRIORITY, HERE,
                               "fstat() for %s returned %s (%d) and "
                               "open() returned EACCESS",
                               szLock, strerror(errno), errno );
                       attemptcnt = retrycnt;
                       nRet = L_ERROR;
                    } 
                    else if ( curTime - (time_t)(byNameStat.st_mtime) < LOCK_LIFE ) {
                        attemptcnt++;
                        if ( DEBUGGING && bDebugging ) {
                            logit ( fTrace, POP_DEBUG, HERE, 
                                    "lock %s busy", szLock );
                        }
                    } 
                    else {
                        attemptcnt = retrycnt;
                        nRet = L_ERROR;
                        logit ( fTrace, POP_DEBUG, HERE,
                                "Lock %s expired but access denied (%lu < %u)",
                                szLock, curTime - (time_t)(byNameStat.st_mtime), 
                                LOCK_LIFE );
                    }
                    break;

                case EEXIST:
                    /* 
                     * Check the lock life 
                     */
                    curTime = time(0);
                    nRet = stat ( szLock, &byNameStat );
                    if ( nRet == -1 ) {
                        logit ( fTrace, POP_PRIORITY, HERE,
                               "fstat() for %s returned %s (%d) and "
                               "open() returned EEXIST",
                               szLock, strerror(errno), errno );
                        attemptcnt = retrycnt;
                        nRet = L_ERROR;
                    } 
                    else if ( curTime - (time_t)(byNameStat.st_mtime) > LOCK_LIFE ) {
                        /* 
                         * Create the lock file.
                         *
                         * This create could be happening to a link. 
                         * Check the race 
                         */
                        if ( DEBUGGING && bDebugging ) { 
                           logit ( fTrace, POP_DEBUG, HERE,
                                   "Lock expired -- usurping; curTime=%u; "
                                   "st_mtime=%u; age=%u; lock_life=%d",
                                   (unsigned) curTime,
                                   (unsigned) byNameStat.st_mtime,
                                   (unsigned) curTime - (time_t)(byNameStat.st_mtime),
                                   LOCK_LIFE );
                        }
                        fd = open ( szLock, O_WRONLY|O_CREAT|O_TRUNC, 0600 );
                        if ( fd == -1 ) {
                            logit ( fTrace, POP_PRIORITY, HERE,
                                    "Open failed for %s: %s (%d)", 
                                    szLock, strerror(errno), errno );
                            nRet = L_TMPLOCK;
                        }
                        attemptcnt = retrycnt;
                    } /* lock not owned by current process */
                    else {
                        if ( DEBUGGING && bDebugging ) { 
                           logit ( fTrace, POP_DEBUG, HERE,
                                   "lock %s busy (attempt %d of %d)", 
                                   szLock, attemptcnt+1, retrycnt );
                        }
                        attemptcnt++;
                        if ( attemptcnt == retrycnt ) 
                            nRet = L_MAXTRYS;
                        else
                            nRet = L_ERROR;
                    }
                    break;

                default:
                    logit ( fTrace, POP_PRIORITY, HERE,
                            "Unexpected error opening lock file %s: %s (%d)", 
                            szLock, strerror(errno), errno );
                    attemptcnt = retrycnt;
                    nRet = L_TMPLOCK;
                    break;
                } /* switch ( errno ) */

            } /* open ( szLock ... ) == -1 */
            else {
                nRet = L_SUCCESS;
                if ( DEBUGGING && bDebugging ) {
                    logit ( fTrace, POP_DEBUG, HERE,
                            "successfully opened (exclusive) lock %s", szLock );
                    }
                break;
            }
        } /* bNo_atomic_open == FALSE */
    } /* while ( nRet != L_SUCCESS && attemptcnt != retrycnt ) */

    if ( fd != -1 ) { /* .lock file created successfully */
        /* 
         * Do the lstat again to check if this is a symbolic link to
         * account for the race conditions 
         */
        if ( lstat ( szLock, &byNameStat ) == -1 ) {
            logit ( fTrace, POP_PRIORITY, HERE,
                    "Error checking lstat on newly-created lock file %s: %s (%d)",
                    szLock, strerror(errno), errno );
            nRet = L_TMPLOCK;
        } else if ( ( (byNameStat.st_mode & S_IFMT) == S_IFLNK ) ||
                    (  byNameStat.st_nlink           >       1 )    ) {
            logit ( fTrace, POP_PRIORITY, HERE,
                    "Newly-created lock file %s has %ld links",
                    szLock, byNameStat.st_nlink );
            nRet = L_TMPLOCK;
        }

        /* 
         * Lock the file, write some data (pid@host) and unlock it.
         *
         * Well, this is advisory locking. 
         */
        if ( flock ( fd, LOCK_EX | LOCK_NB ) == -1 ) {
            if ( DEBUGGING && bDebugging ) {
                logit ( fTrace, POP_DEBUG, HERE,
                        "flock() failed on newly-created lock %s: %s (%d)",
                        szLock, strerror(errno), errno );
            }
            nRet = L_MANLOCK; /* Someone seized the lock since 
                               * this process created it. */
        }
        else {
            char buf [ BUF_SIZE ];
            int  nSize;
            
            nSize = Qsprintf ( buf, "%d@", (int) getpid() );
            gethostname ( buf + nSize, BUF_SIZE - nSize - 1 );
            buf [ BUF_SIZE -1 ] = '\0';
            nSize = strlen(buf);
            if ( write ( fd, buf, nSize ) != nSize ) {
                if ( errno == EDQUOT ) {
                    logit ( fTrace, POP_NOTICE, HERE,
                            "Overquota locking file '%s'; lock proceeding anyway",
                            szLock );
                }
                else {
                    nRet = L_TMPWRITE;
                    logit ( fTrace, POP_PRIORITY, HERE,
                            "write to newly-created lock file %s failed: %s (%d)",
                            szLock, strerror(errno), errno );
                }
            } /* write failed */

            flock ( fd, LOCK_UN );
        }
        close ( fd );
        if ( nRet != L_SUCCESS && fd != -1 ) {
            unlink ( szLock );
            *szLock = '\0';
        }
    } /* .lock file created successfully */

    if ( DEBUGGING && bDebugging ) {
        logit ( fTrace, POP_DEBUG, HERE,
                "maillock() on file %s (%s) [%s:%lu] returning %d (%d attempt(s))", 
                 drop_name, szLock, fn, (unsigned long) ln, nRet, attemptcnt + 1 );
    }
    return nRet;
}


/*
 * Function:    mailunlock
 *
 * Descrption:  Removes the .lock file previously obtained through
 *              maillock().
 *
 * Parameters:  fn:         file name whence called.
 *              ln:         line number whence called.
 *
 * Returns:    L_SUCCESS, L_ERROR - When there wasn't a lock.
 */
int 
Qmailunlock ( const char *fn, size_t ln )

{
    int nRet = L_ERROR;


    if ( *szLock != '\0' ) {
        unlink ( szLock );
        nRet = L_SUCCESS;
        if ( DEBUGGING && gbDebugging ) {
            logit ( gfTrace, POP_DEBUG, HERE,
                    "mailunlock() called [%s:%lu] for %s", 
                    fn, (unsigned long) ln, szLock );
        *szLock = '\0';
        }
    }
    else {
        logit ( gfTrace, POP_PRIORITY, HERE,
                "mailunlock() called [%s:%lu] but szLock null",
                fn, (unsigned long) ln );
    }

    return nRet;
}


/*
 * Function:    touchlock
 *
 * Descrption:  Changes access and modification times of the .lock file.
 *
 * Parameters:  fn:         file name whence called.
 *              ln:         line number whence called.
 *
 * Returns:     L_SUCCESS on a success, L_ERROR when there wasn't lock file.
 */
int Qtouchlock ( const char *fn, size_t ln )
{
    int nRet = L_ERROR;
    int rslt = 0;

    if ( *szLock != '\0' ) {
        if ( DEBUGGING && gbDebugging ) {
            logit ( gfTrace, POP_DEBUG, HERE,
                    "touchlock() called [%s:%lu] for %s", 
                    fn, (unsigned long) ln, szLock );
        }
        rslt = touch_file ( szLock, gfTrace ); /* change access and mod times */
        nRet = ( rslt == 0 ? L_SUCCESS : L_ERROR );
    }
    else {
        if ( DEBUGGING && gbDebugging ) {
            logit ( gfTrace, POP_DEBUG, HERE,
                    "touchlock called [%s:%lu] with null szlock",
                    fn, (unsigned long) ln );
        }
        nRet = L_ERROR;
    }
    return nRet;
}


/*
 * Function:    lockerr
 *
 * Descrption:  Returns description of lock error.
 *
 * Parameters:  e:         error number.
 *
 * Returns:     character pointer to description.
 */
const char *Qlockerr ( int e )
{
    static char error_buffer [ 64 ];


    switch ( e ) {
        case L_NAMELEN:
            strcpy ( error_buffer, "User name too long" );
            break;
        case L_TMPLOCK:
            strcpy ( error_buffer, "Unable to create lockfile" );
            break;
        case L_TMPWRITE:
            strcpy ( error_buffer, "Unable to write pid into lockfile" );
            break;
        case L_MAXTRYS:
            strcpy ( error_buffer, "Max tries exceeded" );
            break;
        case L_ERROR:
            strcpy ( error_buffer, "Other lock error" );
            break;
        case L_MANLOCK:
            strcpy ( error_buffer, "Unable to lock lockfile" );
            break;
        default:
            strcpy ( error_buffer, "???" );
    }
    return ( error_buffer );
}







