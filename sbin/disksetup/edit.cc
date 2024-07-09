/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI edit.cc,v 2.5 1998/08/31 20:43:56 prb Exp	*/

#include <err.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include "fs.h"
#include <paths.h>
#include <sys/wait.h>
#include "showhelp.h"
#include "screen.h"
#include "help.h"
#include "disk.h"
#include "field.h"
#include "util.h"


char tmpfil[] = _PATH_TMP "disksetup.XXXXXX";
#define DEFEDITOR       "/usr/bin/vi"

int editit();

extern Errors ENOTMP;

/*
 * Edit a label using your favorite editor
 */
int
edit()
{
    FILE *fp;

    if (mktemp(tmpfil) == NULL)
	return (ENOTMP);
    if ((fp = fopen(tmpfil, "w")) == NULL) {
	unlink(tmpfil);
	err(1, "opening temporary file %s", tmpfil);
    }

    if (!disk.label_original.Valid() || disk.label_original.Soft()) {
#ifdef sparc
	if (disk.label_sun.Valid()) {
	    printf("# Missing disk label, sun label used.\n");
	    disk.label_original = disk.label_sun;
	} else
#endif
	{
	    fprintf(fp, "# Missing disk label, default label used.\n");
	    fprintf(fp, "# Geometry may be incorrect.\n");
	}
    }   

#ifdef	NEED_FDISK
    disklabel_display(disk.device, fp, &disk.label_original, disk.PartTable());
#else
    disklabel_display(disk.device, fp, &disk.label_original);
#endif
    fclose(fp);

    for (;;) {
	if (!editit()) {
	    unlink(tmpfil);
	    exit(1);
	}
	if ((fp = fopen(tmpfil, "r")) == NULL) {
	    unlink(tmpfil);
	    err(1, "opening temporary file %s", tmpfil);
	}
	disk.label_new.Clean();
#ifdef	NEED_FDISK
	if (disklabel_getasciilabel(fp, &disk.label_new, 0))
	    break;
#else
	if (disklabel_getasciilabel(fp, &disk.label_new))
	    break;
#endif
	print(edit_invalid);
	if (request_yesno(edit_redit, 1) == 0) {
	    unlink(tmpfil);
	    print(edit_invalid_exit);
	    exit(1);
	}
    }
    unlink(tmpfil);
    return (1);
}

int
editit()
{
    int pid, xpid;
    int stat, omask;

    omask = sigblock(sigmask(SIGINT) | sigmask(SIGQUIT) | sigmask(SIGHUP));
    while ((pid = fork()) < 0) {
	if (errno == EPROCLIM) {
	    fprintf(stderr, "You have too many processes\n");
	    return (0);
	}
	if (errno != EAGAIN) {
	    warn("creating edit process");
	    return (0);
	}
	sleep(1);
    }

    if (pid == 0) {
	register char *ed;

	sigsetmask(omask);
	setgid(getgid());
	setuid(getuid());
	if ((ed = getenv("EDITOR")) == (char *) 0)
		ed = DEFEDITOR;
	execlp(ed, ed, tmpfil, 0);
	err(1, "Executing editor %s", ed);
    }
    while ((xpid = wait(&stat)) >= 0)
	    if (xpid == pid)
		    break;
    sigsetmask(omask);
    return (!stat);
}
