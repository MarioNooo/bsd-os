
/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 */


/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The file License.txt specifies the terms for use, modification,
 * and redistribution.
 *
 * Revisions:
 *
 *     02/06/03 [RCG]
 *              - Apply patch from Jeremy Chadwick to fix pathname
 *                trimming of arg[0].
 *
 *     07/25/01 [RCG]
 *              - Allow '-p' to be used even if APOP not defined.
 *
 *     02/12/01 [RCG]
 *              - Save trace file name.
 *
 *     02/08/01 [RCG]
 *              - Setting p->nMaxBulls from compile-time NEWBULLCNT.
 *
 *     02/03/01 [RCG]
 *              - Now logging to p->pop_facility.
 *
 *     01/25/01 [RCG]
 *              - Setting p->chunky_writes from compile-time default.
 *
 *     01/23/01 [RCG]
 *              - login_delay and expire now in p.
 *              - Initialize new -p- variables from compile-time values:
 *                bDo_timing, bCheck_old_spool_loc, bCheck_hash_dir,
 *                bCheck_pw_max, bUpdate_status_hdrs, bUpdate_on_abort,
 *                bAuto_delete, bGroup_bulls, hash_spool, bHome_dir_mail,
 *                bOld_style_uid, bUW_kluge, bKeep_temp_drop, grp_serv_mode,
 *                grp_no_serv_mode, authfile, nonauthfile, bShy,
 *                pMail_command, pCfg_spool_dir, pCfg_temp_dir,
 *                pCfg_temp_name, pCfg_cache_dir, pCfg_cache_name.
 *
 *     12/21/00  [rcg]
 *              - Delete 'a' and change 'l' to integer type.
 *
 *     11/14/00  [rcg]
 *              - Added '-C' to trim domain name.
 *
 *     10/12/00  [rcg]
 *              - Fitted LGL's TLS patches.
 *
 *     09/30/00  [rcg]
 *              - Fixed tracing in stand-alone mode.
 *
 *     09/11/00  [rcg]
 *              - Fixed auto-delete to require EXPIRE 0.
 *
 *     08/17/00  [rcg]
 *              - Added '-B' to set bulldb-nonfatal.
 *
 *     07/12/00  [rcg]
 *              - Added patch from Ken Hornstein for Kerberos v5 et al.
 *
 *     06/30/00  [rcg]
 *              - Added '-D drac-host'.
 *              - Added '-f config-file'.
 *              - Added '-u' to use ~/.qpopper-options'.
 *
 *     06/08/00  [rcg]
 *              - Removed '-Z' run-time option.
 *
 *     06/05/00  [rcg]
 *              - Applied patches from Jeffrey C. Honig at BSDI making
 *                server mode and Kerberos run-time switches.
 *              - Added '-p 0-3' to set clear text password handling when
 *                APOP or SCRAM available.
 *
 *     05/16/00  [rcg]
 *              - Added '-L msgs' to set the msg interval at which
 *                we check if we need to refresh the mail lock.
 *              - Added '-Z' to revert to old (pre-3.x) locking.  This
 *                is dangerous and will likely be removed before release.
 *
 *     03/14/00  [rcg]
 *              - Minor syntactical tweaks.
 *
 *     03/06/00  [rcg]
 *              - Renamed authenticate to kerb_authenticate because of
 *                name conflict on AIX (thanks to Nik Conwell).
 */


#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#if HAVE_SYS_NETINET_IN_H
#  include <sys/netinet/in.h>
#endif

#if HAVE_NETINET_IN_H
#  include <netinet/in.h>
#endif

#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>

#if HAVE_SYS_PARAM_H
#  include <sys/param.h>
#endif

#if HAVE_STRINGS_H
#  include <strings.h>
#endif

#if HAVE_UNISTD_H
#  include <unistd.h>
#endif

#if HAVE_SYS_UNISTD_H
#  include <sys/unistd.h>
#endif

#ifndef HAVE_INDEX
#  define index(s,c) strchr(s,c)
#  define rindex(s,c) strrchr(s,c)
#endif /* HAVE_INDEX */

#ifndef HAVE_BCOPY
#  define bcopy(src,dest,len)   (void) (memcpy(dest, src, len))
#  define bzero(dest,len)       (void) (memset(dest, (char)NULL, len))
#  define bcmp(b1,b2,n)         memcmp(b1,b2,n)
#endif /* HAVE_BCOPY */

#include "get_sub_opt.h"
#include "popper.h"

/* CNS Kerberos IV */
#ifdef KERBEROS
#  ifdef KRB4
  AUTH_DAT kdata;
#  endif /* KRB4 */
#  ifdef KRB5    
#    include <krb5.h>
#    include <com_err.h>
#    include <ctype.h>
#    include <kerberosIV/krb.h>
  krb5_principal ext_client;
  krb5_context pop_context;
  char *client_name;
#    define KRB5_RECVAUTH_V4       4       /* V4 recvauth */
#    define KRB5_RECVAUTH_V5       5       /* V5 recvauth */

#    ifndef KRB5_APPL_VERSION
#      define KRB5_APPL_VERSION     NULL
#    endif /* KRB5_APPLE_VERSION */

#  endif /* KRB5 */
#endif /* KERBEROS */

