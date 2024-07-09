/*-
 * Copyright (c) 1992, 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI checkio.c,v 1.1.1.1 1997/12/08 21:05:04 ewv Exp
 */

#include <sys/types.h>
#include <sys/mman.h>

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define	PATH_IOMEM	"/dev/vga"	/* XXX */
#define	HOLE_BASE 	0xA0000
#define	FILE_BASE 	0xA0000		/* /dev/vga starts here */
#define	BIOS_BASE 	0xF0000		/* skipping main BIOS ROM */
#define	HOLE_LEN	(0x100000 - HOLE_BASE)
#define	TEST_INTERVAL	2048		/* check every 2K, as main BIOS does */

#define	CHECKBYTES	4	/* bytes to check for type */
#define	MSG_SRCH 	512	/* bytes of rom to search for strings */
#define	MINSTR 		6	/* min. string length */

u_char	*iomem;
int	do_write = -1;
int	dflag;
int	vflag;
char	*dumpfile;

enum type { NONE, BIOS_ROM, BIOS_ROM_SHADOWED, ROMRAM, ROM, RAM };

char *typenames[] = {
	"nothing", "BIOS ROM", "BIOS ROM (shadowed)", "ROM or RAM",
	"ROM", "RAM or shadowed ROM"
};

struct {
	enum	type type;
	int	addr;
	u_char	*cp;
} cur = { NONE };

int	check_mem __P((u_char *, int, int *));
void	print __P((enum type, int, int, u_char *));
void	print_strings __P((u_char *, int));
void	usage __P((char *));
int	writable __P((u_char *));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern int optind;
	extern char *optarg;
	int addr, ch, fd, len, mem, type, ramstart;
	u_char *cp;

	ramstart = 0;
	while ((ch = getopt(argc, argv, "df:rw")) != EOF)
		switch (ch) {
		case 'd':
			dflag = 1;
			break;
		case 'f':
			dumpfile = optarg;
			break;
		case 'r':
			do_write = 0;
			break;
#ifdef maybe
		case 'v':
			vflag = 1;
			break;
#endif
		case 'w':
			do_write = 1;
			break;
		default:
			usage(argv[0]);
			/* NOTREACHED */
		}

	argc -= optind;
	argv += optind;

	if (do_write == -1)
		errx(1, "either -r or -w must be specified");

	if ((mem = open(PATH_IOMEM, O_RDWR, 0)) == -1)
		err(1, "%s", PATH_IOMEM);

	if ((iomem = (u_char *)mmap(NULL, HOLE_LEN, PROT_READ|PROT_WRITE,
	    MAP_FILE|MAP_SHARED, mem, HOLE_BASE - FILE_BASE)) == (u_char *)-1)
		err(1, "mmap iomem");
	if (dumpfile) {
		if ((fd =
		    open(dumpfile, O_WRONLY|O_CREAT|O_TRUNC, 0666)) == -1)
			err(1, "%s", dumpfile);
		write(fd, iomem, HOLE_LEN);
		close(fd);
	}
	close(mem);

#define	CONT(n)	{ cp += (n); addr += (n); continue; }
	for (cp = iomem, addr = HOLE_BASE; cp < iomem + HOLE_LEN; ) {
		type = check_mem(cp, addr, &len);
		if (type != cur.type) {
			if (cur.type != NONE)
				print(cur.type, cur.addr, addr - cur.addr,
				    cur.cp);
			cur.type = NONE;
		}
		if (len) {
			print(type, addr, len, cp);
			cur.type = NONE;
			CONT(len);
		}
		if (cur.type == NONE) {
			cur.type = type;
			cur.addr = addr;
			cur.cp = cp;
		}
		CONT(TEST_INTERVAL);
	}
	if (cur.type != NONE)
		print(cur.type, cur.addr, addr - cur.addr, cur.cp);
	exit(0);
}

void
usage(myname)
	char *myname;
{

	fprintf(stderr, "usage: %s -r | -w [-d] [-f outfile]\n", myname);
	exit(1);
}

void
print(type, addr, len, cp)
	enum type type;
	int addr, len;
	u_char *cp;
{

	printf("\n%s at 0x%x - 0x%x (%dK):\n",
	    typenames[type], addr, addr + len - 1, len / 1024);
	print_strings(cp, MSG_SRCH);
}

int
check_mem(cp, addr, plen)
	u_char *cp;
	int addr, *plen;
{
	enum type type = NONE;
	int i;

	if (dflag) {
		fprintf(stderr, "0x%x:", addr);
		for (i = 0; i < CHECKBYTES; i++)
			fprintf(stderr, " %x", cp[i]);
		fprintf(stderr, "\n");
	}
	/*
	 * Check for inititialized BIOS signature.
	 *
	 * XXX
	 * Should check checksum, but I haven't looked up the checksum
	 * algorithm.
	 */
	if (cp[0] == 0x55 && cp[1] == 0xaa) {
		*plen = cp[2] * 512;
		return (writable(cp) ? BIOS_ROM_SHADOWED : BIOS_ROM);
	}
	/*
	 * At least one BIOS seems to start with 0xaa, 0x55
	 * and a size (32K); this may not be significant.
	 */
	if (addr >= BIOS_BASE && cp[0] == 0xaa && cp[1] == 0x55) {
		*plen = cp[2] * 512;
		return (writable(cp) ? BIOS_ROM_SHADOWED : BIOS_ROM);
	}

	/*
	 * Empty/unused areas generally read as 0xff, although some
	 * read as 0x7f (or 0x7f in every other byte).
	 */
	for (i = 0; i < CHECKBYTES; i++)
		if (cp[i] != 0xff && cp[i] != 0x7f)
			type = ROMRAM;

	if (do_write) {
		if (writable(cp))
			type = RAM;
		else if (type == ROMRAM)
			type = ROM;
	}
	*plen = 0;
	return (type);
}

#ifdef unused
check_strings(sp, len)
	u_char *sp;
	int len;
{
	u_char *ep, *first;

	first = NULL;
	ep = sp + len;
	for (; sp < ep; sp++) {
		if (isascii(*sp) && isprint(*sp)) {
			if (first == NULL)
				first = sp;
			continue;
		} else {
			if (first && sp - first > MINSTR)
				return (1);
			first = NULL;
		}
	}
	return (first && sp - first > MINSTR);
}
#endif

void
print_strings(sp, len)
	u_char *sp;
	int len;
{
	u_char *ep, *first;

#ifdef maybe
	if (vflag == 0)
		return;
#endif
	first = NULL;
	ep = sp + len;
	for (; sp < ep; sp++) {
		if (isascii(*sp) && isprint(*sp)) {
			if (first == NULL)
				first = sp;
			continue;
		} else {
			if (first && sp - first > MINSTR)
				printf("%.*s\n", sp - first, first);
			first = NULL;
		}
	}
	if (first && sp - first > MINSTR)
		printf("%.*s\n", sp - first, first);
}

#define	CHECKWRITE	4
int
writable(cp)
	u_char *cp;
{
	int i, write;
	u_char save[CHECKWRITE];

	if (do_write == 0)
		return (0);

	write = 1;
	for (i = 0; i < CHECKWRITE; i++)
		save[i] = cp[i];
	for (i = 0; i < CHECKWRITE; i++)
		cp[i] = save[i] ^ 0xff;
	for (i = 0; i < CHECKWRITE; i++)
		if (cp[i] != (save[i] ^ 0xff))
			write = 0;
	for (i = 0; i < CHECKWRITE; i++)
		cp[i] = save[i];
	return (write);
}
