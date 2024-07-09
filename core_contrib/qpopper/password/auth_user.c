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
 *     03/09/01 [RCG]
 *              - File added.
 *
 */



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

#include "snprintf.h"





static const char *ERRMSG_PW = "Password mismatch for user \"%s\"";
static const char *ERRMSG_NO = "No shadow password entry for user \"%s\"";



/*------------------------------------- SPEC_POP_AUTH */
#ifdef SPEC_POP_AUTH
    char *crypt();

#  ifndef USE_PAM


/*----------------------------------------------- SUNOS4 and not ISC */
#    if defined(SUNOS4) && !defined(ISC)

#      define DECLARED_AUTH_USER

#      include <sys/label.h>
#      include <sys/audit.h>
#      include <pwdadj.h>

pop_result
auth_user ( POP *p, char *password, struct passwd *pw, char *err_buf, int err_len )
{
    struct passwd_adjunct *pwadj;

    /*  
     * Look for the user in the shadow password file 
     */
    pwadj = getpwanam ( p->user );
    if ( pwadj == NULL ) {
        Qsnprintf ( err_buf, err_len, ERRMSG_NO, p->user );
        return POP_FAILURE;
    }

    /*  
     *  Compare the supplied password with the password file entry 
     */
    if ( strcmp ( crypt ( password, pwadj->pwa_passwd ), pwadj->pwa_passwd ) ) {
        Qsnprintf ( err_buf, err_len, 7, p->user );
        return POP_FAILURE;
    }

    return POP_SUCCESS;
}

#    endif  /* SUNOS4 and not ISC */


/*----------------------------------------------- SOLARIS2 or AUX or UXPDS or SGI */
#    if defined(SOLARIS2) || defined(AUX) || defined(UXPDS) || defined(sgi)

#      define DECLARED_AUTH_USER

#      ifdef HAVE_SHADOW_H
#        include <shadow.h>
#      endif /* HAVE_SHADOW_H */

pop_result
auth_user ( POP *p, char *password, struct passwd *pw, char *err_buf, int err_len )
{
    register struct spwd *pwd;

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
            Qsnprintf ( err_buf, err_len, ERRMSG_NO, p->user );
            return POP_FAILURE;
        }
    }

    /*  
     *  Compare the supplied password with the password file entry 
     */
    if ( strcmp ( crypt ( password, pwd->sp_pwdp ), pwd->sp_pwdp ) ) {
        Qsnprintf ( err_buf, err_len, ERRMSG_PW, p->user );
        endspent();
        return POP_FAILURE;
    }

    endspent();
    return ( POP_SUCCESS );
}

#    endif  /* SOLARIS2 or AUX or UXPDS or sgi */


/*----------------------------------------------- PIX or ISC */
#    if defined(PTX) || defined(ISC) 

#      define DECLARED_AUTH_USER

#      ifdef HAVE_SHADOW_H
#        include <shadow.h>
#      endif /* HAVE_SHADOW_H */

