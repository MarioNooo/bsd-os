/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI express.cc,v 2.12 2003/02/13 23:00:17 chrisk Exp	*/

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

#ifndef	__powerpc__

extern int dosspace;
extern int noquestions;

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
	    if (type == NULL)
		;
    	    else if (!strcasecmp(type, "scsi"))
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

    if (disk.d_type == DTYPE_FLOPPY || disk.d_type == DTYPE_PSEUDO) {
	print(express_hard_disk_only);
	exit(1);
    }

    if (!noquestions && request_yesno(express_erase_disk, -1) == 0)
	exit(1);

#ifdef	NEED_FDISK
    //
    // Phase 1 -- FDISK Partitioning
    //
    // Check to make sure we have a geometry.
    //

    disk.dosg = disk.bios;
    if (!disk.dosg.Valid())
	disk.dosg = disk.cmos;
    if (!disk.dosg.Valid()) {
	print(express_no_geometry);
	exit(1);
    }
    disk.use_fdisk = 1;

    for (i = 0; i < 4; ++i)
	disk.part[i].Zero();

    if (disk.d_type == DTYPE_ST506 || disk.d_type == DTYPE_ESDI) {
	/*
	 * Extract the number of sectors from the kernel.
	 * The wd driver gets this right from the parameter block
	 * on the drive, so if we get something, believe it!
	 */
	DiskLabel lab;
	lab.Generic(disk.dfd);
	if (lab.d_secperunit > disk.bsdg.Sectors()) {
	    disk.bsdg.Sectors(lab.d_secperunit);
	    disk.cmos.Sectors(lab.d_secperunit);
	    disk.bios.Sectors(lab.d_secperunit);
	    disk.Sectors(lab.d_secperunit);
	    disk.bsdg.Heads(lab.d_ntracks);
	    disk.bsdg.SecPerTrack(lab.d_nsectors);
	    disk.bsdg.SecPerCyl(lab.d_nsectors * lab.d_ntracks);
	} else {
	    disk.cmos.Sectors(disk.bsdg.Sectors());
	    disk.bios.Sectors(disk.bsdg.Sectors());
	    disk.bsdg = disk.cmos;
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
	}
    } else {
	disk.cmos.Sectors(disk.bsdg.Sectors());
	disk.bios.Sectors(disk.bsdg.Sectors());
	disk.bsdg = disk.cmos;

	if (disk.d_type == DTYPE_SCSI || 
	  disk.d_type == DTYPE_CPQRAID ||
	  disk.d_type == DTYPE_AMIRAID ||
	  disk.d_type == DTYPE_AACRAID) {
	    disk.bsdg.Heads(1);
	    disk.bsdg.SecPerCyl(2048);
	    disk.bsdg.SecPerTrack(2048);
	    disk.bsdg.Cyls(disk.bsdg.Sectors()/disk.bsdg.SecPerCyl());
	    disk.label_new.d_sparespertrack = 0;
	    disk.label_new.d_sparespercyl = 0;
	}
    }

    disk.SwitchToBSD();
#endif

    switch (disk.d_type) {
    case DTYPE_ST506:
    case DTYPE_ESDI:
	disk.badblock = disk.SecPerTrack() + 126;
	break;
    }

#ifdef	NEED_FDISK
    disk.SwitchToCMOS();

    int a = (noquestions || dosspace) ? dosspace
	    : request_number(express_dos_space, 40, 0, disk.MBs());
    int p = 0;

    if (a) {
	//
	// Make the size of the partition a multiple of the cylinder
	// size (should this be a multiple of the BSD/OS cylinder size?)
	// less one track (DOS starts on the second track of the disk)
	// and then put BSD/OS after that, giving it all the remaining
	// space.
	//
	a = disk.FromMB(a);
	a /= disk.SecPerCyl();
	a *= disk.SecPerCyl();
	a -= disk.SecPerTrack();
	
	disk.part[p].number = p + 1;
	disk.part[p].type = MBS_DOS16;
	disk.part[p].AdjustType();
	disk.part[p].length = a;
	disk.part[p].offset = disk.SecPerTrack();

	a += disk.SecPerTrack();

	++p;
    }

    disk.part[p].number = p + 1;
    disk.part[p].type = MBS_BSDI;
    disk.part[p].length = disk.Sectors() - a;
    disk.part[p].offset = a;
    disk.active = p + 1;

    disk.SwitchToBSD();
#endif

#ifdef	sparc
    if ((!disk.label_original.Valid() || disk.label_original.Soft()) &&
	  disk.label_sun.Valid())
	disk.label_original = disk.label_sun;
#endif
    for (i = 0; i < 8; ++i)
	disk.bsd[i].Zero();
    //
    // Make the c partition be the whole disk.
    //
    disk.bsd[0].number = 3;
    disk.bsd[0].length = disk.Sectors();
    disk.bsd[0].type = FS_UNUSED;
    disk.bsd[0].fixed = 1;

#ifdef	NEED_FDISK
    int bp = 4;
    int j;
    for (j = 0; j < 4 && disk.part[j].number; ++j) {
	if (disk.part[j].IsDosType()) {
	    for (i = 0; i < 8 && disk.bsd[i].number; ++i)
		;
	    disk.bsd[i].number = bp++;
	    disk.bsd[i].length = disk.part[j].length;
	    disk.bsd[i].offset = disk.part[j].offset;
	    disk.bsd[i].type = FS_MSDOS;
	    disk.Sort();
	}
    }
#endif

    disk.Sort();
    disk.AddPartition(1);		// Add a root
    disk.Sort();
    disk.AddPartition(2);		// Add a swap
    disk.Sort();
    disk.AddPartition(8);		// Add a /usr

#ifdef	NEED_FDISK
    disk.ComputeBSDBoot();
    if (disk.bsdbootpart < 0) {
fprintf(stderr, "This cannot happen.  No root part...\n");
	exit(1);
    }

    if (disk.use_fdisk) {
	O::Set(O::RequireBootany);
	master = "bootany.sys";
    }
    if (!primary)
	primary = "biosboot";
    if (!secondary)
	secondary = "bootbios";

#endif

    int labelpart = 0;

#ifdef	NEED_FDISK
    if (disk.use_fdisk) {
	if (disk.FindPartition(disk.active).IsBSDType()) {
	    labelpart = disk.active;
	} else {
	    labelpart = 0;
	    for (i = 1; i < 5; ++i) {
		Partition &p = disk.FindPartition(i);
		if (p.IsBSDType()) {
		    if (p.bootany) {
			labelpart = i;
			break;
		    }
		    if (!labelpart)
			labelpart = i;
		}
	    }
	}
    }
#endif

    //
    // The label will be written LABELOFFSET bytes into
    // the LABELSECTOR.  Since disk.bsdoff was initially set
    // to where we read our label from, we must
    // set it to where we plan to write the label
    // (checking to see if it changed, of course)
    //
#define	LO	LABELSECTOR * SECSIZE + LABELOFFSET

#ifdef	NEED_FDISK
    if (labelpart >= 0)
	disk.bsdoff = off_t(disk.FindPartition(labelpart).offset)
			    * SECSIZE + LO;
    else
#endif
	disk.bsdoff = LO;

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
#endif
