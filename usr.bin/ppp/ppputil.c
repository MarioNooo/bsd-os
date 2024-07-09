/*	BSDI ppputil.c,v 2.5 2001/04/13 15:29:44 dab Exp */
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

int
main(int ac, char **av)
{
	struct hostent *h;
	unsigned long a1, a2, nm;

	if (ac != 2 && ac != 4) {
		fprintf(stderr, "Usage:	ppputil hostname\n\tor\n"
				"	ppputil addr1 addr2 netmask\n");
		exit(1);
	}

	if (ac == 4) {
		a1 = ntohl(inet_addr(av[1]));
		a2 = ntohl(inet_addr(av[2]));
		nm = ntohl(inet_addr(av[3]));
		exit((a1 & nm) == (a2 & nm) ? 0 : 1);
	}

	h = gethostbyname(av[1]);
	if (h && h->h_addrtype == AF_INET) {
		printf("%s\n", inet_ntoa(*(struct in_addr *)h->h_addr_list[0]));
		exit (0);
	}
	exit(1);
}
