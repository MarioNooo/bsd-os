 /*
  * This module determines the type of socket (datagram, stream), the client
  * socket address and port, the server socket address and port. In addition,
  * it provides methods to map a transport address to a printable host name
  * or address. Socket address information results are in static memory.
  * 
  * The result from the hostname lookup method is STRING_PARANOID when a host
  * pretends to have someone elses name, or when a host name is available but
  * could not be verified.
  * 
  * When lookup or conversion fails the result is set to STRING_UNKNOWN.
  * 
  * Diagnostics are reported through syslog(3).
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#ifndef lint
static char sccsid[] = "@(#) socket.c 1.14 95/01/30 19:51:50";
#endif

/* System libraries. */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>

extern char *inet_ntoa();

/* Local stuff. */

#include "tcpd.h"

/* Forward declarations. */

static void sock_sink();

#ifdef APPEND_DOT

 /*
  * Speed up DNS lookups by terminating the host name with a dot. Should be
  * done with care. The speedup can give problems with lookups from sources
  * that lack DNS-style trailing dot magic, such as local files or NIS maps.
  */

static struct hostent *gethostbyname_dot(name)
char   *name;
{
    char    dot_name[MAXHOSTNAMELEN + 1];

    /*
     * Don't append dots to unqualified names. Such names are likely to come
     * from local hosts files or from NIS.
     */

    if (strchr(name, '.') == 0 || strlen(name) >= MAXHOSTNAMELEN - 1) {
	return (gethostbyname(name));
    } else {
	sprintf(dot_name, "%s.", name);
	return (gethostbyname(dot_name));
    }
}

#define gethostbyname gethostbyname_dot
#endif

void	convert_if_v4mapped(sa)
struct sockaddr *sa;
{
    if (sa->sa_family != PF_INET6)
	return;
    if (!IN6_IS_ADDR_V4MAPPED(&SIN6(sa)->sin6_addr))
	return;
    sa->sa_family = PF_INET;
    sa->sa_len = sizeof(struct sockaddr_in);
    memcpy(&SIN(sa)->sin_addr, &SIN6(sa)->sin6_addr.s6_addr[12],
		sizeof(SIN(sa)->sin_addr));
    memset(&SIN(sa)->sin_zero, 0, sizeof(SIN(sa)->sin_zero));
}

/* sock_host - look up endpoint addresses and install conversion methods */

void    sock_host(request)
struct request_info *request;
{
    static struct sockaddr_in6 client;
    static struct sockaddr_in6 server;
    int     len;
    char    buf[BUFSIZ];
    int     fd = request->fd;

    sock_methods(request);

    /*
     * Look up the client host address. Hal R. Brand <BRAND@addvax.llnl.gov>
     * suggested how to get the client host info in case of UDP connections:
     * peek at the first message without actually looking at its contents. We
     * really should verify that client.sin_family gets the value AF_INET,
     * but this program has already caused too much grief on systems with
     * broken library code.
     */

    len = sizeof(client);
    if (getpeername(fd, (struct sockaddr *) & client, &len) < 0) {
	request->sink = sock_sink;
	len = sizeof(client);
	if (recvfrom(fd, buf, sizeof(buf), MSG_PEEK,
		     (struct sockaddr *) & client, &len) < 0) {
	    tcpd_warn("can't get client address: %m");
	    return;				/* give up */
	}
#ifdef really_paranoid
	memset(buf, 0 sizeof(buf));
#endif
    }
    convert_if_v4mapped(SA(&client));
    request->client->sa = SA(&client);

    /*
     * Determine the server binding. This is used for client username
     * lookups, and for access control rules that trigger on the server
     * address or name.
     */

    len = sizeof(server);
    if (getsockname(fd, (struct sockaddr *) & server, &len) < 0) {
	tcpd_warn("getsockname: %m");
	return;
    }
#ifdef	IP_RECVDSTADDR
    else if (SA(&server)->sa_family == AF_INET && SIN(&server)->sin_addr.s_addr == 0) {
	struct msghdr msg;
	struct cmsghdr cmsgd[4];
	int on = 1;
	msg.msg_name = (caddr_t)&server;
	msg.msg_namelen = SA(&server)->sa_len;
	msg.msg_iov = 0;
	msg.msg_iovlen = 0;
	msg.msg_control = (caddr_t)cmsgd;
	msg.msg_controllen = sizeof(cmsgd);
	msg.msg_flags = 0;
	if (setsockopt(fd, IPPROTO_IP, IP_RECVDSTADDR, &on, sizeof (on)) < 0) {
	    tcpd_warn("setsockopt(IP_RECVDSTADDR): %m");
	    return;
	}
	if (recvmsg(fd, &msg, MSG_PEEK) < 0)
	    tcpd_warn("can't get server address: %m");
	else if (msg.msg_controllen >= sizeof (struct cmsghdr)
	    && !(msg.msg_flags & MSG_CTRUNC)) {
	    struct cmsghdr *cmsg;

	    for (cmsg = CMSG_FIRSTHDR(&msg);
		 cmsg && cmsg->cmsg_len >= sizeof (struct cmsghdr);
		 cmsg = CMSG_NXTHDR(&msg, cmsg)) {

		if (cmsg->cmsg_level == IPPROTO_IP
		    && cmsg->cmsg_type == IP_RECVDSTADDR
		    && cmsg->cmsg_len >= sizeof(struct in_addr) + sizeof(struct cmsghdr)) {
		    SIN(&server)->sin_addr = *(struct in_addr *) CMSG_DATA(cmsg);
		}
	    }
	}
    }
#endif

