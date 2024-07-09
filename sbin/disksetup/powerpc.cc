/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI powerpc.cc,v 2.4 2001/10/03 17:29:56 polk Exp	*/

#include <stdio.h>
#include <err.h>
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

#ifdef	__powerpc__

#define	PPC_BOOTSIZE	32
#define	PPC_BOOTSECTOR	1

void
express()
{
    int i;

    //
    // Yuch.  I really don't want any questions.
    //
    if (!disk.d_type) {
	print(main_ask_type_long);
    	while (!disk.d_type) {
	    char *type = request_string(main_ask_type);
    	    if (!strcasecmp(type, "scsi"))
		disk.d_type = DTYPE_SCSI;
    	    else if (!strcasecmp(type, "ide"))
		disk.d_type = DTYPE_ST506;
    	    else if (!strcasecmp(type, "mfm"))
		disk.d_type = DTYPE_ST506;
    	    else if (!strcasecmp(type, "rll"))
		disk.d_type = DTYPE_ST506;
    	    else if (!strcasecmp(type, "st506"))
		disk.d_type = DTYPE_ST506;
    	    else if (!strcasecmp(type, "esdi"))
		disk.d_type = DTYPE_ESDI;
    	    else if (!strcasecmp(type, "floppy"))
		disk.d_type = DTYPE_FLOPPY;
    	}
    }

    if (disk.d_type == DTYPE_FLOPPY) {
	print(express_hard_disk_only);
	exit(1);
    }

    if (request_yesno(express_erase_disk, -1) == 0)
	exit(1);

    //
    // Phase 1 -- FDISK Partitioning
    //
    // Check to make sure we have a geometry.
    //

    disk.use_fdisk = 1;

    for (i = 0; i < 4; ++i)
	disk.part[i].Zero();

    disk.cmos.Sectors(disk.bsdg.Sectors());
    disk.bios.Sectors(disk.bsdg.Sectors());
    disk.bsdg = disk.cmos;
    if (disk.d_type == DTYPE_ST506 || disk.d_type == DTYPE_ESDI) {
	if (!disk.bsdg.Valid())
    	    disk.bsdg = disk.bios;
	if (!disk.bsdg.Valid()) {
	    print(express_no_geometry);
	    exit(1);
	}
	while (disk.bsdg.Heads() > 16) {
	    disk.bsdg.Heads(disk.bsdg.Heads()/2);
	    disk.bsdg.SecPerTrack(disk.bsdg.SecPerTrack()*2);
	    disk.bsdg.Cyls(disk.bsdg.Cyls()*2);
	}
    } else if (disk.d_type == DTYPE_SCSI) {
    	disk.bsdg.Heads(1);
    	disk.bsdg.SecPerCyl(2048);
    	disk.bsdg.SecPerTrack(2048);
    	disk.bsdg.Cyls(disk.bsdg.Sectors()/disk.bsdg.SecPerCyl());
    }
    disk.dosg.Heads(16);
    disk.dosg.SecPerTrack(63);
    disk.dosg.SecPerCyl(16*63);
    disk.dosg.Cyls(disk.bsdg.Sectors()/disk.dosg.SecPerCyl());

    disk.SwitchToBSD();

    switch (disk.d_type) {
    case DTYPE_ST506:
    case DTYPE_ESDI:
	disk.badblock = disk.SecPerTrack() + 126;
	break;
    }

    disk.SwitchToCMOS();

    int p = 0;

    disk.active = disk.part[p].number = p + 1;
    disk.part[p].type = MBS_PPCBOOT;
    disk.part[p].length = PPC_BOOTSIZE;
    disk.part[p].offset = PPC_BOOTSECTOR;

    ++p;
    disk.part[p].number = p + 1;
    disk.part[p].type = MBS_BSDI;
    disk.part[p].length = disk.Sectors() - (PPC_BOOTSECTOR + PPC_BOOTSIZE);
    disk.part[p].offset = PPC_BOOTSECTOR + PPC_BOOTSIZE;

    //
    // The label will be written LABELOFFSET bytes into
    // the LABELSECTOR.  Since disk.bsdoff was initially set
    // to where we read our label from, we must
    // set it to where we plan to write the label
    // (checking to see if it changed, of course)
    //
#define	LO	LABELSECTOR * SECSIZE + LABELOFFSET

    disk.bsdoff = off_t(PPC_BOOTSECTOR + PPC_BOOTSIZE) * SECSIZE + LO;

    disk.SwitchToBSD();

    for (i = 0; i < 8; ++i)
	disk.bsd[i].Zero();
    //
    // Make the c partition be the whole disk.
    //
    disk.bsd[0].number = 3;
    disk.bsd[0].length = disk.Sectors();
    disk.bsd[0].type = FS_UNUSED;
    disk.bsd[0].fixed = 1;

    int bp = 4;
    int j;
    for (j = 0; j < 4 && disk.part[j].number; ++j) {
	if (disk.part[j].type == MBS_PPCBOOT) {
	    for (i = 0; i < 8 && disk.bsd[i].number; ++i)
		;
	    disk.bsd[i].number = bp++;
	    disk.bsd[i].length = disk.part[j].length;
	    disk.bsd[i].offset = disk.part[j].offset;
	    disk.bsd[i].type = FS_BOOT;
	    disk.Sort();
	}
    }

    disk.Sort();
    disk.AddPartition(1);		// Add a root
    disk.Sort();
    disk.AddPartition(2);		// Add a swap
    disk.Sort();
    disk.AddPartition(8);		// Add a /usr

    disk.ComputeBSDBoot();
    if (disk.bsdbootpart < 0)
	errx(1, "This cannot happen.  No root part...");

    if (disk.use_fdisk) {
	O::Set(O::RequireBootany);
	master = "bootany.sys";
    }
    if (!bootstrap)
	bootstrap = "bootdisk";

    //
    // Build a label.
    //

    //
    // First copy in the template
    //

    disk.label_new = disk.label_template;

    disk.label_new.d_type = disk.d_type;
    disk.label_new.d_secsize = disk.secsize;
    disk.label_new.d_nsectors = disk.SecPerTrack();
    disk.label_new.d_ntracks = disk.Heads();
    disk.label_new.d_ncylinders = disk.Cyls();
    disk.label_new.d_secpercyl = disk.SecPerCyl();
    disk.label_new.d_secperunit = disk.Sectors();

    disk.label_new.d_flags &= ~(D_DEFAULT|D_SOFT);
    disk.label_new.d_npartitions = 8;
    memset(disk.label_new.d_partitions, 0, sizeof(disk.label_new.d_partitions));
    for (i = 0; i < 8 && disk.bsd[i].number; ++i) {
	FileSystem &fs = disk.bsd[i];
#define	p   disk.label_new.d_partitions[disk.bsd[i].number - 1]
	p.p_size = fs.length;
	p.p_offset = fs.offset;
	p.p_fstype = fs.type;
	if (p.p_fstype == FS_BSDFFS) {
	    p.p_fsize = fs.original ? fs.fsize : 0;
	    p.p_frag = fs.original ? fs.frag : 0;
	    p.p_cpg = fs.original ? fs.cpg : 0;
	}
#undef	p
    }
    disk.label_new.Fixup();
}

void
powerpc_writeboot(int bfd)
{
	char buf[PPC_BOOTSIZE * disk.secsize];
	int r;

	memset(buf, 0, sizeof(buf));

	if (lseek(disk.dfd, PPC_BOOTSECTOR * SECSIZE, L_SET) == -1)
		warn("seeking on %s", disk.path);
	else if ((r = read(bfd, buf, sizeof(buf))) < 0)
		warn("reading %s", bootstrap);
    	else if (r > (PPC_BOOTSIZE - 1) * disk.secsize)
		warnx("%x: too big", bootstrap);
	else if (write(disk.dfd, buf, sizeof(buf)) != sizeof(buf))
		warn("writing %s", bootstrap);
}
#endif
