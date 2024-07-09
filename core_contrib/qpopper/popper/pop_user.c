/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 *
 * Revisions:
 *
 *     03/12/03  [rcg]
 *             - Fixed '-no-mime' appended to user name (reported
 *               by Florian Heinz).
 *
 *     07/25/01  [rcg]
 *             - Enforce ClearTextPassword even without APOP.
 *
 *     06/01/01  [rcg]
 *             - Fix buffer overflow (broken in 4.0b14).
 *
 *     02/14/01  [rcg]
 *             - Fixed problems compiling with Kerberos 5.
 *
 *     01/18/01  [rcg]
 *             - pwnam, run-time vs compile time, etc. changes.
 *
 *     11/14/00  [rcg]
 *             - Enforcing ClearTextTLS.
 *             - Only trim domain name if requested.
 *
 *     09/09/00  [rcg]
 *             - trimming domain name from user name.
 *
 *     06/08/00  [rcg]
 *             - No longer accessing datum ptr after closing db.
 *             - Cast value.dptr to char * when dereferencing to
 *               avoid errors on IRIX, using patch from Rick Troxel.
 *
 *     06/05/00  [rcg]
 *              - Applied patches from Jeffrey C. Honig at BSDI
 *                skipping the check for the principal name being
 *                the same as the specified user name when KUSEROK
 *                is defined.  KUSEROK does all the necessary 
 *                validation.  It's more work for the client to figure
 *                out what the principal name in the ticket is and it
 *                doesn't really matter since this userid is ignored.
 *                Also adds BSD/OS authentication support
 *
 *              - Added clear text handling options when APOP or SCRAM
 *                available.
 *
 *     02/09/00  [rcg]
 *              - Added extended response codes.
 */

/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <pwd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

#if HAVE_STRINGS_H
#  include <strings.h>
#endif /* HAVE_STRINGS_H */

#if HAVE_UNISTD_H
#  include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if HAVE_SYS_UNISTD_H
#  include <sys/unistd.h>
#endif /* HAVE_SYS_UNISTD_H */

#include "flock.h"

#ifndef HAVE_INDEX
#  define index(s, c)   strchr(s, c)
#endif /* HAVE_INDEX */

#if  HAVE_FCNTL_H
#  include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#if HAVE_SYS_FILE_H
#  include <sys/file.h>
#endif /* HAVE_SYS_FILE_H */

#ifdef GDBM
#  include <gdbm.h>
#else
#  if HAVE_NDBM_H
#    include <ndbm.h>
#  endif /* HAVE_NDBM_H */
#endif /* GDBM */

#include "popper.h"
#include "string_util.h"

/* 
 * When AUTHON is defined, SCRAM and/or APOP authentication is available.
 */
#ifdef   SCRAM
#  define  AUTHON
#  define  AUTHDB SCRAM
#else /* not SCRAM */
#  ifdef   APOP
#    define  AUTHON
#    define  AUTHDB APOP
#  endif /* APOP */
#endif /* SCRAM */


extern char    *ERRMSG_PW;


/* 
 *  user:   Prompt for the user name at the start of a POP session
 */

