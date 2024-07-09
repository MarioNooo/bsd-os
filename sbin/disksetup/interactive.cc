/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI interactive.cc,v 2.21 2002/07/30 20:59:36 rfr Exp	*/

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

int
interactive()
{
    Geometry glimit;
    int i;
    int redobsd = 0;
    quad_t swaplen = 0;
    quad_t swapoff = 0;

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
    	    else if (!strcasecmp(type, "compaq"))
		disk.d_type = DTYPE_CPQRAID;
    	    else if (!strcasecmp(type, "pseudo"))
		disk.d_type = DTYPE_PSEUDO;
    	    else if (!strcasecmp(type, "ami"))
		disk.d_type = DTYPE_AMIRAID;
	    else if (!strcasecmp(type, "aac"))
		disk.d_type = DTYPE_AACRAID;
    	}
    }

#ifdef	NEED_FDISK
    //
    // Phase 1 -- FDISK Partitioning
    //
    // Check to make sure we have a geometry.  If running in expert
    // mode allow altering the geometry even if found.
    // 
    // Do not complete this phase until there is a BSD partition
    // (if we are installing a BSD partition table)
    //

#ifndef	__powerpc__
    if ((disk.d_type == DTYPE_FLOPPY || disk.d_type == DTYPE_PSEUDO)
	&& O::Flag(O::UpdateFDISK)) {
	    print(main_no_fdisk_on_floppy);
	    return(1);
    }
#endif

    disk.SwitchToCMOS();

    if (    !(disk.use_fdisk = O::Flag(O::UpdateFDISK))
	 && ! O::Flag(O::UpdateBSD)) {
#ifndef	__powerpc__
    	if (disk.d_type != DTYPE_FLOPPY && disk.d_type != DTYPE_PSEUDO) {
#endif
	    print(disk.has_fdisk ? main_multipleos_long_fdisk
				 : main_multipleos_long);
	    disk.use_fdisk = request_yesno(main_multipleos, disk.has_fdisk);
	    if (disk.use_fdisk != disk.has_fdisk)
		redobsd = 1;
#ifndef	__powerpc__
    	} else {
	    disk.use_fdisk = disk.has_fdisk = 0;
    	}
#endif
    }
    if (!disk.use_fdisk) {
	for (i = 0; i < 4; ++i)
	    disk.part[i].Zero();
    }

#ifndef	__powerpc__
    print(main_possible_geometries);
    if (disk.use_fdisk) {
	printf("%16s     CMOS     BIOS    FDISK   BSD/OS\n", "");
	printf("%16s %8d %8d %8d %8d\n",
	    "Heads", disk.cmos.Heads(), disk.bios.Heads(),
		     disk.fdisk.Heads(), disk.bsdg.Heads());
	printf("%16s %8d %8d %8d %8d\n",
	    "Sectors/Track", disk.cmos.SecPerTrack(), disk.bios.SecPerTrack(),
			     disk.fdisk.SecPerTrack(), disk.bsdg.SecPerTrack());
	printf("%16s %8d %8d %8d %8d\n",
	    "Cylinders", disk.cmos.Cyls(), disk.bios.Cyls(),
			 disk.fdisk.Cyls(), disk.bsdg.Cyls());
	printf("%16s %8qd %8qd %8qd %8qd\n",
	    "Total Sectors", disk.cmos.Sectors(), disk.bios.Sectors(),
			 disk.fdisk.Sectors(), disk.bsdg.Sectors());
	if (disk.bios.Valid()) {
	    if (disk.fdisk.Valid() && !disk.fdisk.Match(disk.bios))
		printf("The fdisk geometry appears to be invalid\n");
	    disk.dosg = disk.bios;
	    print(will_use_bios_geometry);
	} else if (disk.cmos.Valid()) {
	    if (disk.fdisk.Valid() && !disk.fdisk.Match(disk.bios))
		printf("The fdisk geometry appears to be invalid\n");
	    disk.dosg = disk.cmos;
	    print(will_use_cmos_geometry);
	}
    } else {
	printf("%16s     CMOS     BIOS   BSD/OS\n", "");
	printf("%16s %8d %8d %8d\n",
	    "Heads", disk.cmos.Heads(), disk.bios.Heads(), disk.bsdg.Heads());
	printf("%16s %8d %8d %8d\n",
	    "Sectors/Track", disk.cmos.SecPerTrack(), disk.bios.SecPerTrack(),
			     disk.bsdg.SecPerTrack());
	printf("%16s %8d %8d %8d\n",
	    "Cylinders", disk.cmos.Cyls(), disk.bios.Cyls(),
			 disk.bsdg.Cyls());
	printf("%16s %8qd %8qd %8qd\n",
	    "Total Sectors", disk.cmos.Sectors(), disk.bios.Sectors(),
			 disk.bsdg.Sectors());
    }
    printf("\n");
    request_inform(press_return_to_continue);
