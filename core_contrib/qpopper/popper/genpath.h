/*
 * Copyright (c) 2003 QUALCOMM Incorporated. All rights reserved.
 * See License.txt file for terms and conditions for modification and
 * redistribution.
 *
 * Revisions: 
 *
 * 01/18/01  [rcg]
 *         - Handle hash_spool and homedirmail being run-time options.
 *
 * 09/20/00  [rcg]
 *         - Added GNPH_CACHE for path to cache file.
 *
 * 01/07/00  [rcg]
 *         - Added iWhich to indicate path to which file:
 *           spool, .pop, tmpdrop, or tmpxmit.
 *
 * 12/06/99  [rcg]
 *         - Added bDebugging parameter.
 *
 * 11/24/99  [rcg]
 *         - File added.
 *
 */

#ifndef _GENPATH_H_
#define _GENPATH_H_

#include <pwd.h>

#include "config.h"
#include "popper.h"


enum GNPH_TYPE {
     GNPH_SPOOL = 1,  /* spool file */
     GNPH_POP,        /* .pop file  */
     GNPH_TMP,        /* tmpxxxx    */
     GNPH_XMT,        /* xmitxxxx   */
     GNPH_OLDPOP,     /* old .pop  (always in POP_MAILDIR ) */
     GNPH_CACHE,      /* .cache file */
     GNPH_PATH };     /* path only  */
 
typedef enum GNPH_TYPE GNPH_WHICH;


#ifdef DEBUG
const char *GNPH_DEBUG_NAME ( GNPH_WHICH iWhich );
#else /* DEBUG */
#  define GNPH_DEBUG_NAME(X) ""
#endif /* DEBUG */


/*---------------------------------------------------------------------------------------
 * Generate path to desired file.
 *
 * Parameters:
 *    p:          pointer to POP structure.
 *    pszDrop:    pointer to buffer where drop name is placed.
 *    iDroplen:   length of buffer for drop name.
 *    iWhich:     path to which file?
 *
 * Returns:
 *   -1 if there is an error (and error is logged),
 *    1 if all went well.
 */
int genpath ( POP *p, char *pszDrop, int iDropLen, GNPH_WHICH iWhich );





#endif /* _GENPATH_H_ */

