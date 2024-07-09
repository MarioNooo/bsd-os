/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI bootany.c,v 2.9 2001/10/03 17:29:56 polk Exp
 */

#include <sys/types.h>
#include <sys/param.h>

#include <machine/bootblock.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char bootany[512];

#define	_PATH_BOOTANY_CHS	"/usr/bootstraps/bootany.sys"
#define	_PATH_BOOTANY_LBA	"/usr/bootstraps/bootany.lba"

char *bootany_sys;

#define	MAX_BOOTANY		4

typedef enum {
	UNKNOWN_MODE,
	CHS,
	LBA
} disk_mode_t;

struct bapart_lba {
	u_char	part;
	u_char	text[7];
};

struct bapart_chs {
	u_char	part;
	u_char	drive;
	u_char	shead;
	u_char	ssec;
	u_char	scyl;
	u_char	text[8];
};

struct bapart {
	disk_mode_t	mode;
	u_char	part;
	u_char	drive;
	u_char	shead;
	u_char	ssec;
	u_char	scyl;
	u_char	text[8];
};


struct dap {
	u_short	size;
	u_short	sectors;
	u_short	loadoffset;
	u_short	loadsegment;
	u_long	lba_low;
	u_long	lba_high;
};

#define	MBRSIG		(512 - 2)
#define	PARTADDR	(MBRSIG - sizeof(struct mbpart) * 4)
#define	SIGADDR		(PARTADDR - 2)
#define	DAPADDR		(SIGADDR - sizeof(struct dap))

#define	SIGNATURE		0xaa55
#define	BOOTANY_SIGNATURE	0x8a0d

#define	ISBOOTANY(x)	(*((unsigned short *)(&((char *)x)[SIGADDR])) == BOOTANY_SIGNATURE)
#define	ISMBR(x)	(*((unsigned short *)(&((char *)x)[MBRSIG])) == SIGNATURE)
#define	ISLBA(x)	(((struct dap *)(((char *)x)+DAPADDR))->loadoffset == 0x7c00)

#define	MBS_DOSEXT	MBS_DOS

#define	ANYADDR_CHS	(SIGADDR - (sizeof(struct bapart_chs) * MAX_BOOTANY))
#define	ANYADDR_LBA	(DAPADDR - (sizeof(struct bapart_lba) * MAX_BOOTANY))

#define	FDISK_PART(n,b)	(&(((struct mbpart *)((char *)b + PARTADDR))[n]))

char *sysname(int);
int bootable(int);
char *convname(u_char *, int);
void usage();
void extract_bootany_table(char *, char *);
struct bapart *find_free_bootany();
void	printtable __P((char *, char *));
int	request_yesno __P((char *, int));
u_char *request_string __P((char *, char *));
void install_bootany_table(char *);
void print_fdisk(char *, char *);
int disk_open(char *, int);

#define	BOOTSIZE	512
char primary_block[BOOTSIZE];
char secondary_block[BOOTSIZE];
struct bapart parts[MAX_BOOTANY];
int changed = 0;

