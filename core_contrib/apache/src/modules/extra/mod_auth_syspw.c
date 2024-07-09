/*	BSDI	mod_auth_syspw.c,v 1.2 2003/01/03 16:15:32 jch Exp
 *
 * This module was written at BSDI by Tony Sanders <sanders@bsdi.com>.
 * It is based on mod_auth_db by the Apache Group and is available
 * under the same terms as Apache itself.
 */

/* ====================================================================
 * Copyright (c) 1996 The Apache Group.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission.
 *
 * 5. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * IT'S CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group and was originally based
 * on public domain software written at the National Center for
 * Supercomputing Applications, University of Illinois, Urbana-Champaign.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */


/*
 * mod_auth_syspw: Authentication based on system password database.
 *                 This is intended for use in private webs.
 * 
 * Original work by Tony Sanders <sanders@earth.com>
 * 
 * mod_auth_syspw was based on mod_auth_db.
 * 
 */

#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_protocol.h"
#include <sys/types.h>
#include <sys/param.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

#include <login_cap.h>

typedef struct  {
    char *auth_syspw_enabled;
    char *auth_syspw_passthrough;
} syspw_auth_config_rec;

static int auth_setopt_failed;

static void init_syspw (server_rec *s, pool *p)
{
     auth_setopt_failed = auth_setopt("noshared", "yes");
}

void *create_syspw_auth_dir_config (pool *p, char *d)
{
    syspw_auth_config_rec *sec
    = (syspw_auth_config_rec *) ap_pcalloc (p, sizeof(syspw_auth_config_rec));
    return sec;
}

command_rec syspw_auth_cmds[] = {
{ "AuthSYSPWEnable", ap_set_string_slot,
    (void*)XtOffsetOf(syspw_auth_config_rec, auth_syspw_enabled),
    OR_AUTHCFG, TAKE1, NULL },
{ "AuthSYSPWPassThrough", ap_set_string_slot,
    (void*)XtOffsetOf(syspw_auth_config_rec, auth_syspw_passthrough),
    OR_AUTHCFG, TAKE1, NULL },
{ NULL }
};

module syspw_auth_module;

int syspw_authenticate_basic_user (request_rec *r)
{
    syspw_auth_config_rec *sec =
      (syspw_auth_config_rec *)ap_get_module_config (r->per_dir_config,
						&syspw_auth_module);
    conn_rec *c = r->connection;
    const char *sent_pw;
    struct passwd *p;
    int res;
    login_cap_t *class;
    char *passwd, *style, *s;
    
    if ((res = ap_get_basic_auth_pw (r, &sent_pw)))
        return res;
    
    if (!sec->auth_syspw_enabled)
        return DECLINED;

    if ((s = strchr(sent_pw, ':')) != NULL) {
	*s = '\0';
	style = sent_pw;
	passwd = s+1;
    }
    else {
	passwd = sent_pw;
	style = NULL;
    }

    p = getpwnam(c->user);
    if (p == NULL) {
	if (sec->auth_syspw_passthrough)
		return DECLINED;
        ap_log_rerror(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, r,
		"SYSPW user %s does not exist", c->user);
	ap_note_basic_auth_failure (r);
	return AUTH_REQUIRED;
    }

    if ((class = login_getclass(p->pw_class)) == NULL) {
	if (sec->auth_syspw_passthrough)
		return DECLINED;
	ap_log_rerror(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, r,
        	"SYSPW user %s has no login class", c->user);
	ap_note_basic_auth_failure (r);
	return AUTH_REQUIRED;
    }

    if ((style = login_getstyle(class, style, "auth-http")) == NULL) {
	login_close(class);
	if (sec->auth_syspw_passthrough)
		return DECLINED;
	ap_log_rerror(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, r,
        	"SYSPW user %s illegal style request", c->user);
	ap_note_basic_auth_failure (r);
	return AUTH_REQUIRED;
    }

    if (auth_setopt_failed) {
	    auth_setopt_failed = auth_setopt("noshared", "yes");
    }

    if (auth_setopt_failed ||
	auth_response(c->user, class->lc_class, style, "response", 
		NULL, "", passwd) <= 0) {
    	if (sec->auth_syspw_passthrough)
		return DECLINED;
	ap_log_rerror(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, r,
    		"user %s: SYSPW authentication failed",c->user);
    	ap_note_basic_auth_failure (r);
    	return AUTH_REQUIRED;
    }

    if (auth_approve(class, c->user, "http") <= 0) {
    	if (sec->auth_syspw_passthrough)
		return DECLINED;
	ap_log_rerror(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, r,
    		"user %s: SYSPW approve failed",c->user);
    	ap_note_basic_auth_failure (r);
    	return AUTH_REQUIRED;
    }

    return OK;
}
    
