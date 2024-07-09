/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI disk.h,v 2.14 2001/10/03 17:29:56 polk Exp	*/

#if defined(i386) || defined(__powerpc__)
#define	NEED_FDISK
#include <machine/bootblock.h>
#endif

#include "partition.h"
#include "filesys.h"

#if defined(__powerpc__)
#define	MINROOT	32
#else
#define	MINROOT	20
#endif
#define	MAXROOT	64

class Disk;

struct MBPart {
	u_char  active;
		/* start of partition */
	u_char  shead;		/* start: head */
	u_char  ssec;		/* start: sec (and 2 high bits of cylinder) */
	u_char  scyl;		/* start: 8 low bits of cylinder */

	u_char  system;		/* system type indicator - see below */
		/* end of partition */
	u_char  ehead;		/* end: head */
	u_char  esec;		/* end: sec (and 2 high bits of cylinder) */
	u_char  ecyl;		/* end: 8 low bits of cylinder */

	u_char	_start[4];	/* start sector of partition */
	u_char	_size[4];	/* size of partitions (in secs) */

	long	Start()		{ return((_start[0]      )|(_start[1] <<  8) |
					 (_start[2] << 16)|(_start[3] << 24)); }
	long	Size()		{ return((_size[0]      ) | (_size[1] <<  8) |
					 (_size[2] << 16) | (_size[3] << 24)); }
	void	Start(long s)	{ _start[0] = s & 0xff;
				  _start[1] = (s >>  8) & 0xff;
				  _start[2] = (s >>  16) & 0xff;
				  _start[3] = (s >>  24) & 0xff; };
	void	Size(long s)	{ _size[0] = s & 0xff;
				  _size[1] = (s >>  8) & 0xff;
				  _size[2] = (s >>  16) & 0xff;
				  _size[3] = (s >>  24) & 0xff; };
};

struct BootBlock {
    unsigned char	data[512];

    int Write(int fd);
    int Read(int fd);
    int Read(char *);
    void Clean()	{ memset(data, 0, sizeof(data)); }
#ifdef	NEED_FDISK
    void MergePartitions(MBPart *);

    int Signature()	{ return((data[511] << 8) | data[510]); }
    unsigned char *Part(int i)
			{ return(data + 512 - (5 - i) * 16 - 2); }
    unsigned long Length(int i)
			{ unsigned char *t = Part(i)+12;
			  return((unsigned long)(t[0] | (t[1]<<8) |
						(t[2]<<16) | t[3] << 24)); }
    unsigned long Offset(int i)
			{ unsigned char *t = Part(i)+8;
			  return((unsigned long)(t[0] | (t[1]<<8) |
						(t[2]<<16) | t[3] << 24)); }

    unsigned long BTrack(int i) {
			unsigned char *p = Part(i) + 1;
			return(p[0]); }
    unsigned long BCyl(int i) {
			unsigned char *p = Part(i) + 1;
			return(p[2] | (int(p[1] & 0xc0) << 2)); }
    unsigned long BSector(int i) {
			unsigned char *p = Part(i) + 1;
			return(p[1] & 0x3f); }

    unsigned long ETrack(int i) {
			unsigned char *p = Part(i) + 5;
			return(p[0]); }
    unsigned long ECyl(int i) {
			unsigned char *p = Part(i) + 5;
			return(p[2] | (int(p[1] & 0xc0) << 2)); }
    unsigned long ESector(int i) {
			unsigned char *p = Part(i) + 5;
			return(p[1] & 0x3f); }

    unsigned char Type(int i)
			{ return(Part(i)[4]); }
    unsigned char Active(int i)
			{ return(Part(i)[0]); }
#endif
};

struct DiskLabel : public disklabel {
    int Read(int fd, off_t, off_t * = 0);
    int Internal(int);
    int WriteInternal(int);
    int Generic(int);
    int SCSI(char *);
    int Disktab(char *, char * = 0);
    void Fixup();
    void Clean();
    int Valid();
    int Soft();
    void ComputeSum(off_t = 0);
#ifdef	sparc
    int Sun(int fd);
    int WriteSun(int fd);
#endif
};

struct Geometry {
private:
    int			heads;
    int			sectors;
    int			cylinders;
    quad_t		size;
    int	  		secpercyl;
public:
    Geometry()			{ secpercyl = heads =
				  sectors = cylinders = size = 0; }
    void Zero()			{ secpercyl = heads =
				  sectors = cylinders = size = 0; }
    int Valid()			{ return(cylinders && heads && sectors); }

    quad_t Sectors()		{ return(size); }
    quad_t Sectors(quad_t i)	{ return(size = i); }
    int Heads()			{ return(heads); }
    int Heads(int i)		{ return(heads = i); }
    int SecPerTrack()		{ return(sectors); }
    int SecPerTrack(int i)	{ return(sectors = i); }
    int Cyls()			{ return(cylinders); }
    int Cyls(int i)		{ return(cylinders = i); }
    int SecPerCyl(int i)	{ return(secpercyl = i); }
    int SecPerCyl()		{ return(secpercyl
					 ? secpercyl
					 : (secpercyl = heads * sectors)); }

    Geometry &operator =(Geometry &i)
				{ heads = i.heads; sectors = i.sectors;
				  cylinders = i.cylinders;
				  secpercyl = i.SecPerCyl();
				  if (i.size) size = i.size;
				  return(*this); }
    int operator ==(Geometry &i)
				{ return(heads == i.heads &&
					 sectors == i.sectors &&
					 cylinders == i.cylinders &&
					 SecPerCyl() == i.SecPerCyl()); }
    int Match(Geometry &i)	{ return(heads == i.heads &&
					 sectors == i.sectors &&
					 SecPerCyl() == i.SecPerCyl()); }
};