/* Some systems define (in netdb.h) h_errno as get_h_errno() */
#ifndef h_errno
  extern int      h_errno; /* Error external for gethostxxxx library */
#endif

/* Method to get friendly string from h_errno */
#ifndef HAVE_HSTRERROR
  const char * _HERRORS[] = 
      {"(No problem)",                                          /*  0  NETDB_SUCCESS  */
       "Authoritive answer: Host not found",                    /*  1  HOST_NOT_FOUND */
       "Non-Authoritive answer: Host not found, or SERVERFAIL", /*  2  TRY_AGAIN      */
       "Non recoverable errors, FORMERR, REFUSED, NOTIMP",      /*  3  NO_RECOVERY    */
       "Valid name, no data record of requested type",          /*  4  NO_DATA        */
       "(Unknown)"};
#  define hstrerror(x) ( (x >= 0 && x <= NO_DATA) ? _HERRORS[x] : "(unknown)" )
#endif /* HAVE_HSTRERROR */

extern int      errno;
       int      no_rev_lookup;      /*  Avoid reverse lookup? */

/*
#ifdef POPSCO
  extern struct state   _res;
#endif
*/


#ifndef HAVE_STRDUP
#  include <stddef.h>
#  include <stdlib.h>

char *
strdup(str)
        const char *str;
{
    int len;
    char *copy;

    len = strlen(str) + 1;
    if (!(copy = malloc((u_int)len)))
        return((char *)NULL);
    bcopy(str, copy, len);
    return(copy);
}
#endif /* not HAVE_STRDUP */

/* Some systems lack a prototype for gethostname
 * in unistd.h */
extern int gethostname ( );

int
kerb_authenticate ( p, addr )
     POP                *p;
     struct sockaddr_in *addr;
{

#ifdef KERBEROS
#  ifdef KRB4
    Key_schedule   schedule;
    KTEXT_ST       ticket;
    char           instance[INST_SZ];  
    char           version[9];
    int            auth;
  
    if ( p->bKerberos ) {

#    ifdef  K_GETSOCKINST
        k_getsockinst ( 0, instance );
#    else
        strcpy ( instance, "*" );
#    endif /* K_GETSOCKINST */

        auth = krb_recvauth ( 0L, 0, &ticket, p->kerberos_service, instance,
                              addr, (struct sockaddr_in *) NULL,
                              &kdata, "", schedule, version );
        
        if ( auth != KSUCCESS ) {
            pop_msg ( p, POP_FAILURE, HERE, "Kerberos authentication failure: %s", 
                      krb_err_txt[auth] );
            pop_log ( p, POP_WARNING, HERE, "%s: (%s.%s@%s) %s", 
                      p->client, 
                      kdata.pname, kdata.pinst, kdata.prealm, 
                      krb_err_txt[auth] );

            return ( POP_FAILURE );
        } /* auth != KSUCCESS */

        DEBUG_LOG4 ( p, "%s.%s@%s (%s): ok", kdata.pname, 
                     kdata.pinst, kdata.prealm, inet_ntoa(addr->sin_addr) );

        strncpy ( p->user, kdata.pname, sizeof(p->user) );
        p->AuthType = kerberos;

    } /* p->bKerberos */
#  endif /* KRB4 */

#  ifdef KRB5
    krb5_auth_context  auth_context = NULL;
    krb5_error_code    retval;
    krb5_principal     server       = NULL;
    krb5_ticket       *ticket;
    char               v4_instance[INST_SZ];            /* V4 instance */
    char               v4_version[9];                   /* V4 version */
    char               krb_vers[KRB_SENDAUTH_VLEN + 1]; /* Version from sendauth */
    AUTH_DAT          *v4_kdata;                        /* Authorization data */
    krb5_int32         auth_sys      = 0;
    int                sock          = 0;

