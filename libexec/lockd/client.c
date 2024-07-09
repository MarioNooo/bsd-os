/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI client.c,v 1.3 2001/11/01 21:05:37 chrisk Exp
 */
/*
 * Copyright (c) 1995
 *	A.R. Gordon (andrew.gordon@net-tel.co.uk).  All rights reserved.
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
 *	This product includes software developed for the FreeBSD project
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ANDREW GORDON AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/socket.h>

#include <rpc/rpc.h>

#include <netdb.h>
#include <string.h>
#include <syslog.h>

/*
 * get_client --
 *
 * Get a CLIENT * for making RPC calls to lockd on given host, returning a
 * CLIENT * pointer, from clntudp_create, or NULL if error.
 *
 * Creating a CLIENT * is quite expensive, involving a conversation with the
 * remote portmapper to get the port number.  Since a given client is quite
 * likely to make several locking requests in succession, it is desirable to
 * cache the created CLIENT *.
 *
 * Since we are using UDP rather than TCP, there is no cost to the remote
 * system in keeping these cached indefinitely.  Unfortunately there is a
 * snag: if the remote system reboots, the cached portmapper results will
 * be invalid, and we will never detect this since all of the xxx_msg()
 * calls return no result -- we just fire off a udp packet and hope for the
 * best.
 *
 * We solve this by discarding cached values after two minutes, regardless of
 * whether they have been used in the meanwhile (since a bad one might have
 * been used plenty of times, as the host keeps retrying the request and we
 * keep sending the reply back to the wrong port).
 *
 * Given that the entries will always expire in the order that they were
 * created, there is no point in a LRU algorithm for when the cache gets
 * full -- entries are always re-used in sequence.
 */
#define	CLIENT_CACHE_SIZE	 64	/* No. of client sockets cached. */
#define	CLIENT_CACHE_LIFETIME	 20	/* Lifetime in seconds. */

struct clnt {
	CLIENT *clnt_ptr;		/* RPC cache. */
	time_t clnt_timeout;		/* Timeout. */
	struct in_addr clnt_addr;	/* Address. */
	int	prog;			/* Program. */
	int	version;		/* Version. */
} clnt_cache[CLIENT_CACHE_SIZE];

static int clnt_cache_forced = 0;	/* Forced expirations counter. */
static int clnt_cache_next_to_use = 0;	/* Next slot to use. */

CLIENT *
get_client(char *host, struct sockaddr_in *host_addr, int prog, int version)
{
	extern int d_cache;
	extern char *from_addr(struct sockaddr_in *);

	struct clnt *clntp;
	struct hostent *hp;
	struct sockaddr_in _host_addr, tmp;
	struct timeval retry_time;
	CLIENT *client;
	time_t now;
	int i, sock_no;

	/* If all we have is a hostname, get a sockaddr_in. */
	if (host != NULL) {
		if ((hp = gethostbyname(host)) == NULL) {
			syslog(LOG_ERR, "unknown host %s", host);
			return (NULL);
		}
		memset(&_host_addr, 0, sizeof(_host_addr));
		_host_addr.sin_family = hp->h_addrtype;
		_host_addr.sin_port = 0;
		memmove(&_host_addr.sin_addr, hp->h_addr_list[0], hp->h_length);
		host_addr = &_host_addr;
	}

	(void)time(&now);

	/* Search for the given client in the cache. */
	for (i = 0, clntp = clnt_cache; i < CLIENT_CACHE_SIZE; i++, clntp++) {
		if (clntp->clnt_ptr == NULL ||
		    memcmp(&clntp->clnt_addr, &host_addr->sin_addr,
		    sizeof(struct in_addr)) ||
		    clntp->prog != prog || clntp->version != version)
			continue;
		if (clntp->clnt_timeout < now)
			break;
		if (d_cache)
			syslog(LOG_DEBUG,
			    "CLIENT found: %s", from_addr(host_addr));
		return (clntp->clnt_ptr);
	}

	/* Not found in cache, re-use the next entry. */
	if (i == CLIENT_CACHE_SIZE) {
		if (clnt_cache_next_to_use >= CLIENT_CACHE_SIZE)
			clnt_cache_next_to_use = 0;
		clntp = clnt_cache + clnt_cache_next_to_use++;
	}

	/* Discard any cached information in the selected entry. */
	if (clntp->clnt_ptr != NULL) {
		++clnt_cache_forced;

		if (d_cache) {
			tmp = *host_addr;
			tmp.sin_addr = clntp->clnt_addr;
			syslog(LOG_DEBUG,
			    "CLIENT expired: %s", from_addr(&tmp));
		}
		auth_destroy(clntp->clnt_ptr->cl_auth);
		clnt_destroy(clntp->clnt_ptr);
		clntp->clnt_ptr = NULL;
	}

	/*
	 * Create the new client handle, forcing consultation with the
	 * portmapper.
	 */
	sock_no = RPC_ANYSOCK;
	retry_time.tv_sec = 5;
	retry_time.tv_usec = 0;
	host_addr->sin_port = 0;
	if ((client = clntudp_create(host_addr,
	    prog, version, retry_time, &sock_no)) == NULL) {
		syslog(LOG_ERR, clnt_spcreateerror("clntudp_create"));
		syslog(LOG_ERR,
		    "unable to return result to %s", from_addr(host_addr));
		return (NULL);
	}

	/* Update the cache entry. */
	clntp->clnt_ptr = client;
	clntp->clnt_addr = host_addr->sin_addr;
	clntp->clnt_timeout = now + CLIENT_CACHE_LIFETIME;
	clntp->prog = prog;
	clntp->version = version;

	/*
	 * Disable the default timeout, so we can specify our own in calls to
	 * clnt_call().  (Note that the timeout is a different concept from
	 * the retry period set in clntudp_create() above.)
	 */
	retry_time.tv_sec = retry_time.tv_usec = -1;
	clnt_control(client, CLSET_TIMEOUT, (char *)&retry_time);

	if (d_cache)
		syslog(LOG_DEBUG, "CLIENT created: %s", from_addr(host_addr));

	return (client);
}
