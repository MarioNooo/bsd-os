/*
 * Copyright (c) 2003 QUALCOMM Incorporated. All rights reserved.
 * See License.txt file for terms and conditions for modification and
 * redistribution.
 *
 * Revisions: 
 *
 * 03/26/01  [rcg]
 *         - Fixed double-slash in path when hash-spool=1.
 *
 * 01/19/01  [rcg]
 *         - Rewrote to collapse code and handle hash_spool, homedirmail,
 *           spool, temp, and cache directories and file names now being
 *           run-time options.
 *
 * 01/18/01  [rcg]
 *         - Modified based on patches by Fergal Daly to pass in pwnam.
 *
 * 09/20/00  [rcg]
 *         - Added GNPH_CACHE for path to .cache file.
 *
 * 09/08/00  [rcg]
 *         - Selecting both HOMEDIRMAIL and TEMP-DROP no longer causes
 *           an invalid path to be generated.
 *
 * 06/30/00  [rcg]
 *         - Replaced strncpy/strncat with strlcpy/strlcat, which
 *           guarantee to null-terminate the buffer, and are easier
 *           to use safely.
 *
 * 04/21/00  [rcg]
 *         - Replaced sprintf with Qsprintf.
 *
 * 03/08/00  [rcg]
 *         - Trace calls now dependent on DEBUG as well as bDebugging
 *
 * 01/31/00  [rcg]
 *         - Fix syntax error when HOMEDIRMAIL defined.
 *
 * 01/27/00  [rcg]
 *         - genpath wasn't returning correct path for .pop file
 *           when HOMEDIRMAIL set and --enable-temp-drop-dir used,
 *           or no hashed, no home dir used (latter fix sent in by
 *           Gerard Kok).
 *         - Added HERE parameter to logit() calls.
 *
 * 01/07/00  [rcg]
 *         - Added iWhich parameter to specify path to which file. 
 *
 * 12/06/99  [rcg]
 *         - Trace calls now dependent on bDebugging.
 *
 * 12/02/99  [rcg]
 *         - Trace log calls now dependent on DEBUGGING 
 *
 * 11/24/99  [rcg]
 *         - Modifed to pass in individual parameters instead of POP *.
 *         - Moved into its own file.
 *         - Applied patch contributed by Vince Vielhaber to make use of
 *           HOMEDIRMAIL easier (only need define it in popper.h, no need
 *           to edit this file as well).
 *
 */

#include "config.h"

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <flock.h>

#include "genpath.h"
#include "logit.h"
#include "snprintf.h"
#include "popper.h"

#if HAVE_UNISTD_H
#  include <unistd.h>
#endif

#if HAVE_SYS_UNISTD_H
#  include <sys/unistd.h>
#endif

#if HAVE_STRINGS_H
#  include <strings.h>
#endif

#include <sys/stat.h>
#include <sys/file.h>
#include <pwd.h>

#include "popper.h"
#include "string_util.h"


static char *get_hash_dir ( char *pszUser, int iMethod );


int 
genpath ( POP *p, char *pszDrop, int iDropLen, GNPH_WHICH iWhich )
{
    char   *pszUser       = NULL; /* user name */
    int     len1          = 0;    /* used with strlcpy/strlcat */
    int     len2          = 0;    /* used with strlcat */
    int     len3          = 0;    /* used with strlcat */
    int     iChunk        = 0;    /* used with Qsnprintf */
    struct passwd *pw     = &p->pw;


    if ( pw == NULL || pw->pw_name == NULL ) {
        pop_log ( p, POP_PRIORITY, HERE, "Bogus passwd struct" );
        return -1; /*bogus login name*/
    }

    pszUser = pw->pw_name;

    /*
     * First, the parent directory
     */
    switch ( iWhich ) {
        case GNPH_SPOOL:  /* spool file */
        case GNPH_TMP:    /* tmpxxxx    */
        case GNPH_XMT:    /* xmitxxxx   */
        case GNPH_PATH:  /* Just the path, M'am */
            if ( p->pHome_dir_mail != NULL )
                len1 = strlcpy ( pszDrop, pw->pw_dir,        iDropLen );
            else
                len1 = strlcpy ( pszDrop, p->pCfg_spool_dir, iDropLen );
            len2 = strlcat ( pszDrop, "/", iDropLen );
            break;

        case GNPH_POP:    /* .pop file  */
            len1 = strlcpy ( pszDrop, p->pCfg_temp_dir, iDropLen );
            len2 = strlcat ( pszDrop, "/",              iDropLen );
            break;

        case GNPH_OLDPOP: /* old .pop file (always in POP_MAILDIR) */
            len1 = strlcpy ( pszDrop, p->pCfg_spool_dir, iDropLen );
            len2 = strlcat ( pszDrop, "/",               iDropLen );
            break;

        case GNPH_CACHE:   /* .cache file  */
            len1 = strlcpy ( pszDrop, p->pCfg_cache_dir, iDropLen );
            len2 = strlcat ( pszDrop, "/",               iDropLen );
            break;

        default:
            pop_log ( p, POP_PRIORITY, HERE,
                      "Bad iWhich passed to genpath: %d",
                      (int)iWhich );
            return -1;
            break;
    } /* switch on iWhich */

	DEBUG_LOG2 ( p, "...built: (%d) '%.100s'", len1 + len2, pszDrop );

    /*
     * Next, the hashed directory (if in use)
     */
    if ( p->hash_spool != 0 ) {
        len3 = strlcat ( pszDrop, get_hash_dir ( pszUser, p->hash_spool ),
                         iDropLen );
		DEBUG_LOG2 ( p, "...built: (%d) '%.100s'",
                         len1 + len2 + len3, pszDrop );
    }

    /*
     * Check for truncation
     */
    if ( len1 > iDropLen || len2 > iDropLen || len3 > iDropLen ) {
        pop_log ( p, 
                  POP_PRIORITY, 
                  HERE,
                  "Insufficient room to generate path for user %.100s"
                  "; need more than %i; have only %i",
                  pszUser,
                  MAX ( len1, len2 ),
                  iDropLen );
        return -1;
    }

    /*
     * Now, fill in the specific file
     */
    switch ( iWhich ) {
        case GNPH_SPOOL:  /* spool file */
            if ( p->pHome_dir_mail != NULL )
                len1 = strlcat ( pszDrop, p->pHome_dir_mail, iDropLen );
            else
                len1 = strlcat ( pszDrop, pszUser,  iDropLen );
            break;

        case GNPH_POP:    /* .pop file  */
            iChunk = Qsprintf ( pszDrop + strlen(pszDrop),
                                p->pCfg_temp_name, 
                                pszUser );
            break;

        case GNPH_TMP:    /* tmpxxxx    */
            len1 = strlcat ( pszDrop, ".", iDropLen );
            len2 = strlcat ( pszDrop, POP_TMPDROP, iDropLen );
            break;

        case GNPH_XMT:    /* xmitxxxx   */
            len1 = strlcat ( pszDrop, ".", iDropLen );
            len2 = strlcat ( pszDrop, POP_TMPXMIT, iDropLen ) ;
            break;

        case GNPH_OLDPOP: /* old .pop file (always in POP_MAILDIR) */
            iChunk = Qsprintf ( pszDrop + strlen(pszDrop),
                                p->pCfg_temp_name,
                                pszUser );
            break;

        case GNPH_CACHE:   /* .cache file  */
            iChunk = Qsprintf ( pszDrop + strlen(pszDrop),
                                p->pCfg_cache_name,
                                pszUser );
            break;

        case GNPH_PATH:  /* Just the path, M'am */
            pszDrop [ strlen(pszDrop) -1 ] = '\0'; /* erase trailing '/' */
            break;
    } /* switch on iWhich */

    DEBUG_LOG6 ( p, "genpath %s (%d) [hash: %d; home: %s] for user "
                    "%s returning %s", 
                 GNPH_DEBUG_NAME(iWhich), (int) iWhich, p->hash_spool,
                 ( p->pHome_dir_mail ? p->pHome_dir_mail : "NULL" ),
                 pszUser, pszDrop );

    return 1;
}



