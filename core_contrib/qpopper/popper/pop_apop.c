/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The license.txt file specifies the terms for use, 
 * modification, and redistribution.
 *
 * Revisions:
 *
 *     01/29/01  [rcg]
 *             - Added check for "-no-mime".
 *             - Now using p->bDo_timing instead of DO_TIMING.
 *             - Now using p->grp_serv_mode and p->grp_no_serv_mode instead
 *               of SERVER_MODE_GROUP_INCLUDE and SERVER_MODE_GROUP_EXCLUDE.
 *             - Now using p->authfile instead of AUTHFILE.
 *
 *     01/18/01  [rcg]
 *             - pwnam, run-time vs compile time, etc. changes.
 *
 *     11/14/00  [rcg]
 *             - Trim domain name only if requested.
 *
 *     10/02/00  [rcg]
 *             - Now calling check_config_files to check qpopper-options files.
 *
 *     09/09/00  [rcg]
 *             - Now stripping domain name from user name.
 *
 *     07/14/00  [rcg]
 *             - Fixed crash on APOP command with no APOP database.
 *
 *     06/30/00  [rcg]
 *             - Added support for ~/.qpopper-options.
 *
 *     06/16/00  [rcg]
 *              - Now setting server mode on or off based on group
 *                membership.
 *
 *     06/10/00  [rcg]
 *              - Don't use datum.dptr after dbm_close.
 *
 *     06/05/00  [rcg]
 *              - Added drac call.
 *
 *     03/07/00  [rcg]
 *              - Updated authentication OK message to account for hidden
 *                messages.
 *
 *     02/09/00  [rcg]
 *              - Added extended response codes.
 *              - Added log entry if LOG_LOGIN defined.
 */

/*
 * APOP authentication, derived from MH 6.8.3
 */

#include <sys/types.h>
#include "config.h"
#include "md5.h"

#include <time.h>


#ifdef APOP

#include <sys/stat.h>
#include <stdio.h>

#ifdef GDBM
#  include <gdbm.h>
#else /* not GDBM */
#  include <ndbm.h>
#endif /* GDBM */

#include <fcntl.h>
#include <pwd.h>

#if HAVE_STRINGS_H
#  include <strings.h>
#endif /* HAVE_STRINGS_H */

#if HAVE_SYS_FILE_H
# include <sys/file.h>
#endif /* HAVE_SYS_FILE_H */

#if HAVE_UNISTD_H
#  include <unistd.h>
#endif

#if HAVE_SYS_UNISTD_H
#  include <sys/unistd.h>
#endif

#include <string.h>
#include <flock.h>
#include <stdlib.h>

#include "popper.h"
#include "snprintf.h"
#include "string_util.h"


/*
 * Obscure password so a cleartext search doesn't come up with
 * something interesting.  Also de-obscures an already obscured
 * string.
 *
 */

char *
obscure ( string )
char *string;
{
    unsigned char *cp, *newstr;

    cp = newstr = (unsigned char *)strdup(string);

    while ( *cp )
       *cp++ ^= 0xff;

    return ( (char *)newstr );
}


extern char    *ERRMSG_PW;
extern char    *ERRMSG_ACEXP;
extern char    *ERRMSG_PWEXP;
extern char    *ERRMSG_AUTH;


int 
pop_apop ( p )
POP *p;
{
    register char *cp;
    char    buffer[BUFSIZ];
    register unsigned char *dp;
    unsigned char *ep,
                   digest[16];
    struct passwd *pw;
    int            j = 0;
    size_t         user_name_len = 0;

#ifdef GDBM
    char apop_file[BUFSIZ];
#else /* not GDBM */
    char apop_dir[BUFSIZ];
#endif /* GDBM */

    struct stat  st;
    datum        key,
                 ddatum;
    char        *apop_str = NULL;

#ifdef GDBM
    GDBM_FILE db;
#else /* not GDBM */
    DBM    *db;
#endif /* GDBM */

    MD5_CTX mdContext;
    int f;
    time_t my_timer = time(0);

#ifdef SCRAM_ONLY
    return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                             "[AUTH] You must use SCRAM-MD5 authentication "
                             "to connect to this server" ) );
