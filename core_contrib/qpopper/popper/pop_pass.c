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
 * Revisions:
 *
 *     01/16/03  [rcg]
 *             - Renamed PASSWD macro to QPASSWD to avoid redefining
 *               PASSWD in shadow.h.
 *
 *     06/01/01  [rcg]
 *             - Added patch by Clifton Royston to change error message
 *               for nonauthfile and authfile tests.
 *
 *     02/03/01  [rcg]
 *             - Now logging to p->pop_facility.
 *
 *     01/30/01  [rcg]
 *             - Now using p->bDo_timing instead of DO_TIMING.
 *             - Now using p->bCheck_pw_max instead of DONT_CHECK_SP_MAX.
 *             - Now using p->grp_serv_mode and p->grp_no_serv_mode instead
 *               of SERVER_MODE_GROUP_INCLUDE and SERVER_MODE_GROUP_EXCLUDE.
 *             - Now using p->authfile instead of AUTHFILE.
 *
 *     12/27/00  [rcg]
 *             - Added generic auth_user() to pop_pass.c.
 *
 *     11/12/00  [rcg]
 *             - Fixed reference to 'pname' instead of 'p->myname'.
 *
 *     10/03/00  [rcg]
 *             - Reopen log after calling pam_authenticate().
 *
 *     10/02/00  [rcg]
 *             - Call check_config_files to check qpopper-options files.
 *
 *     09/26/00  [rcg]
 *             - Issue error 4 instead of crashing if appdata pointer
 *               is null (known bug in Solaris PAM fixed in Solaris 7
 *               and patch 106257-05).
 *
 *     09/06/00  [rcg]
 *             - Now using loginrestrictions() instead of our own code
 *               on AIX to check if accounts are expired, locked out, etc.
 *
 *     09/01/00  [rcg]
 *             - Made PAM section before OS-specific ones, to eliminate need
 *               to use '&& !defined(USE_PAM)' in each OS section.
 *
 *     07/12/00  [rcg]
 *             - Added patch from Ken Hornstein for Kerberos v5 et al.
 *
 *     06/30/00  [rcg]
 *             - Added support for ~/.qpopper-options.
 *
 *     06/20/00  [rcg]
 *             - Now checking for password as user as well as root
 *               when SECURENISPLU defined, based on patch by Neil
 *               W. Rickert.
 *
 *     06/16/00  [rcg]
 *             - Now setting server mode on or off based on group
 *               membership.
 *
 *     06/10/00  [rcg]
 *              - Added patch from Mathieu Legare to check for expired
 *                accounts on AIX.
 *
 *     06/05/00  [rcg]
 *              - Applied BSDI patches from Jeffrey C. Honig at BSDI.
 *              - Added 'sgi'.
 *              - When DONT_CHECK_SP_MAX defined, omit check on sp_max.
 *
 *     05/06/00  [rcg]
 *              - Added drac call.
 *
 *     03/16/00  [rcg]
 *              - Brought HPUX11 into the HPUX fold.
 *
 *     03/09/00  [rcg]
 *              - Changed AUTH to SPEC_POP_AUTH to avoid conflicts with
 *                RPC .h files.
 *
 *     03/07/00  [rcg]
 *              - Updated authentication OK message to account for hidden
 *                messages.
 *
 *     03/06/00  [rcg]
 *              - Added AIX patch contributed by Nik Conwell.
 *
 *     02/10/00  [rcg]
 *              - Fixed typo causing auth failures when CHECK_SHELL defined.
 *
 *     02/09/00  [rcg]
 *              - Added extended response codes.
 */


#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_STRINGS_H
#  include <strings.h>
#endif

#if HAVE_UNISTD_H
#  include <unistd.h>
#endif

#if HAVE_SYS_UNISTD_H
#  include <sys/unistd.h>
#endif

#include <time.h>
#include <pwd.h>
#include "popper.h"

#ifdef KERBEROS
#  ifdef KRB5
#  include <krb5.h>
#  include <com_err.h>
extern krb5_principal   ext_client;
extern krb5_context     pop_context;
extern char            *client_name;
#  endif /* KRB5 */
#endif /* KERBEROS */

#include "snprintf.h"

#define SLEEP_SECONDS 10

/* This error message is vague on purpose to help improve
   security at the inconvience of administrators and users */

char    *ERRMSG_PW    = "[AUTH] Password supplied for \"%s\" is incorrect.";
char    *ERRMSG_ACEXP = "[AUTH] \"%s\": account expired.";
char    *ERRMSG_PWEXP = "[AUTH] \"%s\": password expired.";
char    *ERRMSG_AUTH  = "[AUTH] \"%s\": access denied.";


/*------------------------------------- NONAUTHFILE */
int
checknonauthfile ( POP *p ) 
{
    char buf [ MAXUSERNAMELEN +1 ];
    FILE *fp;

    fp = fopen ( p->nonauthfile, "r" );
    if ( fp != NULL ) {
        while ( fgets ( buf, MAXUSERNAMELEN +1, fp ) ) {
            buf [ strlen(buf) -1 ] = '\0';
            if ( !strcmp ( buf, p->user ) ) {
                fclose ( fp );
                DEBUG_LOG3 ( p, "...checknonauthfile matched user %s "
                                "in file %s: %s",
                             p->user, p->nonauthfile, buf );
                return ( -1 );
            }
        }

        fclose ( fp );
    }

    DEBUG_LOG2 ( p, "...checknonauthfile didn't match user %s in file %s",
                 p->user, p->nonauthfile );
    return ( 0 );
}