int 
pop_user (p)
POP     * p;
{
    /* 
     * If there is an APOP or SCRAM database entry for this user, then we
     * don't allow a cleartext password unless permitted by p->AllowClearText.
     */
#ifdef AUTHON
#  ifdef GDBM
    char        authdb_file[BUFSIZ];
    GDBM_FILE   db;
#  else /* not GDBM */
    char        auth_dir[BUFSIZ];
    DBM        *db;
#  endif /* GDBM */
    int         fid;
    struct      stat st;
    datum       key, value;
    int         i;
    int         bFoundUser = FALSE;
#endif /* AUTHON */

    size_t      user_name_len = 0;
    struct      passwd *pw    = NULL;

#  ifdef KRB4
    extern AUTH_DAT kdata;
#  endif /* KRB4 */


    /* 
     * Downcase user name if requested 
     */
    if ( p->bDowncase_user )
        downcase_uname ( p, p->pop_parm[1] );
    
    /*
     * Trim domain name
     */
    if ( p->bTrim_domain )
        trim_domain ( p, p->pop_parm[1] );
        
#if defined(KERBEROS) && defined(KRB4) && !defined(KUSEROK)
    if ( p->bKerberos && strcmp(p->pop_parm[1], p->user) ) {
        pop_log ( p, POP_WARNING, HERE, 
                  "%s: auth failed: %s.%s@@%s vs %s",
                  p->client, kdata.pname, kdata.pinst, kdata.prealm, 
                  p->pop_parm[1] );
        return ( pop_msg ( p, POP_FAILURE, HERE,
                           "[AUTH] Wrong username supplied (%s vs. %s).", 
                           p->user, p->pop_parm[1] ) );
    } /* user name mismatch */
#endif /* KERBEROS */

    /*
     * Save the user name 
     */
    user_name_len = strlen ( p->pop_parm[1] );
    
    if ( user_name_len > 8 
         && !strcmp(p->pop_parm[1] + ( user_name_len - 8 ), "-no-mime" ) ) {
        /* turn on mangling! */
        p->bNo_mime     = TRUE;
        user_name_len  -= 8;
        DEBUG_LOG5 ( p, "Turned on MIME mangling; "
                        "user name entered was (%u)'%.100s'; "
                        "user name reset to (%u)'%.*s'",
                     strlen(p->pop_parm[1]),   p->pop_parm[1],
                     (unsigned) user_name_len,
                     user_name_len,            p->pop_parm[1] );
        *(p->pop_parm[1] + user_name_len) = '\0';
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
    
#ifdef SCRAM_ONLY
    return ( pop_auth_fail ( p, POP_FAILURE, HERE, 
                             "[AUTH] You must use SCRAM-MD5 authentication "
                             "to connect to this server" ) );
#else /* not SCRAM_ONLY */
#  ifdef APOP_ONLY
    return ( pop_auth_fail ( p, POP_FAILURE, HERE, "[AUTH] You must use APOP "
                             "authentication to connect to this server" ) );
#  endif /* APOP_ONLY */
#endif /* SCRAM_ONLY */


#ifdef AUTHON

    if ( p->AllowClearText != ClearTextAlways ) {
        DEBUG_LOG0 ( p, "APOP; AllowClearText != ClearTextAlways; checking if "
                        "user exists in APOP db" );

        /*
         * If this call fails then the database is not accessable (doesn't
         * exist?) in which case we can ignore an APOP user trying to
         * access Qpopper with a cleartext password.
         */

        pw = &p->pw;
        if ( ( pw != NULL ) &&
#ifdef GDBM
             ( ( db = gdbm_open ( AUTHDB, 512, GDBM_READER, 0, 0 ) ) != NULL )
#else
             ( ( db = dbm_open  ( AUTHDB, O_RDONLY, 0 ) ) != NULL )
#endif
           ) {

#ifdef GDBM
            strncpy ( authdb_file, AUTHDB, sizeof(authdb_file) - 1 );
            authdb_file [ sizeof(authdb_file) - 1 ] = '\0';
#else
            strncpy ( auth_dir, AUTHDB, sizeof(auth_dir) - 5 );
# if defined(BSD44_DBM)
            strcat ( auth_dir, ".db" );
# else
            strcat ( auth_dir, ".dir" );
# endif
#endif
#ifdef GDBM
            if ( stat (authdb_file, &st) != -1 && (st.st_mode & 0777) != 0600 ) 
#else
            if ( stat (auth_dir, &st)    != -1 && (st.st_mode & 0777) != 0600 ) 
#endif
            {
#ifdef GDBM
                gdbm_close (db);
#else
                dbm_close (db);
#endif
                return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                         "[SYS/PERM] POP authentication DB has wrong "
                                         "mode (0%o)",
                                         (unsigned int) st.st_mode & 0777 ) );
            }
#ifdef GDBM
            fid = open ( authdb_file, O_RDONLY );
#else
            fid = open ( auth_dir, O_RDONLY );
#endif
            if ( fid == -1 ) {
                int e = errno;
#ifdef GDBM
                gdbm_close ( db );
#else
                dbm_close (db);
#endif
                return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                         "[SYS/TEMP] unable to lock POP "
                                         "authentication DB (%s)", 
                                        strerror(e) ) );
            }
            if ( flock ( fid, LOCK_SH ) == -1 ) {
                int e = errno;
                close ( fid );
#ifdef GDBM
                gdbm_close (db);
#else
                dbm_close (db);
#endif 
                return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                         "[SYS/TEMP] unable to lock POP "
                                         "authentication DB (%s)", 
                                         strerror(e) ) );
            }
            key.dsize = strlen (key.dptr = p->user) + 1;
