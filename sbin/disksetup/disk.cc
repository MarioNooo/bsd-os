/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI disk.cc,v 2.29 2002/07/30 20:59:36 rfr Exp	*/

#include "fs.h"
#include <err.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <paths.h>
#include "disk.h"
#include "screen.h"
#include "help.h"
#include "showhelp.h"
#include "field.h"
#include "util.h"

#include "machine/bootparam.h"
#ifdef	sparc
#ifndef	SUNDSKLABEL
#define	SUNDSKLABEL "/sys/sparc/sunos/sun_disklabel.h"
#endif
#include SUNDSKLABEL
#endif

#if	_BSDI_VERSION > 199312
#ifdef	TRUE
#undef TRUE
#undef FALSE
#endif
#include <sys/sysctl.h>
#include <machine/cpu.h>
#if	defined(d_type)
#undef	d_type
#endif

#endif
#define _PATH_SCSICMD   "/sbin/scsicmd"

Errors ENODISKLABEL("No BSD disklabel found");
Errors ENOSUNLABEL("No Sun disklabel found");
Errors EDEFLABEL("No label found or derived");
Errors ENOTCHAR("Not a character special device");
Errors ESCSICMD("Could not execute the command scsicmd");
Errors EBADLABEL("Invalid label returned");
Errors EBLOCKDEVICE("Block (cooked) device given.  Need character (raw) device");
Errors ECANTADD("No available space to add BSD partition");
Errors EWONTADD("No FDISK partitions assigned to BSD");
Errors EALREADYUSED("The requested BSD partition is already assigned");
Errors EUNKOWNTYPE("Unknown disk type");
Errors ENOTMP("Unable to open temporary file");

DiskLabel Disk::empty_label;
FileSystem Disk::empty_filsys;
#ifdef	NEED_FDISK
Partition Disk::empty_partition;
#endif

Disk::Disk()
{
    int i;

    dfd = -1;
    memset(device, 0, sizeof(device));
    path = 0;

    secsize = 512;
    badblock = 0;
    bsdoff = -1;
    bsd_modified = 0;
    d_type = 0;
    for (i = 0; i < 8; ++i)
        bsd[i].Zero();

#ifdef	NEED_FDISK
    active = 0;

    use_bsd = 0;
    use_fdisk = 0;
    has_fdisk = 0;

    bsdbootpart = -1;

    part_modified = 0;


    use_bsdg = 0;

    for (i = 0; i < 4; ++i)
        part[i].Zero();
#endif
}

quad_t
Disk::FromGB(double i)
{
    return(quad_t(i) * quad_t(1024 * 1024 * 1024 / secsize));
}

quad_t
Disk::FromMB(double i)
{
    return(quad_t(i) * quad_t(1024 * 1024 / secsize));
}

quad_t
Disk::FromCyl(double i)
{
    return(quad_t(i) * SecPerCyl());
}

quad_t
Disk::FromTrack(double i)
{
    return(quad_t(i) * SecPerTrack());
}

quad_t
Disk::FromSector(double i)
{
    return(quad_t(i));
}


void
Disk::WriteEnable()
{
    int flag = 1;

    if (ioctl(dfd, DIOCWLABEL, &flag))
	if (errno != ENOTTY)
	    warn("write enabling %s", disk.path);
}

void
Disk::WriteDisable()
{
    int flag = 0;

    if (ioctl(dfd, DIOCWLABEL, &flag))
	if (errno != ENOTTY)
	    warn("write disabling %s", disk.path);
}

#ifdef	NEED_FDISK
void
Disk::ComputeBSDBoot()
{
    bsdbootpart = -1;

    for (int i = 0; i < 8 && bsd[i].number; ++ i) {
    	if (bsd[i].number == 1) {
	    bsdbootpart = FindPartForBSD(bsd[i]);
	    return;
    	}
    }
}
#endif

FileSystem &
Disk::RootFilesystem()
{
    for (int i = 0; i < 8 && bsd[i].number; ++ i) {
    	if (bsd[i].number == 1) {
	    return(bsd[i]);
	}
    }
    return(empty_filsys);
}

#ifdef	NEED_FDISK
Partition &
Disk::FindPartition(int p)
{
    for (int i = 0; i < 4 && part[i].number; ++i)
	if (part[i].number == p)
	    return(part[i]);
    return(empty_partition);
}
#endif

FileSystem &
Disk::FindFileSystem(int p)
{
    for (int i = 0; i < 4 && bsd[i].number; ++i)
	if (bsd[i].number == p)
	    return(bsd[i]);
    return(empty_filsys);
}

off_t
Disk::LabelLocation()
{
    off_t offset = 0;
#ifdef	NEED_FDISK
    if (PartTable()) {
	int labelpart = 0;
	if (FindPartition(active).IsBSDType()) {
	    labelpart = active;
	} else {
	    for (int i = 1; i < 5; ++i) {
		Partition &p = FindPartition(i);
		if (p.IsBSDType()) {
		    if (FindPartition(i).bootany) {
			labelpart = i;
			break;
		    }
		    if (labelpart == -1)
			labelpart = i;
		}
	    }
	}
	if (labelpart) {
	    offset = FindPartition(labelpart).offset; 
	    offset *= SECSIZE;
	}
    }              
#endif
    offset += LABELSECTOR * SECSIZE + LABELOFFSET;
    return(offset);
}

int
DiskLabel::Read(int fd, off_t so, off_t *offset)
{
    unsigned char bootarea[BBSIZE];

    d_flags = D_DEFAULT;

    if (lseek(fd, so, L_SET) == -1)
	return(errno);

    if (read(fd, bootarea, BBSIZE) < BBSIZE)
	return(errno);

    disklabel *lp;

    for (lp = (disklabel *) bootarea;
	lp <= (disklabel *) (bootarea + BBSIZE - sizeof(*lp));
	lp = (disklabel *) ((char *) lp + sizeof(long)))
	    if (lp->d_magic == DISKMAGIC &&
		lp->d_magic2 == DISKMAGIC)
		    break;
    if (lp > (disklabel *) (bootarea + BBSIZE - sizeof(*lp)) ||
	lp->d_magic != DISKMAGIC || lp->d_magic2 != DISKMAGIC ||
	dkcksum(lp) != 0) {
	    // XXX - should we check sector 0?
	    if (so)
		return(Read(fd, 0, offset));
	    return (ENODISKLABEL);
    }
    memcpy(this, lp, sizeof(disklabel));
    if (offset)
	*offset = so + ((unsigned char *)lp - bootarea);
    Fixup();
    return(0);
}

#ifdef	sparc
int
DiskLabel::Sun(int fd)
{
    char buf[SECSIZE];

    sun_disklabel *sl;
    u_short *cp, cs;
    char *p;
    int i;

    Clean();

    if (lseek(fd, 0, 0))
	return(errno);
    errno = ENOSUNLABEL;
    if (read(fd, buf, sizeof(buf)) != sizeof(buf))
	return(errno);
    sl = (sun_disklabel *)buf;

    if (sl->sl_magic != SUN_DKMAGIC)
    	return(ENOSUNLABEL);

    cp = (u_short *)buf;
    cs = 0;
    while (cp < (u_short *)(sl + 1))
	cs ^= *cp++;
    if (cs)
    	return(ENOSUNLABEL);

    sl->sl_xxx1[0] = 0;		/* Make sure sl_text is null terminated */
    if (p = strstr(sl->sl_text, " cyl "))
	*p = 0;
    strncpy(d_packname, sl->sl_text, sizeof(d_packname));
    d_flags = 0;
    d_ncylinders = sl->sl_ncylinders;
    d_acylinders = sl->sl_acylinders;
    d_ntracks = sl->sl_ntracks;
    d_nsectors = sl->sl_nsectors;
    d_sparespercyl = sl->sl_sparespercyl;
    d_secpercyl = d_ntracks * d_nsectors;
    d_rpm = sl->sl_rpm;
    d_npartitions = 8;
    for (i = 0; i < d_npartitions; ++i) {
	d_partitions[i].p_offset = sl->sl_part[i].sdkp_cyloffset * d_secpercyl;
	d_partitions[i].p_size = sl->sl_part[i].sdkp_nsectors;
    }
    Fixup();
    return(0);
}

int
DiskLabel::WriteSun(int fd)
{
    sun_disklabel sl;
    u_short *cp;
    int i;

    memset(&sl, 0, sizeof(sl));

    sl.sl_rpm = d_rpm;
    sl.sl_sparespercyl = d_sparespercyl;
    sl.sl_interleave = d_interleave;
    sl.sl_ncylinders = d_ncylinders;
    if (d_ncylinders * d_secpercyl > d_secperunit)
    	sl.sl_ncylinders = d_secperunit / d_secpercyl;
    else
    	sl.sl_ncylinders = d_ncylinders;
    sl.sl_acylinders = d_acylinders;
    sl.sl_physcyls = sl.sl_ncylinders + sl.sl_acylinders;
    sl.sl_ntracks = d_ntracks;
    sl.sl_nsectors = d_nsectors;
    sl.sl_magic = SUN_DKMAGIC;

    snprintf(sl.sl_text, sizeof(sl.sl_text),
    	"%.16s cyl %d alt %d hd %d sec %d", d_typename,
		sl.sl_ncylinders,
		sl.sl_acylinders,
		sl.sl_ntracks,
		sl.sl_nsectors);

    for (i = 0; i < 8; ++i) {
	sl.sl_part[i].sdkp_cyloffset = d_partitions[i].p_offset / d_secpercyl;
	sl.sl_part[i].sdkp_nsectors = d_partitions[i].p_size;
    }

    cp = &sl.sl_cksum;

    while (cp > (u_short *)&sl)
	sl.sl_cksum ^= *--cp;

    if (lseek(fd, 0, 0))
	return(errno);

    if (write(fd, &sl, sizeof(sl)) != sizeof(sl))
	return(errno);

    return(0);
}
#endif

int
DiskLabel::Internal(int fd)
{
    if (ioctl(fd, DIOCGDINFO, this) < 0) {
	d_flags = D_DEFAULT;
	return(errno);
    }
    Fixup();
    if (d_flags & D_DEFAULT)
	return(EDEFLABEL);
    return(0);
}

