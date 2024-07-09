/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file license.txt specifies the terms for use, modification, or
 * redistribution.
 *
 */

/*
 * Revisions:
 *
 *   03/01/00  [rcg]
 *       - Handle case of hidden first message.
 *
 */


/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */


#include "config.h"
#include <stdio.h>
#include <sys/types.h>

#include "popper.h"

/* 
 *  stat:   Display the status of a POP maildrop to its client
 */

int pop_stat (p)
POP     *   p;
{
    DEBUG_LOG2 ( p, "%d visible message(s) (%ld octets).", 
                 p->visible_msg_count - p->msgs_deleted,
                 p->drop_size - p->bytes_deleted );

    return ( pop_msg ( p, POP_SUCCESS, HERE,
                       "%u %lu",
                       p->visible_msg_count - p->msgs_deleted,
                       p->drop_size - p->bytes_deleted ) );
}
