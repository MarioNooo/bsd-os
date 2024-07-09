#include <stdio.h>

main () {
char hn[10];

	gethostname(hn,10);
	printf("got hostid=%ld, hostname=%s\n",gethostid(), hn);

}
