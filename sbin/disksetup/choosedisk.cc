/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI choosedisk.cc,v 2.2 1995/10/11 18:50:37 prb Exp	*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include "fs.h"
#include <paths.h>
#include <sys/wait.h>
#include "showhelp.h"
#include "screen.h"
#include "help.h"
#include "disk.h"
#include "field.h"
#include "util.h"

char AvailableDisks[17][4];
char NAvailableDisks = 0;

static char *cardinal[] = {
	"First",
	"Second",
	"Third",
	"Forth",
	"Fifth",
	"Sixth",
	"Seventh",
	"Eighth",
};

char **
FindDisks()
{
    int nd; 

    char diskname[] = "/dev/rwd0c";

    for (;;) {
	for (nd = 0; nd < 8; ++nd) {
	    diskname[8] = '0' + nd;

	    int mode = (O::Flag(O::NoWrite) ? O_RDONLY : O_RDWR) |
		       (O::Flag(O::NonBlock) ? O_NONBLOCK : 0);
	    int fd = open(diskname, mode);

	    if (fd >= 0) {
		close(fd);
		AvailableDisks[NAvailableDisks][0] = diskname[6];
		AvailableDisks[NAvailableDisks][1] = 'd';
		AvailableDisks[NAvailableDisks][2] = diskname[8];
		AvailableDisks[NAvailableDisks][3] = 0;
		NAvailableDisks++;
    	    }
	}
    	if (diskname[6] == 's')
	    break;
    	diskname[6] = 's';
    }
    AvailableDisks[NAvailableDisks][0] = 0;
//  return(AvailableDisks);
    return(0);
}

char *
ChooseDisk()
{
    int nd;
    int first = 1;

    if (NAvailableDisks == 0)
	FindDisks();

    if (NAvailableDisks == 1)
	return(AvailableDisks[0]);

    if (NAvailableDisks == 0) {
	fprintf(stderr, "No disks with %s permission found.\n",
			O::Flag(O::NoWrite) ? "read" : "write");
    	if (geteuid())
	    fprintf(stderr, "You should run disksetup as root.\n");
	exit(1);
    }

    if (O::Flag(O::Express))
	print(express_multiple_disks);

    for (;;) {
	printf("Please choose from one of the following:\n\n");
	for (nd = 0; nd < NAvailableDisks; ++nd) {
	    if (first && O::Flag(O::Express)) {
		printf("    %s - %s %s disk\n", AvailableDisks[nd],
			cardinal[AvailableDisks[nd][2] - '0'],
			AvailableDisks[nd][0] == 'w' ? "IDE" : "SCSI");
	    } else {
		if (nd == 8)
		    printf("\n");
		printf("    %s", AvailableDisks[nd]);
    	    }
    	}
    	first = 0;
    	printf("\n\nWhich Disk? [%s] ", AvailableDisks[0]);

    	char buf[256];
    	if (fgets(buf, sizeof(buf), stdin) == 0)
	    exit(0);
    	char *p = buf;
    	while (*p == ' ' || *p == '\t')
	    ++p;
    	char *e = p;
    	while (*e && *e != '\n' && *e != ' ' && *e != '\t')
	    ++e;
    	*e = 0;
    	if (*p == 0)
	    return(AvailableDisks[0]);
    	for (nd = 0; nd < NAvailableDisks; ++nd) {
	    if (!strcmp(p, AvailableDisks[nd]))
	    	return(AvailableDisks[nd]);
    	}
    	printf("\n\n");
    }
}