int
main(int ac, char **av)
{
	int c;
	int i;
	int fd, tfd;
	struct mbpart *m;
	struct bapart *bp;
	int dflag = 0;
	int fflag = 0;
	int iflag = 0;
	int zflag = 0;
	int nflag = 0;
	char *mbr = 0;
	char *dmbr = 0;
	extern int optind;
	extern char *optarg;
	int disk;
	char prompt[64];
	char *save = 0;
	u_char *name;
	u_char *p;
	int active = 0;
	int express = 0;

	char *block1 = NULL;
	char *block2 = NULL;

	while ((c = getopt(ac, av, "dfins:zA:CF:L?S")) != EOF) {
		switch (c) {
		case 'C':
			if (bootany_sys)
			    errx(1, "Only one of -C, -F and -L may be used");
			bootany_sys = _PATH_BOOTANY_CHS;
			break;
		case 'L':
			if (bootany_sys)
			    errx(1, "Only one of -C, -F and -L may be used");
			bootany_sys = _PATH_BOOTANY_LBA;
			break;
		case 'S':
			express++;
			break;
		case 'F':
			if (bootany_sys)
			    errx(1, "Only one of -C, -F and -L may be used");
			bootany_sys = optarg;
			break;
		case 'A':
			active = atoi(optarg);
			if (active < 1 || active > 4)
			    errx(1, "%s: invalid.  Active partition must in the range of 1 and 4.",
				optarg);
			break;
		case 's':
			save = optarg;
			break;
		case 'f':
			++fflag;
			break;
		case 'd':
			++dflag;
			break;
		case 'i':
			++iflag;
			break;
		case 'z':
			++zflag;
			break;
		case 'n':
			++nflag;
			break;
		usage:
		default:
			usage();
		}
	}

	if (optind + 1 != ac && optind + 2 != ac)
		goto usage;

	mbr = av[optind];
	dmbr = av[optind + 1];

	/*
	 * Read in the primary master boot record and extract
	 * the bootany table, if it exists.
	 */
	if ((fd = disk_open(mbr, 2)) < 0)
		err(1, "%s", mbr);

	block1 = primary_block;

	if (read(fd, block1, BOOTSIZE) != BOOTSIZE)
		errx(1, "%s: short read", mbr);

	if (!ISMBR(block1))
		errx(1, "%s: invalid signature (probably not an MBR)", mbr);

	if (save) {
		if ((tfd = creat(save, 0666)) < 0)
			err(1, "%s", save);

		if (write(tfd, block1, BOOTSIZE) != BOOTSIZE)
			errx(1, "%s: short write", save);
	}

	/*
	 * If a secondary disk was provided, read its master boot
	 * record as well.  It might not have an fdisk table.
	 */
	if (dmbr != NULL) {
		if ((tfd = disk_open(dmbr, 0)) < 0)
			err(1, "%s", dmbr);

		block2 = secondary_block;

		if (read(tfd, block2, BOOTSIZE) != BOOTSIZE)
			errx(1, "%s: short read", dmbr);

		close(tfd);
		if (!dflag && !ISMBR(block2))
			errx(1, "%s: invalid signature (probably not an MBR)",
			    dmbr);
	}

	extract_bootany_table(block1, block2);

	/*
	 * If we are not modifying the bootrecords, just
	 * print out the contents.
	 */
	if (!express && !active && !iflag && !zflag && !nflag) {
		printtable(block1, block2);
		if (fflag)
			print_fdisk(block1, block2);
		exit(1);
	}

	/*
	 * Copy in a new master boot record if they requested one.
	 * Don't bother if it does not change anything.
	 */
	if (nflag) {
		/*
		 * We use LBA mode bootany.sys unless they are trying to
		 * set up two drives, then we must use CHS mode
		 */
		if (bootany_sys == NULL)
		    bootany_sys =
			(dmbr || dflag) ? _PATH_BOOTANY_CHS : _PATH_BOOTANY_LBA;

		if ((tfd = open(bootany_sys, 0)) < 0)
			err(1, "%s", bootany_sys);

		if (read(tfd, bootany, sizeof(bootany)) != sizeof(bootany))
			errx(1, "%s: short file", bootany_sys);

		close(tfd);

		if (!ISMBR(bootany))
			errx(1, "%s: invalid FDISK signature", bootany_sys);

		if (!ISBOOTANY(bootany))
			errx(1, "%s: invalid BOOTANY signature", bootany_sys);

		if (ISLBA(bootany) && (dflag || dmbr))
			errx(1, "%s: cannot be used with two disks",
			    bootany_sys);

		if (memcmp(block1, bootany, PARTADDR) != 0) {
			memset(parts, 0, sizeof(parts));
			memcpy(block1, bootany, PARTADDR);
			++changed;
		}
	}

	/*
	 * If they requested us to set a new active partition, do so.
	 * Mark the block as changed if we actually did change it.
	 */
	if (active) {
		struct mbpart *m = FDISK_PART(active - 1, block1);
		if (!m->system || !m->size)
			errx(1, "%d: not an assigned FDISK partition", active);
		if (m->active != 0x80) {
			for (i = 0; i < 4; ++i)
				FDISK_PART(i, block1)->active = 0;
			m->active = 0x80;
	    		++changed;
		}
	}

	/*
	 * We only have to zero out our incore bootany table as we will
	 * install this table into the boot block just before we write
	 * it out.
	 */
	if (zflag) {
		if (!ISBOOTANY(block1))
			errx(1, "%s: does not have bootany installed.", mbr);
		memset(parts, 0, sizeof(parts));
		++changed;
	}

	if (dflag) {
		if (ISLBA(block1))
			errx(1, "Cannot use an LBA bootany with a second disk");
		bp = find_free_bootany();
		if (bp == NULL)
			errx(1, "No more bootany partitions left empty");

		++changed;
		if (express)
			name = (u_char *)"D:";
		else
			name = request_string("What do you wish to call D: (8 char max)? ",
			    "D:");
		if (!name)
		    exit(1);

		memcpy(bp->text, name, sizeof(bp->text));
		name = bp->text;
		for (p = name; *p && p < name + sizeof(bp->text); ++p)
			;
		p[-1] |= 0x80;
		while (p < name + sizeof(bp->text))
			*p++ = 0;
		bp->part = 0x80;
		bp->drive = 0x81;
		bp->shead = 0;
		bp->ssec = 1;
		bp->scyl = 0;

		dmbr = NULL;	/* No need to look here again */
	}

	/*
	 * If we only have a single entry and we are doing an express install
	 * then clear the iflag.
	 */
	if (express && !dflag) {
		c = 0;
		for (i = 0; i < 4; ++i) {
			m = FDISK_PART(i, block1);
			if (m->system && m->size)
				++c;
		}
		if (c == 1)
			iflag = 0;
	}

	if (iflag) {
		int a;
		int once = 0;
		char *block;

		printf("\n");
		disk = 0x80;
		block = block1;
	again:

		a = 0;
		for (i = 1; i <= 4; ++i, ++m) {
			m = FDISK_PART(i - 1, block);
			if (m->system == 0 || m->size == 0)
				continue;

			if (m->active == 0x80 && a++) {
				printf("Deactiving extra active partition\n");
				m->active = 0;
			}

			bp = find_free_bootany();

			if (bp == NULL) {
				if (!once++)
					printf("No more bootany partitions"
					    " left empty\n");
				continue;
			}

			if (express) {
				if (!bootable(m->system))
					continue;
				name = (u_char *)sysname(m->system);
			} else {
				if (m->system == MBS_DOSEXT && ISLBA(block1))
					continue;
				sprintf(prompt,
				    "Is partition %d of disk %c: bootable,"
				    " type %s (%02x)?",
				    i, disk == 0x80 ? 'C' : 'D',
				    sysname(m->system), m->system);
				if (request_yesno(prompt,
				    bootable(m->system)) == 0)
					continue;
				sprintf(prompt, "What do you wish to call it "
				    "(%d char max)? ",
				    ISLBA(block1) ? 7 : 8);	/* XXX */
				name = request_string(prompt,
				    sysname(m->system));
			}
			if (!name)
			    	exit(1);

			++changed;
			memset(bp->text, 0, sizeof(bp->text));
			memcpy(bp->text, name, sizeof(bp->text));
			name = bp->text;
			for (p = name; *p && p < name + sizeof(bp->text); ++p)
				;

			p[-1] |= 0x80;

			bp->part = i;
			bp->drive = disk;
			bp->shead = m->shead;

			if (m->system == MBS_DOSEXT) {
				printf("Warning, allowing boots from extended DOS partition\n");
				printf("Assuming that the actual partition starts 1 track in\n");
				bp->shead++;
			}
			bp->ssec = m->ssec;
			bp->scyl = m->scyl;
		}
		if (block2) {
			block = block2;
			block2 = NULL;
			disk++;
		}
	}
	if (changed && (express || request_yesno("Write out new MBR?", 1))) {
		install_bootany_table(block1);
		lseek(fd, 0, L_SET);
		if (write(fd, block1, BOOTSIZE) != BOOTSIZE)
			err(1, "%s: short write", mbr);
	}
	exit (0);
}

