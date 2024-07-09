/*
 * Copyright (c) 1993, 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI vgafont.c,v 2.6 1996/08/21 21:22:18 bostic Exp
 */

/*
 * Load font to EGA or VGA
 *
 * Format of the font file:
 *
 *      # comment
 *      hex-char-code: hex-byte hex-byte hex-byte...
 */

#include <sys/types.h>
#include <sys/mman.h>

#include <time.h>
#include <sys/resource.h>

#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pathnames.h"

int	gethex __P((char **));
int	inb __P((u_int));
void	outb __P((u_int, int));
void	resetvga __P((void));
void	setvga __P((void));

int   src_mm;

#define	VGA_SIZE	(64 * 1024)
int   vgafd;            /* vga file descriptor */
char *vgabase;          /* vga base memory address */

int
main(argc, argv)
	int argc;
	char *argv[];
{
	FILE *ff;
	sigset_t set;
	uid_t euid;
	int ccode, fd, lineno, nbytes;
	char *fontname, *p, *pl, fn[PATH_MAX], iline[LINE_MAX];

	if (argc > 2)
		errx(1, "usage: vgafont [fontname]");

	fontname = NULL;
	if (argc == 2)
		fontname = argv[1];
	else {
		fontname = getenv("VGAFONT");
		if (fontname == NULL)
			fontname = "default";
	}

	if (strchr(fontname, '/') == NULL) {
		(void)snprintf(fn, sizeof(fn),
		    "%s/%s", _PATH_VGAFONT, fontname);
		fontname = fn;
	}

	/*
	 * Check permissions.  The user should be able to open the console
	 * and read the font file.
	 */
	euid = geteuid();
	if (seteuid(getuid()))
		err(1, "seteuid: %lu", (u_long)getuid());

	if ((fd = open(_PATH_CONSKBD, O_RDWR, 0)) == -1 &&
	    (fd = open(_PATH_CONSOLE, O_RDWR, 0)) == -1)
		err(1, "%s", _PATH_CONSOLE);
	(void)close(fd);

	if ((ff = fopen(fontname, "r")) == NULL)
		err(1, "%s", fontname);

	if (seteuid(euid))
		err(1, "seteuid: %lu", (u_long)euid);

	/* Block signals and drop the priority, we want to go to completion. */
	(void)setpriority(PRIO_PROCESS, 0, -20);
	sigfillset(&set);
	(void)sigprocmask(SIG_BLOCK, &set, NULL);

	setvga();

	/*
	 * Read the font file
	 */
	for (lineno = 1;
	    (p = fgets(iline, sizeof(iline), ff)) != NULL; ++lineno) {
		ccode = gethex(&p);
		if (ccode == -1) {
			while (*p == ' ' || *p == '\t')
				p++;
			if (*p == '#' || *p == '\0' || *p == '\n')
				continue;

syntax:			resetvga();
			errx(1, "syntax error: %s, line %d", fontname, lineno);
		}
		if (ccode & ~0xff) {
			resetvga();
			errx(1, "bad character code %x: %s, line %d",
			    ccode, fontname, lineno);
		}
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p++ != ':')
			goto syntax;

		pl = vgabase + ccode * 32;

		nbytes = 0;
		while ((ccode = gethex(&p)) != -1) {
			if (++nbytes > 32) {
				resetvga();
				errx(1,
				    "too many rows per character: %s, line %d",
				    fontname, lineno);
			}
			if (ccode & ~0xff) {
				resetvga();
				errx(1,
			    "too many columns per character: %s, line %d",
				    fontname, lineno);
			}
			*pl++ = ccode;
		}
		while (nbytes++ < 32)
			*pl++ = 0;
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p != '#' && *p != '\0' && *p != '\n')
			goto syntax;
	}
	resetvga();
	exit(0);
}

int
inb(port)
	u_int port;
{
	u_char x;

	asm volatile("inb %1, %0" : "=a" (x) : "d" ((short)port));
	return ((int) x);
}

void
outb(port, val)
	u_int port;
	int val;
{
	asm volatile("outb %0, %1" : : "a" ((char)val), "d" ((short)port));
}

/*
 * Enable font loading mode
 */
void
setvga()
{
	int vgafd;

	vgafd = open(_PATH_VGA, O_RDWR, 0);
	if (vgafd < 0)
		err(1, "%s", _PATH_VGA);

	vgabase = mmap(NULL,
	    VGA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, vgafd, 0);
	if (vgabase == (caddr_t) -1)
		err(1, "cannot map VGA memory");

	/* Read GC memory mode register */
	outb(0x3ce, 0x6);
	src_mm = inb(0x3cf);
	if (src_mm == 0xff)
		src_mm = 0xe;   /* EGA, sigh; assume color monitor  */

	/* Relocate VGA memory to 64K at a0000 */
	outb(0x3ce, 0x6);       /* GC Misc register */
	outb(0x3cf, 0x4);

	/* Turn on sequential access to the VRAM */
	outb(0x3c4, 0x4);       /* SEQ Memory Mode register */
	outb(0x3c5, 0x6);

	/* Enable memory plane 2 */
	outb(0x3c4, 0x2);       /* SEQ Plane Enable register */
	outb(0x3c5, 0x4);
}

/*
 * Disable font loading mode
 */
void
resetvga()
{
	/* Enable memory planes 0 and 1 */
	outb(0x3c4, 0x2);       /* SEQ Plane Enable register */
	outb(0x3c5, 0x3);

	/* Turn off sequential access to the VRAM */
	outb(0x3c4, 0x4);       /* SEQ Memory Mode register */
	outb(0x3c5, 0x2);

	/* Relocate VGA memory to 32K at b8000 */
	outb(0x3ce, 0x6);       /* GC Misc register */
	outb(0x3cf, src_mm);

	close(vgafd);
}

/*
 * Get a hexadecimal number
 */
int
gethex(pp)
	char **pp;
{
	register int n = 0;
	register int c;
	register char *s;

	s = *pp;
	while (*s == ' ' || *s == '\t')
		s++;
	for (;;) {
		c = *s;
		if (c >= '0' && c <= '9')
			n = (c - '0') | (n << 4);
		else if(c >= 'A' && c <= 'F')
			n = (c - 'A' + 10) | (n << 4);
		else if(c >= 'a' && c <= 'f')
			n = (c - 'a' + 10) | (n << 4);
		else {
			if (s == *pp)
				n = -1;
			*pp = s;
			return (n);
		}
		s++;
	}
	/* NOTREACHED */
}
