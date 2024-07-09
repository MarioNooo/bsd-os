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
 * Modifications:
 *
 *     01/29/01  [rcg]
 *             - Now using p->grp_serv_mode and p->grp_no_serv_mode instead
 *               of SERVER_MODE_GROUP_INCLUDE and SERVER_MODE_GROUP_EXCLUDE.
 *             - Now using p->authfile instead of AUTHFILE.
 *
 *     10/02/00  [rcg]
 *             - Now calling check_config_files to process qpopper-options.
 *
 *     06/30/00  [rcg]
 *             - Added support for ~/.qpopper-options.
 *
 *    06/16/00  [rcg]
 *              - Now setting server mode on or off based on group
 *                membership.
 *
 *    06/10/00  [rcg]
 *              - Fixed incorrect order of parameters in auth OK msg.
 *
 *    03/07/00  [rcg]
 *              - Updated authentication OK message to account for hidden
 *                messages.
 */

#include <sys/types.h>
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pwd.h>
#include <string.h>

#if HAVE_STRINGS_H
#  include <strings.h>
#endif /* HAVE_STRINGS_H */

#include <pwd.h>
#include <netinet/in.h> /* For IPPORT_RESERVED */

#include "popper.h"
#include "snprintf.h"

extern int ruserok();


/*
 * Use /etc/hosts.equiv and the users .rhost file to validate the user.
 */
int pop_rpop (p)
     POP     *   p;
{
    struct passwd  *   pw;

    if ( p->ipport >= IPPORT_RESERVED || p->ipport < IPPORT_RESERVED/2 )  {
        pop_log ( p, POP_PRIORITY, HERE,
                  "RPOP command from %s (%s) on illegal port.",
                  p->client,p->ipaddr);
        return ( pop_msg ( p, POP_FAILURE, HERE,
                           "Permission denied." ) );
    }
    if ( ruserok ( p->client, 0, p->pop_parm[1], p->user ) != 0 ) {
        DEBUG_LOG1 ( p, "ruserok denied user %s", p->user );
        return ( pop_msg ( p, POP_FAILURE, HERE,
                           "Permission denied." ) );
    }
    if ( p->nonauthfile != NULL && checknonauthfile ( p ) != 0 ) {
        return ( pop_msg ( p, POP_FAILURE, HERE,
                           "Permission denied." ) );
    }
    if ( p->authfile != NULL && checkauthfile ( p ) != 0 ) {
        return ( pop_msg ( p, POP_FAILURE, HERE,
                           "Permission denied." ) );
    }

    pw = &p->pw;
    if ( pw->pw_name == NULL )  {  /* "Can't happen" */
        DEBUG_LOG1 ( p, "getpwnam returned NULL for user %s", p->user );
        return ( pop_msg ( p, POP_FAILURE, HERE,
                 "Permission denied." ) );
    }

    if ( pw->pw_uid <= BLOCK_UID ) {
        DEBUG_LOG3 ( p, "User %s (uid %lu) blocked by BLOCK_UID (%lu)",
                     p->user, 
                     (long unsigned) pw->pw_uid,
                     (long unsigned) BLOCK_UID );
        return ( pop_msg ( p, POP_FAILURE, HERE, 
                 "Permission denied" ) );
    }

    /*
     * Check if server mode should be set or reset based on group membership.
     */

    if ( p->grp_serv_mode != NULL
         && check_group ( p, pw, p->grp_serv_mode ) ) {
        p->server_mode = TRUE;
        DEBUG_LOG2 ( p, "Set server mode for user %s; "
                        "member of \"%.128s\" group",
                     p->user, p->grp_serv_mode );
    }

    if ( p->grp_no_serv_mode != NULL
         && check_group ( p, pw, p->grp_no_serv_mode ) ) {
        p->server_mode = FALSE;
        DEBUG_LOG2 ( p, "Set server mode OFF for user %s; "
                        "member of \"%.128s\" group",
                     p->user, p->grp_no_serv_mode );
    }

    /*
     * Process qpopper-options files, if requested and present.
     */
    check_config_files ( p, pw );

    /*  Make a temporary copy of the user's maildrop */
    /*    and set the group and user id */
    if ( pop_dropcopy ( p, pw ) != POP_SUCCESS ) 
        return (POP_FAILURE);

    /*  Initialize the last-message-accessed number */
    p->last_msg = 0;

    p->AuthState = rpop;  /* plain or keberos authenticated successfully */
                          /* using Berk. r-files                         */

    if ( p->pLog_login != NULL )
        do_log_login ( p );

    /*  
     * Authorization completed successfully 
     */
    return ( pop_msg ( p, POP_SUCCESS, HERE,
                       "%s has %d visible message%s (%d hidden) in %ld octets.",
                        p->user,
                        p->visible_msg_count,
                        p->visible_msg_count == 1 ? "" : "s",
                        (p->msg_count - p->visible_msg_count),
                        p->drop_size ) );
}

