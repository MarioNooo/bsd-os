/*-
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI disksetup.cc,v 2.12 2002/03/14 21:36:38 giff Exp	*/

#include <err.h>
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

Disk disk;

int noquestions = 0;

#ifdef	NEED_FDISK
#define	_PATH_BOOTANY	"/sbin/bootany"

char *path_bootany = _PATH_BOOTANY;
#endif

#ifdef	__i386__
char *master = 0;
char *primary = 0;
char *secondary = 0;
int dosspace = 0;

char boot0[MAXPATHLEN];
char boot1[MAXPATHLEN];
char boot2[MAXPATHLEN];
#endif
#if	defined(__powerpc__)
char *master = 0;
char *bootstrap = 0;

char boot0[MAXPATHLEN];
char boot2[MAXPATHLEN];
#endif
#ifdef  sparc
char *bootstrap = 0;
char boot0[MAXPATHLEN];
#endif

void dumptab(char *);

void
O::Usage(int status)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr,
"    %s disk                                      # Read label\n",
			prog);
    fprintf(stderr,
"    %s [-NW] disk                                # Write Disable/Enable\n",
			prog);
#ifdef	NEED_FDISK
    fprintf(stderr,
"    %s -e [disk [xxboot bootxx [mboot]]]         # Edit disk label\n",
			prog);
#endif
#ifdef	sparc
    fprintf(stderr,
"    %s -e [disk [boot]                           # Edit disk label\n",
			prog);
#endif
    fprintf(stderr,
"    %s -i [disk]                                 # Interactive mode\n",
			prog);
#ifdef	NEED_FDISK
    fprintf(stderr,
"    %s -R disk protofile [xxboot bootxx [mboot]] # Restore Label\n",
			prog);
    fprintf(stderr,
"    %s -w disk disktype [xxboot bootxx [mboot]]  # Restore from disktab\n",
			prog);
    fprintf(stderr,
"    %s -B disk [ xxboot bootxx [mboot]]          # Install boot records\n",
			prog);
#endif
#ifdef	sparc
    fprintf(stderr,
"    %s -R disk protofile [ xxboot ]              # Restore Label\n",
			prog);
    fprintf(stderr,
"    %s -w disk disktype [ xxboot ]               # Restore from disktab\n",
			prog);
    fprintf(stderr,
"    %s -B disk [ xxboot ]                        # Install boot records\n",
			prog);
#endif
#if 0
    fprintf(stderr,
"    %s [-m mboot] [-b xxboot bootxx] disk        # Install boot records\n",
			prog);
#endif
    fprintf(stderr, "Additional flags:\n");
#ifdef	NEED_FDISK
    fprintf(stderr, "    -A        Path to the bootany program\n");
#endif
    fprintf(stderr, "    -K        Only read the Kernels (internal) label\n");
    fprintf(stderr, "    -D        Only read the On Disk label\n");
    fprintf(stderr, "    -s        Only read/write incore BSD labels\n");
    fprintf(stderr, "    -n        No writes to disk (open read only and non-blocking)\n");
    fprintf(stderr, "    -I        Ignore current BSD label\n");
#ifdef	NEED_FDISK
    fprintf(stderr, "    -I -I     Ignore current BSD label and FDISK table\n");
    fprintf(stderr, "    -F        Edit FDISK partition (implies -i)\n");
    fprintf(stderr, "    -P        Edit BSD partition (implies -i)\n");
#endif
    fprintf(stderr, "    -E        Expert mode.  Allows broken tables, etc.\n");
    fprintf(stderr, "    -S        Express (simple) mode. No questions asked.\n");
    fprintf(stderr, "    -M mem    Pretend physical memory is mem MB\n");
#if 0
    fprintf(stderr, "    -f        Force the installing of boot blocks (not implemented yet)\n");
    fprintf(stderr, "    -t path   Dump file system names to {path}disk\n");
    fprintf(stderr, "    -x file   Make copes of tables in file\n");
#endif

    exit(status);
}