void
usage()
{
	fprintf(stderr, "Usage: bootany [-F bootany.sys] [-A fpart] [-dinz] [-s save] MBR [D:MBR]\n");
	fprintf(stderr, "   MBR  Master Boot Record to modify (can be /dev/rxd0c)\n");
	fprintf(stderr, " D:MBR  Second drive Master Boot Record (read only)\n");
	fprintf(stderr, "    -C  Use CHS mode bootany.sys (supports second drive)\n");
	fprintf(stderr, "    -L  Use LBA mode bootany.lba (does not support second drive)\n");
	fprintf(stderr, "    -F  path to find bootany.sys\n");
	fprintf(stderr, "    -A  Make 'fpart' the active FDISK partition\n");
	fprintf(stderr, "    -d  Assume D: does not have an FDISK table (D:MBR not requried)\n");
	fprintf(stderr, "    -f  Print fdisk table as well\n");
	fprintf(stderr, "    -i  Interactively assign boot partition table\n");
	fprintf(stderr, "    -n  Install new bootany into MBR\n");
	fprintf(stderr, "    -z  Zero existing boot partition table\n");
	fprintf(stderr, "    -s  Save old MBR in file 'save'\n");
	exit(1);
}

int
disk_open(char *disk, int mode)
{
	int fd;

	char path[MAXPATHLEN];

	if ((fd = open(disk, mode)) >= 0)
		return (fd);
	if (strchr(disk, '/') == NULL) {
		snprintf(path, sizeof(path), "/dev/r%sc", disk);
		if ((fd = open(path, mode)) >= 0)
			return (fd);
		snprintf(path, sizeof(path), "/dev/r%s", disk);
		if ((fd = open(path, mode)) >= 0)
			return (fd);
		snprintf(path, sizeof(path), "/dev/%sc", disk);
		if ((fd = open(path, mode)) >= 0)
			return (fd);
		snprintf(path, sizeof(path), "/dev/%s", disk);
		if ((fd = open(path, mode)) >= 0)
			return (fd);
	}
	return (-1);
}