#ifdef GDBM
            value = gdbm_fetch (db, key);
            if ( ( value.dptr != NULL ) &&
                 ( ( (char *) value.dptr ) [0] != 0 ||
                   ( value.dsize >= ( i = strlen ( value.dptr ) +2 )
                     && ( (char *) value.dptr ) [i] != 0 ) 
                 ) 
               ) {
                bFoundUser = TRUE;
            }
            gdbm_close(db);
#else
            value = dbm_fetch (db, key);
            if ( ( value.dptr != NULL ) &&
                 ( ( (char *) value.dptr) [0] != 0 ||
                   ( value.dsize >= ( i = strlen ( value.dptr ) +2 )
                     && ( (char *) value.dptr ) [i] != 0 ) 
                 ) 
               ) {
                bFoundUser = TRUE;
            }
            dbm_close (db);
#endif
            close ( fid );

            if ( bFoundUser ) {
                return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                         "[AUTH] You must use stronger "
                                         "authentication such as AUTH "
                                         "or APOP to connect "
                                         "to this server" ) );
            }
        } /* able to open db */
    } /* p->AllowClearText == ClearTextDefault */

#endif /* AUTHON */

    
    if ( ( p->AllowClearText == ClearTextNever )    ||
         ( p->AllowClearText == ClearTextLocal &&
           strncmp ( "127.", p->ipaddr, 4 ) != 0 )
       ) {
        DEBUG_LOG1 ( p, "AllowClearText=%u; (user not local)", 
                     p->AllowClearText );
        return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                 "[AUTH] You must use stronger "
                                 "authentication such as APOP to "
                                 "connect to this server" ) );
    }
    if ( ( p->AllowClearText == ClearTextTLS &&
           p->tls_started == FALSE              )
       ) {
        DEBUG_LOG1 ( p, "AllowClearText=%u; (TLS/SSL not used)", 
                     p->AllowClearText );
        return ( pop_auth_fail ( p, POP_FAILURE, HERE,
                                 "[AUTH] You must use TLS/SSL or "
                                 "stronger authentication such as APOP "
                                 "to connect to this server" ) );
    }

#if     defined(__bsdi__) && _BSDI_VERSION >= 199608
    do {
        int s;
        char *cp;

        if ( p->challenge ) {
            free ( p->challenge );
            p->challenge = NULL;
        }

        p->style = strchr ( p->user, ':' );
        if ( p->style != NULL )
            *p->style++ = '\0';
        if ( p->class == NULL ) {
                p->class = login_getclass ( p->pw.pw_class );

        }
        if ( p->bKerberos ) {
            p->style = login_getstyle ( p->class, "kerberos", "auth-popper" );
            if ( p->style == NULL || strcmp ( p->style, "kerberos" ) != 0 )
                return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
        }

        p->style = login_getstyle ( p->class, p->style, "auth-popper" );
        if ( p->style == NULL )
            break;

        if ( auth_check ( p->user, 
                          p->class->lc_class, 
                          p->style, 
                          "challenge",
                          &s ) < 0 )
            break;

        if ( ( s & AUTH_CHALLENGE ) == 0 )
            break;

        p->challenge = auth_value ( "challenge" );
        if ( p->challenge == NULL )
            break;

        for ( cp = p->challenge; 
              (cp = strpbrk ( cp, "\r\n" ) ) != NULL; 
            )
            *cp = ' ';

        return ( pop_msg ( p, POP_SUCCESS, HERE, p->challenge ) );

    } while ( 0 );      /* so a break will work */
#endif  /* defined(__bsdi__) && _BSDI_VERSION >= 199608 */

    if ( p->AuthType == noauth )  /* If authentication method is unknown (i.e.  */
        p->AuthType = plain;      /*  not Kerberos) then assume plain text      */

    p->AuthState = user;          /* User command "succesful"                   */

    /*  
     * Tell the user that a password is required 
     */
    return ( pop_msg ( p, POP_SUCCESS, HERE,
                       "Password required for %s.",
                       p->user ) );
}


void downcase_uname ( POP *p, char *q )
{
    char *r;

    for ( r = q; 
          r != NULL && *r != '\0'; 
          r++ )
        if ( *r >= 'A' && *r <= 'Z' )
            *r = ( (*r) - 'A' + 'a' );

    DEBUG_LOG1 ( p, "user name downcased to '%s'", q );
}


void trim_domain ( POP *p, char *q )
{
    char *r = strchr ( q, '@' );
    if ( r != NULL ) {
        *r = '\0';
        DEBUG_LOG1 ( p, "domain name trimmed to '%s'", q );
    }
}