/*------------------------------------- AUTHFILE */
int
checkauthfile ( POP *p ) 
{
    char buf [ MAXUSERNAMELEN +1 ];
    FILE *fp;

    fp = fopen ( p->authfile, "r" );
    if ( fp != NULL ) {
        while ( fgets ( buf, MAXUSERNAMELEN +1, fp ) ) {
            buf [ strlen(buf) -1 ] = '\0';
            if ( !strcmp ( buf, p->user ) ) {
                fclose ( fp );
                DEBUG_LOG3 ( p, "...checkauthfile matched user %s "
                                "in file %s: %s",
                             p->user, p->authfile, buf );
                return ( 0 );
            }
        }

        fclose ( fp );
    }

    DEBUG_LOG2 ( p, "...checkauthfile didn't match user %s in file %s",
                 p->user, p->authfile );
    return ( -1 );
}


int 
auth_user_kerberos ( p, pw )
POP     *   p;
struct passwd *pw;
{
/*------------------------------------- KERBEROS */
#ifdef KERBEROS
#  ifdef KRB4

    char            lrealm[REALM_SZ];
    int             status;

#  endif /* KRB4 */
#  ifdef KRB5

    char           *lrealm;
    krb5_data      *tmpdata;

#  endif /* KRB5 */

    struct passwd  *pwp;
 
#  ifdef KRB4

    status = krb_get_lrealm ( lrealm, 1 );
    if ( status == KFAILURE  ) {
        pop_log ( p, POP_WARNING, HERE, "%s: (%s.%s@%s) %s", 
                  p->client, kdata.pname, 
                  kdata.pinst, kdata.prealm, krb_err_txt[status] );
        return ( pop_msg ( p, POP_FAILURE, HERE,
                "[AUTH] Kerberos error:  \"%s\".", krb_err_txt[status] ) );
    } /* krb_get_lrealm failed */

#    ifdef KUSEROK

    if ( kuserok ( &kdata, p->user ) ) {
        pop_log ( p, POP_WARNING, HERE, 
                  "%s: (%s.%s@%s): not in %s's ACL.",
                  p->client, kdata.pname, kdata.pinst, 
                  kdata.prealm, p->user );
        return ( pop_msg ( p, POP_FAILURE, HERE, "[AUTH] Not in %s's ACL.", 
                           p->user ) );
    }
#    else /* not KUSEROK */

    if ( strcmp ( kdata.prealm, lrealm ) )  {
         pop_log ( p, POP_WARNING, HERE,
                   "%s: (%s.%s@%s) realm not accepted.", 
                   p->client, kdata.pname, kdata.pinst, kdata.prealm );
         return ( pop_msg ( p, POP_FAILURE, HERE,
                            "[AUTH] Kerberos realm \"%s\" not accepted.", 
                            kdata.prealm ) );
    }

    if ( strcmp ( kdata.pinst, "" ) ) {
        pop_log ( p, POP_WARNING, HERE, 
                 "%s: (%s.%s@%s) instance not accepted.", 
                 p->client, kdata.pname, kdata.pinst, kdata.prealm );
        return ( pop_msg ( p, POP_FAILURE, HERE,
                 "[AUTH] Must use null Kerberos(tm) instance -- \"%s.%s\" not accepted.",
                 kdata.pname, kdata.pinst ) );
    }

#    endif /* KUSEROK */
#  endif /* KRB4 */

#  ifdef KRB5
    /* 
     * only accept one-component names, i.e., realm and name only 
     */
    if ( krb5_princ_size ( pop_context, ext_client ) > 1 ) {
        pop_log ( p, POP_WARNING, HERE, "%s: (%s) instance not accepted.",
                  p->client, client_name);
        return ( pop_msg ( p, POP_FAILURE, HERE,
                           "Must use null Kerberos(tm) \"instance\" -  \"%s\" "
                           "not accepted.",
                           client_name ) );
    }

    /*
     * be careful! we are assuming that the instance and realm have been
     * checked already!  I used to simply copy the pname into p->user
     * but this causes too much confusion and assumes p->user will never
     * change. This makes me feel more comfortable.
     */

#    ifdef KUSEROK

    if ( !krb5_kuserok ( pop_context, ext_client, p->user ) ) {
        pop_log ( p, POP_WARNING, HERE, "%s: (%s): not in %s's ACL.",
                  p->client, client_name, p->user );
        return ( pop_msg ( p,POP_FAILURE, HERE, "Not in %s's ACL.", p->user ) );
    }

#   else /* KUSEROK */

    { /* local block */
        krb5_error_code retval;

        retval = krb5_get_default_realm ( pop_context, &lrealm );
        if ( retval ) {
            pop_log ( p, POP_WARNING, HERE, "%s: (%s) %s", 
                      p->client, client_name,
                      error_message(retval) );
            return ( pop_msg ( p,POP_FAILURE, HERE,
                               "Kerberos error:  \"%s\".", 
                               error_message(retval) ) );
        } /* krb5_get_default_realm failed */

        tmpdata = krb5_princ_realm ( pop_context, ext_client );
        if ( strncmp ( tmpdata->data, lrealm, tmpdata->length ) )  {
            pop_log ( p, POP_WARNING, HERE, "%s: (%s) realm not accepted.",
                      p->client, client_name );
            return ( pop_msg ( p,POP_FAILURE, HERE,
                               "Kerberos realm \"%*s\" not accepted.",
                               tmpdata->length, tmpdata->data ) );
        }

        tmpdata = krb5_princ_component ( pop_context, ext_client, 0 );
        if ( strncmp ( p->user, tmpdata->data, tmpdata->length ) ) {
            pop_log ( p, POP_WARNING, HERE, "%s: auth failed: %s vs %s",
                      p->client, client_name, p->user );
            return ( pop_msg ( p, POP_FAILURE, HERE,
                               "Wrong username supplied (%*s vs. %s).\n", 
                               tmpdata->length, tmpdata->data, p->user ) );
        }
    } /* local block */

#    endif /* KUSEROK */
#  endif /* KRB5 */

