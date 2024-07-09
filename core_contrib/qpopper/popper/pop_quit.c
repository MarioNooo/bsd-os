/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 */

#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

#include "popper.h"

/* 
 *  quit:   Terminate a POP session
 */

int pop_quit (p)
POP     *   p;
{
    /*  
     * Release the message information list 
     */
    if  ( p->mlp != NULL ) 
        free ( (char *)p->mlp );

    if ( p->CurrentState == auth2 ) {
        pop_log ( p, POP_WARNING, HERE, 
                  "Possible probe of account %s from host %s (%s)",
                  p->user, p->client, p->ipaddr );
    }

    return ( POP_SUCCESS );
}