void
extract_bootany_table(char *block, char *block2)
{
	struct mbpart *m;
	struct bapart_lba *lba;
	struct bapart_chs *chs;
	struct bapart *bp = parts;
	int i;

	memset(parts, 0, sizeof(parts));

	if (!ISMBR(block) || !ISBOOTANY(block))
		return;

	lba = (struct bapart_lba *)(block + ANYADDR_LBA);
	chs = (struct bapart_chs *)(block + ANYADDR_CHS);

	for (i = 0; i < 4; ++i, ++lba, ++chs) {
		if (ISLBA(block)) {
			if (lba->part >= MAX_BOOTANY)
				break;
			m = FDISK_PART(lba->part, block);
			bp->mode = LBA;
			bp->part = lba->part + 1;
			bp->drive = 0x80;
			bp->shead = m->shead;
			bp->ssec = m->ssec;
			bp->scyl = m->scyl;
			memcpy(bp->text, lba->text, sizeof(lba->text));
		} else {
			bp->part = chs->part & 0xf;
			if (bp->part < 1 || bp->part > MAX_BOOTANY) {
				bp->part = 0;
				break;
			}
			if ((chs->part & 0x80) != 0) {
				if (chs->drive != 0x81) {
					printf("Invalid bootany partition discarded");
					bp->part = 0;
					continue;
				}
			}
			if ((chs->part & 0x80) == 0)
				m = FDISK_PART(bp->part - 1, block);
			else if (block2)
				m = FDISK_PART(bp->part - 1, block2);
			else
				m = NULL;
			bp->mode = CHS;
			bp->drive = chs->drive;
			bp->shead = chs->shead;
			bp->ssec = chs->ssec;
			bp->scyl = chs->scyl;
			memcpy(bp->text, chs->text, sizeof(chs->text));
		}
		if (m &&
		    (m->size == 0 || m->system == 0 ||
		     (m->system == MBS_DOSEXT && bp->shead != m->shead + 1) ||
		     (m->system != MBS_DOSEXT && bp->shead != m->shead) ||
		     bp->scyl != m->scyl ||
		     bp->ssec != m->ssec)) {
			++changed;
			memset(bp, 0, sizeof(*bp));
			fprintf(stderr, "Removing invald bootany partition\n");
		} else
			++bp;
	}
}