    if ( p->bKerberos ) {
        retval = krb5_init_context ( &pop_context );
        if ( retval ) {
            pop_msg ( p, POP_FAILURE, HERE,
                      "server '%s' mis-configured, cannot initialize "
                      "Kerberos--%s", 
                      p->myhost, error_message(retval) );
            pop_log ( p, POP_WARNING, HERE,
                      "%s: krb5_init_context() failed--%s",
                      p->client, error_message(retval) );
            EXIT ( -1 );
        } /* krb5_init_context failed */

#    ifndef ACCEPT_ANY_PRINCIPAL
        retval = krb5_sname_to_principal ( pop_context, p->myhost,
                                           p->kerberos_service, 
                                           KRB5_NT_SRV_HST, &server );
        if ( retval ) {
            pop_msg ( p, POP_FAILURE, HERE,
                      "server '%s' mis-configured, can't get principal--%s",
                      p->myhost, error_message(retval) );
            pop_log ( p, POP_WARNING, HERE,
                      "%s: mis-configured, can't get principal--%s",
                      p->client, error_message(retval) );
            EXIT ( -1 );
        } /* krb5_sname_to_principal failed */
#    endif /* ACCEPT_ANY_PRINCIPAL */

        /*
         * Since the instance gets filled in, we need to have room for it
         */

        strcpy ( v4_instance, "*" );

        /*
         * Note that here we're using krb5_compat_recvauth so we can handle
         * _both_ V5 and V4 pop services.
         */

        retval = krb5_compat_recvauth ( pop_context, &auth_context,
                               (krb5_pointer) &sock,
                               KRB5_APPL_VERSION, server,
                               0,              /* no flags */
                               NULL,           /* default keytab */
                               0,              /* V4 options */
                               p->kerberos_service,    /* V4 service */
                               v4_instance,    /* V4 instance */
                               addr,           /* Remote address */
                               NULL,           /* Local address (unused) */
                               "",             /* Use default srvtab */
                               &ticket,        /* V5 ticket for client name */
                               &auth_sys,      /* Authentication type */
                               &v4_kdata,      /* V4 kerberos data */
                               NULL,           /* Key schedule (unused) */
                               &v4_version     /* V4 version */
                                      );
        if ( retval ) {
            pop_msg ( p, POP_FAILURE, HERE, "recvauth failed--%s",
                      error_message(retval) );
            pop_log ( p, POP_WARNING, HERE, "%s: recvauth failed--%s",
                      p->client, error_message(retval) );
            EXIT ( -1 );
        } /* krb5_compat_recvauth failed */

        krb5_free_principal ( pop_context, server );

#    ifdef KRB5_KRB4_COMPAT

        /*
         * Handle the case if we were talking to a V4 sendauth
         */

        if ( auth_sys == KRB5_RECVAUTH_V4 ) {

            retval = krb5_425_conv_principal ( pop_context, 
                                               v4_kdata->pname,
                                               v4_kdata->pinst,
                                               v4_kdata->prealm,
                                               &ext_client );
            if ( retval ) {
                pop_msg ( p, POP_FAILURE, HERE,
                          "unable to convert V4 principal to V5--%s",
                          error_message(retval) );
                pop_log ( p, POP_DEBUG, HERE, 
                          "unable to convert V4 principal (%s)", 
                          inet_ntoa(addr->sin_addr) );
                EXIT ( -1 );
            } /* krb5_425_conv_principal failed */
        } /* auth_sys == KRB5_RECVAUTH_V4 */
        else
#    endif /* KRB5_KRB4_COMPAT */
        if ( auth_sys == KRB5_RECVAUTH_V5 ) {

            krb5_auth_con_free ( pop_context, auth_context );

            retval = krb5_copy_principal ( pop_context,
                                           ticket->enc_part2->client,
                                           &ext_client );
            if ( retval ) {
                pop_msg ( p, POP_FAILURE, HERE, 
                          "unable to copy principal--%s",
                          error_message(retval) );
                pop_log ( p, POP_DEBUG, HERE, 
                          "unable to copy principal (%s)",
                          inet_ntoa(addr->sin_addr) );
                EXIT ( -1 );
            } /* krb5_copy_principal failed */

            krb5_free_ticket ( pop_context, ticket );

        } /* auth_sys == KRB5_RECVAUTH_V5 */
        else {
            pop_msg ( p, POP_FAILURE, HERE, "unknown authentication type--%d",
                      auth_sys);
            pop_log ( p, POP_DEBUG, HERE, "unknown authentication type (%s)",
                      inet_ntoa(addr->sin_addr) );
            EXIT ( -1 );
        }

        retval = krb5_unparse_name ( pop_context, ext_client, &client_name );
        if ( retval ) {
            pop_msg ( p, POP_FAILURE, HERE, "name not parsable--%s",
                      error_message(retval) );
            pop_log ( p, POP_DEBUG, HERE, "name not parsable (%s)",
                      inet_ntoa(addr->sin_addr) );
            EXIT ( -1 );
        } /* krb5_unparse_name failed */

        DEBUG_LOG2 ( p, "%s (%s): ok", 
                     client_name, inet_ntoa(addr->sin_addr) );

#    ifdef NO_CROSSREALM
        retval = krb5_aname_to_localname ( pop_context, ext_client, 
                                           sizeof(p->user), p->user );
        if ( retval ) {
            pop_msg ( p, POP_FAILURE, HERE,
                      "unable to convert aname(%s) to localname --%s",
                      client_name, error_message(retval) );
            pop_log ( p, POP_DEBUG, HERE, "unable to convert aname to "
                      "localname (%s)", 
                      client_name );
            EXIT ( -1 );
        } /* krb5_aname_to_localname failed */
#    endif /* NO_CROSSREALM */
    }
#  endif /* KRB5 */
#else  /* not KERBEROS */
    (void) p;
    (void) addr;
#endif /* KERBEROS */

    return ( POP_SUCCESS );
}

/* 
 *  init:   Start a Post Office Protocol session
 */
FILE *g_pTrace = NULL;

