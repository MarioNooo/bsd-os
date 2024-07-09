/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file license.txt specifies the terms for use, modification, or
 * redistribution.
 *
 */

/*
 * Revisions:
 *
 *   10/13/00  [rcg]
 *       - Fitted LGL's TLS changes.
 *
 *   05/23/00  [rcg]
 *       - Fixed case where buffer passed to pop_msg without a format
 *         string.
 *       - EUIDL no longer includes extra newline for single message.
 *
 *   03/14/00  [rcg]
 *       - Handle hidden messages in pop_euidl().
 *       - Minor syntactical tweaks.
 *
 *   03/01/00  [rcg]
 *       - Handle case of hidden first message.
 *
 *   12/03/99  [rcg]:
 *       - 'UIDL x' sent an incorrect UID string if the UID contained 
 *          the '%' character, while 'UIDL' worked correctly.  This is
 *          because 'UIDL x' sends the response using pop_msg, and used
 *          the UID string as the format string.  Now it uses "%s" as
 *          the format string, and the UID string as a parameter.
 *
 */

#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <ctype.h>
#include <string.h>

#if HAVE_STRINGS_H
#  include <strings.h>
#endif

#ifndef HAVE_INDEX
#  define index(s,c)             strchr(s,c)
#endif

#include "popper.h"


/*
 *  uidl:   POP UIDL function to list messages by message-ids
 */

int
pop_uidl (p)
POP     *   p;
{
    char                    buffer [ MAXLINELEN ];     /*  Read buffer */
    char                   *nl;
    MsgInfoList            *mp;         /*  Pointer to message info list */
    int                     msg_id = 0;
    int                     x;
    int                     len    = 0;

    if ( p->parm_count == 1 ) {
        len = strlen ( p->pop_parm[1] );

        /*  
         * Convert the parameter into an integer 
         */
        msg_id = atoi ( p->pop_parm[1] );
    }

    /*  
     * Is requested message out of range? 
     */
    if ( len > 0 ) {
        if ( msg_id == 0 )
            return ( pop_msg ( p, POP_FAILURE, HERE,
                               "Parameter must be a number between 1 and %d", 
                               p->msg_count ) );

        /*  
         * Get a pointer to the message in the message list 
         */
        mp = msg_ptr ( p, msg_id );
        if ( mp == NULL )
            return ( pop_msg ( p, POP_FAILURE, HERE, 
                               "Message %d out of range.", msg_id ) );

        if ( mp->del_flag )
            return ( pop_msg ( p, POP_FAILURE, HERE,
                               "Message %d has been marked for deletion.",
                               msg_id ) );

        sprintf ( buffer, "%d %s", msg_id, mp->uidl_str );
        nl = index ( buffer, NEWLINE );
        if ( nl != NULL ) 
            *nl = 0;
        return ( pop_msg ( p, POP_SUCCESS, HERE, "%s", buffer ) );
    } /* msg number specified */
    else { /* yes, we can do this */
        pop_msg ( p, POP_SUCCESS, HERE, "uidl command accepted." );

        for ( x = 1; x <= p->msg_count; x++ )
        {
            /*  
             * Get a pointer to the message in the message list 
             */
            mp = msg_ptr ( p, x );
            if ( mp == NULL )
                continue;

            /*  
             * Is the message flagged for deletion? 
             */
            if ( mp->del_flag )
                continue;

            sprintf ( buffer, "%d %s", 
                      mp->visible_num, 
                      mp->uidl_str );
            pop_sendline ( p, buffer );
        } /* for loop */

    /*  
     * "." signals the end of a multi-line transmission 
     */
    POP_WRITE_LIT   ( p, ".\r\n" );
    pop_write_flush ( p );
    } /* do all messages */

    return ( POP_SUCCESS );
}

/*
 *  euidl:   POP EUIDL function to list messages by message-ids and adds
 *           message size and From: header text as well.  This is to help
 *           the Newton do some pre-filtering before downloading messages.
 */