main(int ac, char **av)
{
    int i, j;
    int e;
    char *dn;
    int c;

    char *proto = 0;
    char *output = 0;
    char *tabpath = 0;

#ifdef	__i386__
    int mfd = -1;
    int pfd = -1;
    int sfd = -1;
#endif
#ifdef	__powerpc__
    int mfd = -1;
    int bfd = -1;
#endif
#ifdef	sparc
    int bfd = -1;
#endif
    int errno;

    if ((dn = strrchr(av[0], '/')) != NULL)
	O::Program(++dn);
    else
	O::Program(av[0]);

    while ((c = getopt(ac, av, "b:defikm:np:q:rst:wx:A:BDEFIKM:NPQRSWZ:?")) != EOF)
	switch (c) {
#ifdef	NEED_FDISK
	case 'A':
	    path_bootany = optarg;
	    break;
    	case 'm':	// install master boot record
	    O::Set(O::INSTALL);
	    O::Set(O::IgnoreGeometry);
    	    if (master)
		goto usage;
	    master = optarg;
	    break;
	case 'F':	// Update FDISK Table
	    O::Set(O::INTERACTIVE);
	    O::Set(O::UpdateFDISK);
	    break;
	case 'P':	// Update BSD Partition Table
	    O::Set(O::INTERACTIVE);
	    O::Set(O::UpdateBSD);
	    break;
#ifdef	__i386__
    	case 'p':	// install primary boot strap
    	    if (primary)
		goto usage;
	    primary = optarg;
	    break;
    	case 'q':	// install secondary boot strap
    	    if (secondary)
		goto usage;
	    secondary = optarg;
	    break;
#endif
#endif
#ifdef	sparc
    	case 'p':	// install primary boot strap
    	    if (bootstrap)
		goto usage;
	    bootstrap = optarg;
	    break;
#endif
	case 'r':	// romfs filesystem, no disk label
	    O::Set(O::NoDiskLabel);
	    O::Set(O::DontUpdateLabel);
	    break;

    	case 'M':
	    PhysMem(atoi(optarg) * 1024 * 1024);
	    break;
    	case 'b':	// install boot straps
	    O::Set(O::INSTALL);
	    O::Set(O::IgnoreGeometry);
	    O::Set(O::DontUpdateLabel);
#ifdef	__i386__
    	    if (ac < ++optind)
		goto usage;
    	    if (primary || secondary)
	    	goto usage;
	    primary = optarg;
	    secondary = av[optind-1];
#endif
#if	defined(sparc) || defined(__powerpc__)
	    if (bootstrap)
		goto usage;
    	    bootstrap = optarg;
#endif
    	    break;
	case 'e':	// Edit
	    O::Set(O::EDIT);
	    O::Set(O::IgnoreGeometry);
	    break;
	case 'f':	// Force Boot
	    O::Set(O::ForceBoot);
	    break;
	case 's':	// Only save label in core
	    O::Set(O::InCoreOnly);
	    break;
	case 'i':	// Interactive
	    O::Set(O::INTERACTIVE);
	    break;
	case 'n':	// Nowrite
	    O::Set(O::NoWrite);
	    O::Set(O::NonBlock);
	    break;
	case 'B':	// Bootblocks
	    O::Set(O::BOOTBLKS);
	    O::Set(O::ForceBoot);
	    O::Set(O::IgnoreGeometry);
	    O::Set(O::DontUpdateLabel);
#ifdef	__i386__
    	    if (primary || secondary || master)
	    	goto usage;
#endif
#ifdef	__powerpc__
    	    if (bootstrap || master)
	    	goto usage;
#endif
#ifdef	sparc
	    if (bootstrap)
		goto usage;
#endif
	    break;
	case 'E':	// Expert Mode
	    O::Set(O::Expert);
	    break;
	case 'S':	// Simple/Express Mode
	    O::Set(O::Express);
	    O::Set(O::EXPRESS);
	    break;
	case 'I':	// Ignore label (two -> ignore fdisk label)
	    O::Set(O::Ignore);
	    O::Set(O::NonBlock);
	    break;
	case 'N':	// Make disk label not writeable
	    O::Set(O::W_DISABLE);
	    O::Set(O::NonBlock);
	    O::Set(O::Ignore);
	    O::Set(O::Ignore);
	    O::Set(O::IgnoreGeometry);
	    break;
	case 'w':	// Restore from disktab file
	    O::Set(O::DISKTYPE);
	    O::Set(O::Express);		// Don't ask questions
	    O::Set(O::NonBlock);
	    break;
	case 'R':	// Restore from ascii version
	    O::Set(O::RESTORE);
	    O::Set(O::Express);		// Don't ask questions
	    O::Set(O::NonBlock);
	    break;
	case 'W':	// Make disk label writeable
	    O::Set(O::W_ENABLE);
	    O::Set(O::NonBlock);
	    O::Set(O::Ignore);
	    O::Set(O::Ignore);
	    O::Set(O::IgnoreGeometry);
	    break;
	case 'D':	// Only read on disk label
	    O::Set(O::ReadDisk);
	    break;
	case 'd':	// Only read/write disk label
	    O::Set(O::ReadDisk);
	    O::Set(O::WriteDiskOnly);
	    break;
	case 'K':	// Only read kernel label
	    O::Set(O::ReadKernel);
	    break;
	case 'x':	// Undocumented -- make copies of tables in savename
	    output = optarg;
	    break;
	case 't':	// Undocumented -- output tables for EZconfig
	    tabpath = optarg;
	    break;
#ifdef __i386__
	case 'Z':
	    dosspace = atoi(optarg);
	    break;
#endif
	case 'Q':
	    noquestions = 1;
	    break;
    	usage:
	default:
	    O::Usage();
	    break;
	}

    if (O::Flag(O::ReadDisk) && O::Flag(O::ReadKernel)) {
	fprintf(stderr, "Only one of -K and -D may be specified\n");
	exit(1);
    }

    if (O::Cmd() == O::UNSPEC) {
	O::Set(O::READ);
	O::Set(O::NoWrite);
    }

    switch (O::Cmd()) {
    case O::W_ENABLE:
    case O::W_DISABLE:
    case O::READ:
    case O::INSTALL:
	if (optind != ac - 1)
	    O::Usage();
	break;
    case O::INTERACTIVE:
	if (optind < ac - 1)
	    O::Usage();
	break;
    case O::RESTORE:
    case O::DISKTYPE:
#ifdef	__i386__
	if (optind < ac - 5 || optind > ac - 2)
	    O::Usage();
	proto = av[optind + 1];
	if ((primary = av[optind + 2]) != NULL) {
	    if (!(secondary = av[optind + 3]))
		O::Usage();
	    master =  av[optind + 4];
    	}
#endif
#ifdef	__powerpc__
	if (optind < ac - 4 || optind > ac - 2)
	    O::Usage();
	proto = av[optind + 1];
	if (bootstrap = av[optind + 2])
	    master =  av[optind + 3];
#endif
#ifdef	sparc
	if (optind > ac - 2 || optind < ac - 2)
	    O::Usage();
	proto = av[optind + 1];
	bootstrap = av[optind + 2];
#endif
	break;
    case O::EDIT:
    case O::BOOTBLKS:
#ifdef	__i386__
	if (optind < ac - 4 || optind > ac - 1)
	    O::Usage();
	if ((primary = av[optind + 1]) != NULL) {
	    if (!(secondary = av[optind + 2]))
		O::Usage();
	    master =  av[optind + 3];
    	}
#endif
#ifdef	__powerpc__
	if (optind < ac - 3 || optind > ac - 1)
	    O::Usage();
	if (bootstrap = av[optind + 1])
	    master =  av[optind + 2];
#endif
#ifdef	sparc
	if (optind < ac - 2 || optind > ac - 1)
	    O::Usage();
	bootstrap = av[optind + 1];
#endif
	break;
    }

    if (!(dn = av[optind])) {
	if (O::Cmd() != O::EXPRESS && O::Cmd() != O::INTERACTIVE &&
	   			      O::Cmd() != O::EDIT)
	    O::Usage(2);
	dn = ChooseDisk();
    }

    if ((e = disk.Init(dn)) != NULL) {
	fprintf(stderr, "initializing %s: %s\n", dn, Errors::String(e));
	exit(1);
    }

#ifdef	NEED_FDISK
    if (O::Flag(O::UpdateFDISK) && !O::Flag(O::UpdateBSD))
	O::Set(O::DontUpdateLabel);
#endif

    switch (O::Cmd()) {
    case O::W_ENABLE:
    	disk.WriteEnable();
	exit(0);
    case O::W_DISABLE:
    	disk.WriteDisable();
	exit(0);
    case O::READ:
    	if (!disk.label_original.Valid() || disk.label_original.Soft()) {
#ifdef sparc
	    if (disk.label_sun.Valid()) {
		printf("# Missing disk label, sun label used.\n");
		disk.label_original = disk.label_sun;
	    } else
#endif
	    {
		printf("# Missing disk label, default label used.\n");
		printf("# Geometry may be incorrect.\n");
	    }
    	}
#ifdef	NEED_FDISK
    	disk.use_fdisk = disk.has_fdisk;
	disklabel_display(disk.device, stdout, &disk.label_original, disk.PartTable());
#else
	disklabel_display(disk.device, stdout, &disk.label_original);
#endif
	exit(0);
    case O::EDIT:
#ifdef	NEED_FDISK
    	disk.use_fdisk = disk.has_fdisk;
#endif
    	edit();
    	break;
    case O::INTERACTIVE:
    	if ((i = interactive()) != NULL) {
	    if (tabpath)
		dumptab(tabpath);
	    exit(i);
	}
	break;
    case O::EXPRESS:
    	express();
	break;
    case O::BOOTBLKS:
#ifdef	__i386__
    	if (!primary && disk.label_original.d_boot0) {
	    if (*disk.label_original.d_boot0 != '/')
		sprintf(boot0, "%s/%s", _PATH_BOOTSTRAPS,
				disk.label_original.d_boot0);
	    else
		strcpy(boot0, disk.label_original.d_boot0);
	    primary = boot0; 
    	}
        if (!secondary && disk.label_original.d_boot1) {
	    if (*disk.label_original.d_boot1 != '/')
		sprintf(boot1, "%s/%s", _PATH_BOOTSTRAPS,
				disk.label_original.d_boot1);
	    else
		strcpy(boot1, disk.label_original.d_boot1);
	    secondary = boot1;
        }
    	disk.use_fdisk = disk.has_fdisk;
	if (!primary && !secondary)
	    getbootblocks();
#endif
#if	defined(sparc) || defined(__powerpc__)
    	if (!bootstrap && disk.label_original.d_boot0) {
	    if (*disk.label_original.d_boot0 != '/')
		sprintf(boot0, "%s/%s", _PATH_BOOTSTRAPS,
				disk.label_original.d_boot0);
	    else
		strcpy(boot0, disk.label_original.d_boot0);
	    bootstrap = boot0; 
    	}
	if (!bootstrap)
	    getbootblocks();
#endif
    	O::Set(O::INSTALL, 1);
#ifdef	sparc
    	if ((!disk.label_original.Valid() || disk.label_original.Soft()) &&
	     disk.label_sun.Valid())
		disk.label_original = disk.label_sun;
	else
#endif
	disk.label_new = disk.label_original;
    	break;
    case O::INSTALL:
#ifdef	NEED_FDISK
    	disk.use_fdisk = disk.has_fdisk;
#endif
#ifdef	sparc
    	if ((!disk.label_original.Valid() || disk.label_original.Soft()) &&
	     disk.label_sun.Valid())
		disk.label_original = disk.label_sun;
	else
#endif
	disk.label_new = disk.label_original;
    	break;
    case O::RESTORE:
    case O::DISKTYPE:
#ifdef	NEED_FDISK
    	disk.use_fdisk = disk.has_fdisk;
#endif
    	break;
    default:
    	printf("That command is not implemented yet\n");
	exit(0);
    }

    if (proto && O::Cmd() == O::DISKTYPE) {
	int err = disk.label_new.Disktab(proto);

	if (err)
	    errx(1, "%s: %s", proto, Errors::String(e));
    } else if (proto) {
	FILE *fp = fopen(proto, "r");

    	if (!fp)
	    err(1, proto);
#ifdef	NEED_FDISK
	if (disklabel_getasciilabel(fp, &disk.label_new, 0) == 0)
	    errx(1, "%s: bad label prototype\n", proto);
#else
	if (disklabel_getasciilabel(fp, &disk.label_new) == 0)
	    errx(1, "%s: bad label prototype\n", proto);
#endif
	fclose(fp);
    }

#ifdef	NEED_FDISK
    BootBlock MBR;

#ifdef	__i386__
    if (O::Flag(O::InCoreOnly) && (master || primary || secondary)) {
	print(incore_and_bootblocks_long);
	master = primary = secondary = 0;
    }
#endif
#ifdef	__powerpc__
    if (O::Flag(O::InCoreOnly) && bootstrap) {
	print(incore_and_bootblocks_long);
	bootstrap = 0;
    }
#endif

    if (master) {
	if ((mfd = open(master, O_RDONLY)) < 0) {
	    sprintf(boot2, "%s/%s", _PATH_BOOTSTRAPS, master);
	    if ((mfd = open(boot2, O_RDONLY)) < 0)
		err(1, "MBR: %s", master);
	    master = boot2;
    	}
    	if ((e = MBR.Read(mfd)) != NULL)
	    errx(1, "reading MBR %s: %s\n", master, Errors::String(e));
    	close(mfd);
    } else { 
	MBR = disk.bootblock;
	MBR.data[510] = MB_SIGNATURE & 0xff;
	MBR.data[511] = (MB_SIGNATURE >> 8 )& 0xff;
    }

#ifdef	__i386__
    if (primary && (pfd = open(primary, O_RDONLY)) < 0) {
    	if (primary[0] != '/') {
	    sprintf(boot0, "%s/%s", _PATH_BOOTSTRAPS, primary);
	    primary = boot0;
	}
    	if ((pfd = open(primary, O_RDONLY)) < 0)
	    err(1, "primary bootstrap %s", primary);
    }

    if (secondary && (sfd = open(secondary, O_RDONLY)) < 0) {
    	if (secondary[0] != '/') {
	    sprintf(boot1, "%s/%s", _PATH_BOOTSTRAPS, secondary);
	    secondary = boot1;
	}
    	if ((sfd = open(secondary, O_RDONLY)) < 0)
	    err(1, "secondary bootstrap %s", secondary);
    }
#endif

#ifdef	__powerpc__
    if (bootstrap && (bfd = open(bootstrap, O_RDONLY)) < 0) {
    	if (bootstrap[0] != '/') {
	    sprintf(boot0, "%s/%s", _PATH_BOOTSTRAPS, bootstrap);
	    bootstrap = boot0;
	}
    	if ((bfd = open(bootstrap, O_RDONLY)) < 0)
	    err(1, "bootstrap %s", bootstrap);
    }
#endif

    MBPart *t;
    if (disk.use_fdisk && (t = disk.PartTable()))
	MBR.MergePartitions(t);
    else if (O::Flag(O::RequireBootany))
    	request_inform(main_bootany_no_fdisk);
#endif
#ifdef	sparc
    if (O::Flag(O::InCoreOnly) && bootstrap) {
	print(incore_and_bootblocks_long);
	bootstrap = 0;
    }

    if (bootstrap && (bfd = open(bootstrap, O_RDONLY)) < 0) {
    	if (bootstrap[0] != '/') {
	    sprintf(boot0, "%s/%s", _PATH_BOOTSTRAPS, bootstrap);
	    bootstrap = boot0;
	}
    	if ((bfd = open(bootstrap, O_RDONLY)) < 0)
	    err(1, "bootstrap %s", bootstrap);
    }
#endif

    //
    // If disk.bsdoff has not been set (either to where the old label
    // was, or to where we want to put the new label) we must go out
    // and calculate the value for ourselves.
    //
    if (disk.bsdoff == -1)
	disk.bsdoff = disk.LabelLocation();

    disk.label_new.ComputeSum(disk.bsdoff);

    disk.bsdoff -= LABELSECTOR * SECSIZE + LABELOFFSET;

    if (tabpath)
	dumptab(tabpath);

    if (O::Flag(O::NoWrite)) {
	if (O::Cmd() == O::INTERACTIVE)
#ifdef	NEED_FDISK
	    disklabel_display(disk.device, stdout, &disk.label_new, disk.PartTable());
#else
	    disklabel_display(disk.device, stdout, &disk.label_new);
#endif
	exit(0);
    }

    if (output) {
    	disk.dfd = creat(output, 0666);
    	disk.path = output;
    	if (disk.dfd < 0)
	    err(1, "save file %s", output);
    }

#ifdef	NEED_FDISK
    if (master || (disk.use_fdisk && disk.PartTable())) {
    	if (O::Flag(O::RequireBootany) || O::Cmd() == O::EDIT ||
	    memcmp(&disk.bootblock, &MBR, sizeof(BootBlock))) {
	    if (disk.has_fdisk && O::Cmd() != O::EDIT && !O::Flag(O::Express))
		for (i = 1; i <= 4; ++i) {
#define	o	    disk.bootblock
		    if (o.Type(i) && o.Length(i)) {
			for (j = 1; j <= 4; ++j) {
			    if (MBR.Type(j) == o.Type(i) &&
				MBR.Offset(j) == o.Offset(i) &&
				MBR.Length(j) == o.Length(i))
				    break;
			}
			if (j > 4)
			    printf("Current FDISK partition %d of type %s"
				   "(0x%02x) will be destroyed.\n",
				    i, PTypeFind(o.Type(i)).name, o.Type(i));
		    }
#undef	o
		}
	    if (O::Flag(O::Express) || request_yesno(okay_to_write_mbr, 1)) {
		if (lseek(disk.dfd, 0, L_SET) == -1) {
		    warn("%s: seeking to MBR (not written)", disk.path);
		} else {
		    disk.WriteEnable();
		    MBR.Write(disk.dfd);
		    fsync(disk.dfd);
		    if (O::Flag(O::RequireBootany)) {
			char cmdbuf[1024];
			sprintf(cmdbuf, "%s %s-n -i %s", path_bootany, O::Flag(O::Express) ? "-S " : "", disk.path);
			system(cmdbuf);
		    }
		    fsync(disk.dfd);
		    disk.WriteDisable();
		}
	    }
    	} else if (!O::Flag(O::Express))
	    request_inform(mbr_stays_same);
    }
#endif

#ifdef	__i386__
    //
    // If we are not going to update the label, the primary,
    // or the secondary bootstraps, might as well exit.
    //
    if (O::Flag(O::DontUpdateLabel) && sfd < 0 && pfd < 0)
	exit(0);
#endif
#if	defined(sparc) || defined(__powerpc__)
    //
    // If we are not going to update the label or the bootstrap,
    // might as well exit.
    //
    if (O::Flag(O::DontUpdateLabel) && bfd < 0)
	exit(0);
#endif

    unsigned char bootarea[BBSIZE];

    memset(bootarea, 0, sizeof(bootarea));

    //
    // We start assuming we will only write the disk label
    // We increase the size as we figure out more to write.
    //
    int start = LABELSECTOR * SECSIZE + LABELOFFSET;
    int stop = LABELSECTOR * SECSIZE + LABELOFFSET + sizeof(DiskLabel);

#ifdef	__i386__
    if (sfd >= 0) {
	if (read(sfd, &bootarea[SECSIZE], BBSIZE - SECSIZE) < 0)
	    err(1, "reading %s", secondary);
	start -= LABELOFFSET;
    	stop = BBSIZE;
	close(sfd);
    }

    if (pfd >= 0) {
	int o = (disk.bsdoff == 0 && disk.use_fdisk) ? 15 * SECSIZE : 0;
    	if (o == 0)
	    start = 0;
    	else if (sfd < 0) {
	    print(need_secondary_bootstrap);
	    exit(1);
    	}
	if (read(pfd, &bootarea[o], SECSIZE) < 0)
	    err(1, "reading %s", primary);
	close(pfd);
    }
#endif
#if	defined(sparc)
    if (bfd >= 0) {
	if (read(bfd, &bootarea[SECSIZE], BBSIZE - SECSIZE) < 0)
	    err(1, "reading %s", bootstrap);
	start -= LABELOFFSET;
    	stop = BBSIZE;
	close(bfd);
    }
#endif

    //
    // Okay to check space even if not installing label
    //
    unsigned char *p = bootarea + LABELSECTOR * SECSIZE + LABELOFFSET;
    DiskLabel *dl = (DiskLabel *)p;
    while (p < (unsigned char *)(dl+1))
	if (*p++) {
	    print(no_room_for_label);
	    exit(1);
    	}

#ifndef	__powerpc__
    if (!O::Flag(O::NoDiskLabel) && O::Flag(O::DontUpdateLabel)
				    && !output && !proto
#ifdef	__i386__
				    && (sfd >= 0 || pfd >= 0))
#endif
#if	defined(sparc) || defined(__powerpc__)
				    && bfd >= 0)