    return ( POP_SUCCESS );
#else   /* Kerberos not defined */
    (void) pw;

    return ( pop_msg ( p, POP_FAILURE, HERE,
             "[SYS/PERM] Kerberos failure: Qpopper not compiled "
             "with --enable-kerberos" ) );
#endif  /* KERBEROS */
}


/*------------------------------------- SPEC_POP_AUTH */
#ifdef SPEC_POP_AUTH
    char *crypt();


/*----------------------------------------------- PAM */
/* Code based on PAM patch contributed by German Poo.  
 * Patched by Kenneth Porter.
 * Patched by others since.
 */
#  ifdef USE_PAM

#    ifdef HAVE_SECURITY_PAM_APPL_H
#      include <security/pam_appl.h>
#    endif /* HAVE_SECURITY_PAM_APPL_H */

/* 
 * Some globals to make it easier to communicate between functions 
 */
static int   gp_errcode   = 0;
static char *GP_ERRSTRING =
            "[AUTH] PAM authentication failed for user \"%.100s\": %.128s (%d)";

static int 
PAM_qpopper_conv ( int                        num_msg, 
                   const struct pam_message **msg, 
                   struct pam_response      **resp, 
                   void                      *appdata_ptr )
{
    int                   replies  = 0;
    struct pam_response  *reply    = NULL;
    POP                  *p        = appdata_ptr;


    if ( p == NULL )
        return  PAM_SYSTEM_ERR;

    DEBUG_LOG1 ( p, "PAM_qpopper_conv: num_msg=%i", num_msg );

    reply = (struct pam_response*) malloc ( sizeof (struct pam_response) * num_msg );
    if ( reply == NULL ) 
        return PAM_CONV_ERR;

    for ( replies = 0; replies < num_msg; replies++ ) {
        DEBUG_LOG2 ( p, "PAM_qpopper_conv: msg_style[%i]=%i", 
                     replies, msg[replies]->msg_style );
        switch (msg[replies]->msg_style) {
        
        case PAM_PROMPT_ECHO_ON: /* assume it wants user name */
            reply[replies].resp_retcode = PAM_SUCCESS;
            reply[replies].resp = strdup ( p->user );
            /* PAM frees resp */
            break;
            
        case PAM_PROMPT_ECHO_OFF: /* assume it wants password */
            reply[replies].resp_retcode = PAM_SUCCESS;
            reply[replies].resp = strdup ( p->pop_parm[1] );
            /* PAM frees resp */
            break;
        
        case PAM_TEXT_INFO:
        case PAM_ERROR_MSG:
            reply[replies].resp_retcode = PAM_SUCCESS;
            reply[replies].resp = NULL;
            break;
            
        default:
            free (reply);
            gp_errcode = 1;
            return PAM_CONV_ERR;
        } /* switch */
    } /* for */
    
    *resp = reply;
    return PAM_SUCCESS;    
}


static struct pam_conv PAM_conversation = {
    &PAM_qpopper_conv,      /* address of our interface function */
    NULL                    /* will be -p-, the source of all things */
};


static int
auth_user(p, pw)
POP     * p;
struct passwd *pw;
{
    pam_handle_t    *pamh           = NULL;
    int              pamerror       = 0;
    int              erc            = 0;
    const char      *errmsg         = NULL;

    /* 
     * Let conv function access POP structure 
     */
    PAM_conversation.appdata_ptr = p;

    pamerror = pam_start ( USE_PAM, p->user, &PAM_conversation, &pamh );
    DEBUG_LOG3 ( p, "pam_start (service name %s) returned %i; gp_errcode=%i", 
                 USE_PAM, pamerror, gp_errcode );
    if ( ( gp_errcode != 0 ) || ( pamerror != PAM_SUCCESS ) ) {
        pam_end ( pamh, 0 );
        sleep   ( SLEEP_SECONDS );
        erc = pamerror ? pamerror : gp_errcode;
        return  ( pop_msg ( p, POP_FAILURE, HERE, GP_ERRSTRING, p->user,
                            pam_strerror(NULL, erc), erc ) );
    }
    pamerror = pam_authenticate ( pamh, 0 );

    /*
     * Need to reopen log after calling pam_authenticate, since
     * it opens log and sets facility to LOG_AUTH.
     */
    closelog();
#ifdef SYSLOG42
    openlog ( p->myname, 0 );
#else
    openlog ( p->myname, POP_LOGOPTS, p->log_facility );
#endif

    DEBUG_LOG2 ( p, "pam_authenticate returned %i; gp_errcode=%i", 
                 pamerror, gp_errcode );
    if ( ( gp_errcode != 0 ) || ( pamerror != PAM_SUCCESS ) ) {
        erc = pamerror ? pamerror : gp_errcode;
        errmsg = pam_strerror ( pamh, erc );
        pam_end ( pamh, 0 );
        sleep   ( SLEEP_SECONDS );
        return  ( pop_msg ( p, POP_FAILURE, HERE, GP_ERRSTRING, 
                            p->user, errmsg, erc ) );
    }
    pamerror = pam_acct_mgmt ( pamh, 0 );
    DEBUG_LOG1 ( p, "pam_acct_mgmt returned %i", pamerror );
    if ( pamerror != PAM_SUCCESS ) {
        errmsg = pam_strerror ( pamh, pamerror );
        pam_end ( pamh, 0 );
        sleep   ( SLEEP_SECONDS );
        return  ( pop_msg ( p, POP_FAILURE, HERE, GP_ERRSTRING, 
                            p->user, errmsg, pamerror ) );
    }
    pamerror = pam_setcred ( pamh, PAM_ESTABLISH_CRED );
    DEBUG_LOG1 ( p, "pam_setcred returned %i", pamerror );
    if ( pamerror != PAM_SUCCESS ) {
        errmsg = pam_strerror ( pamh, pamerror );
        pam_end ( pamh, 0 );
        sleep   ( SLEEP_SECONDS );
        return  ( pop_msg ( p, POP_FAILURE, HERE, GP_ERRSTRING, 
                            p->user, errmsg, pamerror ) );
    }