#endif /* SCRAM_ONLY */

    /* 
     * Downcase user name if requested 
     */
    if ( p->bDowncase_user )
        downcase_uname ( p, p->pop_parm[1] );

    /* 
     * strip domain name if requested
     */
    if ( p->bTrim_domain )
        trim_domain ( p, p->pop_parm[1] );

    /*
     * Save the user name 
     */
    user_name_len = strlen ( p->pop_parm[1] );
    
    if ( user_name_len > 8 
         && !strcmp(p->pop_parm[1] + ( user_name_len - 8 ), "-no-mime" ) ) {
        /* turn on mangling! */
        p->bNo_mime     = TRUE;
        user_name_len  -= 8;
        DEBUG_LOG4 ( p, "Turned on MIME mangling; "
                        "user name entered was (%u)'%.100s'; "
                        "user name reset to (%u)'%.100s'",
                     strlen(p->pop_parm[1]),   p->pop_parm[1],
                     (unsigned) user_name_len, p->pop_parm[1] );
      }
    
    if ( user_name_len > ( sizeof(p->user) -1 ) ) {
        pop_log ( p, POP_WARNING, HERE, "Excessive user name length (%lu); "
                                  "truncated to %lu: %.100s",
                  (long unsigned) user_name_len, 
                  (long unsigned) (sizeof(p->user) -1), 
                  p->pop_parm[1] );
        user_name_len = sizeof(p->user) -1;
    }
    
    strlcpy ( p->user,   p->pop_parm[1], sizeof(p->user) );

#ifdef    SCRAM
    strlcpy ( p->authid, p->user, sizeof(p->authid) );  /* userid is also authentication id */
#endif /* SCRAM */

    pop_log ( p, POP_INFO, HERE, "apop \"%s\"", p->user );

    if ( p->authfile != NULL && checkauthfile ( p ) != 0 ) {
        return pop_msg ( p, POP_FAILURE, HERE, "[AUTH] Permission denied." );
    }

    if ( p->nonauthfile != NULL && checknonauthfile ( p ) != 0 ) {
        return pop_msg ( p, POP_FAILURE, HERE, "[AUTH] Permission denied." );
    }

    /*
     * Cache passwd struct for use later; this memory gets freed at the end
     * of the session.
     */
    pw = getpwnam ( p->user );               /* get pointer to info */
    if ( pw != NULL ) {
        p->pw        = *pw;                  /* copy it */
        p->pw.pw_dir = strdup(pw->pw_dir);   /* copy home directory */
        DEBUG_LOG2 ( p, "home (%d): '%s'",
                     strlen(p->pw.pw_dir), p->pw.pw_dir );
    }
    if ( ( pw == NULL ) || (  pw->pw_passwd == NULL )
                        || ( *pw->pw_passwd == '\0' ) ) {
        return ( pop_auth_fail ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
    }

    if ( pw->pw_uid == 0 )
        return ( pop_auth_fail ( p, POP_FAILURE, HERE, ERRMSG_AUTH, p->user ) );

#ifdef GDBM
    db = gdbm_open ( APOP, 512, GDBM_READER, 0, 0 );
#else
    db = dbm_open  ( APOP, O_RDONLY, 0 );
#endif
    if ( db == NULL )
        return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                 "[SYS/TEMP] POP authentication DB not "
                                 "available (user %s): %s (%d)",
                                 p->user, strerror(errno), errno ) );

#ifdef GDBM
    strlcpy ( apop_file, APOP, sizeof(apop_file) - 1 );
    j = stat ( apop_file, &st  );
    if ( ( j != -1 )
         && ( ( st.st_mode & 0777 ) != 0600 ) ) {
        gdbm_close ( db );
        DEBUG_LOG4 ( p, "stat returned %d; %s mode is %o (%x)",
                     j, apop_file, (int) st.st_mode, (int) st.st_mode );
        return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                 "[SYS/PERM] POP authentication DB "
                                 "has wrong mode (%.3o)",
                                 (unsigned int) st.st_mode & 0777 ) );
    }
    f = open ( apop_file, O_RDONLY );
    if ( f == -1 ) {
        int e = errno;
        gdbm_close ( db );
        return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                 "[SYS/TEMP] unable to lock POP "
                                 "authentication DB: %s (%d)",
                                 strerror(e), e ) );
    }
#else  
    strlcpy ( apop_dir, APOP, sizeof(apop_dir) - 5 );
#  if defined(BSD44_DBM)
    strcat ( apop_dir, ".db" );
#  else
    strcat ( apop_dir, ".dir" );
#  endif /* BSD44_DBM */
    j = stat ( apop_dir, &st );
    if ( ( j != -1 )
         && ( ( st.st_mode & 0777 ) != 0600 ) ) {
        dbm_close ( db );
        DEBUG_LOG4 ( p, "stat returned %d; %s mode is %o (%x)",
                     j, apop_dir, st.st_mode, st.st_mode );
        return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                 "[SYS/PERM] POP authentication DB has "
                                 "wrong mode (%.3o)",
                                 st.st_mode & 0777 ) );
    }
    f = open ( apop_dir, O_RDONLY );
    if ( f == -1 ) {
        int e = errno;
        dbm_close ( db );
        return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                 "[SYS/TEMP] unable to lock POP authentication DB: %s (%d)",
                                 strerror(e), e ) );
    }