#endif
    {
	//
	// Okay, we are not supposed to modify the label.  We just read
	// the sector where the label would go and read it up.
	//
    	DiskLabel label;
    	off_t offset;

    	//
    	// This could be a problem on an unlabled disk...
    	//
	int e = label.Read(disk.dfd, disk.bsdoff + LABELSECTOR * SECSIZE
					         + LABELOFFSET, &offset);
    	if (e)
	    errx(1, "reading existing disk label from %s: %s\n", disk.path,
		Errors::String(e));
    	if (offset != disk.bsdoff + LABELSECTOR * SECSIZE + LABELOFFSET)
    	    request_inform(use_old_label);
    	label.ComputeSum(disk.bsdoff + LABELSECTOR * SECSIZE + LABELOFFSET);
    	*dl = label;
    } else
#endif
    if (!O::Flag(O::DontUpdateLabel))
	*dl = disk.label_new;

    if (!O::Flag(O::Express) && !noquestions)
    	printf("\n");

    if (!O::Flag(O::Express) && disk.label_original.Valid()
			     && !disk.label_original.Soft()) {
	for (i = 0; i < 8; ++i) {
#define	o   disk.label_original.d_partitions[i]
	    if (o.p_size && o.p_fstype != FS_SWAP && o.p_fstype != FS_UNUSED) {
		for (j = 0; j < 8; ++j) {
#define	n	dl->d_partitions[j]
		    if (o.p_size == n.p_size &&
			o.p_offset == n.p_offset &&
			o.p_fstype == n.p_fstype)
			    break;
		}
		if (j >= 8)
		    printf("Current BSD partition %c will be destroyed.\n",
			    i + 'a');
	    } 
#undef n
#undef o
	}
    }


    if (O::Flag(O::InCoreOnly) || O::Flag(O::Express) || noquestions ||
#ifdef	__i386__
	request_yesno((sfd < 0 && pfd < 0) ? okay_to_write_dl
					   : okay_to_write_dlbb, 1))
