 /*
  * Misc routines that are used by tcpd and by tcpdchk.
  * 
  * Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
  */

#ifndef lint
static char sccsic[] = "@(#) misc.c 1.2 96/02/11 17:01:29";
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#include "tcpd.h"

extern char *fgets();

#ifndef	INADDR_NONE
#define	INADDR_NONE	(-1)		/* XXX should be 0xffffffff */
#endif

/* xgets - fgets() with backslash-newline stripping */

char   *xgets(ptr, len, fp)
char   *ptr;
int     len;
FILE   *fp;
{
    int     got;
    char   *start = ptr;

    while (fgets(ptr, len, fp)) {
	got = strlen(ptr);
	if (got >= 1 && ptr[got - 1] == '\n') {
	    tcpd_context.line++;
	    if (got >= 2 && ptr[got - 2] == '\\') {
		got -= 2;
	    } else {
		return (start);
	    }
	}
	ptr += got;
	len -= got;
	ptr[0] = 0;
    }
    return (ptr > start ? start : 0);
}

/*
 * split_at - break string at delimiter or return NULL
 *	This function also allows for escapes and quotes.  If
 *	it finds "\x", it will replace it with "x".  It will not
 *	find delimiters between double-quotes.  The quotes and
 *	escapes will be stripped out of the section of the string
 *	up to the delimiter (or the end of the string if there is
 *	no delimiter).
 */

char   *split_at(string, delimiter)
char   *string;
int     delimiter;
{
    char   c, *sp, *dp;
    int    inquote = 0;

    sp = dp = string;

    while (c = *sp++) {
	switch (c) {
	case '\\':
	    c = *dp++ = *sp++;
	    if (c == '\0')
		return (NULL);
	    break;
	case '"':
	    inquote ^= 1;
	    break;
	default:
	    if (!inquote && c == delimiter) {
		*dp = '\0';
		return(sp);
	    }
	    *dp++ = c;
	}
    }
    return (NULL);
}

/* dot_quad_addr - convert dotted quad to internal form */

unsigned long dot_quad_addr(str)
char   *str;
{
    int     in_run = 0;
    int     runs = 0;
    char   *cp = str;

    /* Count the number of runs of non-dot characters. */

    while (*cp) {
	if (*cp == '.') {
	    in_run = 0;
	} else if (in_run == 0) {
	    in_run = 1;
	    runs++;
	}
	cp++;
    }
    return (runs == 4 ? inet_addr(str) : INADDR_NONE);
}

ipv6_addr(str, addr)
char	*str;
struct in6_addr *addr;
{
	struct in6_addr taddr;

	return(inet_pton(PF_INET6, str, addr ? addr : &taddr) == 1);
}

int get_masklen(mask_tok, max)
char *mask_tok;
int max;
{
    int masklen;
    char *endp;

    masklen = strtol(mask_tok, &endp, 10);
    if (endp == mask_tok || *endp != '\0' || masklen < 0 || masklen > max)
	return (-1);
    return (masklen);
}