    pamerror = pam_set_item ( pamh, PAM_TTY, p->ipaddr );
    DEBUG_LOG1 ( p, "pam_set_item returned %i", pamerror );
    if ( pamerror != PAM_SUCCESS ) {
        pop_log ( p, POP_NOTICE, HERE,
                  "Error setting pam item TTY: %s (%i)", 
                  pam_strerror ( pamh, pamerror ), pamerror );
    }

    pamerror = pam_set_item ( pamh, PAM_RHOST, p->client );
    DEBUG_LOG1 ( p, "pam_set_item returned %i", pamerror );
    if ( pamerror != PAM_SUCCESS ) {
        pop_log ( p, POP_NOTICE, HERE,
                  "Error setting pam item RHOST: %s (%i)", 
                  pam_strerror ( pamh, pamerror ), pamerror );
    }

/* -- we can't call pam_open_session, because once we give up root
   -- privileges we are unable to call pam_close_session
   --
    pamerror = pam_open_session ( pamh, 0 );
    DEBUG_LOG1 ( p, "pam_open_session returned %i", pamerror );
    if ( pamerror != PAM_SUCCESS ) {
        pop_log ( p, POP_NOTICE, HERE,
                  "Error opening PAM session: %s (%i)", 
                  pam_strerror ( pamh, pamerror ), pamerror );
    }
*/

    pam_end ( pamh, PAM_SUCCESS );

    return POP_SUCCESS;
}
#  else /* not PAM */


/*----------------------------------------------- SUNOS4 and not ISC */
#    if defined(SUNOS4) && !defined(ISC)

#      define DECLARED_AUTH_USER

#      include <sys/label.h>
#      include <sys/audit.h>
#      include <pwdadj.h>

static int
auth_user ( p, pw )
POP     *   p;
struct passwd *pw;
{
    struct passwd_adjunct *pwadj;

    /*  
     * Look for the user in the shadow password file 
     */
    pwadj = getpwanam ( p->user );
    if ( pwadj == NULL ) {
        sleep  ( SLEEP_SECONDS );
        return ( pop_msg ( p, POP_FAILURE, HERE,
                "[AUTH] (shadow) Password supplied for \"%s\" is empty.", 
                p->user ) );
    } else {
        pw->pw_passwd = (char *)strdup(pwadj->pwa_passwd);
    }

    /*  
     * We don't accept connections from users with null passwords 
     *
     *  Compare the supplied password with the password file entry 
     */
    if ( (  pw->pw_passwd == NULL ) || 
         ( *pw->pw_passwd == '\0' ) ||
           strcmp ( crypt ( p->pop_parm[1], pw->pw_passwd ), pw->pw_passwd ) 
       ) {
        sleep  ( SLEEP_SECONDS );
        return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
    }

    return ( POP_SUCCESS );
}

#    endif  /* SUNOS4 and not ISC */


/*----------------------------------------------- SOLARIS2 or AUX or UXPDS or SGI */
#    if defined(SOLARIS2) || defined(AUX) || defined(UXPDS) || defined(sgi)

#      define DECLARED_AUTH_USER

#      ifdef HAVE_SHADOW_H
#        include <shadow.h>
#      endif /* HAVE_SHADOW_H */

static int
auth_user ( p, pw )
POP     *   p;
struct passwd *pw;
{
    register struct spwd *pwd;
    long today;

#      ifdef SECURENISPLUS
    UID_T uid_save;
#      endif /* SECURENISPLUS */

    /*
     * According to Neil W. Rickert, with SECURENISPLUS, when 'compat'
     * is used in nsswitch.conf, there is information in '/etc/shadow'
     * that must be consulted.  We need to first do getspnam() as root.
     * If that fails to get a useful encrypted passwd, then repeat as
     * the user, whose nis+ credentials may be required.
     */

    /*  
     * Look for the user in the shadow password file 
     */

#      ifdef SECURENISPLUS
    uid_save = geteuid();
    if ( uid_save )
        seteuid ( 0 ) ; /* must first try as root */
    pwd = getspnam ( p->user );
    if ( pwd == NULL || strlen ( pwd->sp_pwdp ) < 8 ) {
        seteuid ( uid_save ); /* now try as the user */
        pwd = getspnam ( p->user );
    }
#      else /* not using SECURENISPLUS */
    pwd = getspnam ( p->user );
#      endif /* SECURENISPLUS */

    if ( pwd == NULL ) {
        if ( !strcmp ( pw->pw_passwd, "x" ) ) {      /* This my be a YP entry */
            sleep  ( SLEEP_SECONDS );
            return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
        }
    } else {
        today = (long) time ( (time_t *) NULL ) / 24 / 60 / 60;

        /* 
         * Check for expiration date 
         */
        if ( pwd->sp_expire > 0 && today > pwd->sp_expire ) {
            sleep  ( SLEEP_SECONDS );
            return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_ACEXP, p->user ) );
        }

        /* 
         * Check if password not changed within required time
         */
        if ( p->bCheck_pw_max
             && pwd->sp_max > 0
             && today > pwd->sp_lstchg + pwd->sp_max ) {
            sleep  ( SLEEP_SECONDS );
            return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PWEXP, p->user ) );
        }

        pw->pw_passwd = (char *)strdup(pwd->sp_pwdp);
        endspent();
    }

    /*  
     * We don't accept connections from users with null passwords 
     *
     *  Compare the supplied password with the password file entry 
     */
    if ( (  pw->pw_passwd == NULL ) || 
         ( *pw->pw_passwd == '\0' ) ||
           strcmp ( crypt ( p->pop_parm[1], pw->pw_passwd ), pw->pw_passwd ) 
       ) {
        sleep  ( SLEEP_SECONDS );
        return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
    }

    return ( POP_SUCCESS );
}