int
DiskLabel::WriteInternal(int fd)
{
    if (ioctl(fd, DIOCSDINFO, this) < 0)
	return(errno);
    return(0);
}

int
DiskLabel::Generic(int fd)
{
    d_secperunit = 0;
    if (ioctl(fd, DIOCGHWINFO, this) < 0) {
	d_flags = D_DEFAULT;
	return(errno);
    }
    if (d_nsectors <= 0 || d_ntracks <= 0 || d_ncylinders <= 0) {
	d_nsectors = d_ntracks = d_ncylinders = 0;
	d_flags |= D_DEFAULT;
    } else if (d_secperunit &&
	d_secperunit > d_nsectors * d_ntracks * d_ncylinders) {
	    /*
	     * Deal with brain dead Seagate 20.4 GB disks.
	     * These disks cannot utilize the last partial cylinder
	     * if not in LBA mode.  Because of them we will not allow
	     * anyone to use the last partial cylinder, by default.
	     */
	    d_secperunit = d_nsectors * d_ntracks * d_ncylinders;
    }
	
    Fixup();
    if (d_flags & D_DEFAULT)
	return(EDEFLABEL);
    return(0);
}

int
DiskLabel::SCSI(char *device)
{
    char tmpnam[64];
    char cmdbuf[PATH_MAX * 2];
    FILE *fp;
#ifdef	NEED_FDISK
    MBPart bogus[MB_MAXPARTS];
#endif

    strcpy(tmpnam, _PATH_TMP "disksetup.XXXXXX");
    if (mktemp(tmpnam) == NULL)
	return (ENOTMP);
    sprintf(cmdbuf, "%s -f %s -l > %s", _PATH_SCSICMD, device, tmpnam);
    printf("Executing scsicmd to build a label:\n");
    if (system(cmdbuf) != 0)
	    return (ESCSICMD);
    if ((fp = fopen(tmpnam, "r")) == NULL)
    	return(errno);

#ifdef	NEED_FDISK
    bzero(bogus, sizeof(bogus));
    if (!disklabel_getasciilabel(fp, this, bogus))
	return(EBADLABEL);
#else
    if (!disklabel_getasciilabel(fp, this))
	return(EBADLABEL);
#endif
    fclose(fp);
    Fixup();
    return(0);
}

int
DiskLabel::Disktab(char *type, char *name)
{
    register struct disklabel *dp;

    dp = getdiskbyname(type);
    if (dp == NULL)
	return(EUNKOWNTYPE);

    memcpy(this, dp, sizeof(disklabel));

#ifdef	__i386__
    /* Check if disktab specifies the bootstraps (b0 or b1). */
    if (!primary && d_boot0) {
	    if (*d_boot0 != '/')
		sprintf(boot0, "%s/%s", _PATH_BOOTSTRAPS, d_boot0);
	    else
		strcpy(boot0, d_boot0);
	    primary = boot0;
    }
    if (!secondary && d_boot1) {
	    if (*d_boot1 != '/')
		sprintf(boot1, "%s/%s", _PATH_BOOTSTRAPS, d_boot1);
	    else
		strcpy(boot1, d_boot1);
	    primary = boot1;
    }
#endif
#if	defined(sparc) || defined(__powerpc__)
    /* Check if disktab specifies the bootstraps (b0 or b1). */
    if (!bootstrap && d_boot0) {
	    if (*d_boot0 != '/')
		sprintf(boot0, "%s/%s", _PATH_BOOTSTRAPS, d_boot0);
	    else
		strcpy(boot0, d_boot0);
	    bootstrap = boot0;
    }
#endif

    /* d_packname is union d_boot[01], so zero */
    bzero(d_packname, sizeof(d_packname));
    if (name)
	strncpy(d_packname, name, sizeof(d_packname));

    return(0);
}

void
DiskLabel::Clean()
{
    memset(this, 0, sizeof(disklabel));
    d_flags |= D_DEFAULT;
}

int
DiskLabel::Valid()
{
    return((d_flags & D_DEFAULT) ? 0 : 1);
}

int
DiskLabel::Soft()
{
    return((d_flags & D_SOFT) ? 1 : 0);
}

void
DiskLabel::Fixup()
{
    if (d_secpercyl == 0)
	d_secpercyl = (d_nsectors - d_sparespertrack) * d_ntracks
		    - d_sparespercyl;
    //
    // Use the length of the C partition, if possible, as the length
    // of the disk.
    //
    if (d_secperunit == 0)
	d_secperunit = d_partitions[2].p_size;
    if (d_secperunit == 0)
	d_secperunit = d_secpercyl * d_ncylinders;
    if (d_rpm == 0)
	d_rpm = 3600;
    if (d_interleave == 0)
	d_interleave = 1;
    if (d_secsize == 0)
	d_secsize = 512;
}

void
DiskLabel::ComputeSum(off_t start)
{
#ifdef  d_bsd_startsec
    d_bsd_startsec = (start / SECSIZE) - 1;
#endif  
    d_bbsize = BBSIZE;
    d_sbsize = SBSIZE;
    
    d_magic = DISKMAGIC;
    d_magic2 = DISKMAGIC;
    d_checksum = 0;
    d_checksum = dkcksum(this);
}

#ifdef	NEED_FDISK
static void
diskgeo(int bt, int bc, int bs, int et, int ec, int es, int rel, int len,
	int &heads, int &tracks, int &maxcyl)
{
    quad_t N, T;
    quad_t n, d;

    if (heads < 0)
	return;

    bs -= 1;
    len += rel;

    n = et * (rel - bs) - bt * (len - es);
    d = bc * (len - es) - ec * (rel - bs);

    if (n == 0 || d == 0) {
        return;
    }

    N = n / d;

    if (N * d != n) {
        return;
    }

    T = (rel - bs)/(bt + bc * N);

    if (heads && heads != N || tracks && tracks != T) {
	heads = tracks = -1;
	maxcyl = 0;
	return;
    }

    heads = N;
    tracks = T;

//  n = (len + N * T - 1) / (N * T);

    if (maxcyl <= ec)
	maxcyl = ec + 1;
}
#endif

