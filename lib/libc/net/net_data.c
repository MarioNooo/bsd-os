/* BSDI net_data.c,v 2.1 1995/12/01 23:44:17 bostic Exp */

char *h_errlist[25] = {
	"Resolver Error 0 (no error)",
	"Unknown host",				/* 1 HOST_NOT_FOUND */
	"Host name lookup failure",		/* 2 TRY_AGAIN */
	"Unknown server error",			/* 3 NO_RECOVERY */
	"No address associated with name",	/* 4 NO_ADDRESS */
#define	TABLE_SIZE	5
};
int h_nerr = TABLE_SIZE;


int __check_rhosts_file = 1;