#    endif  /* SOLARIS2 or AUX or UXPDS or sgi */


/*----------------------------------------------- PIX or ISC */
#    if defined(PTX) || defined(ISC) 

#      define DECLARED_AUTH_USER

#      ifdef HAVE_SHADOW_H
#        include <shadow.h>
#      endif /* HAVE_SHADOW_H */

static int
auth_user ( p, pw )
POP     *   p;
struct passwd *pw;
{
    register struct spwd * pwd;
    long today;

    /*  
     * Look for the user in the shadow password file 
     */
    pwd = getspnam ( p->user );
    if ( pwd == NULL ) {
        if ( !strcmp ( pw->pw_passwd, "x" ) ) {      /* This my be a YP entry */
            sleep  ( SLEEP_SECONDS );
            return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
        }
    } else {
        pw->pw_passwd = (char *)strdup(pwd->sp_pwdp);
    }

    /*  
     * We don't accept connections from users with null passwords 
     *
     *  Compare the supplied password with the password file entry 
     */
    if ( (  pw->pw_passwd == NULL ) || 
         ( *pw->pw_passwd == '\0' ) ||
           strcmp ( crypt ( p->pop_parm[1], pw->pw_passwd ), pw->pw_passwd ) 
       ) {
        sleep  ( SLEEP_SECONDS );
        return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
    }

    return ( POP_SUCCESS );
}

#    endif  /* PTX or ISC */


/*----------------------------------------------- POPSCO or HPUX */
#    if defined(POPSCO) || defined(HPUX)

#      define DECLARED_AUTH_USER

#      ifdef POPSCO
#        include <sys/security.h>
#        include <sys/audit.h>
#      else /* must be HPUX */
#        include <hpsecurity.h>
#      endif /* POPSCO */

#    include <prot.h>

#    define QPASSWD(p)      p->ufld.fd_encrypt

static int
auth_user ( p, pw )
POP     *   p;
struct passwd *pw;
{
    register struct pr_passwd *pr;

    pr = getprpwnam ( p->user );
    if ( pr == NULL ) {
        if ( !strcmp ( pw->pw_passwd, "x" ) ) {
            sleep  ( SLEEP_SECONDS );
            return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
        }

    /*  
     * We don't accept connections from users with null passwords 
     */
    if ( (  pw->pw_passwd == NULL ) || 
         ( *pw->pw_passwd == '\0' ) ||
            ( strcmp ( bigcrypt ( p->pop_parm[1], pw->pw_passwd ), pw->pw_passwd ) &&
              strcmp ( crypt    ( p->pop_parm[1], pw->pw_passwd ), pw->pw_passwd )
            ) 
        ) {
        sleep  ( SLEEP_SECONDS );
        return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
        }
    } else {
        /* 
         * We don't accept connections from users with null passwords 
         *
         * Compare the supplied password with the password file entry 
         */
        if ( ( QPASSWD(pr) == NULL ) || ( *QPASSWD(pr) == '\0' ) ) {
            sleep  ( SLEEP_SECONDS );
            return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
        }
        
        if ( strcmp ( bigcrypt ( p->pop_parm[1], QPASSWD(pr) ), QPASSWD(pr) ) &&
             strcmp ( crypt    ( p->pop_parm[1], QPASSWD(pr) ), QPASSWD(pr) )  ) {
            sleep  ( SLEEP_SECONDS );
            return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
        }
    }

    return ( POP_SUCCESS );
}

#    endif  /* POPSCO || HPUX */


/*----------------------------------------------- ULTRIX */
#    ifdef ULTRIX

#      define DECLARED_AUTH_USER

#      include <auth.h>