void
install_bootany_table(char *block)
{
	struct mbpart *m;
	struct bapart_lba *lba;
	struct bapart_chs *chs;
	struct bapart *bp = parts;
	int i;

	if (!ISMBR(block) && !ISBOOTANY(block))
		return;

	lba = (struct bapart_lba *)(block + ANYADDR_LBA);
	chs = (struct bapart_chs *)(block + ANYADDR_CHS);

#define	LBATEXT	sizeof(lba->text)

	for (i = 0; i < 4; ++i, ++lba, ++chs, ++bp) {
		if (ISLBA(block)) {
			if (bp->part == 0) {
				lba->part = 0xff;
				memset(lba->text, 0, LBATEXT);
			} else {
				lba->part = bp->part - 1;
				memcpy(lba->text, bp->text, LBATEXT);
				if (lba->text[LBATEXT - 1])
					lba->text[LBATEXT - 1] |= 0x80;
			}
		} else {
			if (bp->part == 0) {
				memset(chs, 0, sizeof(chs));
			} else {
				chs->part = bp->part;
				if (bp->drive != 0x80)
					chs->part |= 0x80;
				memcpy(chs->text, bp->text, sizeof(chs->text));
				chs->drive = bp->drive;
				chs->shead = bp->shead;
				chs->ssec = bp->ssec;
				chs->scyl = bp->scyl;
			}
		}
	}
}

struct bapart *
find_free_bootany()
{
	struct bapart *bp = parts;
	int i;

	for (i = 0; i < 4; ++i, ++bp) {
		if (bp->part == 0)
			return (bp);
	}
	return (NULL);
}

void
print_fdisk(char *block, char *block2)
{
	struct mbpart *m;
	char drive = 'C';
	int i;

	printf("FDISK Tables:\n");

	while (block) {
		m = (struct mbpart *)(block + PARTADDR);

		for (i = 0; i < 4; ++i, ++m) {
		    if (m->system && m->size) {
			printf("%c:%d:%02x 0x%02x:%-8s %d/%d/%d [%ld]%s\n",
				 drive,
				 i + 1,
				 m->active,
				 m->system,
				 sysname(m->system),
				 m->scyl | (m->ssec & 0xC0) << 2,
				 m->shead,
				 (m->ssec & 0x3F) - 1,
				 m->start,
				 m->active == 0x80 ? " active" : "");
		    }
		}
		block = block2;
		block2 = NULL;
		++drive;
	}
}