/*
 * Hashing to a spool directory helps reduce the lookup time for sites
 * with thousands of mail spool files.  Unix uses a linear list to
 * save directory information and the following methods attempt to
 * improve the performance.
 *
 * Method 1:  add the value of the first 5 chars mod 26 and open the
 *            spool file in the directory 'a' - 'z'/user.
 *            Brian Buhrow <buhrow@cats.ucsc.edu>
 *
 * Method 2:  Use the first 2 characters to determine which mail spool
 *            to open.  E.g., /usr/spool/u/s/user.
 *            Larry Schwimmer <rosebud@cyclone.stanford.edu>
 *            (if only one character it is repeated.)
 *
 * All these methods require that local mail delivery and client programs
 * use the same algorithm.  Only one method to a customer :-)
 */

static char hash_buf [ 5 ];

static char *
get_hash_dir ( char *pszUser, int iMethod )
{
    if ( iMethod == 1 ) {
        int     seed          = 0;

        /*Now, perform the hash*/

        switch ( strlen(pszUser) ) {
            case 1:
                seed = ( pszUser[0] );
                break;
            case 2:
                seed = ( pszUser[0] + pszUser[1] );
                break;
            case 3:
                seed = ( pszUser[0] + pszUser[1] + pszUser[2] );
                break;
            case 4:
                seed = ( pszUser[0] + pszUser[1] + pszUser[2]+  pszUser[3] );
                break;
            default:
                seed = ( pszUser[0] + pszUser[1] + pszUser[2] + 
                         pszUser[3] + pszUser[4] );
                break;
        } /* switch on pszUser length */

        hash_buf [ 0 ] = (char) ( (seed % 26) + 'a' );
        hash_buf [ 1 ] = '/';
        hash_buf [ 2 ] = '\0';

        return hash_buf;
    } /* hash_spool == 1 */
    else
    if ( iMethod == 2 ) {
        hash_buf [ 0 ] = *pszUser;
        hash_buf [ 1 ] = '/';
        hash_buf [ 2 ] = ( *(pszUser + 1) ? *(pszUser + 1) : *pszUser );
        hash_buf [ 3 ] = '/';
        hash_buf [ 4 ] = '\0';
        return hash_buf;
    } /* hash_spool == 2 */
    else
        return NULL;
}





#ifdef DEBUG
const char *GNPH_DEBUG_NAME ( GNPH_WHICH iWhich )
{
   static char error_buffer[64];
   switch ( iWhich ) {
       case GNPH_SPOOL:
           strcpy ( error_buffer, "Spool" );
           break;
       case GNPH_POP:
           strcpy ( error_buffer, ".pop" );
           break;
       case GNPH_TMP:
           strcpy ( error_buffer, "TMPxxxx" );
           break;
       case GNPH_XMT:
           strcpy ( error_buffer, "XMITxxxx" );
           break;
       case GNPH_OLDPOP:
           strcpy ( error_buffer, "old .pop" );
           break;
       case GNPH_CACHE:
           strcpy ( error_buffer, ".cache"  );
           break;
       case GNPH_PATH:
           strcpy ( error_buffer, "path only" );
           break;
       default:
           strcpy ( error_buffer, "???" );
   }
    return ( error_buffer );
}
#endif /* DEBUG */