#endif 
    if ( flock ( f, LOCK_SH ) == -1 ) {
        int e = errno;
        close ( f );
#ifdef GDBM
        gdbm_close ( db );
#else
        dbm_close  ( db );
#endif
        return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                 "[SYS/TEMP] unable to lock POP authentication DB: %s (%d)",
                                 strerror(e), e ) );
    }
    key.dsize = strlen  ( key.dptr = p->user ) + 1;
#ifdef GDBM
    ddatum = gdbm_fetch ( db, key );
#else
    ddatum = dbm_fetch  ( db, key );
#endif
    if ( ddatum.dptr == NULL ) {
        close ( f );
#ifdef GDBM
        gdbm_close ( db );
#else
        dbm_close  ( db );
#endif
        DEBUG_LOG1 ( p, "User %.128s not in popauth db", p->user );
        return ( pop_auth_fail ( p, POP_FAILURE, HERE, 
                                 "[AUTH] not authenticated" ) );
    }

    /*
     * (n)dbm invalidates the datum pointer after dbm_close (gdbm
     * returns a copy), so we copy and de-obscure it before the close.
     */
    apop_str = obscure ( ddatum.dptr );

#ifdef GDBM
    free ( ddatum.dptr );
    gdbm_close ( db );
#else
    dbm_close  ( db );
#endif
    close ( f );

    MD5Init   ( &mdContext );
    MD5Update ( &mdContext, (unsigned char *) p->md5str, strlen(p->md5str) );
    MD5Update ( &mdContext, (unsigned char *) apop_str,  strlen(apop_str)  );
    MD5Final  ( digest, &mdContext );

    cp = buffer;
    for (ep = (dp = digest) + sizeof digest / sizeof digest[0];
             dp < ep;
             cp += 2)
        (void) sprintf (cp, "%02x", *dp++ & 0xff);
    *cp = '\0';

#ifdef TRACE_APOP_KEYS
    {
    DEBUG_LOG2 ( p, "AUTH KEYS : %s to %s", apop_str, buffer );
    }
#endif /* TRACE_APOP_KEYS */

    free ( apop_str );
    apop_str = NULL;

    if ( strcmp(p->pop_parm[2], buffer) ) {
        return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                 "[AUTH] authentication failure" ) );
    }

    DEBUG_LOG1 ( p, "APOP authentication ok for \"%s\"", p->user );

    if ( p->bDo_timing )
        p->login_time = time(0) - my_timer;

    /*
     * Check if server mode should be set or reset based on group membership.
     */

    if ( p->grp_serv_mode != NULL
         && check_group ( p, pw, p->grp_serv_mode ) ) {
        p->server_mode = TRUE;
        DEBUG_LOG2 ( p, "Set server mode for user %s; "
                        "member of \"%.128s\" group",
                     p->user, p->grp_serv_mode );
    }

    if ( p->grp_no_serv_mode != NULL
         && check_group ( p, pw, p->grp_no_serv_mode ) ) {
        p->server_mode = FALSE;
        DEBUG_LOG2 ( p, "Set server mode OFF for user %s; "
                        "member of \"%.128s\" group",
                     p->user, p->grp_no_serv_mode );
    }

    /*
     * Process qpopper-options files, if requested and present.
     */
    check_config_files ( p, pw );

    /*  
     * Make a temporary copy of the user's maildrop 
     *
     * and set the group and user id 
     */
    if ( pop_dropcopy ( p, pw ) != POP_SUCCESS ) 
        return ( POP_FAILURE );

    /*  
     * Initialize the last-message-accessed number 
     */
    p->last_msg = 0;

    p->AuthType  = apop;      /* authentication method is APOP       */
    p->AuthState = apopcmd;   /* authenticated successfully via APOP */
    /*  
     * Authorization completed successfully 
     */
    if ( p->pLog_login != NULL )
        do_log_login ( p );


#ifdef DRAC_AUTH
    drac_it ( p );
#endif /* DRAC_AUTH */

    return ( pop_msg ( p, POP_SUCCESS, HERE,
                       "%s has %d visible message%s (%d hidden) in %ld octets.",
                        p->user,
                        p->visible_msg_count,
                        p->visible_msg_count == 1 ? "" : "s",
                        (p->msg_count - p->visible_msg_count),
                        p->drop_size ) );
}

#endif /* APOP */

