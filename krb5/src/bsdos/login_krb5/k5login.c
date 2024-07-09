/*-
 * Copyright (c) 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char sccsid[] = "@(#)k5login.c	8.3 (Berkeley) 4/2/94";
#endif /* not lint */

#ifdef KRB5
#include <sys/param.h>
#include <sys/syslog.h>
#include <krb5.h>

#include <err.h>
#include <netdb.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef KRB4_VERIFY
# define	KRB4_VERIFY_SERVICE	"rcmd"
#endif
#ifdef KRB5_VERIFY
# define	KRB5_VERIFY_SERVICE	"host"
#endif

extern int		notickets;
extern int		always_use_klogin;
extern char		*krb5ccname_env;
extern krb5_context	context;
extern char		*progname;

static char		realm[64];	/* XXX REALM_SZ equiv for krb5? */
static krb5_ccache	ccache;

int k5oktologin __P((char *, char *, char *));

/*
 * Attempt to log the user in using Kerberos authentication
 *
 * return 0 on success (will be logged in)
 *	  1 if Kerberos failed (try local password in login)
 */
int
k5login(pw, instance, localhost, password)
	struct passwd *pw;
	char *instance, *localhost, *password;
{
	struct hostent		*hp;
	unsigned long		faddr;
	char			savehost[MAXHOSTNAMELEN];
	char			cc_name[MAXPATHLEN];
	krb5_error_code		code;
	int			options = (KDC_OPT_FORWARDABLE);
	krb5_timestamp		now;
	krb5_deltat		lifetime = 60*60*10;	/* 10 hours */
	krb5_principal		user_princ, server;
#ifdef KRB5_VERIFY
	krb5_principal		verify_princ;
	krb5_keyblock		*keyblk = NULL;
	krb5_auth_context	auth_context = NULL;
	krb5_data		packet;
	krb5_ticket		*ticket = NULL;
	int			havekeys;	/* Do we have keytab keys? */
#endif
	krb5_creds		my_creds;
	char			*lrealm;

	char *user = strcmp(instance, "root") == 0 ? instance : pw->pw_name;

	/*
	 * Root logins don't use Kerberos.
	 * If we have a realm, try getting a ticket-granting ticket
	 * and using it to authenticate.  Otherwise, return
	 * failure so that we can try the normal passwd file
	 * for a password.  If that's ok, log the user in
	 * without issuing any tickets.
	 */
	if (strcmp(pw->pw_name, "root") == 0 ||
	    realm[0] == '\0' && krb5_get_default_realm(context, &lrealm) != 0)
		return (1);

	if (realm[0] == '\0') {
		strncpy(realm, lrealm, sizeof(realm));
	}

	/* Random initialization stuff... */
	initialize_krb5_error_table();

	if ((code = krb5_timeofday(context, &now))) {
		syslog(LOG_ERR, "krb5_timeofday() failure: %s",
			error_message(code));
		return(1);
	}

	/* Build a krb5_principal for the user we're getting a ticket for... */
	if (instance && (strlen(instance))) {
		code = krb5_build_principal(context, &user_princ,
				strlen(realm), realm,
				pw->pw_name, instance, 0);
		if (code) {
			syslog(LOG_ERR, "while building principal: %s",
				error_message(code));
			return(1);
		}
	} else {
		code = krb5_parse_name(context, pw->pw_name, &user_princ);
		if (code) {
			syslog(LOG_ERR, "while parsing name into principal: %s",
				error_message(code));
			return(1);
		}
	}
					
	/*
	 * Where do we store the ccache?  It's different for root...
	 */
	if (strcmp(instance, "root") != 0) {
		/* Oh, piss.  We can't use krb5_cc_default(), as it will */
		/* incorporate the UID, which is currently 0.  :-(  */
#if should_but_cant_use
		if ((code = krb5_cc_default(context, &ccache))) {
			syslog(LOG_ERR, "while getting default ccache: %s",
				error_message(code));
			return(1);
		}
#else
		/* We'll assume this is the "default" ccache location, and */
		/* not set krb5ccname_env... */
		(void)sprintf(cc_name, "/tmp/krb5cc_%d", pw->pw_uid);
		if ((code = krb5_cc_resolve (context, cc_name, &ccache))) {
			syslog(LOG_ERR, "resolving ccache %s: %s",
				cc_name, error_message(code));
			return(1);
		}
#endif
	} else {
		(void)sprintf(cc_name, "/tmp/krb5cc_root_%d", pw->pw_uid);
		if ((code = krb5_cc_resolve (context, cc_name, &ccache))) {
			syslog(LOG_ERR, "resolving ccache %s: %s",
				cc_name, error_message(code));
			return(1);
		}
		if (krb5ccname_env)
			free(krb5ccname_env);
		krb5ccname_env = strdup(cc_name);
	}
	code = krb5_cc_initialize(context, ccache, user_princ);
	if (code) {
		syslog(LOG_ERR, "error initializing krb5 ccache %s: %s",
			krb5ccname_env ? krb5ccname_env : "",
			error_message(code));
		return(1);
	}

	/*
	 * Set real as well as effective ID to 0 for the moment,
	 * to make the kerberos library do the right thing.
	 */
#if 1
	if (setuid(0) < 0) {
		warnx("setuid");
		return (1);
	}
#endif
	memset(&my_creds, 0, sizeof(my_creds));

	my_creds.client = user_princ;

	if ((code = krb5_build_principal(context, &server, strlen(realm),
					realm, KRB5_TGS_NAME, realm, 0))) {
		syslog(LOG_ERR, "building server/service name: %s",
			error_message(code));
		return(1);
	}
	my_creds.server = server;
	my_creds.times.starttime = 0;
	my_creds.times.endtime = now + lifetime;
	if (options & KDC_OPT_RENEWABLE) {
		/* Random renewable period... */
		my_creds.times.renew_till = now + 86400;
	} else {
		my_creds.times.renew_till = 0;
	}

	/*
	 * There is no such thing as a null password in kerberos, and if
	 * we pass a NULL or "\0" to krb5_get_in_tkt_with_password(), it
	 * will try to prompt for one.  This is the wrong answer (especially
	 * when invoked from xlock, where there isn't a tty).  Just consider
	 * an empty password as a failure...
	 */
	if (!password || (password[0] == '\0')) {
		krb5_cc_destroy(context, ccache);
		return(1);
	}

	code = krb5_get_in_tkt_with_password(context, options, NULL,
						NULL, NULL, password, ccache,
						&my_creds, 0);

	if (code) {
		if (code != KRB5KRB_AP_ERR_BAD_INTEGRITY) {
			syslog(LOG_ERR, "Kerberos V5 in_tkt error: %s",
				error_message(code));
			krb5_cc_destroy(context, ccache);
		}
		return(1);
	}

	if (chown(krb5_cc_get_name(context, ccache), pw->pw_uid, pw->pw_gid) < 0)
		syslog(LOG_ERR, "chown tkfile (%s): %m",
			krb5_cc_get_name(context, ccache));

#if KRB4_VERIFY
	/* KRB4 verify stuff...... */

	/*
	 * If we got a TGT, get a local "rcmd" ticket and check it so as to
	 * ensure that we are not talking to a bogus Kerberos server.
	 *
	 * There are 2 cases where we still allow a login:
	 *	1: the KRB4_VERIFY_SERVICE doesn't exist in the KDC
	 *	2: local host has no srvtab, as (hopefully) indicated by a
	 *	   return value of RD_AP_UNDEC from krb_rd_req().
	 */
	(void)strncpy(savehost, krb_get_phost(localhost), sizeof(savehost));
	savehost[sizeof(savehost)-1] = NULL;

	/*
	 * if the "KRB4_VERIFY_SERVICE" doesn't exist in the KDC for this host,
	 * still allow login with tickets, but log the error condition.
	 */

	kerror = krb_mk_req(&ticket, KRB4_VERIFY_SERVICE, savehost, realm, 33);
	if (kerror == KDC_PR_UNKNOWN) {
		syslog(LOG_NOTICE,
    		    "warning: TGT not verified (%s); %s.%s not registered, or srvtab is wrong?",
		    krb_err_txt[kerror], KRB4_VERIFY_SERVICE, savehost);
		if (koktologin(pw->pw_name, instance, user))
			return (1);
		notickets = 0;
		return (0);
	}

	if (kerror != KSUCCESS) {
		warnx("unable to use TGT: (%s)", krb_err_txt[kerror]);
		syslog(LOG_NOTICE, "unable to use TGT: (%s)",
		    krb_err_txt[kerror]);
		dest_tkt();
		return (1);
	}

	if (!(hp = gethostbyname(localhost))) {
		syslog(LOG_ERR, "couldn't get local host address");
		dest_tkt();
		return (1);
	}

	memmove((void *)&faddr, (void *)hp->h_addr, sizeof(faddr));

	kerror = krb_rd_req(&ticket, KRB4_VERIFY_SERVICE, savehost, faddr,
	    &authdata, "");

	if (kerror == KSUCCESS) {
		if (koktologin(pw->pw_name, instance, user))
			return (1);

		notickets = 0;
		return (0);
	}

	/* undecipherable: probably didn't have a srvtab on the local host */
	if (kerror == RD_AP_UNDEC) {
		syslog(LOG_NOTICE, "krb_rd_req: (%s)\n", krb_err_txt[kerror]);
		dest_tkt();
		return (1);
	}
	/* failed for some other reason */
	warnx("unable to verify %s ticket: (%s)", KRB4_VERIFY_SERVICE,
	    krb_err_txt[kerror]);
	syslog(LOG_NOTICE, "couldn't verify %s ticket: %s", KRB4_VERIFY_SERVICE,
	    krb_err_txt[kerror]);
	dest_tkt();
	/*return (1);*/
#endif
#if KRB5_VERIFY
	/* KRB5 verify stuff; taken from login.krb5 */

	/*
	 * If we got a TGT, get a local "host/<host>" ticket and check it
	 * so as to ensure that we are not talking to a bogus Kerberos server.
	 *
	 * There are 2 cases where we still allow a login:
	 *	1: the "host/<host>" doesn't exist in the KDC
	 *	2: local host has no keytab, as indicated by a failure
	 *	   of krb5_kt_read_service_key()
	 */

	/* get the server principal for the local host */
	/* (use defaults of "host" and canonicalized local name) */
	code = krb5_sname_to_principal(context, 0, 0,
					KRB5_NT_SRV_HST, &verify_princ);
	if (code) {
		syslog(LOG_ERR,
			"constructing local service name for verify: %s",
			error_message(code));
		return (1);
	}

	/* since krb5_sname_to_principal has done the work for us, just */
	/* extract the name directly */
	strncpy(savehost, krb5_princ_component(context, verify_princ, 1)->data,
		sizeof(savehost));

	/* Do we have host/<host> keys? */
	/* (use default keytab, kvno IGNORE_VNO to get the first match, */
	/*  and enctype is currently ignored anyhow.) */
	code = krb5_kt_read_service_key(context, NULL, verify_princ, 0,
					ENCTYPE_DES_CBC_CRC, &keyblk);
	if (keyblk)
		krb5_free_keyblock(context, keyblk);
	/* any failure means we don't have keys at all. */
	havekeys = ((code) ? 0 : 1);
		
	/*
	 * if the "host/<host>" service doesn't exist in the KDC for this
	 * host, still allow login with tickets, but log the error condition.
	 */
	code = krb5_mk_req(context, &auth_context, 0, KRB5_VERIFY_SERVICE,
				savehost, 0, ccache, &packet);
	/* wipe the auth context for mk_req */
	if (auth_context) {
		krb5_auth_con_free(context, auth_context);
		auth_context = NULL;
	}
	if (code == KRB5KDC_ERR_S_PRINCIPAL_UNKNOWN) {
		if (havekeys) {
			/* We have keys, so the principal should be known. */
			/* It might be a bogus error from an attacker.  Try */
			/* to foil him... */
			syslog(LOG_ERR,
			    "KDC said principal (%s) unknown when we had keys",
			    KRB5_VERIFY_SERVICE, savehost);
			return (1);
		} else {
			/* but if it is unknown and we've got no key, we */
			/* don't have any security anyhow, so it is ok. */
			syslog(LOG_NOTICE,
				"warning: TGT not verified (%s); %s/%s not registered, or keytab is wrong?",
				error_message(code),
				KRB5_VERIFY_SERVICE, savehost);
			if (k5oktologin(pw->pw_name, instance, user))
				return (1);
			notickets = 0;
			return (0);
		}
	}

	if (code) {
		warnx("unable to use TGT: (%s)", error_message(code));
		syslog(LOG_NOTICE, "unable to use TGT: (%s)",
			error_message(code));
		krb5_cc_destroy(context, ccache);
		return (1);
	}

	if (!(hp = gethostbyname(localhost))) {
		syslog(LOG_ERR, "couldn't get local host address");
		krb5_cc_destroy(context, ccache);
		return (1);
	}

	memmove((void *)&faddr, (void *)hp->h_addr, sizeof(faddr));

	/* got ticket, try to use it */
	code = krb5_rd_req(context, &auth_context, &packet, verify_princ,
				NULL, NULL, &ticket);

	if (code) {
		if (havekeys) {
			syslog(LOG_NOTICE, "krb5_rd_req: (%s)",
				error_message(code));
			krb5_cc_destroy(context, ccache);
			return (1);
		} else {
			/* I think this handles the expected krb5 errors */
			switch (code) {
			    case ENOENT:
				/* no keytab */
#if 0
	/* Don't depend on the nameserver for security.  Assume that if */
	/* we have a keytab, it must contain the right host/FQDN key... */
			    case KRB5_KT_NOTFOUND:
				/* keytab found, missing entry for local host */
#endif
				if (k5oktologin(pw->pw_name, instance, user))
					return (1);
				notickets = 0;
				return (0);
				/* NOTREACHED */
			    default:
				/* failed for some other reason */
				warnx("unable to verify %s ticket: (%s)",
					KRB5_VERIFY_SERVICE,
					error_message(code));
				syslog(LOG_NOTICE,
					"couldn't verify %s ticket: %s",
					KRB5_VERIFY_SERVICE,
					error_message(code));
				krb5_cc_destroy(context, ccache);
				return (1);
			}
		}
	}

	/*
	 * The "host/<host>" ticket has been received _and_ verified.
	 */

	/* do cleanup and return */
	if (auth_context)
		krb5_auth_con_free(context, auth_context);
	krb5_free_principal(context, verify_princ);
	/* possibly ticket and packet need freeing here as well */
	/* memset (&ticket, 0, sizeof (ticket)); */
	return(0);
#endif
	return (0);
}

/* [Potentially] check the user's k5login file to see if the specified */
/* user/instance is premitted to log in... */
int
k5oktologin(name, instance, user)
	char *name, *instance, *user;
{
	krb5_principal	k5princ;
	char		*lrealm;

	if (realm[0] == '\0') {
		if (krb5_get_default_realm(context, &lrealm) != 0)
			return(1);
		else
			strncpy(realm, lrealm, sizeof(realm));
	}

	if (always_use_klogin == 0 && *instance == '\0')
		return (0);

#if 0
	kdata = &kdata_st;
	memset((char *)kdata, 0, sizeof(*kdata));
	(void)strcpy(kdata->pname, name);
	(void)strcpy(kdata->pinst, instance);
	(void)strcpy(kdata->prealm, realm);
#else
	if (strlen(instance)) {
		krb5_build_principal(context, &k5princ, strlen(realm), realm,
					name, instance, 0);
	} else {
		krb5_parse_name(context, name, &k5princ);
	}
#endif
	if (!krb5_kuserok(context, k5princ, user)) {
		warnx("not in %s's ACL", user);
		krb5_cc_destroy(context, ccache);
		return (1);
	}
	return (0);
}
#endif