int
Disk::Init(char *dev)
{
    int e;
    int cnt = 0;
    int mode = (O::Flag(O::NoWrite) ? O_RDONLY : O_RDWR) |
	       (O::Flag(O::NonBlock) ? O_NONBLOCK : 0);

    do {
	if (strchr(dev, '/') == 0) {
	    strncpy(device, dev[0] == 'r' ? dev+1 : dev, sizeof(device));
	    device[sizeof(device)-1] = 0;

	    char buf[5 + strlen(device) + 1];
	    sprintf(buf, "/dev/r%s", device);

	    if (!O::Flag(O::Expert)) {
		e = device[strlen(device)-1];
		if (e >= '0' && e <= '9')
		    goto select_c_partition;
	    }

	    if ((dfd = open(buf, mode)) < 0) {
    select_c_partition:
		sprintf(buf, "/dev/r%sc", device);
		dfd = open(buf, mode);
	    }
	    path = strdup(buf);
	} else {
	    strncpy(device, dev, sizeof(device));
	    device[sizeof(device)-1] = 0;
	    path = strdup(dev);
	    dfd = open(path, mode);
	}
    } while (dfd < 0 && ++cnt < 3 && (sleep(cnt) || 1));

    if (dfd < 0)
	return(errno);

    struct stat sb;

    if (fstat(dfd, &sb) < 0) {
	e = errno;
    	close(dfd);
	dfd = -1;
	return(e);
    }

    switch (sb.st_mode & S_IFMT) {
    case S_IFBLK:
	return(EBLOCKDEVICE);
    case S_IFCHR:
	/*
	 * The call to Type() sets d_type, it must stay in
	 */
    	if (Type() == 0 && !O::Flag(O::Expert))
		return(EUNKOWNTYPE);
	break;
    default:
	if (!O::Flag(O::Expert))
	    return(ENOTCHAR);
	if (sb.st_size <= 18 * 80 * 1024)
	    d_type = DTYPE_FLOPPY;
    	else
	    d_type = 0;
	break;
    }

#ifdef	sparc
    if (e = bootblock.Read(dfd))
	return(e);
    bsdoff = 1;
#endif
#ifdef	NEED_FDISK
    if (!O::Flag(O::IgnoreFDISK)) {
	if ((e = bootblock.Read(dfd)) != 0)
	    return(e);
    } else
    	bootblock.Clean();

    int bsdpart = 0;

    if (
#ifndef	__powerpc__
	d_type != DTYPE_FLOPPY &&
#endif
				  bootblock.Signature() == MB_SIGNATURE) {
    	int valid = 0;
    	int invalid = 0;
    	int any = 0;
	memcpy(part_table, bootblock.Part(1), sizeof(part_table));

    	int i;
    	for (i = 1; i <= 4; ++i) {
	    Partition &p = part[i-1];
	    if ((p.length = bootblock.Length(i)) != 0) {
    	    	if (!bootblock.BSector(i) || !bootblock.ESector(i)) {
		    invalid = 1;
    	    	    p.length = 0;
		    continue;
    	    	}

		int a;

		if ((a = bootblock.Active(i)) == 0x80) {
		    active = i;
    	    	}else if (a && a != 0x80 && p.type != MBS_BOOTANY) {
		    invalid = 1;
    	    	    p.length = 0;
		    continue;
    	    	}

		int h = fdisk.Heads();
		int t = fdisk.SecPerTrack();
		int c = fdisk.Cyls();
		diskgeo(bootblock.BTrack(i),
			bootblock.BCyl(i),
			bootblock.BSector(i),
		        bootblock.ETrack(i),
			bootblock.ECyl(i),
			bootblock.ESector(i),
			bootblock.Offset(i),
			bootblock.Length(i),
			h, t, c);

		if (fdisk.Heads(h) && fdisk.SecPerTrack(t) && fdisk.Cyls(c))
	    	    valid = 1;
		fdisk.SecPerCyl(fdisk.Heads() * fdisk.SecPerTrack());
    	    	any = 1;

		has_fdisk = 1;
		p.offset = bootblock.Offset(i);
		p.type = bootblock.Type(i);
		p.number = i;
		p.original = 1;

		if (BSDType(p.type) || (p.type == MBS_BOOTANY && BSDType(a))) {
		    if (!bsdpart || active == i || active != bsdpart)
			    bsdpart = i;
		    /*
		     * Hack the starting sector for BSDI to be 0 if it is
		     * set to 15.  Real ugly
		     */
		    if (p.offset == 15) {
			p.offset = 0;
			p.length += 15;
		    }
		}
    	    } else {
		/*
		 * Original, but empty.
		 */
		p.original = 1;
	    }
    	}
    	if (!invalid && !valid && any) {
	    if (O::Flag(O::Expert) && O::Cmd() == O::INTERACTIVE)
		print(disk_bad_fdisk_long);
	    else
    	    	valid = 1;
    	}

    	if (!any || invalid || (!valid && !request_yesno(disk_bad_fdisk, 1))) {
	    has_fdisk = 0;
    	    for (i = 0; i < 4; ++i)
		part[i].Zero();
    	    active = 0;
	    fdisk.Zero();
    	}
	if (fdisk.Sectors(fdisk.SecPerCyl() * fdisk.Cyls()))
	    Sort();
    } else
	has_fdisk = 0;

#ifdef	__i386__
    if (!O::Flag(O::IgnoreGeometry) && (sb.st_mode & S_IFMT) == S_IFCHR) {
	struct biosgeom b[4];
	int mib[] = { CTL_MACHDEP, CPU_BIOS, BIOS_DISKGEOM, };
	size_t len = sizeof(b);
    	int unit = dv_unit(sb.st_rdev);

	if (sysctl(mib, sizeof(mib)/sizeof(int), b, &len, NULL, 0) >= 0) {
	    if (len < sizeof(b))
		b[3].flags = 0;
	    if (len < sizeof(b) - sizeof(biosgeom))
		b[2].flags = 0;
	    if (len < sizeof(b) - sizeof(biosgeom) * 2)
	 	b[1].flags = 0;
	    if (len < sizeof(b) - sizeof(biosgeom) * 3)
		b[0].flags = 0;

    	    if ((d_type == DTYPE_ST506 || d_type == DTYPE_ESDI) && unit < 2) {
		if (b[unit].flags & BIOSGEOM_PRESENT) {
		    cmos.Zero();
		    cmos.Heads(b[unit].ntracks);
		    cmos.SecPerTrack(b[unit].nsectors);
		    cmos.Cyls(b[unit].ncylinders);
		    cmos.Sectors(cmos.Cyls() * cmos.SecPerCyl());
		}
    	    	if (b[unit+2].flags & BIOSGEOM_PRESENT) {
		    unit += 2;
		    bios.Zero();
		    bios.Heads(b[unit].ntracks);
		    bios.SecPerTrack(b[unit].nsectors);
		    bios.Cyls(b[unit].ncylinders);
		    bios.Sectors(bios.Cyls() * bios.SecPerCyl());
		}
// XXX - else confirm that using cmos geometry is okay
    	    } else if (d_type == DTYPE_SCSI || d_type == DTYPE_CPQRAID ||
	      d_type == DTYPE_AMIRAID || DTYPE_AACRAID) {
    	    	unit += 2;
    	    	if (b[0].flags & BIOSGEOM_PRESENT)
	    	    ++unit;
    	    	if (b[1].flags & BIOSGEOM_PRESENT)
	    	    ++unit;
		if (unit < 4 && (b[unit].flags & BIOSGEOM_PRESENT)) {
		    bios.Zero();
		    bios.Heads(b[unit].ntracks);
		    bios.SecPerTrack(b[unit].nsectors);
		    bios.Cyls(b[unit].ncylinders);
		    bios.Sectors(bios.Cyls() * bios.SecPerCyl());
		}
    	    }
	}
	if (bios.Sectors() > Sectors())
		Sectors(bios.Sectors());
	if (cmos.Sectors() > Sectors())
		Sectors(cmos.Sectors());
    }
#endif

    label_original.Clean();

    if (!O::Flag(O::IgnoreBSD) && !O::Flag(O::InCoreOnly)
			       && !O::Flag(O::ReadKernel)
			       && (bsdpart || part[0].length == 0)) {

	bsdoff = 0;
	if (bsdpart) {
	    for (e = 0; e < 4 && part[e].number; ++e) {
		if (part[e].number == bsdpart) {
		    bsdoff = part[e].offset;
		    break;
		}
	    }
	}
#else
    {
#endif
#ifdef	sparc
    	if (label_sun.Sun(dfd) == 0) {
	    secsize = label_sun.d_secsize;
	    bsdg.Zero();
	    bsdg.SecPerTrack(label_sun.d_nsectors);
	    bsdg.Heads(label_sun.d_ntracks);
	    bsdg.Cyls(label_sun.d_ncylinders);
	    bsdg.SecPerCyl(label_sun.d_secpercyl);
	    bsdg.Sectors(label_sun.d_secperunit);
	    if (bsdg.Sectors() > Sectors())
		Sectors(bsdg.Sectors());
	}
#endif

	if ((e = label_original.Read(dfd, bsdoff * 512, &bsdoff)) != 0)
#ifdef	sparc
	    if (label_sun.Valid())
		label_template = label_sun;
	    else
#endif
	    	goto nobsd;
    	else
	    label_template = label_original;
		

	secsize = label_template.d_secsize;
    	bsdg.Zero();
	bsdg.SecPerTrack(label_template.d_nsectors);
	bsdg.Heads(label_template.d_ntracks);
	bsdg.Cyls(label_template.d_ncylinders);
	bsdg.SecPerCyl(label_template.d_secpercyl);
	bsdg.Sectors(label_template.d_secperunit);
	if (bsdg.Sectors() > Sectors())
	    Sectors(bsdg.Sectors());

	return(0);
    }
nobsd:
    bsdoff = -1;

    if (!O::Flag(O::IgnoreBSD) && !O::Flag(O::ReadDisk))
	label_original.Internal(dfd);

    if (!label_original.Valid())
	return(0);

#ifndef sparc
    /*
     * On a soft label we should only count on the secperunit field.
     */
    if (label_original.Soft()) {
	bsdg.Sectors(label_original.d_secperunit);
	label_template = label_original;
	return(0);
    }
#endif

    secsize = label_original.d_secsize;
    bsdg.Zero();
    bsdg.SecPerTrack(label_original.d_nsectors);
    bsdg.Heads(label_original.d_ntracks);
    bsdg.Cyls(label_original.d_ncylinders);
    bsdg.SecPerCyl(label_original.d_secpercyl);
    bsdg.Sectors(label_original.d_secperunit);
    if (bsdg.Sectors() > Sectors())
	Sectors(bsdg.Sectors());

    label_template = label_original;
    return(0);
}

int
Disk::Type()
{
    static int sd_dev = 0;
    static int sr_dev = 0;
    static int wd_dev = 0;
    static int fd_dev = 0;
    static int sp_dev = 0;
    static int vnd_dev = 0;
    static int cr_dev = 0;
    static int amir_dev = 0;
    static int aacr_dev = 0;
    struct stat sb;

    if (!d_type) {
	if (!sd_dev && !wd_dev && !fd_dev) {
	    if (stat("/dev/rwd0c", &sb) == 0)
		wd_dev = major(sb.st_rdev);
	    else
		wd_dev = -1;
	    if (stat("/dev/rsd0c", &sb) == 0)
		sd_dev = major(sb.st_rdev);
	    else
		sd_dev = -1;
	    if (stat("/dev/rsr0c", &sb) == 0)
		sr_dev = major(sb.st_rdev);
	    else
		sr_dev = -1;
	    if (stat("/dev/rfd0c", &sb) == 0)
		fd_dev = major(sb.st_rdev);
	    else
		fd_dev = -1;
	    if (stat("/dev/rsp0c", &sb) == 0)
		sp_dev = major(sb.st_rdev);
	    else
		sp_dev = -1;
	    if (stat("/dev/rcr0c", &sb) == 0)
		cr_dev = major(sb.st_rdev);
	    else
		cr_dev = -1;
	    if (stat("/dev/rvnd0c", &sb) == 0)
		vnd_dev = major(sb.st_rdev);
	    else
		vnd_dev = -1;
	    if (stat("/dev/ramir0c", &sb) == 0)
		amir_dev = major(sb.st_rdev);
	    else
		amir_dev = -1;
	    if (stat("/dev/raacr0c", &sb) == 0)
		aacr_dev = major(sb.st_rdev);
	    else
		aacr_dev = -1;
	}

	if (fstat(dfd, &sb) == 0 && (sb.st_mode & S_IFMT) == S_IFCHR) {
    	    if (major(sb.st_rdev) == sd_dev)
		d_type = DTYPE_SCSI;
    	    else if (major(sb.st_rdev) == sr_dev)
		d_type = DTYPE_SCSI;
    	    else if (major(sb.st_rdev) == wd_dev)
		d_type = DTYPE_ST506;
    	    else if (major(sb.st_rdev) == fd_dev)
		d_type = DTYPE_FLOPPY;
    	    else if (major(sb.st_rdev) == sp_dev)
		d_type = DTYPE_PSEUDO;
    	    else if (major(sb.st_rdev) == vnd_dev)
		d_type = DTYPE_PSEUDO;
    	    else if (major(sb.st_rdev) == cr_dev)
		d_type = DTYPE_CPQRAID;
    	    else if (major(sb.st_rdev) == amir_dev)
		d_type = DTYPE_AMIRAID;
	    else if (major(sb.st_rdev) == aacr_dev)
		d_type = DTYPE_AACRAID;
	}
    }
    return(d_type);
}

void
Disk::Sort()
{
    int i;

    for (i = 0; i < 7; ++i) {
	for (int j = i + 1; j < 8; ++j) {
	    if (!bsd[i].number || bsd[j].number == 3
			       || bsd[i].offset > bsd[j].offset) {
		FileSystem p = bsd[i];
    	    	bsd[i] = bsd[j];
    	    	bsd[j] = p;
    	    	if (bsd[j].number)
		    --j;
	    }
	}
    }

#ifdef	NEED_FDISK
    for (i = 0; i < 3; ++i) {
	for (int j = i + 1; j < 4; ++j) {
	    if (!part[i].number || part[i].offset > part[j].offset) {
		Partition p = part[i];
    	    	part[i] = part[j];
    	    	part[j] = p;
    	    	if (part[j].number)
		    --j;
	    }
	}
    }
 
    for (i = 0; i < 8 && bsd[i].number; ++i)
	bsd[i].part = FindPartForBSD(i);
#endif
}

#ifdef	NEED_FDISK
int
Disk::FindFPart(int p)
{
    for (int i = 0; i < 4 && part[i].number; ++i)
	if (part[i].number == p)
	    return(i);
    return(-1);
}
#endif

int
Disk::FindFSys(int p)
{
    for (int i = 0; i < 8 && bsd[i].number; ++i)
	if (bsd[i].number == p)
	    return(i);
    return(-1);
}

#ifdef	NEED_FDISK
int
PickFPart(int mask, char *msg, ...)
{
    int c = 0;
    int i;

    char buf[160];

    va_list ap;
    va_start(ap, msg);
    vsprintf(buf, msg, ap);
    va_end(ap);

    move(15, 0);
    clrtobot();
    mvprint(17, 4, "Press <ESC> to cancel");
    mvprint(16, 4, buf);
    print(" [");

    for (i = 1; i <= 4; ++i) {
    	if ((1 << i) & mask) {
	    if (c++)
		print(",");
	    print("%c", i + '0');
    	}
    }
    print("] ");
    refresh();

    while ((c = readc()) != 0) {
    	move(18, 0);
	clrtobot();
    	switch(c) {
	case '\033':
	    return(0);
    	case '1': case '2': case '3': case '4':
	    if ((1 << (c - '0')) & mask)
	    	return(c - '0');
	    mvprint(19, 4, "Invalid partition number");
    	    break;
    	default:
	    break;
    	}
    }
    return(0);
}
#endif

int
PickFSys(int mask, char *msg, ...)
{
    int c = 0;
    int i;

    char buf[160];
    
    va_list ap;
    va_start(ap, msg);
    vsprintf(buf, msg, ap);
    va_end(ap);

    move(15, 0);
    clrtobot();
    mvprint(17, 4, "Press <ESC> to cancel");
    mvprint(16, 4, buf);
    print(" [");

    for (i = 1; i <= 8; ++i) {
    	if ((1 << i) & mask) {
	    if (c++)
		print(",");
	    print("%c", i + 'a' - 1);
    	}
    }
    print("] ");
    refresh();

    while ((c = readc()) != 0) {
    	switch(c) {
	case '\033':
	    return(0);
    	case 'A': case 'B': case 'C': case 'D':
    	case 'E': case 'F': case 'G': case 'H':
	    c |= 040;
    	case 'a': case 'b': case 'c': case 'd':
    	case 'e': case 'f': case 'g': case 'h':
	    if ((1 << (c - 'a' + 1)) & mask)
	    	return(c - 'a' + 1);
	    mvprint(19, 4, "Invalid filesystem");
    	    break;
    	default:
	    break;
    	}
    }
    return(0);
}

#define	GOFF	40

#ifdef	NEED_FDISK
int
Disk::FDisk()
{
    startvisual();
    part_modified = 0;

    help_info hi(&fdisk_help[0]);

    Partition savep[4];
    savep[0] = part[0];
    savep[1] = part[1];
    savep[2] = part[2];
    savep[3] = part[3];
top:
    clear();

    mvprint(0, 4, "FDISK Partitioning                    ");
    mvprint(2, 4, "<?> for help");
    Home::Set(2, 5);

    mvprint(0, GOFF, "%8s Device to Partition", device);
    mvprint(1, GOFF, "%8d Sectors/Track", SecPerTrack());
    mvprint(2, GOFF, "%8d Sectors/Cylinder", SecPerCyl());
    mvprint(3, GOFF, "%8d Heads", Heads());
    mvprint(4, GOFF, "%8d Cylinders", Cyls());

    mvprint(6, 0, 
"   |    Type of    |     Length of Partition in     | Starting  |   Ending  |");
    mvprint(7, 0, 
"P# |    Partition  |   Sectors (   MBytes      Cyls)|    Sector |    Sector |Gap");
    mvprint(8, 0, 
"---|---------------|--------------------------------|-----------|-----------|---");

    quad_t gap;
    int i;

    for (i = 0; i < 4 && part[i].number; ++i) {
	gap = part[i].Start();

	if (i > 0)
	    gap -= part[i-1].End();
	part[i].Print(9 + i, gap);
    }

    if (i > 0) {
	gap = UseSectors() - part[i-1].End();
    } else {
	gap = UseSectors();
    }
    while (i < 4) {
	mvprint(9+i, 0, 
"   |               |                                |           |           |");
	++i;
    }

    mvprint(13, 0, "   |  Size of Disk |%10qd (%9.1f %9.1f)|         0 |%10qd | ",
	    UseSectors() ,
	    ToMB(UseSectors()),
	    ToCyl(UseSectors()),
	    UseSectors() - 1);
    if (gap)
	print("*", gap);
    mvprint(14, 0, 
"---|---------------|--------------------------------|-----------|-----------|---");


    int y;

    y = 15;

    FDraw();

    move(y, 0);
    clrtobot();

    for (i = 0; fdisk_qhelp[i]; ++i)
	mvprint(++y, 4, "%s", fdisk_qhelp[i]);

    Home::Go();

    refresh();

    char c;

    while ((c = readc()) != 0) {
	int mask = 0;

    	switch (c) {
    	case 'x': case 'X':
	    mvprint(0, 4, "FDISK Partitioning - Abort Screen      ");
	    if (verify(fdisk_loose_changes)) {
		part[0] = savep[0];
		part[1] = savep[1];
		part[2] = savep[2];
		part[3] = savep[3];
		endvisual();
		return(-1);
	    }
	    break;
    	case 'n': case 'N':
	    mvprint(0, 4, "FDISK Partitioning - Next Screen       ");
	    if (!active) {
	    	inform(fdisk_need_active_partition);
		break;
    	    }
	    if (!part_modified || verify(fdisk_save_changes)) {
		endvisual();
		return(1);
    	    }
	    break;
    	case 'd': case 'D': {
	    help_info hi(&fdisk_delete_help[0]);
	    mvprint(0, 4, "FDISK Partitioning - Delete Partition  ");

	    for (i = 0; i < 4 && part[i].number; ++i)
    	    	mask |= 1 << part[i].number;

    	    if (!mask) {
	    	inform(fdisk_cant_delete);
	    	break;
    	    }
    	    c = PickFPart(mask, fdisk_delete_which[0]);

    	    if (c == 0)
	    	goto top;

    	    if ((i = FindFPart(c)) >= 0) {
		if (verify("Really delete partition %d (%s)?%s",
			    part[i].number,
			    part[i].TypeName(),
			    part[i].original ?
		    "\nDoing so will cause any information currently\n"
		      "contained in that partition to be lost." : "")) {
		    part_modified |= part[i].original;
    	    	    part[i].Clean();
		    Sort();
		}
	    }
	    goto top;
    	  }
    	case '*': {
	    help_info hi(&fdisk_active_help[0]);
	    mvprint(0, 4, "FDISK Partitioning - Assign Active Part");

	    for (i = 0; i < 4 && part[i].number; ++i)
    	    	mask |= 1 << part[i].number;

    	    c = PickFPart(mask, fdisk_active_which[0]);

    	    if (c == 0)
	    	goto top;

    	    if ((i = FindFPart(c)) >= 0)
		active = part[i].number;

	    goto top;
    	  }
    	case 'e': case 'E': {
	    help_info hi(&fdisk_edit_help[0]);
	    mvprint(0, 4, "FDISK Partitioning - Edit Partition    ");

	    for (i = 0; i < 4 && part[i].number; ++i)
    	    	mask |= 1 << part[i].number;

    	    c = PickFPart(mask, fdisk_edit_which[0]);

    	    if (c == 0)
	    	goto top;

    	    if ((i = FindFPart(c)) >= 0) {
		Partition p = part[i];
		if (EditPart(i) && part[i].original && part[i].Changed(p)) {
		    if (verify(fdisk_really_edit)) {
			part_modified |= part[i].original;
			part[i].original = 0;
		    } else
			part[i] = p;
		}
	    }
	    goto top;
    	  }
	case 'a': case 'A': {

    	    for (i = 0; i < 4 && part[i].number; ++i)
		;
    	    if (i >= 4) {
		inform(fdisk_cant_add);
	    	break;
	    }

	    mvprint(0, 4, "FDISK Partitioning - Add Partition   ");

	    for (part[i].number = 1; part[i].number < 4; part[i].number++) {
	    	int j;
	    	for (j = 0; j < i; ++j)
		    if (part[j].number == part[i].number)
			break;
    	    	if (j >= i)
		    break;
	    }
    	    part[i].type = 0;
    	    if (i > 0) {
		int lp = 0;
		gap = part[0].Start();

		int j;
		for (j = 1; j < i; ++j) {
		    if (gap < part[j].Start() - part[j-1].End()) {
			gap = part[j].Start() - part[j-1].End();
			lp = j;
		    }
		}
    	    	if (UseSectors() - part[i - 1].End() > gap) {
		    part[i].offset = part[i-1].End();
		    part[i].length = UseSectors() - part[i].Start();
		} else if (lp == 0) {
		    part[i].offset = 0;
		    part[i].length = part[0].Start();
    	    	} else {
		    part[i].offset = part[lp-1].End();
		    part[i].length = part[lp].Start() - part[i].Start();
    	    	}
    	    } else {
		part[i].offset = 0;
		part[i].length = UseSectors();
    	    }
    	    int n = part[i].number;
	    Sort();
    	    for (i = 0; i < 4; ++i)
		if (part[i].number == n)
		    break;
	    if (!EditPart(i))
		part[i].Zero();
	    else if (active == 0)
		active = part[i].number;
	    Sort();
    	    goto top;
    	  }
    	}
    }
    endvisual();
    fprintf(stderr, "This point cannot be reached. (%s::%d)\n"
		    "Please save the disksetup.core image\n"
		    "Please contact support@bsdi.com\n", __FILE__, __LINE__);
    abort();
}

int
Disk::FindPartForBSD(int i)
{
    int p;

    if (part[0].number == 0)
	return(0);

    for (p = 0; p < 4 && part[p].number; ++p)
	if (bsd[i].Start() >= part[p].Start() && bsd[i].End() <= part[p].End())
	    return(p);
    return(-1);
}

int
Disk::FindPartForBSD(FileSystem &fs)
{
    int p;

    if (part[0].number == 0)
	return(0);

    for (p = 0; p < 4 && part[p].number; ++p)
	if (fs.Start() >= part[p].Start() && fs.End() <= part[p].End())
	    return(p);
    return(-1);
}
#endif

int
Disk::FSys()
{
    startvisual();
    bsd_modified = 0;

    help_info hi(&bsd_help[0]);
    Sort();

    FileSystem savep[8];
    savep[0] = bsd[0];
    savep[1] = bsd[1];
    savep[2] = bsd[2];
    savep[3] = bsd[3];
    savep[4] = bsd[4];
    savep[5] = bsd[5];
    savep[6] = bsd[6];
    savep[7] = bsd[7];
top:
    clear();

    mvprint(0, 4, "BSD Partitioning                      ");
    mvprint(1, 4, "<?> for help");
    Home::Set(1, 5);

    mvprint(0, GOFF, "%8s Device to Partition", device);

    mvprint(3, 0, 
"  FS    Mount      |     Length of File System in   |  Starting |    Ending |");
    mvprint(4, 0, 
"  Type  Point      |    Sectors (   MBytes     Cyls)|    Sector |    Sector |Gap");
    mvprint(5, 0, 
"-------------------|--------------------------------|-----------|-----------|---");

    int gap;
    int i;

    for (i = 0; i < 8; ++i) {
	mvprint(6+i, 0, 
"%c %-5.5s            |                                |           |           |", i + 'a', "-----");
    }

    for (i = 0; i < 8 && bsd[i].number; ++i) {
#ifdef	NEED_FDISK
    	if (bsd[i].part >= 0 && BSDType(part[bsd[i].part].type)) {
	    gap = bsd[i].Start();
	    if (i > 1 && bsd[i-1].part >= 0 && bsd[i-1].part == bsd[i].part)
		gap -= bsd[i-1].End();
    	    else
		gap -= part[bsd[i].part].Start();
    	} else
	    gap = 0;
#else
	gap = bsd[i].Start();
	if (i > 1)
	    gap -= bsd[i-1].End();
#endif

	bsd[i].Print(6 + bsd[i].number - 1, gap);
    }

    if (i > 1) {
	gap = UseSectors() - bsd[i-1].End();
    } else {
	gap = UseSectors();
    }
    bsd[0].Print(6 + bsd[0].number - 1, gap);
    mvprint(14, 0, 
"-------------------|--------------------------------|-----------|-----------|---");


    int y;
    int mask = 0;

    y = 15;

    BDraw();

    move(y, 0);
    clrtobot();

    for (i = 0; bsd_qhelp[i]; ++i)
	mvprint(++y, 4, "%s", bsd_qhelp[i]);

    Home::Go();

    refresh();

    char c;

    while ((c = readc()) != 0) {
    	switch (c) {
	case 'x': case 'X':
	    mvprint(0, 4, "BSD Partitioning - Abort Screen      ");
	    if (verify(bsd_loose_changes)) {
		for (c = 0; c < 8; ++c)
		    bsd[c] = savep[c];
		endvisual();
		return(-1);
	    }
	    break;
	case 'e': case 'E': {
	    help_info hi(&bsd_edit_help[0]);
	    mvprint(0, 4, "BSD Partitioning - Edit File System    ");

	    for (i = 0; i < 8 && bsd[i].number; ++i) {
		if (!bsd[i].fixed)
		    mask |= 1 << bsd[i].number;
    	    }

    	    c = PickFSys(mask, bsd_which_edit[0]);

    	    if (c == 0)
	    	goto top;

    	    if ((i = FindFSys(c)) >= 0) {
		FileSystem p = bsd[i];
		if (EditBSDPart(i) && bsd[i].original && bsd[i].Changed(p)) {
		    if (verify(fdisk_really_edit)) {
			bsd_modified |= bsd[i].original;
			bsd[i].original = 0;
		    } else
			bsd[i] = p;
		}
	    }
	    goto top;
    	  }
#ifdef	NEED_FDISK
    	case 'i': case 'I': {
    	    int bmask = 0xff << 1;

    	    quad_t f;

	    for (i = 0; i < 4 && part[i].number; ++i)
		if (FindFreeSpace(i, f, f))
		    mask |= 1 << part[i].number;

    	    if (!mask) {
	    	inform(bsd_cant_import);
    	    	break;
    	    }

	    for (i = 0; i < 8 && bsd[i].number; ++i)
		bmask &= ~(1 << bsd[i].number);

    	    if (!bmask) {
	    	inform(bsd_wont_import);
    	    	break;
    	    }

	    help_info hi(&bsd_import_help[0]);
	    mvprint(0, 4, "BSD Partitioning - Import Partition    ");

    	    c = PickFPart(mask, bsd_import_which[0]);

    	    if (c == 0)
	    	goto top;

    	    c = FindFPart(c);

    	    if (bmask & (1 << 4))
	    	y = 4;
    	    else if (bmask & (1 << 5))
	    	y = 5;
    	    else if (bmask & (1 << 6))
	    	y = 6;
    	    else if (bmask & (1 << 7))
	    	y = 7;
    	    else if (bmask & (1 << 8))
	    	y = 8;
    	    else if (bmask & (1 << 1))
	    	y = 1;
    	    else if (bmask & (1 << 2))
	    	y = 2;
    	    else {
		// Can't get here?
	    	inform(bsd_wont_import);
    	    	break;
    	    }

    	    if (y == 0)
	    	goto top;

    	    for (i = 0; i < 8 && bsd[i].number; ++i)
		;

	    bsd[i].number = y;
    	    FindFreeSpace(c, bsd[i].offset, bsd[i].length);
	    bsd[i].fixed = 0;
	    bsd[i].original = 0;
	    bsd[i].type = PartToBSDType(part[c].type);
    	    bsd[i].mount[0] = 0;
	    EditBSDPart(i);
	    Sort();
	    goto top;
    	  }
#endif
    	case 'a': case 'A': {
    	    i = AddPartition();
    	    if (i == 0)
		goto top;
	    if (i < 0)
	    	break;
	    if (!EditBSDPart(i))
	    	bsd[i].Zero();
	    Sort();
	    goto top;
    	  }
	case 'd': case 'D': {
    	    for (i = 0; i < 8 && bsd[i].number; ++i)
    	    	if (!bsd[i].fixed)
		    mask |= 1 << bsd[i].number;

    	    help_info hi(&bsd_delete_help[0]);
    	    if (!mask) {
		inform(bsd_cant_delete);
	    	break;
    	    }
    	    c = PickFSys(mask, bsd_delete_which[0]);

    	    if (c) {
		c = FindFSys(c);
		if (verify("Really delete filesystem %c (%s)?%s",
			    bsd[c].number + 'a' - 1, 
			    bsd[c].mount,
			    bsd[c].original ?
		    "\nDoing so will cause any information currently\n"
		      "contained in that filesystem to be lost." : "")) {
		    bsd_modified |= bsd[c].original;
    	    	    bsd[c].Zero();
		    Sort();
		}
    	    }
	    goto top;
    	  }
	case 'n': case 'N':
	    mvprint(0, 4, "BSD Partitioning - Next Screen       ");
	    if (!bsd_modified || verify(bsd_save_changes)) {
		endvisual();
		return(1);
    	    }
	    break;
	}
    }
    endvisual();
    fprintf(stderr, "This point cannot be reached. (%s::%d)\n"
		    "Please save the disksetup.core image\n"
		    "Please contact support@bsdi.com\n", __FILE__, __LINE__);
    abort();
    return(0);
}

int
Disk::AddPartition(int use)
{
    int bmask = 0xff << 1;
    int bpart = 0;
    int mask = 0;

    quad_t y;
    quad_t f;
    int i;

    if (use < 0 || use > 8)
	return(ECANTADD);

#ifdef	NEED_FDISK
    for (i = 0; i < 4 && part[i].number; ++i) {
	if (BSDType(part[i].type) && FindFreeSpace(i, f, f)) {
	    mask |= 1 << part[i].number;
	    if (bpart)
		bpart = -1;
	    else
		bpart = i;
	}
    }

    if (!mask && part[0].number) {
    	if (!use)
	    inform(bsd_cant_add);
    	return(ECANTADD);
    }
#endif

    for (i = 0; i < 8 && bsd[i].number; ++i)
	bmask &= ~(1 << bsd[i].number);

    if (use) {
	if (!(bmask &= (1 << use)))
	    return(EALREADYUSED);
    }

    if (!bmask) {
    	if (!use)
	    inform(bsd_wont_add);
    	return(EWONTADD);
    }

    if (!use)
	mvprint(0, 4, "BSD Partitioning - Add Partition    ");

#ifdef	NEED_FDISK
    if (bpart < 0) {
    	if (!use) {
	    bpart = PickFPart(mask, bsd_fdisk_add[0]);
	    if (bpart == 0)
    	    	return(0);
    	} else {
	    //
    	    // If we cannot ask the user which partition, then
	    // either use the active partition or the first one
	    // found.
    	    //
    	    if (mask & (1 << active))
		bpart = active;
	    else {
    	    	bpart = 1;
    	    	while (!(mask && (1 << bpart)))
		    ++bpart;
    	    }
    	}
	bpart = FindFPart(bpart);
    }
#endif

    if (bmask & (1 << 1))
	y = 1;
    else if (bmask & (1 << 2))
	y = 2;
    else if (bmask & (1 << 8))
	y = 8;
    else if (bmask & (1 << 7))
	y = 7;
    else if (bmask & (1 << 6))
	y = 6;
    else if (bmask & (1 << 5))
	y = 5;
    else if (bmask & (1 << 4))
	y = 4;
    else {
    	if (!use)
	    inform(bsd_wont_add);
    	return(EWONTADD);
    }

    if (y == 0)
    	return(0);

    for (i = 0; i < 8 && bsd[i].number; ++i)
	;

    bsd[i].number = y;
    FindFreeSpace(bpart, bsd[i].offset, bsd[i].length);
    bsd[i].fixed = 0;
    bsd[i].original = 0;
    bsd[i].type = FS_BSDFFS;

    f = bsd[i].length;
    switch(y) {
    case 1:
	y = DefaultRoot(bpart);
	if (bsd[i].length > y)
	    bsd[i].length = y;
	strcpy(bsd[i].mount, "/");
	break;
    case 2:
	y = DefaultSwap(bpart);
	if (bsd[i].length > y)
	    bsd[i].length = y;
	bsd[i].type = FS_SWAP;
	strcpy(bsd[i].mount, "swap");
	break;
    case 8:
	strcpy(bsd[i].mount, "/usr");
	break;
    case 7:
	strcpy(bsd[i].mount, "/var");
	break;
    default:
	bsd[i].mount[0] = 0;
    }
    bsd[i].ForceCyl(f);
    return(i);
}

#define PL      15

void
display_length(quad_t value, quad_t off)
{
    mvprint(PL, 32, "%9.1f", disk.ToMB(value));
    mvprint(PL, 42, "%9.1f", disk.ToCyl(value));
    mvprint(PL, 65, "%10qd", off + value - 1);
}

void
display_offset(quad_t value, quad_t len)
{
    mvprint(PL, 65, "%10qd", len + value - 1);
}

void
display_end(quad_t value, quad_t off)
{
	value -= off - 1;

	mvprint(PL, 20, "%10qd", value);
	mvprint(PL, 32, "%9.1f", disk.ToMB(value));
	mvprint(PL, 42, "%9.1f", disk.ToCyl(value));
}

#ifdef	NEED_FDISK
Field PartFields[] = {
    { PL, 8, 10, fdisk_type_help, fdisk_type_qhelp, },
    { PL, 20, 10, fdisk_length_help, fdisk_length_qhelp, display_length, },
    { PL, 53, 10, fdisk_offset_help, fdisk_offset_qhelp, display_offset, },
    { PL, 65, 10, fdisk_offset_help, fdisk_offset_qhelp, display_end, },
    { 0, }
};  
 
int
validate_type(char *s)
{           
    if (!PFindType(s)) {
        inform("You have entered an invalid partition type (%s).\n"
               "You may either type the symbolic name (e.g. BSDI, DOS, ...)\n"
               "or you may type the hexadecimal value of the\n"
               "partition type directly.", s);
        return(0);
    }   
    return(1);
}

int
Disk::EditPart(int i)
{
    move(PL, 0);
    clrtobot();

    Partition p = part[i];

    int r = -1;
    quad_t oldoff = -1;
    quad_t oldlen = -1;
    quad_t endsec;

    if (0) {
top:
	r = 1;
    }

    for (;;) {
	PType pt = PTypeFind(p.type);

	if (r != -1) {
	    p.length -= pt.start(p.offset) - p.offset;
	    p.offset = pt.start(p.offset);
	} else
	    r = 1;

	if (r != 1)
	    break;

	move(PL, 0);
	mvprint(PL,  0, "%1d",	 p.number);
	mvprint(PL,  1, "%c",	 p.number == active ? '*' : ' ');
	mvprint(PL,  3, "|");
	mvprint(PL,  5, "%02X",	 p.type);
	mvprint(PL,  8, "%-10.10s",	 p.TypeName());
	mvprint(PL, 19, "|");
	mvprint(PL, 20, "%10qd", p.length);
	mvprint(PL, 31, "(");
	mvprint(PL, 32, "%9.1f", ToMB(p.length));
	mvprint(PL, 42, "%9.1f", ToCyl(p.length));
	mvprint(PL, 51, ")|");
	mvprint(PL, 53, "%10qd", p.offset);
	mvprint(PL, 64, "|");
	mvprint(PL, 65, "%10qd", p.offset + p.length - 1);
	mvprint(PL, 76, "|");

	mvprint(20, 4, "  <TAB> - change fields to edit");
	mvprint(21, 4, "  <ESC> - cancel the edit");
	mvprint(22, 4, "<ENTER> - accept the changes");
	mvprint(23, 4, "      ? - display help");
	refresh();

	quad_t egap = 0;

    	if (p.type) {
	    if (i < 3 && part[i+1].number)
		egap = part[i+1].offset - p.offset;
	    else
		egap = UseSectors() - p.offset;

	    oldlen = p.length;
	    oldoff = -1;
	    r = PartFields[1].NumericEdit(p.length, egap, force_sector,
				          p.offset % SecPerCyl(), p.offset);
	    p.AdjustType();
	    mvprint(PL,  5, "%02X",	 p.type);
	    mvprint(PL,  8, "%-10.10s",	 p.TypeName());
	    if (r != 1) {
		break;
	    }

	    mvprint(PL, 32, "%9.1f",	 ToMB(p.length));
	    mvprint(PL, 42, "%9.1f",	 ToCyl(p.length));

	    if (i > 0)
		egap = pt.start(part[i-1].offset + part[i-1].length);
	    else
		egap = pt.start(pt.minsector);

    	    for (;;) {
		oldlen = -1;
		oldoff = p.offset;
		r = PartFields[2].NumericEdit(p.offset, egap, pt.start, 0, p.length);
		if (r < 1)
		    break;
		if (p.offset < pt.start(pt.minsector))
		    inform("Partitions of this type must start on at least sector %qd\n", pt.start(pt.minsector));
		else if (disk.ToCyl(p.offset) > 1023)
		    inform("All partitions in an fdisk table must start within the first 1024 cylinders.");
    	    	else
		    break;
	    }
    	    if (r != 1)
		break;

	    oldlen = p.length;
	    oldoff = -1;
	    endsec = p.End() - 1;

	    if (i < 4 && part[i+1].number) {
		egap = part[i+1].offset - 1;
	    } else {
		egap = UseSectors() - 1;
	    }

	    r = PartFields[3].NumericEdit(endsec, egap, force_sector, p.offset % SecPerCyl(), p.offset);
	    if (endsec != p.End() - 1)
		p.length = endsec + 1 - p.offset;
    	}

	char type[11];

    	strncpy(type, pt.name, 11);

    	r = PartFields[0].TextEdit(type, validate_type);
    	if (r && p.type == 0) {
	    PType &ptt = PTypeFind(PFindType(type));
	    if (p.offset < ptt.minsector) {
	    	p.length -= ptt.minsector - p.offset;
	    	p.offset = ptt.minsector;
    	    }
    	}
	p.type = PFindType(type);
    	p.AdjustType();
	if (r && !p.type) {
	    inform("You must set the partition type before continuing (%s).\n"
		   "You may either type the symbolic name (e.g. BSDI, DOS, ...)\n"
		   "or you may type the hexadecimal value of the\n"
	    	   "partition type directly.", type);
	    
	    r = 1;
	}
    }

    if (r == 0) {
	p = part[i];
	return(0);
    } else {
    	if (p.length < 1) {
	    inform(fdisk_negative_length);
    	    goto top;
    	}
    	if (p.offset < 0) {
	    inform(fdisk_negative_offset);
    	    goto top;
    	}
    	if (p.offset + p.length > UseSectors()) {
	    inform(fdisk_too_long);
    	    if (oldoff >= 0)
		p.offset = UseSectors() - p.length;
    	    else if (oldlen >= 0)
		p.length = UseSectors() - p.offset;
    	    goto top;
    	}
	if (disk.ToCyl(p.offset + p.length) > 1023) {
	    if (p.IsDosType()) {
		inform("DOS Partitions must fit within the first\n"
		       "1024 cylinders of the disk.");
		goto top;
	    } else if (!p.IsBSDType()) {
		inform("Warning: This is not a BSD partition and it\n"
		       "extends beyond the first 1024 cylinders of the disk.\n"
		       "This may cause a problem with some operating systmes.\n");
    	    }
    	}

	for (r = 0; r < 4 && part[r].number; ++r) {
	    Partition &rp = part[r];
	    if (i != r) {
		if (p.End() >= rp.End() && p.Start() <= rp.Start()) {
		    inform("This partition now fully overlaps partition %d.\n"
			   "You should first delete partition %d.\n",
			   rp.number, rp.number);
		    if (!O::Flag(O::Expert))
			goto top;
    	    	}
		if (rp.End() >= p.End() && rp.Start() <= p.Start()) {
		    inform("This partition is now fully overlapped by partition %d.\n"
			   "You should first delete partition %d.\n",
			   rp.number, rp.number);
		    if (!O::Flag(O::Expert))
			goto top;
    	    	}
		if (p.End() >= rp.End() && p.Start() < rp.End()) {
		    if (!verify("This partition overlaps partition %d.\n"
			        "Would you like partition %d to be shrunk\n"
			   	"to allow this partition to fit?",
			   	rp.number, rp.number)) {
			if (!O::Flag(O::Expert))
			    goto top;
			goto out;
		    }
		    if (rp.original && !verify(
				    "Any data in partitin %d will be lost.\n"
				    "Shrink partition %d anyways?",
				    rp.number, rp.number)) {
			if (!O::Flag(O::Expert))
			    goto top;
			goto out;
		    }
		    rp.original = 0;
		    rp.length = p.Start() - rp.Start();
		}
		if (p.Start() <= rp.Start() && p.End() > rp.Start()) {
		    if (!verify("This partition overlaps partition %d.\n"
			        "Would you like partition %d to be shrunk\n"
			   	"to allow this partition to fit?",
			   	rp.number, rp.number)) {
			if (!O::Flag(O::Expert))
			    goto top;
			goto out;
		    }
		    if (rp.original && !verify(
				    "Any data in partitin %d will be lost.\n"
				    "Shrink partition %d anyways?",
				    rp.number, rp.number)) {
			if (!O::Flag(O::Expert))
			    goto top;
			goto out;
		    }
		    rp.original = 0;
		    rp.length = rp.End() - p.End();
		    rp.offset = p.End();
		}
out:		    ;
	    }
	}
	part[i] = p;
    	return(1);
    }
}
#endif

Field BSDFields[] = {
    {   PL, 2, 5,  bsd_type_help, bsd_type_qhelp, },
    {   PL, 8, 67, bsd_mount_help, bsd_mount_qhelp, },
    {   PL, 20, 10, bsd_length_help, bsd_length_qhelp, display_length, },
    {   PL, 53, 10, bsd_offset_help, bsd_offset_qhelp, display_offset, },
    {   PL, 0, 1,  bsd_partition_help, bsd_partition_qhelp, },
    {   PL, 65, 10,  bsd_end_help, bsd_end_qhelp, display_end, },
    { 0, }
};  

int
Disk::EditBSDPart(int i)
{
    quad_t oldoff = -1;
    quad_t oldlen = -1;
    quad_t endsec;
    move(PL, 0);
    clrtobot();

    FileSystem p = bsd[i];

    int r = -1;

    if (0) {
top:
    	r = 1;
    }

    for (;;) {
	PType pt = FTypeFind(p.type);

	if (r != -1) {
	    p.length -= pt.start(p.offset) - p.offset;
	    p.offset = pt.start(p.offset);
	} else
	    r = 1;

	if (r != 1)
	    break;

	mvprint(PL, 0, "%c %-5.5s %-10.10s |%10qd (%9.1f %9.1f)|%10qd |%10qd | ", 
	    p.number + 'a' - 1, 
	    FTypeFind(p.type).name,
	    p.mount,
	    p.length,
	    ToMB(p.length),      
	    ToCyl(p.length),     
	    p.offset,
	    p.offset + p.length - 1);

	mvprint(20, 4, "  <TAB> - change fields to edit");
	mvprint(21, 4, "  <ESC> - cancel the edit");
	mvprint(22, 4, "<ENTER> - accept the changes");
	mvprint(23, 4, "      ? - display help");
	refresh();

	int egap = 0;

    	if (i < 8 && bsd[i+1].number) {
	    egap = bsd[i+1].offset - p.offset;
    	} else {
	    egap = UseSectors() - p.offset;
    	}

#ifdef	NEED_FDISK
    	if ((p.part = FindPartForBSD(p)) < 0) {
	    egap = 0;
    	} else if (part[p.part].number && p.offset+egap > part[p.part].End()) {
    	    egap = part[p.part].End() - p.offset;
    	}
#endif

    	oldlen = p.length;
    	oldoff = -1;

	r = BSDFields[2].NumericEdit(p.length, egap, force_sector, p.offset % SecPerCyl(), p.offset);
    	if (r != 1)
	    break;

	mvprint(PL, 20, "%10qd", p.length);
	mvprint(PL, 32, "%9.1f", ToMB(p.length));
	mvprint(PL, 42, "%9.1f", ToCyl(p.length));

	if (i > 1)
	    egap = bsd[i-1].End();
	else
	    egap = 0;

    	oldoff = p.offset;
    	oldlen = -1;
	r = BSDFields[3].NumericEdit(p.offset, egap, force_sector, 0, p.length);
    	if (r != 1)
	    break;

	oldlen = p.length;
	oldoff = -1;
	endsec = p.End() - 1;

    	if (i < 8 && bsd[i+1].number) {
	    egap = bsd[i+1].offset;
    	} else {
	    egap = UseSectors() - 1;
    	}

#ifdef	NEED_FDISK
    	if ((p.part = FindPartForBSD(p)) < 0) {
	    egap = -1;
    	} else if (part[p.part].number && p.offset+egap > part[p.part].End()) {
    	    egap = part[p.part].End() - 1;
    	}
#endif

    	r = BSDFields[5].NumericEdit(endsec, egap, force_sector, 0, p.offset);
    	if (endsec != p.End() - 1)
	    p.length = endsec + 1 - p.offset;

    	if (r != 1)
	    break;

    	char num = p.number + 'a' - 1;

    	r = BSDFields[4].LetterEdit(num);
    	if (r > 0) {
	    num -= 'a' - 1;
    	    if (num != p.number) {
		for (int j = 0; j < 8 && bsd[j].number; ++j) {
		    if (bsd[4].number == num) {
			inform("Partition %c is already assigned.",
				num + 'a' - 1);
			goto top;
		    }
    	    	}
		p.number = num;
    	    }
    	}
    	if (r != 1)
	    break;

    	char tmp[64];

    	strcpy(tmp, FTypeFind(p.type).name);
    	r = BSDFields[0].TextEdit(tmp, FFindType);
    	if (r > 0)
	    p.type = FFindType(tmp);
    	mvprint(PL, 2, "%-5.5s", FTypeFind(p.type).name);
    	if (r != 1)
	    break;

    	strcpy(tmp, p.mount);
    	r = BSDFields[1].TextEdit(tmp);
    	if (r > 0)
	    strcpy(p.mount, tmp);
    	if (r != 1)
	    break;
    }


    if (r == 0) {
	p = bsd[i];
	return(0);
    } else {
    	if (p.length < 1) {
	    inform(fdisk_negative_length);
	    goto top;
    	}
    	if (p.offset < 0) {
	    inform(fdisk_negative_offset);
	    goto top;
    	}
    	if (p.offset + p.length > UseSectors()) {
	    inform(fdisk_too_long);
	    if (!O::Flag(O::Expert)) {
		if (oldoff >= 0)
		    p.offset = UseSectors() - p.length;
		else if (oldlen >= 0)
		    p.length = UseSectors() - p.offset;
		goto top;
    	    }
    	}
#ifdef	NEED_FDISK
    	if ((p.part = FindPartForBSD(p)) < 0) {
    	    inform(cross_part_error);
	    if (!O::Flag(O::Expert))
		goto top;
    	}
#endif
	for (r = 0; r < 8 && bsd[r].number; ++r) {
	    FileSystem &rp = bsd[r];

	    //
	    // Do not need to check partition 3 (the C partition)
	    // as it should overlap everything anyhow.
	    //
	    if (i != r && rp.number != 3) {
		if (p.End() >= rp.End() && p.Start() <= rp.Start()) {
		    inform("This partition now fully overlaps partition %d.\n"
			   "You should first delete partition %d.\n",
			   rp.number, rp.number);
		    if (!O::Flag(O::Expert))
			goto top;
    	    	} else if (rp.End() >= p.End() && rp.Start() <= p.Start()) {
		    inform("This partition is now fully overlapped by partition %d.\n"
			   "You should first delete partition %d.\n",
			   rp.number, rp.number);
		    if (!O::Flag(O::Expert))
			goto top;
    	    	} else if (p.End() >= rp.End() && p.Start() < rp.End()) {
		    if (!verify("This partition overlaps partition %d.\n"
			        "Would you like partition %d to be shrunk\n"
			   	"to allow this partition to fit?",
			   	rp.number, rp.number)) {
			if (!O::Flag(O::Expert))
			    goto top;
			goto out;
		    }
		    if (rp.original && !verify(
				    "Any data in partitin %d will be lost.\n"
				    "Shrink partition %d anyways?",
				    rp.number, rp.number)) {
			if (!O::Flag(O::Expert))
			    goto top;
			goto out;
		    }
		    rp.original = 0;
		    rp.length = p.Start() - rp.Start();
		} else if (p.Start() <= rp.Start() && p.End() > rp.Start()) {
		    if (!verify("This partition overlaps partition %d.\n"
			        "Would you like partition %d to be shrunk\n"
			   	"to allow this partition to fit?",
			   	rp.number, rp.number)) {
			if (!O::Flag(O::Expert))
			    goto top;
			goto out;
		    }
		    if (rp.original && !verify(
				    "Any data in partitin %d will be lost.\n"
				    "Shrink partition %d anyways?",
				    rp.number, rp.number)) {
			if (!O::Flag(O::Expert))
			    goto top;
			goto out;
		    }
		    rp.original = 0;
		    rp.length = rp.End() - p.End();
		    rp.offset = p.End();
		}
out:		;
	    }
	}
#ifdef	NEED_FDISK
    	if (p.number == 1) {
	    if (p.offset != part[p.part].offset) {
		if (use_fdisk)
		    inform(bad_root_part_fdisk);
    	    	else
		    inform(bad_root_part);
		if (!O::Flag(O::Expert))
		    goto top;
    	    }
	    if (part[p.part].number && !part[p.part].IsBSDType()) {
		inform(root_on_bsd_only);
		if (!O::Flag(O::Expert))
		    goto top;
    	    }
    	}
#endif
	bsd[i] = p;
    	return(1);
    }
}

#define	FREEC	'.'
#define	CONTC	'-'

void
Disk::BDraw()
{
//  if (!use_fdisk)
//	return;

    move(2,0);
    clrtoeol();

    int p = 0;
    int f = 0;
    int i = 0;
    quad_t o = 0;

    quad_t cl[33];
    quad_t cc[33];

#ifdef	NEED_FDISK
    while (use_fdisk && p < 4 && f < 8 && part[p].number && bsd[f].number) {
    	if (bsd[f].number == 3) {
	    ++f;
	    continue;
    	}
    	if (o > part[p].End()) {
	    ++p;
    	    continue;
    	}
    	if (o > bsd[f].End()) {
	    ++f;
    	    continue;
    	}
	if (part[p].Start() <= bsd[f].Start()) {
    	    if (part[p].Start() > o) {
		cc[i] = FREEC;
		cl[i++] = part[p].Start() - o;
    	    	o = part[p].Start();
    	    }
    	    if (part[p].End() <= bsd[f].Start()) {
		cc[i] = part[p].number + '1' - 1;
		cl[i++] = part[p].End() - o;
    	    	o = part[p++].End();
    	    } else if (o < bsd[f].Start()) {
		cc[i] = part[p].number + '1' - 1;
		cl[i++] = bsd[f].Start() - o;
    	    	o = bsd[f].Start();
    	    } else {
		cc[i] = bsd[f].number + 'a' - 1;
		cl[i++] = bsd[f].End() - o;
    	    	o = bsd[f++].End();
    	    }
    	} else {
    	    if (bsd[f].Start() > o) {
		cc[i] = FREEC;
		cl[i++] = bsd[f].Start() - o;
    	    	o = bsd[f].Start();
    	    }
    	    if (bsd[f].End() > o) {
		cc[i] = bsd[f].number + 'a' - 1;
		cl[i++] = bsd[f].End() - o;
    	    	o = bsd[f++].End();
    	    }
    	}
	while (p < 4 && part[p].number && part[p].End() <= o)
	    ++p;
    }

    while (use_fdisk && p < 4 && part[p].number) {
	if (part[p].Start() > o) {
	    cc[i] = FREEC;
	    cl[i++] = part[p].Start() - o;
	    o = part[p].Start();
	}
	cc[i] = part[p].number + '1' - 1;
	cl[i++] = part[p].End() - o;
	o = part[p++].End();
    }
#endif

    while (f < 8 && bsd[f].number) {
    	if (bsd[f].number == 3) {
	    ++f;
	    continue;
    	}
	if (bsd[f].Start() > o) {
	    cc[i] = FREEC;
	    cl[i++] = bsd[f].Start() - o;
	    o = bsd[f].Start();
	}
	cc[i] = bsd[f].number + 'a' - 1;
	cl[i++] = bsd[f].End() - o;
	o = bsd[f++].End();
    }

    if (o < UseSectors()) {
    	cc[i] = FREEC;
	cl[i++] = UseSectors() - o;
    }

    quad_t t = 0;
    for (p = 0; p < i; ++p) {
    	if (cl[p])
	    cl[p] = (cl[p] * 78) / UseSectors() + 1;
    	t += cl[p];
    }

    while (t > 78) {
    	int m = 0;

    	for (p = 1; p < i; ++p) {
	    if (cl[p] > cl[m])
	    	m = p;
    	}
    	--cl[m];
    	--t;
    }

    print("|");
    for (p = 0; p < i; ++p) {
	print("%c", cc[p]);
    	for (f = 1; f < cl[p]; ++f)
	    print("%c", cc[p] == FREEC ? FREEC : CONTC);
    }

    print("|");
    refresh();
}

#ifdef	NEED_FDISK
void
Disk::FDraw()
{
    move(5,0);
    clrtoeol();

    int p = 0;
    int f = 0;
    int i = 0;
    quad_t o = 0;
    quad_t cl[33];
    quad_t cc[33];

    Sort();

    while (p < 4 && part[p].number) {
	if (part[p].Start() > o) {
	    cc[i] = FREEC;
	    cl[i++] = part[p].Start() - o;
	    o = part[p].Start();
	}
	cc[i] = part[p].number + '1' - 1;
	cl[i++] = part[p].End() - o;
	o = part[p++].End();
    }

    if (o < UseSectors()) {
    	cc[i] = FREEC;
	cl[i++] = UseSectors() - o;
    }

    quad_t t = 0;
    for (p = 0; p < i; ++p) {
    	if (cl[p])
	    cl[p] = (cl[p] * 78) / UseSectors() + 1;
    	t += cl[p];
    }

    while (t > 78) {
    	int m = 0;

    	for (p = 1; p < i; ++p) {
	    if (cl[p] > cl[m])
	    	m = p;
    	}
    	--cl[m];
    	--t;
    }

    print("|");
    for (p = 0; p < i; ++p) {
	print("%c", cc[p]);
    	for (f = 1; f < cl[p]; ++f)
	    print("%c", cc[p] == FREEC ? FREEC : CONTC);
    }

    print("|");
    refresh();
}
#endif

int
Disk::FindFreeSpace(int pt, quad_t &offset, quad_t &length)
{
    struct Parts {
    	quad_t offset;
	quad_t length;

    	quad_t End()	{ return(offset + length); }
    	quad_t Start()	{ return(offset); }
    	void Fix()	{ if (length <= 0) { offset = length = 0; } }
    } parts[9];

    int f;
    int j;
    int i = 1;

#ifdef	NEED_FDISK
    if (part[pt].number) {
	parts[0].offset = part[pt].offset;
	parts[0].length = part[pt].length;
    } else {
	parts[0].offset = 0;
	parts[0].length = UseSectors();
    }
#else
    parts[0].offset = 0;
    parts[0].length = UseSectors();
#endif
    parts[0].Fix();

    for (f = 1; f < 8 && bsd[f].number; ++f) {

    	FileSystem &b = bsd[f];

    	for (j = 0; j < i; ++j) {
    	    Parts &p = parts[j];

	    if (b.End() >= p.End() && b.Start() < p.End()) {
		p.length = b.Start() - p.Start();
		p.Fix();
	    }
	    if (b.Start() <= p.Start() && b.End() > p.Start()) {
		p.length = p.End() - b.End();
		p.offset = b.End();
		p.Fix();
	    }
	    if (b.Start() > p.Start() && b.End() < p.End()) {
		Parts &np = parts[i++];

    	    	np.offset = b.End();
    	    	np.length = p.End() - b.End();
    	    	p.length = b.Start() - p.Start();
	    }
    	}
    }

    f = 0;
    for (j = 1; j < i; ++j)
	if (parts[j].length > parts[f].length)
	    f = j;

    offset = parts[f].offset;
    length = parts[f].length;
    if (length < SecPerCyl())
	length = 0;
    return(length);
}

#ifdef	NEED_FDISK
void
Disk::UpdatePart()
{
    /*
     * When in the edit mode we should not copy in the partition table.
     * it is already there (edited by the user)
     */
    if (!use_fdisk || O::Cmd() == O::EDIT)
	return;

    int p = 0;

    for (int i = 0; i < 4; ++i) {
	if (part_table[i].system) {
	    part[p].number = i + 1;
	    part[p].length = part_table[i].Size();
	    part[p].offset = part_table[i].Start();
	    part[p].type = part_table[i].system;
    	    if (part_table[i].active == 0x80)
		active = i + 1;
	    part[p].original = 0;
	    if (BSDType(part[p].type)) {
		/*
		 * Hack the starting sector for BSDI to be 0 if it is
		 * set to 15.  Real ugly
		 */
		if (part[p].offset == 15) {
		    part[p].offset = 0;
		    part[p].length += 15;
		}
	    }

	    ++p;
    	}
    }
}

void
Disk::UpdatePartTable()
{
    MBPart *lastpart = 0;

    if (!has_fdisk && !use_fdisk || O::Cmd() == O::EDIT || O::Cmd() == O::READ)
	return;


    int bg = use_bsdg;

    SwitchToCMOS();

    memset(part_table, 0, sizeof(part_table));

    for (int p = 0; p < 4 && part[p].number; ++p) {
    	MBPart *m = &part_table[part[p].number - 1];
    	if (active == part[p].number)
	    m->active = 0x80;
    	m->system = part[p].type;
    	m->Start(part[p].offset);
	m->Size(part[p].length);
	if (lastpart == 0 || m->Start() > lastpart->Start())
		lastpart = m;

    	if (BSDType(m->system) && m->Start() == 0) {
	    m->Start(15);
	    m->Size(m->Size() - 15);
    	}

    	long n = m->Start();

    	int cyl = n / SecPerCyl();

    	if (cyl > 1023)
	    cyl = 1023;

    	m->scyl = cyl & 0xff;
    	m->ssec = (cyl & 0x300) >> 2;
    	n -= (n / SecPerCyl()) * SecPerCyl();
    	m->shead = n / SecPerTrack();
    	n -= m->shead * SecPerTrack();
    	m->ssec |= n + 1;

    	n = m->Start() + m->Size() - 1;

    	cyl = n / SecPerCyl();

    	if (cyl > 1023)
	    cyl = 1023;

    	m->ecyl = cyl & 0xff;
    	m->esec = (cyl & 0x300) >> 2;
    	n -= (n / SecPerCyl()) * SecPerCyl();
    	m->ehead = n / SecPerTrack();
    	n -= m->ehead * SecPerTrack();
    	m->esec |= n + 1;
    }
    /*
     * Recompute the last partition on the disk to end on a cylinder boundary
     */
    if (lastpart) {
    	MBPart *m = lastpart;
    	long n = m->Start() + m->Size();

    	int cyl = n / SecPerCyl();

	n = cyl * SecPerCyl();

	m->Size(n - m->Start());

    	cyl = --n / SecPerCyl();

    	if (cyl > 1023)
	    cyl = 1023;

    	m->ecyl = cyl & 0xff;
    	m->esec = (cyl & 0x300) >> 2;
    	n -= (n / SecPerCyl()) * SecPerCyl();
    	m->ehead = n / SecPerTrack();
    	n -= m->ehead * SecPerTrack();
    	m->esec |= n + 1;
    }
    use_bsdg = bg;
}
#endif

quad_t
Disk::DefaultSwap(int pt)
{
    quad_t dlen;
    quad_t mem = (PhysMem() + (512 * 1024)) / (1024 * 1024);
    quad_t swap;

#ifdef	NEED_FDISK
    if (pt >= 0 && part[pt].number)
	dlen = part[pt].length;
    else
#endif
	dlen = UseSectors();
    dlen = quad_t(ToMB(dlen) + .5);

    if (mem <= 16)
	swap = mem * 3;
    else if (mem <= 32)
	swap = 48;
    else if (mem <= 64)
	swap = mem + (mem >> 1);
    else if (mem <= 96)
	swap = 96;
    else if (mem <= 128)
	swap = mem;
    else
	swap = 128;

    //
    // If we have lots of room, double the size of swap.
    // This will give 96MB on a 16-32MB system.
    // This will give 192MB on a 64-96MB system.
    //
    if (dlen > 800)
	swap *= 2;

    if (swap > dlen / 4)
	swap = dlen / 4;

    //
    // Okay, make swap at least 16MB if we have at lest a 32MB disk,
    // otherwise stay with the size/4 (note that it is really silly
    // to try and put swap and root on a 20MB disk...
    //
    if (swap < 16 && dlen >= 32)
	swap = 16;

    return(FromMB(swap));
}

quad_t
Disk::DefaultRoot(int pt)
{
    quad_t dlen;
    quad_t mem = (PhysMem() + (512 * 1024)) / (1024 * 1024);
    quad_t root;

#ifdef	NEED_FDISK
    if (part[pt].number)
	dlen = part[pt].length;
    else
#endif
	dlen = UseSectors();
    dlen = quad_t(ToMB(dlen) + .5);

    root = MINROOT + (mem / 8);

    if (dlen > 800 && root < 16)
	root = 16;

    if (root > MAXROOT)
	root = MAXROOT;

    if (root > dlen / 12)
	root = dlen / 12;

    if (root < MINROOT)
	root = MINROOT;

    return(FromMB(root));
}
