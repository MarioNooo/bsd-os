/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 *
 * Modifications:
 *
 * 03/01/00  [rcg]
 *          -  File added.
 */


#include "config.h"
#include <stdio.h>
#include <sys/types.h>
#include "popper.h"

/*
 * Given a visible message number, return a pointer to the message.
 */
MsgInfoList *
msg_ptr ( POP *p, int msg_num )
{
    /*  
     * Is requested message out of range? 
     */
    if ( ( msg_num < 1 ) || ( msg_num > p->msg_count ) )
        return NULL;

    /* 
     * If the first message is hidden, adjust
     */
    if ( p->first_msg_hidden ) {
        DEBUG_LOG2 ( p, "first msg hidden; visible msg %d is really msg %d",
                     msg_num, msg_num +1 );
        msg_num++;
        if ( msg_num > p->msg_count ) 
            return NULL;
    }
    
    /*
     * If we ever start hiding messages other than the first, add code here to
     * compare our msg_num parameter with the messages visible_number, and skip
     * forward until they match.
     */

    /*  
     * Get a pointer to the message in the message list 
     */
    return ( &p->mlp [ msg_num -1 ] );
}
