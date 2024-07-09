/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 */


/* Changes:
 *
 *   06/16/00  [rcg]
 *            - file added.
 */


#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

#include "config.h"
#include "popper.h"

int
check_group ( POP *p, struct passwd *pwd, const char *group_name )
{
    char          **u_list;
    struct group   *grp;


    /* 
     * First, let's find specified group 
     */
    grp = getgrnam ( group_name );
    if ( grp == NULL ) {
        DEBUG_LOG1 ( p, "No such group: \"%.128s\"", group_name );
        return FALSE;
    }
    
    DEBUG_LOG2 ( p, "Primary gid for user %s is %u", 
                 p->user, (unsigned) pwd->pw_gid );
    
    /*
     * If user's primary group matches the desired group, we matched
     * it the easy way.
     */
    if ( pwd->pw_gid == grp->gr_gid ) {
        DEBUG_LOG1 ( p, "User's primary gid matches \"%.128s\"",
                     group_name );
        return TRUE;
    }

    /* 
     * Look for the user in this group's member list.
     */
    u_list = grp->gr_mem; 
    while ( *u_list != NULL ) {
        if ( strcmp ( *u_list, p->user ) == 0 ) {
            DEBUG_LOG2 ( p, "Found user %s in group list for \"%.128s\"",
                         p->user, group_name );
            return TRUE; /* user found */
            }
        u_list++;
    }

    DEBUG_LOG2 ( p, "User %s not found in group list for \"%.128s\"",
                 p->user, group_name );
    return FALSE;
}
