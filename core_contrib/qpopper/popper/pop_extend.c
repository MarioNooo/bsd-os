/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 *
 *
 * File: pop_extend.c 
 *
 * Revisions:
 *    03/12/03 [RCG]
 *            - Free allocated memory in more MDEF error cases.
 *            - Impose maximum size on MDEF name.
 *            - Don't use free'd memory in pop_msg call (reported
 *              by Florian Heinz).
 *
 *    09/17/02 [RCG]
 *            - pop_stls now return POP_SUCCESS (even though it
 *              generates a POP_FAILURE message) except for a 
 *              timeout, so that the state machine stays in auth1
 *              and doesn't go into halt (thus these errors are
 *              not fatal).
 *
 *    01/15/01 [RCG]
 *            - login_delay and expire now in p.
 *            - bShy now in p.
 *
 *    12/21/00 [RCG]
 *            - Handle tls_support instead of stls / alternate_port.
 *
 *    11/20/00 [RCG]
 *            - Inactivity timeout for STLS aborts.
 *
 *    10/12/00 [RCG}
 *            - Fitted LGL's TLS changes.
 *
 *    06/05/00 [RCG]
 *            - CAPA IMPLEMENTATION tag omitted if SHY and not 
 *              authenticated.
 *
 *    02/09/00 [RCG]
 *            - Added AUTH-RESP-CODE.
 *
 *    03/18/98 [PY]
 *            -  File added.
 * 
 */


#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <string.h>

#include "popper.h"

#ifndef HAVE_BCOPY
#  define bcopy(src,dest,len)   (void) (memcpy(dest,src,len))
#  define bzero(dest,len)       (void) (memset(dest, (char)NULL, len))
#  define bcmp(b1,b2,n)         memcmp(b1,b2,n)
#endif /* not HAVE_BCOPY */

#ifndef HAVE_INDEX
#  define index(s,c)             strchr(s,c)
#  define rindex(s,c)            strrchr(s,c)
#endif /* not HAVE_INDEX */


extern volatile BOOL poptimeout;


int 
pop_capa ( POP *p )
{
    POP_WRITE_LIT  ( p, "+OK Capability list follows\r\n" );
    POP_WRITE_LIT  ( p, "TOP\r\n"                         );

/*
    POP_WRITE_LIT  ( p, "PIPELINING\r\n",                 );
*/
    
# if defined(SCRAM)
    POP_WRITE_LIT  ( p, "SASL SCRAM-MD5\r\n",             );
# endif

# if !defined(APOP_ONLY) && !defined(SCRAM_ONLY)
    if ( p->AllowClearText != ClearTextNever )
        POP_WRITE_LIT ( p, "USER\r\n" );
#endif

    if ( p->login_delay != -1 )
        pop_write_fmt ( p, "LOGIN-DELAY %d\r\n", p->login_delay );
    if ( p->expire == -1 ) 
        POP_WRITE_LIT ( p, "EXPIRE NEVER\r\n" );
    else
        pop_write_fmt ( p, "EXPIRE %d\r\n", p->expire );

    POP_WRITE_LIT  ( p, "UIDL\r\n"                        );
    POP_WRITE_LIT  ( p, "RESP-CODES\r\n"                  );
    POP_WRITE_LIT  ( p, "AUTH-RESP-CODE\r\n"              );

    POP_WRITE_LIT  ( p, "X-MANGLE\r\n"                    );
    POP_WRITE_LIT  ( p, "X-MACRO\r\n"                     );

    POP_WRITE_LIT  ( p, "X-LOCALTIME "                    );
    pop_write_line ( p, get_time()                        );

    if ( p->tls_support == QPOP_TLS_STLS )
        POP_WRITE_LIT ( p, "STLS\r\n" );

    if ( p->bShy == FALSE || p->CurrentState == trans ) {
        pop_write_fmt ( p, 
                        "IMPLEMENTATION %s%.*s%s-version-%s\r\n",
                        QPOP_NAME,
                        (strlen(IMPLEMENTSFX)>0 ? 1 : 0), "-",
                        IMPLEMENTSFX, VERSION );
    }

    pop_write_line  ( p, "." );
    
    pop_write_flush ( p );
    return ( POP_SUCCESS );
}