char *
from_hdr ( p, mp, buf, len )
     POP         *p;
     MsgInfoList *mp;
     char        *buf;
     size_t       len;
{
  char *cp;
  char *nl;

    fseek ( p->drop, mp->offset, 0 );
    while ( fgets ( buf, len, p->drop ) != NULL ) {
      if ( buf[0] == '\n' ) 
        break;    /* From header not found */
      if ( !strncasecmp ( "From:", buf, 5 ) ) {
        cp = index ( buf, ':' );
        while ( *++cp && ( *cp == ' ' || *cp == '\t' ) );
        nl = index ( cp, NEWLINE );
        if ( nl != NULL ) 
            *nl = 0;
        return ( cp );
      }
    }
    return ( "" );
}

int
pop_euidl (p)
POP     *   p;
{
    char                    buffer [ MAXLINELEN ];     /*  Read buffer */
    char                    fromln [ MAXLINELEN ];     /*  Holds 'From:' line */
    char                   *nl;
    MsgInfoList            *mp;            /*  Pointer to message info list */
    int                     msg_id = 0;
    int                     x;
    int                     len    = 0;

    if ( p->parm_count == 1 ) {
        len = strlen ( p->pop_parm[1] );

        /*  
         * Convert the parameter into an integer 
         */
        msg_id = atoi ( p->pop_parm[1] );
    }

    /*  
     * Is requested message out of range? 
     */
    if ( len > 0 && msg_id == 0 ) {
        return ( pop_msg ( p, POP_FAILURE, HERE,
                           "Parameter must be a number (range 1 to %d)", 
                           p->visible_msg_count ) );
    }

    if ( len > 0 && ( msg_id < 1 || msg_id > p->visible_msg_count ) ) {
        return ( pop_msg ( p, POP_FAILURE, HERE,
                           "Message out of range.  %d visible messages in mail drop.",
                           p->visible_msg_count ) );
    }

    if ( msg_id > 0 ) {
        /*  
         * Get a pointer to the message in the message list 
         */
        mp = msg_ptr ( p, msg_id );
        if ( mp == NULL ) {
            return ( pop_msg ( p, POP_FAILURE, HERE,
                               "Message %d out of range", msg_id ) );
        } /* bad msg */

        if ( mp->del_flag ) {
            return ( pop_msg ( p, POP_FAILURE, HERE,
                               "Message %d has been marked for deletion.",
                               msg_id ) );
        } /* deleted */
      else {
          /*
           * We have a valid message.
           */
          sprintf ( buffer, "%d %s", msg_id, mp->uidl_str );
          nl = index ( buffer, NEWLINE );
          if ( nl != NULL )
              *nl = 0;
          return  ( pop_msg ( p, POP_SUCCESS, HERE, "%s %ld %.128s",
                    buffer, mp->length, 
                    from_hdr ( p, mp, fromln, sizeof(fromln) ) ) );
        } /* valid single message */
    } /* single message */
    else {
        /* 
         * yes, we can do this 
         */
        pop_msg ( p, POP_SUCCESS, HERE, "uidl command accepted." );

        for ( x = 1; x <= p->msg_count; x++ )
        {
            /*  
             * Get a pointer to the message in the message list 
             */
            mp = msg_ptr ( p, x );
            if ( mp == NULL )
                continue;

            /*  
             * Is the message flagged for deletion? 
             */
            if ( mp->del_flag ) 
                continue;

            sprintf ( buffer, "%d %s", x, mp->uidl_str );
            nl = index ( buffer, NEWLINE );
            if ( nl != NULL ) 
                *nl = 0;       
            sprintf ( buffer, "%s %ld %.128s\n", 
                      buffer, mp->length, 
                      from_hdr ( p, mp, fromln, sizeof(fromln) ) );
            pop_sendline ( p, buffer );
        } /* for loop */
    } /* all messages */

    /*  
     * "." signals the end of a multi-line transmission 
     */
    pop_write_line  ( p, "." );
    pop_write_flush ( p );

    return ( POP_SUCCESS );
}