/* Checking ID */
    
int syspw_check_auth(request_rec *r) {
    syspw_auth_config_rec *sec =
      (syspw_auth_config_rec *)ap_get_module_config (r->per_dir_config,
						&syspw_auth_module);
    char *user = r->connection->user;
    int m = r->method_number;		/* GET, PUT, etc */

    /* holds the information for the user in question */
    struct passwd *p;
    gid_t ugroups[NGROUPS];
    int ngroups = NGROUPS;
    
    const array_header *reqs_arr = ap_requires (r);
    require_line *reqs = reqs_arr ? (require_line *)reqs_arr->elts : NULL;

    register int x;	/* interates through the requirements */
    const char *t;	/* the list of requirements */
    char *w;		/* a "word" from that list, usage varies */

    /* decline the request if we are not enabled */
    if (!sec->auth_syspw_enabled)
	return DECLINED;

    /* no require directives means anything is OK */
    if (!reqs_arr)
	return OK;

    /* get password and group information for user */
    p = getpwnam(user);
    if (p == NULL) {
	if (sec->auth_syspw_passthrough)
		return DECLINED;
	ap_log_rerror(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, r,
        	"SYSPW user %s does not exist", user);
	ap_note_basic_auth_failure (r);
	return AUTH_REQUIRED;
    }
    /* an error here would indicate ngroups overflow, we don't care */
    (void) getgrouplist(user, p->pw_gid, ugroups, &ngroups);

    /* check user privs against the reqs array */
    for (x=0; x < reqs_arr->nelts; x++) {

	/* does this reqs apply to the method being used? */
	if (! (reqs[x].method_mask & (1 << m)))
	    continue;

	t = reqs[x].requirement;

	/* look for requirements (valid-user, user, or group) */
	w = ap_getword(r->pool, &t, ' ');

	/* if valid-user then we are OK */
	if (!strcmp(w,"valid-user"))
	    return OK;

	/* check for user name match against reqs list */
	if(!strcmp(w,"user")) {
	    while(*t) {
		w = ap_getword_conf (r->pool, &t);
		if(!strcmp(user,w))
		    return OK;
	    }
	}

	/* check to see if user is in specified group(s) */
	if (!strcmp(w,"group")) {
	    int i;
	    struct group *g;

	    while (*t) {
		/* w is the next group to check from the require list (t) */
		w = ap_getword(r->pool, &t, ' ');
		if (g = getgrnam(w))		/* ignore invalid groups */
		    for (i = 0; i < ngroups; i++)
			if (g->gr_gid == ugroups[i])
			    return OK;
	    }

	    if (sec->auth_syspw_passthrough)
		return DECLINED;
	    ap_log_rerror(APLOG_MARK, APLOG_NOERRNO|APLOG_ERR, r,
	    	"user %s not in right group",user);
	    ap_note_basic_auth_failure(r);
	    return AUTH_REQUIRED;
	}
    }
    
    return DECLINED;
}

module syspw_auth_module = {
   STANDARD_MODULE_STUFF,
   init_syspw,			/* initializer */
   create_syspw_auth_dir_config,/* dir config creater */
   NULL,			/* dir merger --- default is to override */
   NULL,			/* server config */
   NULL,			/* merge server config */
   syspw_auth_cmds,		/* command table */
   NULL,			/* handlers */
   NULL,			/* filename translation */
   syspw_authenticate_basic_user,/* check_user_id */
   syspw_check_auth,		/* check auth */
   NULL,			/* check access */
   NULL,			/* type_checker */
   NULL,			/* fixups */
   NULL				/* logger */
};
