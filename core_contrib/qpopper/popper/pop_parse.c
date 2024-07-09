/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 *
 * Revisions:
 *
 *     06/01/01  [rcg]
 *             - Fixed empty password treated as empty command
 *               (patch submitted by Michael Smith and others).
 *
 *     06/20/00  [rcg]
 *             - Fixed compiler warnings.
 *
 */


/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "config.h"

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_STRINGS_H
#  include <strings.h>
#endif

#include "popper.h"

/* 
 *  parse:  Parse a raw input line from a POP client 
 *  into null-delimited tokens
 */

int
pop_parse ( p, buf )
POP         *   p;
char        *   buf;        /*  Pointer to a message containing 
                                the line from the client */
{
    char            *   mp;
    register int        i;
    
    /*
     * Loop through the POP command array 
     */
    for (mp = buf, i = 0; ; i++) {
    
        /*
         * Skip leading spaces and tabs in the message 
         */
        while ( isspace ( (int) *mp ) )
            mp++;

        /*
         * Are we at the end of the message? 
         */
        if ( *mp == 0 )
            break;

        /*
         * Have we already obtained the maximum allowable parameters? 
         */
        if ( i >= MAXPARMCOUNT ) {
            pop_msg ( p, POP_FAILURE, HERE, "Too many arguments supplied." );
            return ( -1 );
        }

        /*
         * Point to the start of the token 
         */
        p->pop_parm[i] = mp;

        /*
         * Search for the first space character (end of the token) 
         */
        while ( isspace ( (int) *mp ) == FALSE && *mp != 0 )
            mp++;

        /*
         * Delimit the token with a null 
         */
        if ( *mp != 0 )
            *mp++ = 0;

        if (i == 0) {
            /*
             * Convert the first token (POP command) to lower case
             */
            pop_lower ( p->pop_command );
            /*
             * This is kinda gross.  Passwords have to be parsed diffrently
             * as they may contain spaces.  If you think of a cleaner way,
             * do it.  The "p->pop_command[0] == 'p'" is to save a call to
             * strcmp() on every call to pop_parse();  This parsing keeps
             * leading and trailing speces behind for the password command.
             */
            if ( p->pop_command[0] == 'p' && strcmp(p->pop_command, "pass") == 0 ) {
                if ( *mp != 0 ) {
                    p->pop_parm[1] = mp;
                    if ( strlen(mp) > 0 ) {
                        mp = mp + strlen(mp) - 1;
                        while ( *mp == 0xa || *mp == 0xd )
                            *mp-- = 0;
                    }
                    return ( 1 );
                } else
                    return ( 0 );
            }
        }
        else {
            /*
             * Search in the Mdef Array for the presence of this arg.
             */
            int j;
            extern MDEFRecordType mdrArr[MAX_MDEF_INDEX];
            extern int last_index;

            for( j = 0; j <= last_index; j++ ) {
                if ( strcasecmp(mdrArr[j].mdef_macro, p->pop_parm[i]) == 0 ) {
                    p->pop_parm[i] = mdrArr[j].mdef_value;
                    break;
                }
            }
        }
    }

    /*  Were any parameters passed at all? */
    if (i == 0) return (-1);

    /*  Return the number of tokens extracted minus the command itself */
    return (i-1);
    
}
