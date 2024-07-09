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
 * 10/13/00  [rcg]
 *          -  Added LGL's changes for TLS.
 *
 * 03/01/00  [rcg]
 *          -  Now calling msg_ptr to adjust for hidden msgs and check range.
 *
 * 02/04/00  [rcg]
 *          -  Changed len param of mangle_count from int to long.
 */


#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_STRINGS_H
#  include <strings.h>
#endif

#include "popper.h"
#include "mmangle/mime.h"
#include "mmangle/mangle.h"

void 
mangle_count ( void *mstate, char *buf, long len )
{
    char *p;

    *(long *)mstate += len;

    for ( p = buf; len > 0; p++, len-- )
        if ( *p == '\n' )
            (*(long *)mstate)++;

}



/* 
 *  list:   List the contents of a POP maildrop
 */

int 
pop_list (p)
POP     * p;
{
    MsgInfoList         *   mp;         /*  Pointer to message info list */
    register int            i;
    register int            msg_num;
    int                     show_mangled_length = 0;
    unsigned long           mangled_length      = 0;
    char                    buffer [ MAXMSGLINELEN ];
    MimeParsePtr            mimeParse;
    ManglerStateType        mangleState;

    /*  
     * Were arguments provided ? 
     */
    pop_lower ( p->pop_parm[1] );
    if ( p->parm_count == 1 && 
         strncmp ( p->pop_parm[1], "mangle", 6 ) == 0 ) {
        show_mangled_length++;
    } 
    else 
    if ( p->parm_count == 1 && 
         strncmp ( p->pop_parm[1], "x-mangle", 8 ) == 0 ) {
        show_mangled_length++;
    }
    else 
    if ( p->parm_count > 0 ) {        
        /* 
         * First arg must be a message number 
         */
        msg_num = atoi ( p->pop_parm[1] );

        /*  
         * Get a pointer to the message in the message list 
         */
        mp = msg_ptr ( p, msg_num );
        if ( mp == NULL )
            return ( pop_msg ( p, POP_FAILURE, HERE,
                               "Message %d does not exist.", msg_num ) );

        /*  
         * Is the message already flagged for deletion? 
         */
        if ( mp->del_flag )
            return ( pop_msg ( p, POP_FAILURE, HERE,
                               "Message %d has been deleted.", msg_num ) );

        /* 
         * If second arg, it must be "mangle" or "x-mangle" 
         */
        if ( p->parm_count == 2 ) {
            pop_lower ( p->pop_parm[2] );
            if ( ( strncmp ( p->pop_parm[2], "mangle",   6 ) != 0 ) &&
                 ( strncmp ( p->pop_parm[2], "x-mangle", 8 ) != 0 )  )
                return ( pop_msg ( p, POP_FAILURE, HERE,
                                   "Unknown LIST argument: %.128s", 
                                    p->pop_parm[2] ) );
            else 
                show_mangled_length++;
        }
        if ( show_mangled_length ) {
            memset ( (void *)&mangleState, 0, sizeof(ManglerStateType) );
            mangleState.outFn             = mangle_count;
            mangleState.outFnState        = &(mangled_length);
            mangleState.lastWasGoodHeader = 0;
            mangleState.outFnConstructor  = mangleState.outFnDestructor 
                                          = NULL;
            if ( FillMangleInfo ( p->pop_parm[2], 0, &mangleState.rqInfo ) == -1 ) {
                return ( pop_msg ( p, POP_FAILURE, HERE,
                                   "Syntax error in x-mangle" ) );
            }
            mimeParse = MimeInit ( MangleMapper, &mangleState, p->drop );
            fseek ( p->drop, mp->offset, 0 );
            /* 
             * Read the from envelope 
             */
            fgets ( buffer, MAXMSGLINELEN, p->drop );
            MSG_LINES = mp->lines;
            while ( fgets ( buffer, MAXMSGLINELEN, p->drop ) && MSG_LINES-- ) {
                MimeInput ( mimeParse, buffer, strlen(buffer) );
            }
            FreeMangleInfo ( &mangleState.rqInfo );
            MimeFinish ( mimeParse );
        }

        /*  
         * Display message information 
         */
        return ( pop_msg ( p, POP_SUCCESS, HERE, 
                           "%u %lu", 
                           msg_num,
                           ( show_mangled_length ? mangled_length
                                                 : mp->length    ) ) );
    } /* p->parm_count > 0 */

    
    /*  
     * Display the entire list of messages 
     */
    if ( FillMangleInfo ( p->pop_parm[1], 0, &mangleState.rqInfo ) == -1 ) {
        return ( pop_msg ( p, POP_FAILURE, HERE, 
                           "Syntax error in Mangle" ) );
    }
    pop_msg ( p, POP_SUCCESS, HERE,
              "%d visible messages (%ld octets)",
               p->visible_msg_count - p->msgs_deleted,
               p->drop_size - p->bytes_deleted );

    /*  
     * Loop through the message information list.  Skip deleted & hidden messages 
     */
    for ( i = p->msg_count, mp = p->mlp; i > 0; i--, mp++ ) {
        if ( mp->del_flag || mp->hide_flag )
            continue;
        if ( show_mangled_length ) {
            memset ( (void *)&mangleState, 0, sizeof(ManglerStateType) );
            mangled_length                = 0;
            mangleState.outFn             = mangle_count;
            mangleState.outFnState        = &(mangled_length);
            mangleState.lastWasGoodHeader = 0;
            mangleState.outFnConstructor  = mangleState.outFnDestructor 
                                          = NULL;
            FillMangleInfo ( p->pop_parm[1], 0, &mangleState.rqInfo );
            mimeParse = MimeInit ( MangleMapper, &mangleState, p->drop );
            fseek ( p->drop, mp->offset, 0 );
            /* 
             * Read the from envelope 
             */
            fgets ( buffer, MAXMSGLINELEN, p->drop );
            MSG_LINES = mp->lines;
            while ( fgets ( buffer, MAXMSGLINELEN, p->drop ) && MSG_LINES-- ) {
                MimeInput ( mimeParse, buffer, strlen(buffer) );
            }
            FreeMangleInfo ( &mangleState.rqInfo );
            MimeFinish ( mimeParse );
        } /* show_mangled_length */

        pop_write_fmt ( p, "%u %lu\r\n", 
                        mp->visible_num,
                        (show_mangled_length ? mangled_length
                                             : mp->length ) );
    } /* for loop */

    /*  
     * "." signals the end of a multi-line transmission 
     */
    POP_WRITE_LIT   ( p, ".\r\n" );
    pop_write_flush ( p );

    return ( POP_SUCCESS );
}