pop_result
auth_user ( POP *p, char *password, struct passwd *pw, char *err_buf, int err_len )
{
    register struct spwd *pwd;


    /*  
     * Look for the user in the shadow password file 
     */
    pwd = getspnam ( p->user );
    if ( pwd == NULL ) {
        if ( !strcmp ( pw->pw_passwd, "x" ) ) {      /* This my be a YP entry */
            Qsnprintf ( err_buf, err_len, ERRMSG_NO, p->user );
            return POP_FAILURE;
        }
    }

    /*  
     *  Compare the supplied password with the password file entry 
     */
    if ( strcmp ( crypt ( password, pwd->sp_pwdp ), pwd->sp_pwdp ) ) {
        Qsnprintf ( err_buf, err_len, ERRMSG_PW, p->user );
        return POP_FAILURE;
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

#    define QPASSWD(p)       p->ufld.fd_encrypt

pop_result
auth_user ( POP *p, char *password, struct passwd *pw, char *err_buf, int err_len )
{
    register struct pr_passwd *pr;

    pr = getprpwnam ( p->user );
    if ( pr == NULL ) {
        if ( !strcmp ( pw->pw_passwd, "x" ) ) {
            Qsnprintf ( err_buf, err_len, ERRMSG_NO, p->user );
            return POP_FAILURE;
        }

    if ( ( strcmp ( bigcrypt ( password, pw->pw_passwd ), pw->pw_passwd ) &&
           strcmp ( crypt    ( password, pw->pw_passwd ), pw->pw_passwd )
         ) 
        ) {
        Qsnprintf ( err_buf, err_len, ERRMSG_PW, p->user );
        return POP_FAILURE;
        }
    } else {
        /* 
         * Compare the supplied password with the password file entry 
         */
        if ( strcmp ( bigcrypt ( password, QPASSWD(pr) ), QPASSWD(pr) ) &&
             strcmp ( crypt    ( password, QPASSWD(pr) ), QPASSWD(pr) )  ) {
            Qsnprintf ( err_buf, err_len, ERRMSG_PW, p->user );
            return POP_FAILURE;
        }
    }

    return ( POP_SUCCESS );
}

#    endif  /* POPSCO || HPUX */


/*----------------------------------------------- ULTRIX */
#    ifdef ULTRIX

#      define DECLARED_AUTH_USER

#      include <auth.h>

pop_result
auth_user ( POP *p, char *password, struct passwd *pw, char *err_buf, int err_len )
{
    AUTHORIZATION *auth, *getauthuid();

    auth = getauthuid ( pw->pw_uid );
    if ( auth == NULL ) {
        if ( !strcmp(pw->pw_passwd, "x") ) {      /* This my be a YP entry */
            Qsnprintf ( err_buf, err_len, ERRMSG_NO, p->user );
            return POP_FAILURE;
        }
    } else {
        pw->pw_passwd = (char *) strdup ( auth->a_password );
    }

    /*  
     *  Compare the supplied password with the password file entry 
     */
    if ( strcmp ( crypt16 ( password, pw->pw_passwd ), pw->pw_passwd ) &&
         strcmp ( crypt   ( password, pw->pw_passwd ), pw->pw_passwd )  ) {
        Qsnprintf ( err_buf, err_len, ERRMSG_PW, p->user );
        return POP_FAILURE;
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

pop_result
auth_user ( POP *p, char *password, struct passwd *pw, char *err_buf, int err_len )
{
    register struct pr_passwd *pr;

    pr = getprpwnam ( p->user );
    if ( pr == NULL ) {
        if ( !strcmp ( pw->pw_passwd, "x" ) ) {      /* This my be a YP entry */
            Qsnprintf ( err_buf, err_len, ERRMSG_NO, p->user );
            return POP_FAILURE;
        }
    } else {
        pw->pw_passwd = (char *) strdup ( QPASSWD(pr) );
    }

    /*  
     *  Compare the supplied password with the password file entry 
     */
    if ( strcmp ( bigcrypt ( password, pw->pw_passwd ), pw->pw_passwd ) &&
         strcmp ( crypt    ( password, pw->pw_passwd ), pw->pw_passwd )  ) {
        Qsnprintf ( err_buf, err_len, ERRMSG_PW, p->user );
        return POP_FAILURE;
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

pop_result
auth_user ( POP *p, char *password, struct passwd *pw, char *err_buf, int err_len )
{
    register struct spwd * pwd;


    /*  
     * Look for the user in the shadow password file 
     */
    pwd = getspnam ( p->user );
    if ( pwd == NULL ) {
        if ( !strcmp ( pw->pw_passwd, "x" ) ) {      /* This my be a YP entry */
            Qsnprintf ( err_buf, err_len, ERRMSG_NO, p->user );
            return POP_FAILURE;
        }
    } else {
        pw->pw_passwd = (char *) strdup ( pwd->sp_pwdp );
        endspent();
    }

    /*  
     *  Compare the supplied password with the password file entry 
     */
    if ( strcmp ( crypt ( password, pw->pw_passwd ), pw->pw_passwd ) ) {
        Qsnprintf ( err_buf, err_len, ERRMSG_PW, p->user );
        return POP_FAILURE;
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

pop_result
auth_user ( POP *p, char *password, struct passwd *pw, char *err_buf, int err_len )
{
    register struct spwd * pwd;

    /*  
     * Look for the user in the shadow password file 
     */
    pwd = getspnam ( p->user );
    if ( pwd == NULL ) {
        if ( !strcmp ( pw->pw_passwd, "x" ) ) {      /* This my be a YP entry */
            Qsnprintf ( err_buf, err_len, ERRMSG_NO, p->user );
        }
    } else {
        pw->pw_passwd = (char *) strdup ( pwd->sp_pwdp );
        endspent();
    }

    /*  
     *  Compare the supplied password with the password file entry
     *
     *  Kernels older than 2.0.x need pw_encrypt() for shadow support 
     */
    if ( ( strcmp ( crypt      ( password, pw->pw_passwd ), pw->pw_passwd ) &&
#      ifdef HAVE_PW_ENCRYPT
           strcmp ( pw_encrypt ( password, pw->pw_passwd ), pw->pw_passwd )
#      else /* not HAVE_PW_ENCRYPT */
           strcmp ( crypt      ( password, pw->pw_passwd ), pw->pw_passwd )
#      endif /* HAVE_PW_ENCRYPT */
         )
       ) {
        Qsnprintf ( err_buf, err_len, ERRMSG_NO, p->user );
        return POP_FAILURE;
    }

    return ( POP_SUCCESS );
}

#    endif  /* LINUX */


/*----------------------------------------------- BSD/OS */
#    if defined(__bsdi__) && _BSDI_VERSION >= 199608

#      define DECLARED_AUTH_USER

pop_result
auth_user ( POP *p, char *password, struct passwd *pw, char *err_buf, int err_len )
{
    int r;


    /*
     * Compare the supplied password with the password file entry
     */
    r = auth_response ( p->user, p->class->lc_class, p->class->lc_style,
        "response", NULL, p->challenge ? p->challenge : "", password );

    if ( p->challenge ) {
        free ( p->challenge );
        p->challenge = NULL;
    }

    if ( r <= 0 || auth_approve ( p->class, p->user, "poppassd" ) <= 0 ) {
        Qsnprintf ( err_buf, err_len, ERRMSG_PW, p->user );
        return POP_FAILURE;
    }

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


pop_result
auth_user ( POP *p, char *password, struct passwd *pw, char *err_buf, int err_len )
{
    int     reenter   = 0;
    char   *message   = NULL;
    int     rslt      = POP_FAILURE;


    /* 
     * N.B.: authenticate is probably going to blow away struct passwd *pw
     * static area.  Luckily, nobody who got it before auth_user() is expecting it
     * to still be valid after the auth_user() call. 
     */
    if ( authenticate ( p->user, password, &reenter, &message) == 0 ) {
        rslt = POP_SUCCESS;
    } else {
        Qsnprintf ( err_buf, err_len, "Authentication failed for user %s: %s",
                    p->user, message );
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

pop_result
auth_user ( POP *p, char *password, struct passwd *pw, char *err_buf, int err_len )
{
    register struct spwd *pwd;

    /*  
     * Look for the user in the shadow password file 
     */
    pwd = getspnam ( p->user );
    if ( pwd == NULL ) {
        if ( !strcmp ( pw->pw_passwd, "x" ) ) {      /* This my be a YP entry */
            Qsnprintf ( err_buf, err_len, ERRMSG_NO, p->user );
            return POP_FAILURE;
        }
    } else {
        pw->pw_passwd = (char *) strdup ( pwd->sp_pwdp );
    }

    /*  
     * We don't accept connections from users with null passwords 
     *
     *  Compare the supplied password with the password file entry 
     */
    if ( strcmp ( crypt ( password, pw->pw_passwd ), pw->pw_passwd ) ) {
        Qsnprintf ( err_buf, err_len, ERRMSG_PW, p->user );
        return POP_FAILURE;
    }

    return ( POP_SUCCESS );
}

#    endif  /* DECLARED_AUTH_USER */


#  endif /* USE_PAM */


/*------------------------------------- not SPEC_POP_AUTH */
#else   /* not SPEC_POP_AUTH */

char *crypt();

pop_result
auth_user ( POP *p, char *password, struct passwd *pw, char *err_buf, int err_len )
{
    /*  
     *  Compare the supplied password with the password file entry 
     */
    if ( strcmp ( crypt ( password, pw->pw_passwd ), pw->pw_passwd ) ) {
        Qsnprintf ( err_buf, err_len, ERRMSG_PW, p->user );
        return POP_FAILURE;
    }

    return ( POP_SUCCESS );
}

#endif  /* SPEC_POP_AUTH */