#endif
#if	defined(sparc) || defined(__powerpc__)
	request_yesno(bfd < 0 ? okay_to_write_dl : okay_to_write_dlbb, 1))
#endif
    {


	//
	// First write the label in core
	//
#ifdef	__powerpc__
        if (!O::Flag(O::DontUpdateLabel))
#endif
    	if (!O::Flag(O::NoDiskLabel) && !O::Flag(O::WriteDiskOnly)
				     && (errno = dl->WriteInternal(disk.dfd)))
		if (errno != EBUSY)
			warn("%s: writing internal label", disk.path);

    	if (!O::Flag(O::InCoreOnly)) {
	    disk.WriteEnable();

#ifdef	__powerpc__
          if (!O::Flag(O::DontUpdateLabel)) {
#endif
#ifdef	sparc
	    if (dl->WriteSun(disk.dfd))
		warn("%s: writing sun label", disk.path);
#endif

    	    int o;
    	    char tbuf[SECSIZE];

    	    if ((o = (start % SECSIZE)) != 0) {
	    	if (lseek(disk.dfd, disk.bsdoff + start - o, L_SET) == -1 ||
		    read(disk.dfd, tbuf, SECSIZE) != SECSIZE) {

		    warn("%s: merging boot blocks", disk.path);
		    fsync(disk.dfd);
		    exit(0);
    	    	}
		memcpy(bootarea + start - o, tbuf, o);
    	    	start -= o;
    	    }

    	    if ((o = (stop % SECSIZE)) != 0) {
	    	if (lseek(disk.dfd, disk.bsdoff + stop - o, L_SET) == -1 ||
		    read(disk.dfd, tbuf, SECSIZE) != SECSIZE) {

		    warn("%s: merging boot blocks", disk.path);
		    fsync(disk.dfd);
		    exit(0);
    	    	}
		memcpy(bootarea + stop, tbuf + o, SECSIZE - o);
    	    	stop += SECSIZE - o;
    	    }

#ifdef	__powerpc__
	    if (bfd >= 0)
		powerpc_writeboot(bfd);
#endif
	    if (lseek(disk.dfd, disk.bsdoff + start, L_SET) == -1)
		warn("%s: seeking to boot blocks", disk.path);
	    else if ((c = write(disk.dfd, bootarea+start, stop-start)) != stop-start)
		warn("%s: writing boot blocks (%d != %d)", disk.path, c, stop-start);

#ifdef	__powerpc__
	  } else if (bfd >= 0)
	    powerpc_writeboot(bfd);
#endif
	    disk.WriteDisable();
    	}
	fsync(disk.dfd);
    }
    exit(0);
}