static int
auth_user ( p, pw )
struct passwd  *   pw;
POP     *   p;
{
    AUTHORIZATION *auth, *getauthuid();

    auth = getauthuid ( pw->pw_uid );
    if ( auth == NULL ) {
        if ( !strcmp(pw->pw_passwd, "x") ) {      /* This my be a YP entry */
            sleep  ( SLEEP_SECONDS );
            return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
        }
    } else {
        pw->pw_passwd = (char *)strdup(auth->a_password);
    }

    /*  
     * We don't accept connections from users with null passwords 
     *
     *  Compare the supplied password with the password file entry 
     */
    if ( (  pw->pw_passwd == NULL ) || 
         ( *pw->pw_passwd == '\0' )  {
        sleep ( SLEEP_SECONDS );
        return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
    }

    if ( strcmp ( crypt16 ( p->pop_parm[1], pw->pw_passwd ), pw->pw_passwd ) &&
         strcmp ( crypt   ( p->pop_parm[1], pw->pw_passwd ), pw->pw_passwd )  ) {
        sleep  ( SLEEP_SECONDS );
        return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
    }

    return ( POP_SUCCESS );
}

#    endif  /* ULTRIX */


/*----------------------------------------------- OSF1 */
/* this is a DEC Alpha / Digital Unix system */
#    ifdef OSF1

#      define DECLARED_AUTH_USER

#      include <sys/types.h>
#      include <sys/security.h>
#      include <prot.h>

#      define   QPASSWD(p)   (p->ufld.fd_encrypt)

static int
auth_user ( p, pw )
POP     *   p;
struct passwd *pw;
{
    register struct pr_passwd *pr;

    pr = getprpwnam ( p->user );
    if ( pr == NULL ) {
        if ( !strcmp ( pw->pw_passwd, "x" ) ) {      /* This my be a YP entry */
            sleep  ( SLEEP_SECONDS );
            return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
        }
    } else {
        pw->pw_passwd = (char *)strdup(QPASSWD(pr));
    }

    /*  
     * We don't accept connections from users with null passwords 
     *
     *  Compare the supplied password with the password file entry 
     */
    if ( (  pw->pw_passwd == NULL ) || 
         ( *pw->pw_passwd == '\0' )  ) {
        sleep  ( SLEEP_SECONDS );
        return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
    }

    if ( strcmp ( bigcrypt ( p->pop_parm[1], pw->pw_passwd ), pw->pw_passwd ) &&
         strcmp ( crypt    ( p->pop_parm[1], pw->pw_passwd ), pw->pw_passwd )  ) {
        sleep  ( SLEEP_SECONDS );
        return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
    }

    return ( POP_SUCCESS );
}

#    endif        /* OSF1 */


/*----------------------------------------------- UNIXWARE */
#    ifdef UNIXWARE

#      define DECLARED_AUTH_USER

#      ifdef HAVE_SHADOW_H
#        include <shadow.h>
#      endif /* HAVE_SHADOW_H */

static int
auth_user ( p, pw )
struct passwd  *   pw;
POP     *   p;
{
    register struct spwd * pwd;
    long today;

    /*  
     * Look for the user in the shadow password file 
     */
    pwd = getspnam ( p->user );
    if ( pwd == NULL ) {
        if ( !strcmp ( pw->pw_passwd, "x" ) ) {      /* This my be a YP entry */
            sleep  ( SLEEP_SECONDS );
            return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
        }
    } else {
        today = (long)time((time_t *)NULL)/24/60/60;

        /* 
         * Check for expiration date 
         */
        if ( pwd->sp_expire > 0 && today > pwd->sp_expire ) {
            sleep  ( SLEEP_SECONDS );
            return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_ACEXP, p->user ) );
        }

        /* 
         * Check if password not changed within required time 
         */
        if ( p->bCheck_pw_max
             && pwd->sp_max > 0
             && today > pwd->sp_lstchg + pwd->sp_max ) {
            sleep  ( SLEEP_SECONDS );
            return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PWEXP, p->user ) );
        }

        pw->pw_passwd = (char *)strdup(pwd->sp_pwdp);
        endspent();
    }

    /*  
     * We don't accept connections from users with null passwords 
     *
     *  Compare the supplied password with the password file entry 
     */
    if ( (  pw->pw_passwd == NULL ) || 
         ( *pw->pw_passwd == '\0' ) ||
         strcmp ( crypt ( p->pop_parm[1], pw->pw_passwd ), pw->pw_passwd ) 
       ) {
        sleep  ( SLEEP_SECONDS );
        return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
    }

    return ( POP_SUCCESS );
}

#    endif  /* UNIXWARE */


/*----------------------------------------------- LINUX */
#    if defined(LINUX)

#      define DECLARED_AUTH_USER

#      ifdef HAVE_SHADOW_H
#        include <shadow.h>
#      endif /* HAVE_SHADOW_H */

static int
auth_user ( p, pw )
POP     *   p;
struct passwd *pw;
{
    register struct spwd * pwd;
    long today;

    /*  
     * Look for the user in the shadow password file 
     */
    pwd = getspnam ( p->user );
    if ( pwd == NULL ) {
        if ( !strcmp ( pw->pw_passwd, "x" ) ) {      /* This my be a YP entry */
            sleep  ( SLEEP_SECONDS );
            return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
        }
    } else {
        today = (long)time((time_t *)NULL)/24/60/60;

        /* 
         * Check for expiration date 
         */
        if ( pwd->sp_expire > 0 && today > pwd->sp_expire ) {
            sleep  ( SLEEP_SECONDS );
            return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_ACEXP, p->user ) );
        }

        /* 
         * Check if password not changed within required time 
         */
        if ( p->bCheck_pw_max
             && pwd->sp_max > 0
             && today > pwd->sp_lstchg + pwd->sp_max ) {
            sleep  ( SLEEP_SECONDS );
            return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PWEXP, p->user ) );
        }

        pw->pw_passwd = (char *)strdup(pwd->sp_pwdp);
        endspent();
    }

    /*  
     * We don't accept connections from users with null passwords 
     *
     *  Compare the supplied password with the password file entry
     *
     *  Kernels older than 2.0.x need pw_encrypt() for shadow support 
     */
    if ( (  pw->pw_passwd == NULL ) || 
         ( *pw->pw_passwd == '\0' ) || 
         ( strcmp ( crypt      ( p->pop_parm[1], pw->pw_passwd ), pw->pw_passwd ) &&
#      ifdef HAVE_PW_ENCRYPT
           strcmp ( pw_encrypt ( p->pop_parm[1], pw->pw_passwd ), pw->pw_passwd )
#      else /* not HAVE_PW_ENCRYPT */
           strcmp ( crypt      ( p->pop_parm[1], pw->pw_passwd ), pw->pw_passwd )
#      endif /* HAVE_PW_ENCRYPT */
         )
       ) {
        sleep  ( SLEEP_SECONDS);
        return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
    }

    return ( POP_SUCCESS );
}

#    endif  /* LINUX */


/*----------------------------------------------- BSD/OS */
#    if defined(__bsdi__) && _BSDI_VERSION >= 199608

#      define DECLARED_AUTH_USER

static int
auth_user(p, pw)
POP     *   p;
struct passwd  *   pw;
{
    int r;