#endif

    if (!O::Flag(O::UpdateFDISK) || O::Flag(O::UpdateBSD)) {
	disk.use_bsd = 1;
#endif
	if (!disk.bsdg.Valid() || request_yesno(main_get_bsd_geom, 0)) {
	    int first = 1;
	    for(;;) {
		if (!first)
		    request_inform(invalid_geometry);
		first = 0;
	    	glimit.Zero();
		switch (disk.d_type) {
		case DTYPE_ST506:
		case DTYPE_ESDI:
			glimit.SecPerTrack(0xffff);
			glimit.Cyls(O::Flag(O::Expert) ? 0 : 0xffff);
			glimit.Heads(16);
			break;
		case DTYPE_SCSI:
		case DTYPE_CPQRAID:
		case DTYPE_AMIRAID:
		case DTYPE_AACRAID:
		case DTYPE_PSEUDO:
			glimit.SecPerTrack(0xffff);
			glimit.Cyls(0);
			glimit.Heads(0xff);
			break;
		}
		SetGeometry(
#ifdef	NEED_FDISK
			    disk.use_fdisk ? main_need_bsd_geom_coex_long :
#endif
					     main_need_bsd_geom_long,
			    disk.bsdg,
			    glimit);
		if (disk.bsdg.Valid()) {
		    disk.Sectors(disk.bsdg.Sectors());
		    break;
		}
	    }
	}

#ifdef	NEED_FDISK
	disk.SwitchToBSD();
#endif

	switch (disk.d_type) {
	case DTYPE_ST506:
	case DTYPE_ESDI:
	    disk.badblock = disk.SecPerTrack() + 126;
	    break;
	}
#ifdef	NEED_FDISK
    }

    if (disk.use_fdisk) {
	int yn = 0;

#ifdef	__powerpc__
	if (disk.d_type != DTYPE_FLOPPY) {
	    disk.cmos.Sectors(disk.bsdg.Sectors());
	    disk.cmos.Heads(16);
	    disk.cmos.SecPerTrack(63);
	    disk.cmos.SecPerCyl(16*63);
	    disk.cmos.Cyls(disk.bsdg.Sectors()/disk.cmos.SecPerCyl());
	} else
	    disk.cmos = disk.bsdg;
	disk.bios = disk.cmos;

        if (!disk.fdisk.Valid())
	    disk.fdisk = disk.bios;
        if (!disk.dosg.Valid())
	    disk.dosg = disk.fdisk;
#else
	if (!disk.dosg.Valid() || O::Flag(O::Expert)) {
	    print(main_get_dos_geom_long);

	    if (disk.bios.Valid() && !disk.bios.Match(disk.dosg)) {
		    print(main_fgeom_not_bios);
		    yn = 1;
	    }

	    if (!disk.dosg.Valid() && disk.d_type != DTYPE_SCSI 
				   && disk.d_type != DTYPE_CPQRAID 
				   && disk.d_type != DTYPE_AMIRAID
				   && disk.d_type != DTYPE_AACRAID
				   && disk.use_bsd) {
		disk.dosg = disk.bsdg;
		if (disk.dosg.Cyls() > 1024)
			disk.dosg.Cyls(1024);
		if (disk.dosg.SecPerTrack() > 63)
			disk.dosg.SecPerTrack(63);
		if (disk.dosg.Heads() > 255)
			disk.dosg.Heads(255);
		request_inform(will_use_bsd_geometry);
		yn = 1;
	    }

	    if (!disk.dosg.Valid() || request_yesno(main_get_dos_geom, yn)) {
		int first = 1;
		for (;;) {
		    if (!first)
			request_inform(invalid_geometry);
		    first = 0;
		    glimit.Zero();
		    glimit.SecPerTrack(63);
		    glimit.Heads(255);
		    glimit.Cyls(1024);
		    SetGeometry(main_need_dos_geom_long, disk.dosg, glimit);
		    if (disk.dosg.Valid()) {
			if (disk.dosg.Sectors() > disk.Sectors())
			    disk.Sectors(disk.dosg.Sectors());
			break;
		    }
		}
	    }
	}
#endif
    }

    disk.SwitchToCMOS();

    for (;;) {
	int ret = 1;
	swaplen = swapoff = 0;

	if (O::Flag(O::UpdateFDISK)) {
	    ret = disk.FDisk();
	} else if (disk.use_fdisk) {
	    int has_fdisk = disk.has_fdisk;

	    if (has_fdisk) {
		print(main_newfdisk_long);

		if (request_yesno(main_newfdisk, 1) == 0) {
		    redobsd = 1;
		    for (i = 0; i < 4; ++i)
			disk.part[i].Zero();
		    has_fdisk = 0;
		}
	    }

#ifndef	__powerpc__
	    if (!has_fdisk && (print(main_typical_long),
				    request_yesno(main_typical, 1))) {
		redobsd = 1;
		print(main_space_long);
    	    	int p = 0;
	    	printf("\nYou have %qdMB of disk space\n", disk.MBs());
		int a = request_number(main_space, 40, 1, disk.MBs());
		int b;

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

    	    	print(&main_coswap_long[1]);
	moreswap:
		printf("\n");
    	    	printf(main_coswap_long[0],
		       int(disk.ToMB(disk.DefaultSwap() + disk.SecPerCyl())));
	    	printf("\nYou have %dMB of disk space remaining\n",
		       int(disk.MBs() - disk.ToMB(a)));
    	    	if ((b = request_number(main_coswap, 0, 0,
		        int(disk.MBs() - disk.ToMB(a)))) != 0) {
		    b = disk.FromMB(b);
		    b /= disk.SecPerCyl();
		    b *= disk.SecPerCyl();

		    if (disk.ToMB(b - disk.SecPerCyl()) < 4.0) {
			print(main_swap_to_small);
			goto moreswap;
		    }
		    disk.part[p].number = p + 1;
		    disk.part[p].type = MBS_DOS;
		    swaplen = disk.part[p].length = b;
		    swapoff = disk.part[p].offset = a;
		    swaplen -= disk.SecPerCyl();
		    swapoff += disk.SecPerCyl();
		    ++p;
    	    	}

		disk.part[p].number = p + 1;
		disk.part[p].type = MBS_BSDI;
		disk.part[p].length = disk.UseSectors() - a - b;
		disk.part[p].offset = a + b;
    	    	disk.active = p + 1;
		if (request_yesno(main_view_fdisk, 0)) {
		    if (disk.FDisk() == -1) {
			print(main_aborted_fdisk_long);
			if (!request_yesno(main_aborted_fdisk, 0))
			    return(1);
		    }
	    	}
	    } else
#endif
	    {
		ret = disk.FDisk();
		int p;
		for (p = 0; p < 4 && disk.part[p].number; ++p)
			if (disk.part[p].original == 0)
				redobsd = 1;
	    }
	}

    	if (disk.use_fdisk) {
	    if (ret == 1) {
		if (!O::Flag(O::UpdateFDISK) || O::Flag(O::UpdateBSD)) {
		    for (i = 0; i < 4 && disk.part[i].number; ++i) {
			if (BSDType(disk.part[i].type))
			    break;
		    }
		    if (i < 4 && disk.part[i].number)
			break;
		    if (request_yesno(main_need_bsd_part, 1) == 0) {
#if 0
			O::Set(O::UpdateFDISK);
			disk.use_bsd = 0;
#endif
		    	break;
		    }
		} else
		    break;
	    } else {
		print(main_redo_fdisk_long);
		if (request_yesno(main_redo_fdisk, 0) == 0) {
		    print(main_need_fdisk_for_cores);
		    return(1);
		}
	    }
    	} else
	    break;
    }

    if (disk.use_bsd) {
	disk.SwitchToBSD();
#endif

	for (;;) {
	    int ret = 1;

#ifdef	sparc
    	    if ((!disk.label_original.Valid() || disk.label_original.Soft()) &&
		  disk.label_sun.Valid())
		disk.label_original = disk.label_sun;
#endif
	    if (disk.label_original.Valid() && !disk.label_original.Soft()) {
		if (redobsd)
		    print(main_cant_keep_existing_bsd_long);
		else
		    print(main_keep_existing_bsd_long);
    	    }

	    if (!disk.label_original.Valid() ||
		 disk.label_original.Soft() ||
		 request_yesno(main_keep_existing_bsd, redobsd)) {

		for (i = 0; i < 8; ++i)
		    disk.bsd[i].Zero();
		//
		// Make the c partition be the whole disk.
		//
		disk.bsd[0].number = 3;
		disk.bsd[0].length = disk.Sectors();
		disk.bsd[0].type = FS_UNUSED;
		disk.bsd[0].fixed = 1;
    	    	if (swaplen) {
		    disk.Sort();

		    for (i = 0; i < 8 && disk.bsd[i].number; ++i)
			;
		    disk.bsd[i].number = 2;
		    disk.bsd[i].length = swaplen;
		    disk.bsd[i].offset = swapoff;
		    disk.bsd[i].type = FS_SWAP;
		    disk.Sort();
		}

#ifdef	NEED_FDISK
    	    	int bp = 4;
    	    	for (int j = 0; j < 4 && disk.part[j].number; ++j) {
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

		print(main_standard_bsd_long);
		if (request_yesno(main_standard_bsd, 1)) {
		    disk.Sort();
		    disk.AddPartition(1);		// Add a root
		    if (!swaplen) {
			disk.Sort();
			disk.AddPartition(2);		// Add a swap
    	    	    }
		    disk.Sort();
		    disk.AddPartition(8);		// Add a /usr
		    //
		    // XXX - complain if something does not work.
		    //
		    if (request_yesno(main_view_new_bsd, 0)) {
			if (disk.FSys() == -1) {
			    print(main_aborted_bsd_long);
			    if (!request_yesno(main_aborted_bsd, 0))
				return(1);
			}
		    }
		} else
		    ret = disk.FSys();
	    } else {
		for (i = 0; i < 8; ++i) {
		    disk.bsd[i].Load(disk.label_original.d_partitions + i);
		    disk.bsd[i].number = disk.bsd[i].length ? i + 1 : 0;
		}
		disk.bsd[2].fixed = 1;
	    	disk.Sort();
	    	if (request_yesno(main_view_bsd, 0))
		    ret = disk.FSys();
	    }

	    if (ret == 1)
		break;
	    print(main_redo_bsd_long);
	    if (request_yesno(main_redo_bsd, 0) == 0) {
		print(main_need_bsd);
		return(1);
	    }
	}

root_again:
#ifdef	NEED_FDISK
	disk.ComputeBSDBoot();
    	if (disk.bsdbootpart < 0) {
    	    do {
		print(main_no_root_long);
		if (request_yesno(main_no_root, 1) == 0)
		    break;
		disk.FSys();
		disk.ComputeBSDBoot();
    	    } while (disk.bsdbootpart < 0);
    	}

	//
    	// Okay, if we have a root partition, make sure it is either
	// at the start of a BSD partition or the start of the disk.
	//
    	if (disk.bsdbootpart >= 0) {
	    Partition &fp = disk.part[disk.bsdbootpart];
    	    FileSystem &rf = disk.RootFilesystem();
	    if (fp.offset != rf.offset) {
		print(fp.number ? bad_root_part_fdisk : bad_root_part);
		if (request_yesno(main_no_root, 1)) {
		    disk.FSys();
		    goto root_again;
    	    	}
    	    } else if (fp.number && !fp.IsBSDType()) {
	    	print(root_on_bsd_only);
		if (request_yesno(main_no_root, 1)) {
		    disk.FSys();
		    goto root_again;
    	    	}
    	    }
    	}
#else
	for (;;) {
	    for (i = 0; i < 8 && disk.bsd[i].number; ++i)
		if (disk.bsd[i].number == 1)
		    break;
	    if (i < 8)
		break;
	    print(main_no_root_long);
	    if (request_yesno(main_no_root, 1) == 0)
		break;
	    disk.FSys();
	}
#endif

#ifdef	NEED_FDISK
    }


    if (disk.use_fdisk && !O::Flag(O::InCoreOnly)) {
	print(main_new_master_long);
    	if (request_yesno(main_new_master, disk.has_fdisk ? 0 : 1)) {
	    print(main_bsd_master_long);
    	    master = request_string(main_bsd_master, "bootany.sys");
    	    if (!master)
		master = "bootany.sys";
    	    if (!strcmp(master, "bootany.sys")) {
	    	request_inform(bootany_long);
		O::Set(O::RequireBootany);
    	    }
    	}
    }


    if (disk.use_bsd) {
#endif
	int no_boot_blocks = O::Flag(O::InCoreOnly);
    	int labelpart = 0;

	if (disk.d_type == DTYPE_PSEUDO)
	    no_boot_blocks = 1;
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
	    if (O::Flag(O::InCoreOnly)) {
		labelpart = -1;
		disk.bsdbootpart = -1;
		no_boot_blocks = 1;
	    } else {
		if (labelpart == 0) {
		    print(no_place_to_label);
		    if (request_yesno(label_at_0, 0) == 0)
			return(1);
		    labelpart = -1;
		    disk.bsdbootpart = -1;
		    no_boot_blocks = 1;
		} else if (disk.bsdbootpart < 0) {
		    request_inform(wont_boot_part);
		    no_boot_blocks = 1;
		} else if (labelpart != disk.part[disk.bsdbootpart].number) {
		    request_inform(wrong_boot_part);
		    no_boot_blocks = 1;
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
	if (disk.bsdoff == -1) {
	    if (!no_boot_blocks) {
		print(main_no_bootblocks_long);
    	    }
    	} else if (!no_boot_blocks) {
	    if ((labelpart < 0 && disk.bsdoff - LO != 0) ||
		(labelpart > 0 && disk.bsdoff - LO !=
		    off_t(disk.FindPartition(labelpart).offset) * SECSIZE))
		print(main_moved_bootblocks_long);
    	}
	if (labelpart >= 0)
	    disk.bsdoff = off_t(disk.FindPartition(labelpart).offset)
				* SECSIZE + LO;
	else
#endif
	    disk.bsdoff = LO;

    	if (!no_boot_blocks) {
	    getbootblocks();
    	} else
#ifdef	NEED_FDISK
#ifdef	__powerpc__
	    bootstrap = 0;
#endif
#ifdef	__i386__
	    primary = secondary = 0;
#endif
#endif
#ifdef	sparc
	    bootstrap = 0;
#endif

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
	memset(disk.label_new.d_partitions, 0,
					sizeof(disk.label_new.d_partitions));
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
#ifdef	NEED_FDISK
    }
#endif
    return(0);
}

void
getbootblocks()
{
    print(main_new_bootblocks_long);
    if (request_yesno(main_new_bootblocks, 1)) {
	char *def;
	char *type;
#ifdef	__i386__
	def = "bios";
	if (disk.d_type == DTYPE_FLOPPY)
	    def = "fd";
	else if (disk.d_type == DTYPE_ST506 || disk.d_type == DTYPE_ESDI) {
	    if (disk.use_fdisk && disk.dosg.Valid() && disk.dosg.Heads() > 16)
		request_inform(must_use_bios_boot);
	    else
		def = "wd";
	}
#endif
#if	defined(sparc)
	char boot1_str[] = "boot1";
	def = bootstrap ? bootstrap : boot1_str;
#endif
#if	defined(__powerpc__)
	def = "bootdisk";
#endif
	print(main_new_bootblocks_choice_long);
	for (;;) {
	    if ((type = request_string(main_new_bootblocks_choice, def)) != 0) {
#if	defined(sparc) || defined(__powerpc__)
		bootstrap = type;
		break;
#endif
#ifdef	__i386__
		if (!strcasecmp(type, "bios")) {
		    primary = "biosboot";
		    secondary = "bootbios";
		    break;
		} else if (!strcasecmp(type, "wd")) {
		    primary = "wdboot";
		    secondary = "bootwd";
		    break;
		} else if (!strcasecmp(type, "fd")) {
		    primary = "fdboot";
		    secondary = "bootfd";
		    break;
		} else if (!strcasecmp(type, "aha")) {
		    primary = "ahaboot";
		    secondary = "bootaha";
		    break;
		} else if (!strcasecmp(type, "eaha")) {
		    primary = "eahaboot";
		    secondary = "booteaha";
		    break;
		} else {
		    static char _primary[64], _secondary[64];
		    snprintf(_primary, sizeof(_primary), "%s/%sboot",
			_PATH_BOOTSTRAPS, type);
		    snprintf(_secondary, sizeof(_secondary), "%s/boot%s",
			_PATH_BOOTSTRAPS, type);
		    primary = _primary;
		    secondary = _secondary;
		    if (access(primary, R_OK) == 0 &&
		        access(secondary, R_OK) == 0)
			    break;
		}
#endif
	    }
	}
    }
}