#if 0
#define	SECSIZE	disk.secsize
#else
#define	SECSIZE	DEV_BSIZE
#endif

struct Disk {
public:
    int			dfd;
    char		device[PATH_MAX];
    char		*path;
    int			d_type;

    off_t		bsdoff;			// Offset to BSD label

    int			secsize;		// # Bytes/sector
    quad_t		badblock;
    Geometry		bsdg;

    BootBlock		bootblock;
    DiskLabel		label_original;
    DiskLabel		label_template;		// Filled in by scsicmd
    DiskLabel		label_new;		// The label we will write out
    static DiskLabel	empty_label;

    FileSystem		bsd[8];			// Where we build BSD parts
    static FileSystem	empty_filsys;
    unsigned		bsd_modified : 1;	// Original BSD part changed

#ifdef	sparc
    DiskLabel		label_sun;		// Any SUN Label found
#endif
#ifdef	NEED_FDISK
    int			bsdbootpart;		// Bootable BSD partition
    Geometry		cmos;
    Geometry		bios;
    Geometry		fdisk;
    Geometry		dosg;
    u_quad_t		size;			// Number of sectors on the disk

    unsigned		use_bsdg : 1;		// Only use bsd geometry
    unsigned		use_bsd : 1;		// if we are doing bsd
    unsigned		has_fdisk : 1;		// if we have an fdisk label
    unsigned		use_fdisk : 1;		// if we want an fdisk label

    unsigned		part_modified : 1;	// Original FDISK part changed
    MBPart		part_table[4];

    Partition		part[4];		// Where we build FDISK parts
    static Partition	empty_partition;

    unsigned char	active;			// partition # which is active
#endif

    Disk();
    int Init(char *);
    int Type();
    int FSys();
    int EditBSDPart(int);
    void Sort();
    void BDraw();
    quad_t DefaultSwap(int = -1);
    quad_t DefaultRoot(int = 0);
    FileSystem &RootFilesystem();
    FileSystem &FindFileSystem(int);
    off_t LabelLocation();
    int FindFSys(int);
    int FindFreeSpace(int pt, quad_t &offset, quad_t &length);

#ifdef	NEED_FDISK
    void Sectors(quad_t i)	{ size = i; }
    quad_t Sectors()		{ return(size); }
#else
    void Sectors(quad_t i)	{ bsdg.Sectors(i); }
    quad_t Sectors()		{ return(bsdg.Sectors()); }
#endif
    quad_t UseSectors()		{ return(Sectors() - badblock); }
    quad_t MBs()		{ return(int(ToMB(Sectors()))); }
    double ToGB(quad_t i)	{ return(i / (1024.0 * 1024.0 * 1024.0 / secsize)); }
    double ToMB(quad_t i)	{ return(i / (1024.0 * 1024.0 / secsize)); }
    double ToCyl(quad_t i)	{ return(SecPerCyl() ? i / double(SecPerCyl())
					 : 0); }
    quad_t FromGB(double i);
    quad_t FromMB(double i);
    quad_t FromCyl(double i);
    quad_t FromTrack(double i);
    quad_t FromSector(double i);
    quad_t Max(quad_t i)	{ return(i > UseSectors() ? UseSectors() : i); }

    void AddNumber(quad_t &v, char *b)
	{ AddNumber(v, b, &Disk::FromSector); }
    void AddNumber(quad_t &, char *, quad_t (Disk::*func)(double));
    int AddPartition(int = 0);

    void WriteEnable();
    void WriteDisable();

#ifdef	sparc
    int SecPerTrack()		{ return(bsdg.SecPerTrack()); }
    int Heads()			{ return(bsdg.Heads()); }
    int Cyls()			{ return(bsdg.Cyls()); }
    int SecPerCyl()		{ return(bsdg.SecPerCyl()); }
#endif

#ifdef	NEED_FDISK
    int FDisk();
    int EditPart(int);
    void FSort();
    void FDraw();
    void ComputeBSDBoot();
    MBPart *PartTable()		{ UpdatePartTable();
				  return((has_fdisk||use_fdisk)?part_table:0); }

    Partition &FindPartition(int);

    int FindPartForBSD(int);
    int FindPartForBSD(FileSystem &);
    int FindFPart(int);

    void SwitchToCMOS()		{ if (!dosg.Valid()) dosg = fdisk;
				  use_bsdg = !dosg.Valid(); }
    void SwitchToBSD()		{ use_bsdg = bsdg.Valid(); }

    void UpdatePart();
    void UpdatePartTable();

    int SecPerTrack()		{ return(use_bsdg ? bsdg.SecPerTrack()
						  : dosg.SecPerTrack()); }
    int Heads()			{ return(use_bsdg ? bsdg.Heads()
						  : dosg.Heads()); }
    int Cyls()			{ return(use_bsdg ? bsdg.Cyls()
						  : dosg.Cyls()); }
    int SecPerCyl()		{ return(use_bsdg ? bsdg.SecPerCyl()
						  : dosg.SecPerCyl()); }
#endif
};

#ifdef	NEED_FDISK
inline int
BSDType(int t)
{
    return(t == 0x9f);
}
#endif

extern Disk disk;

#ifdef	NEED_FDISK
extern "C" void disklabel_display(char *, FILE *, disklabel *, MBPart * = 0);
extern "C" int disklabel_getasciilabel(FILE *, disklabel *, MBPart * = 0);
#else
extern "C" void disklabel_display(char *, FILE *, disklabel *);
extern "C" int disklabel_getasciilabel(FILE *, disklabel *);
#endif
extern "C" int disklabel_checklabel(disklabel *);
extern "C" u_short dkcksum(struct disklabel *);