MDEFRecordType mdrArr [ MAX_MDEF_INDEX ];
int last_index = -1;


int 
pop_mdef ( POP *p )
{
    int len             = 0;
    int current_index   = 0;
    char *char_ptr      = NULL;
    char *pt            = NULL;
    MDEFRecordType local_element;
    
    
    pop_lower ( p->pop_parm[1] );
    len = strlen ( p->pop_parm[1] );
    if ( len > MAX_MDEF_NAME )
        return pop_msg ( p, POP_FAILURE, HERE, "MDEF name too long" );

    char_ptr = strdup ( p->pop_parm[1] );
    if ( char_ptr == NULL )
        return pop_msg ( p, POP_FAILURE, HERE,
                         "[SYS/TEMP] internal error" );

    pt = strchr ( char_ptr,'(' );
    if ( !pt ) {
        free ( char_ptr );
        return pop_msg ( p, POP_FAILURE, HERE, "Syntax error" );
    }

    *pt = '\0';
    local_element.mdef_macro = char_ptr;
    char_ptr = pt + 1;
    pt = strrchr ( char_ptr,')' );
    if ( !pt ) {
        free ( local_element.mdef_macro );
        return pop_msg ( p, POP_FAILURE, HERE, "Syntax error" );
    }

    *pt = '\0';
    local_element.mdef_value = char_ptr;
    for ( current_index = 0;
          current_index <= last_index;
          current_index++ )
        if ( strcmp ( mdrArr[current_index].mdef_macro, 
                      local_element.mdef_macro ) == 0 ) {
            if ( strcmp ( mdrArr[current_index].mdef_value,
                          local_element.mdef_value ) == 0 ) { 
                free ( local_element.mdef_macro ); /* From strdup */
                return pop_msg ( p, POP_SUCCESS, HERE,
                                 "Macro \"%s\" accepted", 
                                 mdrArr[current_index].mdef_value );
            }
            break;
        }
    if ( current_index > last_index ) 
        last_index = current_index;
    if ( current_index >= MAX_MDEF_INDEX ) {
        free ( local_element.mdef_macro ); /* From strdup */
        return pop_msg ( p, POP_FAILURE, HERE, 
                         "Server only takes a maximum of %d MDEFs",
                         MAX_MDEF_INDEX);
    }
    mdrArr [ current_index ].mdef_macro = local_element.mdef_macro;
    /* Redundant */
    mdrArr [ current_index ].mdef_value = local_element.mdef_value;
    return ( pop_msg ( p,POP_SUCCESS, HERE, "Macro \"%s\" accepted",
                       local_element.mdef_macro ) );
}




/* 
 *  Initiate a TLS handshake
 */

int
pop_stls ( POP *p )
{
    if ( p->tls_support != QPOP_TLS_STLS )
    {
        pop_msg ( p, POP_FAILURE, HERE, "Command not enabled" );
        return POP_SUCCESS; /* "success" so the error isn't fatal */
    }
    else if ( p->tls_started )
    {
        pop_msg ( p, POP_FAILURE, HERE, "TLS in progress" );
        return POP_SUCCESS; /* "success" so the error isn't fatal */
    }
    else
        pop_msg ( p, POP_SUCCESS, HERE, "STLS" );

    if ( pop_tls_handshake ( p->tls_context ) == 0 )
        p->tls_started = TRUE;

    /*
     * If TLS fails we just go on.  RFC 2595 seems a bit ambiguous
     * about this.  Also we don't redisplay the banner with the APOP
     * cookie.  This doesn't take advantage of a little extra security
     * we could get for APOP by having the cookie integrity checked.
     *
     * Note that if the handshake timed out we abort, just as for any
     * other inactivity timeout.
     */

    DEBUG_LOG1 ( p, "pop_stls returning %d",
                 poptimeout ? POP_FAILURE : POP_SUCCESS );
    return ( poptimeout ? POP_FAILURE : POP_SUCCESS );
}