int
pop_init ( p, argcount, argmessage )
POP     *       p;
int             argcount;
char    **      argmessage;
{

    struct sockaddr_in      cs;                 /*  Communication parameters */
    struct hostent      *   ch;                 /*  Client host information */
    int                     errflag = 0;
    int                     c;
    int                     len;
    extern char         *   optarg;
    int                     options = 0;
    int                     sp = 0;             /*  Socket pointer */
    int                     pw_handling = 0;    /*  How clear text passwords handled */
    int                     tls_support = 0;    /*  Desired TLS/SSL support */
    struct hostent      *   hp = NULL;


#if DISABLE_CANNONICAL_LOOKUP
    no_rev_lookup = 1;
#else
    no_rev_lookup = 0;
#endif

    /*  
     * Initialize the POP parameter block 
     */
    bzero ( (char *) p, sizeof ( POP ) );

    /*  
     * Initialize maildrop status variables in the POP parameter block 
     *
     * Note that we zeroed the block above, so we really only need to
     * set any desired non-zero/non-NULL values.
     */
    p->bulldir             = BULLDIR;
    p->AuthType            = noauth;
    p->AuthState           = none;
    p->AllowClearText      = ClearTextDefault;
    p->check_lock_refresh  = 5000; /* just a wild guess */

#ifdef     KERBEROS
    p->kerberos_service    = KERBEROS_SERVICE;
    DEBUG_LOG1 ( p, "Set kerberos_service to %s", p-.kerberos_service );
#endif  /* KERBEROS */

#ifdef    SERVER_MODE
    p->server_mode         = TRUE;
    DEBUG_LOG0 ( p, "Server mode now on by default for all users" );
#endif /* SERVER_MODE */

#ifdef    BULLDB
#  ifdef    HAVE_USLEEP
    p->bulldb_max_tries    = 75; /* when sleeping for microseconds */
#  else
    p->bulldb_max_tries    = 10; /* when sleeping for seconds */
#  endif /* HAVE_USLEEP */
    DEBUG_LOG1 ( p, "Set bulldb_max_tries to %d", p->bulldb_max_tries );
#endif /* BULLDB */

#ifdef    USE_BULL_GROUPS
    p->bGroup_bulls        = TRUE;
    DEBUG_LOG0 ( p, "Using group bulletins" );
#endif /* USE_BULL_GROUPS */

#ifdef    AUTO_DELETE
    p->expire              = 0;
    p->bAuto_delete        = TRUE;
    DEBUG_LOG0 ( p, "Auto-expiring messages" );
#endif /* AUTO_DELETE */

#ifdef    FAST_UPDATE
    p->bFast_update         = TRUE;
    DEBUG_LOG0 ( p, "Set fast-update" );
#endif /* FAST_UPDATE */

#ifdef    DO_TIMING
    p->bDo_timing          = TRUE;
    DEBUG_LOG0 ( p, "Generating timing records" );
#endif /* DO_TIMING */

#ifndef       DONT_CHECK_OLD_SPOOL
    p->bCheck_old_spool_loc= TRUE;
#else
    DEBUG_LOG0 ( p, "Omitting check for old temp files in old locations" );
#endif /* not DONT_CHECK_OLD_SPOOL */

#ifndef       DONT_CHECK_HASH_SPOOL_DIR
    p->bCheck_hash_dir     = TRUE;
#else
    DEBUG_LOG0 ( p, "Omitting check for hashed spool directories" );
#endif /* not DONT_CHECK_HASH_SPOOL_DIR */

#ifdef    CHECK_UW_KLUDGE
    p->bUW_kluge           = TRUE;
    DEBUG_LOG0 ( p, "Checking for and hiding UW folder status messages" );
#endif /* CHECK_UW_KLUDGE */

#ifndef       DONT_CHECK_SP_MAX
    p->bCheck_pw_max       = TRUE;
#else
    DEBUG_LOG0 ( p, "Omitting check for expired passwords" );
#endif /* not DONT_CHECK_SP_MAX */

#ifdef    KEEP_TEMP_DROP
    p->bKeep_temp_drop     = TRUE;
    DEBUG_LOG0 ( p, "Keeping temp drop files around" );
#endif /* KEEP_TEMP_DROP */

#ifndef       NO_STATUS
    p->bUpdate_status_hdrs = TRUE;
#else
    DEBUG_LOG0 ( p, "Will not generate/update Status/UIDL headers" );
#endif /* not NO_STATUS */

#ifdef    HASH_SPOOL
    p->hash_spool          = HASH_SPOOL;
    DEBUG_LOG1 ( p, "Using hashed spool dirs; method %d", p->hash_spool );
#endif /* HASH_SPOOL */

#ifdef    HOMEDIRMAIL
    p->pHome_dir_mail      = HOMEDIRMAIL;
    DEBUG_LOG1 ( p, "Spool is %s in user's home dir", p->pHome_dir_mail );
#endif /* HOMEDIRMAIL */

#ifdef    OLD_STYLE_UIDL
    p->bOld_style_uid      = TRUE;
    DEBUG_LOG0 ( p, "Generating old (pre-3.x) style UIDs" );
#endif /* OLD_STYLE_UIDL */

#ifndef       NOUPDATEONABORT
    p->bUpdate_on_abort    = TRUE;
#else
    DEBUG_LOG0 ( p, "Will not enter UPDATE state on abort" );
#endif /* not NOUPDATEONABORT */

#ifdef    SERVER_MODE_GROUP_INCL
    p->grp_serv_mode       = SERVER_MODE_GROUP_INCL;
    DEBUG_LOG1 ( p, "Server mode on by default for users in group %s",
                 p->grp_serv_mode );
#endif /* SERVER_MODE_GROUP_INCL */

#ifdef    SERVER_MODE_GROUP_EXCL
    p->grp_no_serv_mode    = SERVER_MODE_GROUP_EXCL;
    DEBUG_LOG1 ( p, "Server mode off by default for users in group %s",
                 p->grp_no_serv_mode );
#endif /* SERVER_MODE_GROUP_EXCL */

#ifdef    AUTHFILE
    p->authfile            = AUTHFILE;
    DEBUG_LOG1 ( p, "Permitting access only to users listed in %s",
                 p->authfile );
#endif /* AUTHFILE */

#ifdef    NONAUTHFILE
    p->nonauthfile         = NONAUTHFILE;
    DEBUG_LOG1 ( p, "Denying access to users listed in %s", p->nonauthfile );
#endif /* NONAUTHFILE */

#ifdef    SHY
    p->bShy                = TRUE;
    DEBUG_LOG0 ( p, "Hiding version" );
#endif /* SHY */

#ifdef    MAIL_COMMAND
    p->pMail_command       = MAIL_COMMAND;
    DEBUG_LOG1 ( p, "Mail command (for XTND XMIT) is '%s'", p->pMail_command );
#endif /* MAIL_COMMAND */

/* Location of the spool directory */
#ifdef    POP_MAILDIR
    p->pCfg_spool_dir      = POP_MAILDIR;
    DEBUG_LOG1 ( p, "Spool directory: %s", p->pCfg_spool_dir );
#endif /* POP_MAILDIR */

/* Location of the temporary mail drop directory */
#ifdef    POP_DROP_DIR
    p->pCfg_temp_dir       = POP_DROP_DIR;
    DEBUG_LOG1 ( p, "Temp drop directory: %s", p->pCfg_temp_dir );
#endif /* POP_DROP_DIR */

/* The temporary mail drop file name constant */
#ifdef    POP_DROP
    p->pCfg_temp_name      = POP_DROP;
    DEBUG_LOG1 ( p, "Temp drop file name: %s", p->pCfg_temp_name );
#endif /* POP_DROP */

/* Location of the cache directory */
#ifdef    POP_CACHE_DIR
    p->pCfg_cache_dir      = POP_CACHE_DIR;
    DEBUG_LOG1 ( p, "Cache directory: %s", p->pCfg_cache_dir );
#endif /* POP_CACHE_DIR */

/* The cache file name constant */
#ifdef    POP_CACHE
    p->pCfg_cache_name     = POP_CACHE;
    DEBUG_LOG1 ( p, "Cache file name: %s", p->pCfg_cache_dir );
#endif /* POP_CACHE */

#ifdef    CHUNKY_WRITES
    p->chunky_writes       = (chunky_type) CHUNKY_WRITES;
    DEBUG_LOG1 ( p, "Set chunky_writes to %d", (int) p->chunky_writes );
#endif /* CHUNKY_WRITES */

#ifdef    NO_ATOMIC_OPEN
    p->bNo_atomic_open     = TRUE;
    DEBUG_LOG0 ( p, "Set no_automic_open" );
#endif /* NO_ATOMIC_OPEN */

#ifdef    NEWBULLCNT
    p->nMaxBulls           = NEWBULLCNT;
#else
    p->nMaxBulls           = 1;
#endif /* NEWBULLCNT */
    DEBUG_LOG1 ( p, "Set max-bulletins to %d", p->nMaxBulls );

#ifndef   POP_FACILITY
#  if defined(OSF1) || defined(LINUX)
#    define POP_FACILITY    LOG_MAIL
#  else
#    define POP_FACILITY    LOG_LOCAL0
#  endif /* OSF1 or Linux */
#endif /* POP_FACILITY not defined */

    p->log_facility        = (log_facility_type) POP_FACILITY;

#ifdef    LOG_LOGIN
    p->pLog_login = "(v%0) POP login by user \"%1\" at (%2) %3";
#endif /* LOG_LOGIN */

    StackInit ( & ( p->InProcess ) );
    
    /*  
     * Save my name in a global variable 
     */
    p->myname = argmessage [ 0 ];
    if ( p->myname != NULL && *p->myname != '\0' )
    {
        char *ptr = NULL;
        ptr = strrchr ( p->myname, '/' );
        if ( ptr != NULL && strlen(ptr) > 1 )
            p->myname = ptr + 1;
    }


    /*  
     * Open the log file 
     */
#ifdef SYSLOG42
    openlog ( p->myname, 0 );
#else
    openlog ( p->myname, POP_LOGOPTS, p->log_facility );
#endif

    /*  
     * Process command line arguments 
     */
    while ( ( c = getopt ( argcount, argmessage, "b:BcCdD:e:f:FkK:l:L:p:RsSt:T:uUvy:") ) != EOF )
        switch ( c ) {

        case 'b': /* Bulletins requested */
            p->bulldir = optarg;
            DEBUG_LOG1 ( p, "bulletin directory = %s", p->bulldir );
            break;

#ifdef    BULLDB
        case 'B': /* Allow session to proceed without bulletin DB */
            p->bulldb_nonfatal = TRUE;
            DEBUG_LOG0 ( p, "Sessions can proceed without bulletin db" );
            break;
#endif /* BULLDB */

        case 'c': /* Force user names to be lower case */
            p->bDowncase_user = TRUE;
            DEBUG_LOG0 ( p, "user names will be downcased (-c)" );
            break;

        case 'C': /* Trim domain name from user name */
            p->bTrim_domain = TRUE;
            DEBUG_LOG0 ( p, "trimming domain names from user names (-C)" );
            break;

        case 'd': /* Debugging requested */
            p->debug = TRUE;
            options |= SO_DEBUG;
            DEBUG_LOG0 ( p, "Debugging turned on (-d)" );
            break;

#ifdef    DRAC_AUTH
        case 'D': /* drac host specified */
            p->drac_host = optarg;
            DEBUG_LOG1 ( p, "drac host is %.64s", p->drac_host );
            break;
#endif /* DRAC_AUTH */

        case 'e': /* Pop Extensions */
            {     /* -e login_delay=xx,expire=xx */
                char *extn_options[] = { "login_delay", "expire", NULL };
                char *value, *option = optarg;
                while ( *option ) {
                    switch ( get_sub_option ( &option, extn_options, &value ) ) {
                    case 0:
                        p->login_delay = value ? atoi ( value ) : -1;
                        DEBUG_LOG1 ( p, "announcing login_delay %d", p->login_delay );
                        break;
                    case 1:
                        p->expire = value ? atoi ( value ) : -1;
                        DEBUG_LOG1 ( p, "announcing expire %d", p->expire );
                        break;
                    }
                }
            }
            break;

        case 'f': /* Get options from a file */
            if ( pop_config ( p, optarg, CfgInit ) == POP_FAILURE ) {
                fprintf ( stderr, "Unable to process config file %s\n",
                          optarg );
                EXIT ( 1 );
            }
            break;

        case 'F': /* Set fast-update */
            p->bFast_update = TRUE;
            DEBUG_LOG0 ( p, "set fast-update" );
            break;

#ifdef KERBEROS
        case 'k':
            p->bKerberos = TRUE;
            DEBUG_LOG0 ( p, "Using Kerberos (-k)" );
            break;

        case 'K':
            p->kerberos_service = optarg;
            DEBUG_LOG1 ( p, "Kerberos service: %.64s", 
                         p->kerberos_service );
            break;
#endif  /* KERBEROS */


        case 'l': /* TLS/SSL handling */
            tls_support = atoi ( optarg );
            switch ( tls_support ) {
                case 0:
                    p->tls_support = QPOP_TLS_NONE;
                    break;
                case 1:
                    p->tls_support = QPOP_TLS_STLS;
                    break;
                case 2:
                    p->tls_support = QPOP_TLS_ALT_PORT;
                    break;
                default:
                    fprintf ( stderr, "Unrecognized -l value; 0 = default; "
                                      "1 = stls; 2 = alternate-port\n" );
                    errflag++;
            }
            DEBUG_LOG1 ( p, "tls-support=%d (-l)", tls_support );
            break;

        case 'L': /*  Check if we need to touch the mail lock every
                   *  this many messages (crude)  */
            p->check_lock_refresh = atoi(optarg);
            DEBUG_LOG1 ( p, "Checking for maillock refresh every %i msgs", 
                         p->check_lock_refresh );
            break;

        case 'p': /*  Set clear text password handling */
            pw_handling = atoi ( optarg );
            switch ( pw_handling ) {
                case 0:
                    p->AllowClearText = ClearTextDefault;
                    break;
                case 1:
                    p->AllowClearText = ClearTextNever;
                    break;
                case 2:
                    p->AllowClearText = ClearTextAlways;
                    break;
                case 3:
                    p->AllowClearText = ClearTextLocal;
                    break;
                case 4:
                    p->AllowClearText = ClearTextTLS;
                    break;
                default:
                    fprintf ( stderr, "Unrecognized -p value; 0 = default; "
                                      "1 = never; 2 = always (fallback); "
                                      "3 = local only; 4 = TLS/SSL only\n" );
                    errflag++;
            }
            DEBUG_LOG1 ( p, "Plain-text password handling=%d (-p)",
                         pw_handling );

#if !defined(APOP) && !defined(SCRAM)
            if ( p->AllowClearText == ClearTextNever ) {
                fprintf ( stderr, "Can't set -p to 1 without enabling APOP\n" );
                errflag++;
                DEBUG_LOG0 ( p, "Can't set -p to 1 without enabling APOP" );
            }
#endif /* not APOP and not SCRAM */

#ifndef   QPOP_SSL
            if ( p->AllowClearText == ClearTextTLS ) {
                fprintf ( stderr, "Can't set -p to 4 without enabling TLS/SSL\n" );
                errflag++;
                DEBUG_LOG0 ( p, "Can't set -p to 1 without enabling TLS/SSL" );
            }
#endif /* QPOP_SSL */

            break;

        case 'R': /*  Omit reverse lookups on client IP */
            no_rev_lookup = TRUE;
            DEBUG_LOG0 ( p, "Avoiding reverse lookups (-R)" );
            break;

        case 's': /* Stats requested */
            p->bStats = TRUE;
            DEBUG_LOG0 ( p, "Will generate stats records (-s)" );
            break;

        case 'S': /* Server mode */
                p->server_mode = TRUE;
                DEBUG_LOG0 ( p, "server mode is the default (-S)" );
                break;

        case 't': /*  Debugging trace file specified */
            p->debug = TRUE;

#ifdef STANDALONE
            {
            extern FILE *trace_file;
            p->trace = trace_file;
            }
#else  /* not STANDALONE */
            p->trace = fopen ( optarg, "a" );
#endif /* STANDALONE */

            if ( p->trace == NULL ) {
                pop_log ( p, POP_PRIORITY, HERE,
                          "Unable to open trace file \"%s\": %s (%d)",
                          optarg, STRERROR(errno), errno );
                fprintf ( stderr, "Unable to open trace file \"%s\": %s (%d)\n",
                          optarg, STRERROR(errno), errno );
                EXIT ( 1 );
            }
            p->trace_name = optarg;
            DEBUG_LOG1 ( p, "Trace and Debug destination is file \"%s\"",
                         p->trace_name );
            break;

        case 'T': /*  Timeout value passed.  Default changed */
            pop_timeout = atoi ( optarg );
            DEBUG_LOG1 ( p, "timeout = %i (-T)", pop_timeout );
            break;

        case 'u': /* Use ~/.qpopper-options file */
            p->bUser_opts = TRUE;
            DEBUG_LOG0 ( p, "Will use ~/qpopper-options file (-u)" );
            break;

        case 'U': /* Use .qpopper-options file in spool location */
            p->bSpool_opts = TRUE;
            DEBUG_LOG0 ( p, "Will use .qpopper-options file in spool "
                            "directory" );
            break;

        case 'v': /* RESERVED (handled in main.c) -- DO NOT USE */
            break;

        case 'y': /* log facility */
            if ( set_option ( p, "log-facility", optarg ) == POP_SUCCESS ) {
                closelog();
#ifdef SYSLOG42
                openlog ( p->myname, 0 );
#else
                openlog ( p->myname, POP_LOGOPTS, p->log_facility );
#endif
            } else {
                fprintf ( stderr, "Error setting '-y' to %s\n", optarg );
                errflag++;
            }

        default: /*  Unknown option received */
            errflag++;
        }

    /*  Exit if bad options specified */

#ifdef AUTO_DELETE
    if ( p->expire != 0 ) {
        fprintf ( stderr, "EXPIRE must be 0 when AUTO_DELETE defined.\n" );
        errflag++;
    }
#endif /* AUTO_DELETE */

    if ( errflag ) {

#ifdef    BULLDB
        char *xB = "[-B] ";
#else
        char *xB = "";
#endif /* BULLDB */

#ifdef    DRAC_AUTH
        char *xD = "[-D drac-host] ";
#else
        char *xD = "";
#endif /* DRAC_AUTH */

#if defined(APOP) || defined(SCRAM)
        char *xp = "[-p 0-4] ";
#else
        char *xp = "";
#endif /* APOP or SCRAM */

#ifdef KERBEROS
        char *xk = "[-k] ";
        char *xK = "[-K service] ";
#else
        char *xk = "";
        char *xK = "";
#endif /* KERBEROS */

#ifdef QPOP_SSL
        char *xl = "[-l 0-2] ";
#else
        char *xl = "";
#endif /* QPOP_SSL */


        fprintf ( stderr, "Usage: %s "
                          "[-b bulldir] "
                          "%s"         /* -B / bulldb_nonfatal, if BULLDB set */
                          "[-c] "
                          "[-C] "
                          "[-d] "
                          "[-e login_delay=nn,expire=nn] "
                          "[-f config-file] "
                          "[-F] "
                          "%s"          /* -D / drac-host, if DRAC set */
                          "%s"          /* -k / kerberos, if KERBEROS set */
                          "%s"          /* -K / kerberos service, if KERBEROS set */
                          "%s"          /* -l / tls_support, if QPOP_SSL set */
                          "[-L lock refresh] "
                          "%s"          /* -p / plain-text passwd stuff, if APOP or SCRAM */
                          "[-R] "
                          "[-s] "
                          "[-S] "
                          "[-t trace-file] "
                          "[-T timeout] "
                          "[-u] "
                          "[-U] "
                          "[-v] "
                          "\n",
                  argmessage[0],
                  xB,                   /* -B (or not) */
                  xD,                   /* -D (or not) */
                  xk,                   /* -k (or not) */
                  xK,                   /* -K (or not) */
                  xl,                   /* -l (or not) */
                  xp );                 /* -p (or not) */
        EXIT ( 1 );
    }

#ifndef   QPOP_SSL
    if ( p->tls_support != QPOP_TLS_NONE ) {
        /* This the main ifdef for SSL in the bulk of qpopper. Most of the
           dependencies are in pop_tls.c */
         fprintf ( stderr, "TLS/SSL (the -l option) is not supported "
                           "in this build.\nDetails on adding suport are in "
                           "the installation instructions\n" );
         EXIT ( 1 );
    }
#endif /* QPOP_SSL */


    /* set defaults if needed */
#ifdef    DRAC_AUTH
    if ( p->drac_host == NULL )
        p->drac_host = strdup ( "localhost" );
#endif /* DRAC_AUTH */

    /*  
     * Get the name of our host 
     */
    p->myhost = (char *) malloc ( MAXHOSTNAMELEN +1 );
    if ( p->myhost == NULL ) {
        perror ( "malloc" );
        EXIT ( 1 ); 
    }
#ifdef _DEBUG
    p->ipaddr = strdup("* _DEBUG *");
    p->client = p->ipaddr;
#else
    (void) gethostname ( p->myhost, MAXHOSTNAMELEN );
    hp = gethostbyname ( p->myhost );
    if ( hp != NULL ) {
        if ( index ( hp->h_name, '.' ) == NULL ) {         /* FQN not returned */
            /*
             * SVR4 resolver is stupid and returns h_name as whatever
             * you gave gethostbyname.  Thus do a reverse lookup
             * on the first address and hope for the best.
             */
            u_long x = *(u_long *) hp->h_addr_list [ 0 ];
            hp = gethostbyaddr ( (char *) &x, 4, AF_INET );
            if ( hp != NULL ) {
                (void) strncpy ( p->myhost, hp->h_name, MAXHOSTNAMELEN );
                p->myhost [ MAXHOSTNAMELEN ] = '\0';
            }
        } /* index ( hp->h_name, '.' ) == NULL */
        else {
            (void) strncpy ( p->myhost, hp->h_name, MAXHOSTNAMELEN );
            p->myhost [ MAXHOSTNAMELEN ] = '\0';
        }
    }

    /*  
     * Get the address and socket of the client to whom I am speaking 
     */
    len = sizeof(cs);
    if ( getpeername ( sp, (struct sockaddr *) &cs, &len ) < 0 ) {
        pop_log ( p, POP_PRIORITY, HERE,
                  "Unable to obtain socket and address of client: %s (%d)",
                  STRERROR(errno), errno );
        EXIT ( 1 );
    }

    /*
     * Save the dotted decimal form of the client's IP address 
     * in the POP parameter block 
     */
    p->ipaddr = (char *) strdup ( inet_ntoa ( cs.sin_addr ) );

    /*  
     * Save the client's port 
     */
    p->ipport = ntohs ( cs.sin_port );

    if ( no_rev_lookup )
        p->client = p->ipaddr;
    else {
        /*  
         * Get the canonical name of the host to whom I am speaking 
         */
        ch = gethostbyaddr ( (char *) &cs.sin_addr, sizeof(cs.sin_addr), AF_INET );
        if ( ch == NULL ) {
            pop_log ( p, POP_DEBUG, HERE,
                      "(v%s) Unable to get canonical name of client %s: %s (%d)",
                      VERSION, p->ipaddr, hstrerror(h_errno), h_errno );
            p->client = p->ipaddr;
        }
        /*  
         * Save the cannonical name of the client host in 
         * the POP parameter block 
         */
        else {

#ifndef BIND43
            p->client = (char *) strdup ( ch->h_name );
#else

# ifndef SCOR5
#       include <arpa/nameser.h>
#       include <resolv.h>
# endif

        /*  Distrust distant nameservers */

#if !(defined(BSD) && (BSD >= 199103)) && !defined(OSF1) && !defined(HPUX10)
# if (!defined(__RES)) || (__RES < 19940415)
#  ifdef SCOR5
        extern struct __res_state       _res;
#  else
        extern struct state         _res;
#  endif
# endif
#endif
        struct hostent      *   ch_again;
        char            *   *   addrp;
        char                    h_name[MAXHOSTNAMELEN + 1];

        /*  We already have a fully-qualified name */
#ifdef RES_DEFNAMES
        _res.options &= ~RES_DEFNAMES;
#endif

        strncpy(h_name, ch->h_name, sizeof(h_name));

        /*  See if the name obtained for the client's IP 
            address returns an address */
        if ( ( ch_again = gethostbyname(h_name) ) == NULL ) {
            pop_log (p, POP_PRIORITY, HERE,
                     "Client at \"%s\" resolves to an unknown host name \"%s\"",
                     p->ipaddr, h_name );
            p->client = p->ipaddr;
        }
        else {
            /*  Save the host name (the previous value was 
                destroyed by gethostbyname) */
            p->client = (char *)strdup(ch_again->h_name);

            /*  Look for the client's IP address in the list returned 
                for its name */
            for ( addrp=ch_again->h_addr_list; *addrp; ++addrp )
                if ( bcmp ( *addrp, &(cs.sin_addr), sizeof(cs.sin_addr) ) == 0 ) break;

                if ( !*addrp ) {
                    pop_log ( p, POP_PRIORITY, HERE,
                              "Client address \"%s\" not listed for its host name \"%s\"",
                              p->ipaddr, h_name );
                    p->client = p->ipaddr;
                }
        }

#ifdef RES_DEFNAMES
        /* 
         *  Must restore nameserver options since code in crypt uses
         *  gethostbyname call without fully qualified domain name!
         */
        _res.options |= RES_DEFNAMES;
#endif

#endif /* BIND43 */
                }

    } /* no_rev_lookup */

#endif /* _DEBUG */

    /* Create input file stream for TCP/IP communication */
    /* Just use standard input */
    p->input_fd = 0;

#ifdef _DEBUG
    sp = 1;
#endif
    /*  Create output file stream for TCP/IP communication */
    if ( ( p->output = fdopen ( sp, "w" ) ) == NULL ) {
        pop_log ( p, POP_PRIORITY, HERE,
                  "Unable to open communication stream for output: %s (%d)",\
                  STRERROR(errno), errno);
        EXIT ( 1 );
    }

    DEBUG_LOG3 ( p, "(v%s) Servicing request from \"%s\" at %s",
                 VERSION, p->client, p->ipaddr );
    return ( kerb_authenticate ( p, &cs ) );
}