    /*
     * We don't accept connections from users with null passwords
     */
    if ( ( pw->pw_passwd == NULL ) || ( *pw->pw_passwd == '\0' ) )
        return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );

    /*
     * Compare the supplied password with the password file entry
     */
    r = auth_response ( p->user, p->class->lc_class, p->class->lc_style,
        "response", NULL, p->challenge ? p->challenge : "", p->pop_parm[1] );

    if ( p->challenge ) {
            free ( p->challenge );
            p->challenge = NULL;
    }

    if ( r <= 0 || auth_approve ( p->class, p->user, "popper" ) <= 0 )
        return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );

    return ( POP_SUCCESS );
}

#    endif  /* defined(__bsdi__) && _BSDI_VERSION >= 199608 */


/*----------------------------------------------- AIX  */
#    ifdef AIX

#      define DECLARED_AUTH_USER

/*
 * AIX specific special authentication
 *
 * authenticate() is a standard (AIX) subroutine for password authentication
 * It's located in libs.a.    nik@bu.edu   11/19/1998 
 */

#      include <usersec.h>
#      include <time.h>
#      include <errno.h>
#      include <stddef.h>
#      include <login.h>

extern int loginrestrictions ( char *, int, char *, char ** );

static int
auth_user ( POP *p, struct passwd *pw )
{
    int     reenter   = 0;
    char   *message   = NULL;
    int     rslt      = POP_FAILURE;


    /*
     * loginrestrictions() checks if the user is expired, locked out, or not
     * permitted access here and now.
     */
    if ( loginrestrictions ( p->user, S_LOGIN, NULL, &message ) ) {
        int save_err = errno;
        pop_log ( p, POP_NOTICE, HERE, "Account %s access denied: (%d) %s",
                  p->user, save_err, message );
        if ( save_err == ESTALE )
            pop_msg ( p, POP_FAILURE, HERE, ERRMSG_ACEXP, p->user );
        else
            pop_msg ( p, POP_FAILURE, HERE, ERRMSG_AUTH, p->user );
        goto Exit;
    }

    /*
     * We don't accept connections from users with null passwords
     */
    if ( (  pw->pw_passwd == NULL ) ||
         ( *pw->pw_passwd == '\0' ) 
       ) {
        sleep ( SLEEP_SECONDS );
        pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user );
        goto Exit;
    }

    /* 
     * N.B.: authenticate is probably going to blow away struct passwd *pw
     * static area.  Luckily, nobody who got it before auth_user() is expecting it
     * to still be valid after the auth_user() call. 
     */
    if ( authenticate ( p->user, p->pop_parm[1], &reenter, &message) == 0 ) {
        rslt = POP_SUCCESS;
    } else {
        pop_log ( p, POP_NOTICE, HERE, "Authentication failed for user %s: %s",
                  p->user, message );
        sleep  ( SLEEP_SECONDS );
        pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user );
    }

Exit:
    if ( message != NULL )
      free ( message );
    return rslt;
}

#    endif /* AIX */


/*----------------------------------------------- generic AUTH_USER */
#    ifndef DECLARED_AUTH_USER 

#      ifdef HAVE_SHADOW_H
#        include <shadow.h>
#      endif /* HAVE_SHADOW_H */

static int
auth_user ( p, pw )
POP     *   p;
struct passwd *pw;
{
    register struct spwd *pwd;
    long today;

    /*  
     * Look for the user in the shadow password file 
     */
    pwd = getspnam ( p->user );
    if ( pwd == NULL ) {
        if ( !strcmp ( pw->pw_passwd, "x" ) ) {      /* This my be a YP entry */
            sleep  ( SLEEP_SECONDS );
            return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
        }
    } else {
        pw->pw_passwd = (char *)strdup(pwd->sp_pwdp);
    }

    /*  
     * We don't accept connections from users with null passwords 
     *
     *  Compare the supplied password with the password file entry 
     */
    if ( (  pw->pw_passwd == NULL ) || 
         ( *pw->pw_passwd == '\0' ) ||
           strcmp ( crypt ( p->pop_parm[1], pw->pw_passwd ), pw->pw_passwd ) 
       ) {
        sleep  ( SLEEP_SECONDS );
        return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
    }

    return ( POP_SUCCESS );
}

#    endif  /* DECLARED_AUTH_USER */


#  endif /* USE_PAM */


/*------------------------------------- not SPEC_POP_AUTH */
#else   /* not SPEC_POP_AUTH */

char *crypt();

static int
auth_user ( p, pw )
POP     *   p;
struct passwd  *   pw;
{
    /*  
     * We don't accept connections from users with null passwords 
     *
     *  Compare the supplied password with the password file entry 
     */
    if ( (  pw->pw_passwd == NULL ) || 
         ( *pw->pw_passwd == '\0' ) ||
            strcmp ( crypt ( p->pop_parm[1], pw->pw_passwd ), pw->pw_passwd )
       ) {
        sleep ( SLEEP_SECONDS );
        return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
    }

    return ( POP_SUCCESS );
}

#endif  /* SPEC_POP_AUTH */


/* 
 *  pass:   Obtain the user password from a POP client
 */

#ifdef SECURENISPLUS
#  include <rpc/rpc.h>
#  include <rpc/key_prot.h>
#endif /* SECURENISPLUS */

