/*
 * Copyright (c) 2003 QUALCOMM Incorporated. All rights reserved.
 * 
 * Description: 
 *   maillock() and mailunlock() try to emulate the SysV functions
 *   to create the user.lock file.
 *   call touchlock() to update the lock.
 *
 * Caveats:
 *   mailunlock() will only remove the lockfile created from previous
 *   maillock(). maillock() called for different users with out the
 *   intermediate calls for mailunlock() would leave the initial locks.
 *   Process termination would not clean the .lock files.
 * 
 * Revisions:
 *
 *   01/30/01 [rcg]
 *        - Qmaillock now takes bNo_atomic_open and pSpool_dir parameters
 *          instead of using compile-time constants.
 *
 *   01/19/01 [rcg]
 *        - Modified to pass drop name instead of user name (based on
 *          patch by Fergal Daly).
 *
 *   06/07/00 [rcg]
 *        - Added file and line from whence called to trace records.
 *        - Added Qmailerr to return lock error string.
 *
 *   04/21/00 [rcg]
 *        - Renamed to Qmaillock(), Qmailunlock(), and Qtouchlock().
 *
 *   12/06/99 [rcg]
 *        - Added bDebugging parameter.
 *        - Removed K&R junk.
 *
 *   11/23/99 [rcg]
 *        - File added.
 *
 */




#ifndef _QMAILLOCK_H
#  define   _QMAILLOCK_H

#include "utils.h" /* for TRUE, FALSE, and BOOL */


/*
 * Return values for Qmaillock()
 */
#define L_SUCCESS   0
#define L_NAMELEN   1   /* recipient name > 13 chars */
#define L_TMPLOCK   2   /* problem creating temp lockfile */
#define L_TMPWRITE  3   /* problem writing pid into temp lockfile */
#define L_MAXTRYS   4   /* cannot link to lockfile after N tries */
#define L_ERROR     5   /* Something other than EEXIST happened */
#define L_MANLOCK   6   /* cannot set mandatory lock on temp lockfile */

/*
 * Must call Qtouchlock at least this many seconds
 */
#define LOCK_REFRESH_INTERVAL ( 60 * 3 ) /* three minutes */


int Qmaillock   ( char *drop_name, int retrycnt, BOOL bNo_atomic_open, char *pSpool_dir, void *fTrace, const char *fn, size_t ln, int bDebugging );
int Qmailunlock ( const char *fn, size_t ln );
int Qtouchlock  ( const char *fn, size_t ln );
const char *Qlockerr  ( int e );


#endif /* _QMAILLOCK_H */

