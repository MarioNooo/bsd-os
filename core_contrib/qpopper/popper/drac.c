/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 *
 * Revisions:
 *
 *     06/27/00  [rcg]
 *              - Fixed pop_log call.
 *              - Drac host now set by -D run-time option instead
 *                of reading file.
 *
 *     06/05/00  [rcg]
 *              - File added.  Based on patches by Mike McHenry,
 *                Forrest Aldrich, Steven Champeon, and others.
 */

#include "config.h"

#ifdef DRAC_AUTH

#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "popper.h"


int
drac_it ( POP *p )
{
    char     *err         = NULL;
    int       rslt        = 0;


    if ( p->drac_host != NULL && *p->drac_host != '\0' )   
    {
        rslt = dracauth ( p->drac_host, inet_addr(p->ipaddr), &err );
        if ( rslt != 0 )
            pop_log ( p, POP_PRIORITY, HERE, 
                      "[drac]: dracauth returned %d: %s",
                      rslt, err );
        else
        { 
            pop_log ( p, POP_PRIORITY, HERE,
                      "[drac]: login by %s from host %s (%s)",
                      p->user, p->client, p->ipaddr );
            return POP_SUCCESS;
        }
    }
    else
    {
        pop_log ( p, POP_PRIORITY, HERE, "drac not used; p->drac_host null" );
    }
    return POP_FAILURE;
}

#endif /* DRAC_AUTH */
