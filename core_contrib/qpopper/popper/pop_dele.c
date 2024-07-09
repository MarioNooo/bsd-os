/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 *
 * Modifications:
 *
 * 01/31/01  [rcg]
 *          - Added delete_msg() and undelete_msg() to do guts of msg
 *            deletion and undeletion, to ensure consistent actions.
 *
 * 03/01/00  [rcg]
 *          -  Now calling msg_ptr to adjust for hidden msgs and check range.
 */


/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>

#include "popper.h"

/* 
 *  dele:   Delete a message from the POP maildrop
 */
int
pop_dele (p)
POP     *   p;
{
    MsgInfoList     *   mp;         /*  Pointer to message info list */
    int                 msg_num;

    /*  
     * Convert the message number parameter to an integer 
     */
    msg_num = atoi ( p->pop_parm [ 1 ] );

    /*  
     * Get a pointer to the message in the message list 
     */
    mp = msg_ptr ( p, msg_num );
    if ( mp == NULL )
        return ( pop_msg ( p, POP_FAILURE, HERE,
                           "Message %d does not exist.", msg_num ) );

    /*
     * Do the work of marking it for deletion.
     */
    delete_msg ( p, mp );

    return ( pop_msg ( p, POP_SUCCESS, HERE,
                       "Message %d has been deleted.",
                       msg_num ) );
}


/* 
 *  Mark a message as deleted.
 */
pop_result
delete_msg ( POP *p, MsgInfoList *mp )
{


    if ( mp->del_flag == FALSE ) {
        /*  
         * Flag the message for deletion 
         */
        mp->del_flag = TRUE;

        DEBUG_LOG3 ( p, "Deleting message %u at offset %lu of length %lu",
                     mp->number, mp->offset, mp->length );

        /*  
         * Update the messages_deleted and bytes_deleted counters 
         */
        p->msgs_deleted++;
        p->bytes_deleted += mp->length;
        DEBUG_LOG2 ( p, "msgs_deleted now %d; bytes_deleted now %ld",
                     p->msgs_deleted, p->bytes_deleted );

        p->dirty = TRUE;

        /*  
         * Update the last-message-accessed number if it is lower than 
         * the deleted message 
         */
        if ( p->last_msg < mp->visible_num ) 
            p->last_msg = mp->visible_num;

        return POP_SUCCESS;
    }

    return POP_FAILURE;
}


/* 
 *  Mark a message as not deleted.
 */
pop_result
undelete_msg ( POP *p, MsgInfoList *mp )
{


    if ( mp->del_flag == TRUE ) {
        /*  
         * Un-flag the message for deletion 
         */
        mp->del_flag = FALSE;

        DEBUG_LOG3 ( p, "Undeleting message %u at offset %lu of length %lu",
                     mp->number, mp->offset, mp->length );

        /*  
         * Update the messages_deleted and bytes_deleted counters 
         */
        p->msgs_deleted--;
        p->bytes_deleted -= mp->length;
        DEBUG_LOG2 ( p, "msgs_deleted now %d; bytes_deleted now %ld",
                     p->msgs_deleted, p->bytes_deleted );

        /*  
         * Update the last-message-accessed number if it is lower than 
         * the deleted message and the deleted message was accessed. 
         */
        if ( mp->retr_flag && p->last_msg < mp->visible_num ) 
            p->last_msg = mp->visible_num;

        return POP_SUCCESS;
    }

    return POP_FAILURE;
}
