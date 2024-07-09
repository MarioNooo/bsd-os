 /*---------------------------------------------------------------------

	(C) 1991-92 - Cyclades Corporation.

	Version Date	 Author		Comentts.

	V_1.0.0	12/07/92 Marcio Saito	First release. 
	V_1.0.1	12/15/93 Marcio Saito	Messages change.
	V_1.0.2 04/10/95 Marcio Saito	Reverse flow control added.

------------------------------------------------------------------------*/

/* includes */

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/tty.h>


/* global variables */


unsigned int tty;


/* main routine */

main(argc,argv)

int argc;
char *argv [];

{
	int cmd;
	char device[100];
	 

	device[0] = '\0';

	/* check command line */

	if ( (argc != 3) && (argc != 2) ) {
		err_syntax();
	}

	cmd = -1;
	if (strcmp("nrm",argv[1]) == 0)	cmd = 1;
	if (strcmp("ext",argv[1]) == 0) cmd = 2;
	if (strcmp("chk",argv[1]) == 0)	cmd = 3;
	if (strcmp("nfl",argv[1]) == 0)	cmd = 4;
	if (strcmp("rfl",argv[1]) == 0)	cmd = 5;
	if (cmd == -1) {
		err_syntax();
	}

	if (argc == 2) {
		strcpy(device,"/dev/tty");
	} else {
		if ( strncmp(argv[2],"/dev/",5) != 0 ) {
			strcpy(device,"/dev/");
		}
		strcat(device,argv[2]);
	}

	/* open the channel */

	if ( (tty = open (device,O_RDWR|O_NDELAY)) == -1 ) {
		printf("Can't open %s.\n",device);
		exit(1);
	}

	switch(cmd) {
/*
	    case 1:	if( ioctl(tty,CYS_NRM_BAUD) == -1 ) {
				printf("cytty : ioctl error.\n");
				close (tty);
				exit(1);
			}
			break;

	    case 2:	if( ioctl(tty,CYS_EXT_BAUD) == -1 ) {
				printf("cytty : ioctl error.\n");
				close (tty);
				exit(1);
			}
			break;

	    case 3:	if( ioctl(tty,CYS_CHK_BAUD) == 1 ) {
				printf("Extended Baud Rate is active.\n");
			} else {
				printf("Extended Baud Rate not active.\n");
			}
			break;
*/

	    case 4:	if( ioctl(tty,0x4381) == -1 ) {
				printf("cytty : ioctl error.\n");
				close (tty);
				exit(1);
			}
			break;

	    case 5:	if( ioctl(tty,0x4382) == -1 ) {
				printf("cytty : ioctl error.\n");
				close (tty);
				exit(1);
			}
			break;
	}
	close (tty);
}


err_syntax()
{
	printf("\n");
	printf("Syntax:\n\n");
	printf("    cytty <nrm|ext|nfl|rfl> [<device-name>]\n\n");
/*
	printf("    nrm - disable extended baud rate (termio compatible).\n");
	printf("    ext - enable extended baud rate mode.\n");
*/
	printf("    nfl - disable reverse HW flow control.\n");
	printf("    rfl - enable reverse HW flow control.\n");
	printf("\n");
	exit(1);
}
