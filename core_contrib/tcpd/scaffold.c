 /*
  * Routines for testing only. Not really industrial strength.
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#ifndef lint
static char sccs_id[] = "@(#) scaffold.c 1.5 95/01/03 09:13:48";
#endif

/* System libraries. */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <syslog.h>
#include <setjmp.h>
#include <string.h>

#ifndef INADDR_NONE
#define	INADDR_NONE	(-1)		/* XXX should be 0xffffffff */
#endif

extern char *malloc();

/* Application-specific. */

#include "tcpd.h"
#include "scaffold.h"

 /*
  * These are referenced by the options module and by rfc931.c.
  */
int     allow_severity = SEVERITY;
int     deny_severity = LOG_WARNING;
int     rfc931_timeout = RFC931_TIMEOUT;

/*
 * find_inet_addr - find all addresses for this host,
 * result to freeaddrinfo()
 */

struct addrinfo *find_inet_addr(host)
char   *host;
{
    struct addrinfo *ai, *ai2, **aip, hints;

    /*
     * Map host name to a series of addresses. Watch out for non-internet
     * forms or aliases.
     */
    if (NOT_INADDR(host) == 0) {
	tcpd_warn("%s: not an internet address", host);
	return (0);
    }
    bzero(&hints, sizeof(hints));
    hints.ai_flags = AI_CANONNAME;
    if (getaddrinfo(host, NULL, &hints, &ai) != 0) {
	tcpd_warn("%s: host not found", host);
	return (0);
    }

    /* Strip out non-IP and non-IPv6 addresses. */
    aip = &ai;
    for (ai2 = ai; ai2; ai2 = *aip) {
	if (ai2->ai_family != AF_INET && ai2->ai_family != AF_INET6) {
	    *aip = ai2->ai_next;
	    ai2->ai_next = NULL;
	    freeaddrinfo(ai2);
	} else
	    aip = &ai2->ai_next;
    }
	
    if (ai == NULL) {
	tcpd_warn("%s: not an internet host", host);
	return (0);
    }
    if (STR_NE(host, ai->ai_canonname)) {
	tcpd_warn("%s: hostname alias", host);
	tcpd_warn("(official name: %s)", ai->ai_canonname);
    }
    return(ai);
}

/* check_dns - give each address thorough workout, return address count */

int     check_dns(host)
char   *host;
{
    struct request_info request;
    struct sockaddr_in6 sin;
    struct addrinfo *ai, *ai2;
    int     count;

    if ((ai = find_inet_addr(host)) == 0)
	return (0);
    request_init(&request, RQ_CLIENT_SIN, &sin, 0);
    sock_methods(&request);

    for (count = 0, ai2 = ai; ai2; count++, ai2 = ai2->ai_next) {
	memcpy((char *) &sin, ai2->ai_addr, ai2->ai_addrlen);

	/*
	 * Force host name and address conversions. Use the request structure
	 * as a cache. Detect hostname lookup problems. Any name/name or
	 * name/address conflicts will be reported while eval_hostname() does
	 * its job.
	 */
	request_set(&request, RQ_CLIENT_ADDR, "", RQ_CLIENT_NAME, "", 0);
	if (STR_EQ(eval_hostname(request.client), unknown))
	    tcpd_warn("host address %s->name lookup failed",
		      eval_hostaddr(request.client));
    }
    freeaddrinfo(ai);
    return (count);
}

/* dummy function to intercept the real shell_cmd() */

/* ARGSUSED */

void    shell_cmd(command)
char   *command;
{
    if (hosts_access_verbose)
	printf("command: %s", command);
}

/* dummy function  to intercept the real clean_exit() */

/* ARGSUSED */

void    clean_exit(request)
struct request_info *request;
{
    exit(0);
}

/* dummy function  to intercept the real rfc931() */

/* ARGSUSED */

void    rfc931(request)
struct request_info *request;
{
    strcpy(request->user, unknown);
}

/* check_path - examine accessibility */

int     check_path(path, st)
char   *path;
struct stat *st;
{
    struct stat stbuf;
    char    buf[BUFSIZ];

    if (stat(path, st) < 0)
	return (-1);
#ifdef notdef
    if (st->st_uid != 0)
	tcpd_warn("%s: not owned by root", path);
    if (st->st_mode & 020)
	tcpd_warn("%s: group writable", path);
#endif
    if (st->st_mode & 002)
	tcpd_warn("%s: world writable", path);
    if (path[0] == '/' && path[1] != 0) {
	strrchr(strcpy(buf, path), '/')[0] = 0;
	(void) check_path(buf[0] ? buf : "/", &stbuf);
    }
    return (0);
}