    convert_if_v4mapped(SA(&server));
    request->server->sa = SA(&server);
}

/* sock_hostaddr - map endpoint address to printable form */

void    sock_hostaddr(host)
struct host_info *host;
{
    struct sockaddr *sa = host->sa;

    if (sa != 0)
	getnameinfo(sa, sa->sa_len, host->addr, sizeof(host->addr),
	    NULL, 0, NI_NUMERICHOST);
}

/* sock_hostname - map endpoint address to host name */

void    sock_hostname(host)
struct host_info *host;
{
    struct sockaddr *sa = host->sa;
    int     i;
    struct addrinfo *ai, *ai2, hints;
    char thost[NI_MAXHOST];

    /*
     * On some systems, for example Solaris 2.3, gethostbyaddr(0.0.0.0) does
     * not fail. Instead it returns "INADDR_ANY". Unfortunately, this does
     * not work the other way around: gethostbyname("INADDR_ANY") fails. We
     * have to special-case 0.0.0.0, in order to avoid false alerts from the
     * host name/address checking code below.
     */
    if (sa == 0)
	return;
    if (sa->sa_family == AF_INET) {
	if (SIN(sa)->sin_addr.s_addr == 0)
	    return;
    } else if (sa->sa_family == AF_INET6) {
	if (IN6_IS_ADDR_UNSPECIFIED(&SIN6(sa)->sin6_addr))
	    return;
    }
    if (getnameinfo(sa, sa->sa_len, host->name, sizeof(host->name),
		    NULL, 0, NI_NAMEREQD))
	return;

    /*
     * Verify that the address is a member of the address list returned
     * by getaddrino(hostname, ...).
     * 
     * Verify also that getnameinfo() and getaddrinfo() return the same
     * hostname, or rshd and rlogind may still end up being spoofed.
     * 
     * On some sites, getaddrinfo("localhost", ...) returns "localhost.domain".
     * This is a DNS artifact. We treat it as a special case. When we
     * can't believe the address list from getaddrinfo("localhost", ...)
     * we're in big trouble anyway.
     */

    bzero(&hints, sizeof(hints));
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = sa->sa_family;
    if ((i = getaddrinfo(host->name, NULL, &hints, &ai)) != 0) {
	/*
	 * Unable to verify that the host name matches the address. This
	 * may be a transient problem or a botched name server setup.
	 */

	tcpd_warn("can't verify hostname: getaddrinfo(%s) failed: %s",
		      host->name, gai_strerror(i));
	ai = NULL;
    } else if (STR_NE(host->name, ai->ai_canonname)
		&& STR_NE(host->name, "localhost")) {

	/*
	 * The getnameinfo() and getaddrinfo() calls did not return
	 * the same hostname. This could be a nameserver configuration
	 * problem. It could also be that someone is trying to spoof us.
	 */

	tcpd_warn("host name/name mismatch: %s != %s",
		      host->name, ai->ai_canonname);

    } else {

	/*
	 * The address should be a member of the address list returned by
	 * getaddrinfo().
	 */
	for (ai2 = ai; ai2; ai2 = ai2->ai_next) {

	    if (ai2->ai_addr->sa_family != sa->sa_family)
		continue;
	    /* don't let the port number cause the memcmp() to fail */
	    SIN(ai2->ai_addr)->sin_port = SIN(sa)->sin_port;
	    if (memcmp(ai2->ai_addr, (char *)sa, sa->sa_len) == 0)
		goto done;		/* name is good, keep it */
	}

	/*
	 * The host name does not map to the initial address. Perhaps
	 * someone has messed up. Perhaps someone compromised a name
	 * server.
	 */

	thost[0] = '\0';
	getnameinfo(sa, sa->sa_len, thost, sizeof(thost),
		    NULL, 0, NI_NUMERICHOST);
	tcpd_warn("host name/address mismatch: %s != %s",
		  thost, ai->ai_canonname);
    }
    strcpy(host->name, paranoid);		/* name is bad, clobber it */
done:
    if (ai)
	freeaddrinfo(ai);
}

/* sock_sink - absorb unreceived IP datagram */

static void sock_sink(fd)
int     fd;
{
    char    buf[BUFSIZ];
    struct sockaddr_in6 sin;
    int     size = sizeof(sin);

    /*
     * Eat up the not-yet received datagram. Some systems insist on a
     * non-zero source address argument in the recvfrom() call below.
     */

    (void) recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *) & sin, &size);
}