void
printtable(char *block, char *block2)
{
	int i;
	int p;
	struct mbpart *m;
	struct bapart *bp = parts;

	if (!ISBOOTANY(block))
		return;

	printf("Bootany Table (%s mode):\n", ISLBA(block) ? "LBA" : "CHS");

	for (i = 0; i < MAX_BOOTANY && bp->part; ++i, ++bp) {

		printf("%c:%d %-*.*s %d/%d/%d",
		    (bp->part & 0x80) ? 'D' : 'C',
		     bp->part & 0x7F,
		     sizeof(bp->text), sizeof(bp->text),
		     convname(bp->text, sizeof(bp->text)),
		     bp->scyl | (bp->ssec & 0xC0) << 2,
		     bp->shead,
		     (bp->ssec & 0x3F) - 1);

		if (bp->drive != 0x80)
			m = (struct mbpart *)(block2 + PARTADDR);
		else
			m = (struct mbpart *)(block + PARTADDR);

		if (m)
			m += (bp->part & 0xF) - 1;

		if (m &&
		    ((m->system == MBS_DOSEXT && bp->shead != m->shead + 1) ||
		     (m->system != MBS_DOSEXT && bp->shead != m->shead) ||
		     bp->scyl != m->scyl ||
		     bp->ssec != m->ssec))
			printf(" no longer valid");
		if (m && m->active == 0x80 && bp->drive == 0x80)
			printf(" active");
		printf("\n");
	}
}

int
truth(char *s, int def)
{   
    while (isspace(*s))
        ++s;
    if (!*s || *s == '\n')
        return(def);
    if (*s == 'Y' || *s == 'y')
        return(1);
    if (*s == 'N' || *s == 'n')
        return(0);
    return(-1);
}       

int
request_yesno(char *prompt, int def)
{
    char buf[64];
    int t;

    do {
	printf("%s [%s] ", prompt, def ? "YES" : "NO");
	if (!fgets(buf, sizeof(buf), stdin))
	    return(def);
    } while ((t = truth(buf, def)) == -1);
    return(t);
}

u_char *
request_string(char *prompt, char *def)
{
    static char buf[1024];
    u_char *s, *e;

    do {
    	printf("%s ", prompt);
    	if (def)
    	    printf("[%s] ", def);
	if(!fgets(buf, sizeof(buf), stdin))
	    return(0);
	s = (u_char *)buf;
    	while (*s == ' ' || *s == '\t' || *s == '\n')
	    ++s;
    	e = s;
	while (*e)
	    ++e;
    	while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\n'))
	    --e;
	*e = 0;
    	if (*s == '\0')
	    s = (u_char *)def;
    } while (s && *s == '\0');
    return(s);
}

char *
sysname(int i)
{
	switch (i) {
	case MBS_DOS12:
	case MBS_DOS16:
	case MBS_DOS4:	return("DOS");
	case MBS_DOSEXT:	return("DOS-EXT");
	case MBS_BSDI:	return("BSD/OS");
	case 0x02:		return("XENIX");
	case 0x03:		return("XENIX-USR");
	case 0x07:		return("WIN-HPFS");
	case 0x08:		return("AIX");
	case 0x09:		return("AIX-BOOT");
	case 0x0A:		return("OS/2 BM");
	case 0x0B:		return("WIN-FAT32");
	case 0x40:		return("VENIX");
	case 0x41:		return("PPC-BOOT");
	case 0x51:		return("NOVEL");
	case 0x52:		return("CP/M");
	case 0x81:		return("MINIX");
	case 0x82:		return("LINUX");
	case 0x93:		return("AMOEBA");
	default:		return("Unknown");
	}
}

int
bootable(int i)
{
    switch (i) {
    case MBS_DOS12:
    case MBS_DOS16:
    case MBS_DOS4:
    case MBS_BSDI:
    case 0x07:	/* WIN-HPFS */
    case 0x0A:	/* OS2 */
    case 0x0B:	/* WIN-FAT32 */
    case 0x82:	/* LINUX */
	return(1);
    case MBS_DOSEXT:
    default:
	return(0);
    }
}

char *
convname(u_char *n, int max)
{
    static char name[9];
    char *c = name;
    n[max - 1] |= 0x80;

    while ((*c++ = (*n & 0x7f)) != '\0') {
    	if (*n++ & 0x80) {
	    *c = 0;
	    break;
    	}
    }
    return(name);
}