void
dumptab(char *tabpath)
{
    FILE *fp;
    char *type;
    int i;
    char tabname[strlen(tabpath) + strlen(disk.device) + 1];
    
    strcpy(tabname, tabpath);
    strcat(tabname, disk.device);

    if ((fp = fopen(tabname, "w")) == NULL)
	return;
    for (i = 0; i < 8 && disk.bsd[i].number; ++i) {
	FileSystem &f = disk.bsd[i];
	if (f.number == 3)
	    continue;
	type = FTypeFind(f.type).name;
	if (f.mount[0] == '\0')
	    strcat(f.mount, strcmp(type, "swap") ? "UNKNOWN" : "swap");
	/*
    	 * the 4.2 filesystem type is really ufs (most of the time)
	 */
    	if (strcmp(type, "4.2") == 0)
	    type = "ufs";
	/*
	 * Only put in real lines for ufs, swap, and msdos filesystems
	 */
	if (f.mount[0] != '/' && strcmp(f.mount, "swap")
			      || strcmp(type, "ufs") &&
				 strcmp(type, "swap") &&
				 strcmp(type, "msdos"))
	    fprintf(fp, "# ");
	fprintf(fp, "/dev/%s%c\t%s%s%s\t",
		disk.device, f.number + 'a' - 1, f.mount, 
		strlen(f.mount) > 7 ? "\t" : "\t\t", type);
	if (strcmp(type, "ufs") == 0)
	    fprintf(fp, "rw\t1 %d\n", strcmp(f.mount, "/") ? 2 : 1);
	else if (strcmp(type, "swap") == 0)
	    fprintf(fp, "sw\t0 0\n");
	else if (strcmp(type, "msdos") == 0)
	    fprintf(fp, "rw\n");
	else
	    fprintf(fp, "na,rw,nodev,nosuid\n");
    }
    fclose(fp);
}