int 
pop_pass ( p )
POP     *  p;
{
    struct passwd *pwp;
    time_t my_timer = time(0);

#ifdef CHECK_SHELL
    char *getusershell();
    void endusershell();
    char *shell;
    char *cp;
    int shellvalid;
#endif /* CHECK_SHELL */

#ifdef SECURENISPLUS
    UID_T uid_save;
    char net_name  [ MAXNETNAMELEN ],
         secretkey [ HEXKEYBYTES + 1];

    *secretkey = '\0';
#endif /* SECURENISPLUS */

    /* 
     * Is the user not authorized to use POP? 
     */
    if ( p->nonauthfile != NULL && checknonauthfile ( p ) != 0 ) {
        sleep  ( SLEEP_SECONDS );
        return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_AUTH, p->user ) );
    }

    /* 
     * Is the user authorized to use POP? 
     */
    if ( p->authfile != NULL && checkauthfile ( p ) != 0 ) {
        sleep  ( SLEEP_SECONDS );
        return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_AUTH, p->user ) );
    }

    /*  
     * Verify user known by system. 
     */
    pwp = &p->pw;
    if ( pwp->pw_name == NULL ) {
        DEBUG_LOG1 ( p, "User %.128s not known by system",
                     p->user );
        sleep ( SLEEP_SECONDS );
        return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
    }

#ifdef SECURENISPLUS
    /*  
     * we must do this keyserv stuff (as well as auth_user()!) as the user 
     */
    uid_save = geteuid();
    seteuid ( pwp->pw_uid );

    /*  
     * see if DES keys are already known to the keyserv(1m) 
     */
    if ( ! key_secretkey_is_set() ) {
        /*  
         * keys are not known, so we must get the DES keys
         * and register with the keyserv(1m) 
         */
        getnetname ( net_name );

        if ( getpublickey ( net_name, secretkey ) ) {
            if ( strlen ( p->pop_parm[1] ) > 8 ) 
                ( p->pop_parm [1] ) [8] = '\0';

            if ( ! getsecretkey ( net_name, secretkey, p->pop_parm[1] ) ||
                 *secretkey == '\0' ) {
                sleep  ( SLEEP_SECONDS );
                return ( pop_msg ( p, POP_FAILURE, HERE, ERRMSG_PW, p->user ) );
            }

            key_setsecret ( secretkey );
            memset ( secretkey, '\0', sizeof(secretkey) );
        } else {
            /* 
             * if there are no keys defined, we assume that password entry
             * either resides in /etc/shadow or "root" has access to the
             * corresponding NIS+ entry 
             */
            seteuid ( 0 );
        }
    }
#endif /* SECURENISPLUS */

#ifdef BLOCK_UID
    if ( pwp->pw_uid <= BLOCK_UID ) {
        return ( pop_msg ( p, POP_FAILURE, HERE,
                           "[AUTH] Access is blocked for UIDs below %d", 
                           BLOCK_UID ) );
    }
#endif /* BLOCK_UID */

#ifdef CHECK_SHELL
    /*  
     * Disallow anyone who does not have a standard shell as returned by
     * getusershell(), unless the sys admin has included the wildcard
     * shell in /etc/shells.  (default wildcard - /POPPER/ANY/SHELL)
     */
    shell = pwp->pw_shell;
    if ( shell == NULL || *shell == 0 ) {
        /* 
         * You can default the shell, but I don't think it's a good idea 
         */
        /* 
         shell = "/usr/bin/sh"; 
         */
        return ( pop_msg ( p, POP_FAILURE, HERE, 
                           "[AUTH] No user shell defined" ) );
    }
    
    for ( shellvalid = 0; 
          shellvalid == 0 && ( cp = getusershell() ) != NULL;
        )
        if ( !strcmp ( cp, WILDCARD_SHELL ) || 
             !strcmp ( cp, shell )           )
             shellvalid = 1;
    endusershell();

    if ( shellvalid == 0 ) {
        return ( pop_msg ( p, POP_FAILURE, HERE, 
                           "[AUTH] \"%s\": shell not found.", 
                           p->user ) );
    }
#endif /* CHECK_SHELL */

    if ( ( p->bKerberos ? auth_user_kerberos ( p, pwp ) 
                        : auth_user          ( p, pwp ) 
         ) != POP_SUCCESS 
       ) {
        pop_log ( p, POP_PRIORITY, HERE,
                 "[AUTH] Failed attempted login to %s from host (%s) %s",
                  p->user, p->client, p->ipaddr );
        sleep  ( SLEEP_SECONDS );
        return ( POP_FAILURE );
    }

#ifdef SECURENISPLUS
    seteuid ( uid_save );
#endif /* SECURENISPLUS */

    /*
     * Check if server mode should be set or reset based on group membership.
     */

    if ( p->grp_serv_mode != NULL
         && check_group ( p, &p->pw, p->grp_serv_mode ) ) {
        p->server_mode = TRUE;
        DEBUG_LOG2 ( p, "Set server mode for user %s; "
                        "member of \"%.128s\" group",
                     p->user, p->grp_serv_mode );
    }

    if ( p->grp_no_serv_mode != NULL
         && check_group ( p, &p->pw, p->grp_no_serv_mode ) ) {
        p->server_mode = FALSE;
        DEBUG_LOG2 ( p, "Set server mode OFF for user %s; "
                        "member of \"%.128s\" group",
                     p->user, p->grp_no_serv_mode );
    }

    /*
     * Process qpopper-options files, if requested and present.
     */
    check_config_files ( p, pwp );

    if ( p->bDo_timing )
        p->login_time = time(0) - my_timer;

    /*  
     * Make a temporary copy of the user's maildrop 
     * and set the group and user id 
     * and get information about the maildrop 
     */
    if ( pop_dropcopy ( p, pwp ) != POP_SUCCESS ) {
        sleep  ( SLEEP_SECONDS );
        return ( POP_FAILURE );
        }

    /*  
     * Initialize the last-message-accessed number 
     */
    p->last_msg = 0;

    p->AuthState = pass;   /* plain or kerberos authenticated successfully */
    
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

