/*
 * Copyright (c) 1998 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 * 
 *	BSDI patchhdr.c,v 1.1 1998/09/12 02:46:26 torek Exp
 */

#include <sys/types.h>
#include <sys/elf.h>

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* elf64 field types */
enum e64type {
	EF_IDENT,	/* special */
	EF_HALF,	/* Elf64_Half */
	EF_WORD,	/* Elf64_Word */
	EF_ADDR,	/* etc. */
	EF_OFF,
	EF_XWORD,
	EF_SXWORD
};

struct field {
	const char *f_name;
	enum e64type f_type;
	unsigned f_off;
} fields[] = {
	{ "e_ident",	EF_IDENT,	offsetof(Elf64_Ehdr, e_ident) },
	{ "e_type",	EF_HALF,	offsetof(Elf64_Ehdr, e_type) },
#define IX_TYPE 1
	{ "e_machine",	EF_HALF,	offsetof(Elf64_Ehdr, e_machine) },
#define IX_MACHINE 2
	{ "e_version",	EF_WORD,	offsetof(Elf64_Ehdr, e_version) },
	{ "e_entry",	EF_ADDR,	offsetof(Elf64_Ehdr, e_entry) },
	{ "e_phoff",	EF_OFF,		offsetof(Elf64_Ehdr, e_phoff) },
	{ "e_shoff",	EF_OFF,		offsetof(Elf64_Ehdr, e_shoff) },
	{ "e_flags",	EF_WORD,	offsetof(Elf64_Ehdr, e_flags) },
	{ "e_ehsize",	EF_HALF,	offsetof(Elf64_Ehdr, e_ehsize) },
	{ "e_phentsize",EF_HALF,	offsetof(Elf64_Ehdr, e_phentsize) },
	{ "e_phnum",	EF_HALF,	offsetof(Elf64_Ehdr, e_phnum) },
	{ "e_shentsize",EF_HALF,	offsetof(Elf64_Ehdr, e_shentsize) },
	{ "e_shnum",	EF_HALF,	offsetof(Elf64_Ehdr, e_shnum) },
	{ "e_shstrndx",	EF_HALF,	offsetof(Elf64_Ehdr, e_shstrndx) },
	{ 0 }
};

struct value {
	const char *v_name;
	int	v_value;
};

struct value e_machines[] = {
	{ "oldsparc64",	11 },		/* XXX, but the point of it all */
	{ "none",	EM_NONE },
	{ "m32",	EM_M32 },
	{ "sparc",	EM_SPARC },
	{ "386",	EM_386 },
	{ "68k",	EM_68K },
	{ "88k",	EM_88K },
	{ "486",	EM_486 },
	{ "860",	EM_860 },
	{ "mips",	EM_MIPS },
	{ "mips_rs3_le",EM_MIPS_RS3_LE },
	{ "rs6000",	EM_RS6000 },
	{ "sparc32plus",EM_SPARC32PLUS },
	{ "ppc",	EM_PPC },
	{ "sparcv9",	EM_SPARCV9 },
	{ 0 }
};

struct value e_types[] = {
	{ "none",	ET_NONE },
	{ "rel",	ET_REL },
	{ "exec",	ET_EXEC },
	{ "dyn",	ET_DYN },
	{ "core",	ET_CORE },
	{ 0 }
};

void	show(const char *);
void	patch(const char *, int, char **);
int	readhdr(int, Elf64_Ehdr *);

const char *valtoname(struct value *, int);
int	nametoval(struct value *, const char *);

int
main(int argc, char **argv)
{
	char *file;

	if (argc < 2)
		errx(1, "usage: %s file field=val...", argv[0]);
	file = argv[1];
	if (argc == 2)
		show(file);
	else
		patch(file, argc - 2, argv + 2);
	exit(0);
}

void
show(const char *file)
{
	int fd, i;
	Elf64_Ehdr eh;

	if ((fd = open(file, O_RDONLY)) < 0 || readhdr(fd, &eh))
		err(1, "%s", file);
	printf("%s:\n", file);
	printf("\te_ident = { magic=");
	if (memcmp(eh.e_ident + EI_MAG0, ELFMAG, SELFMAG) == 0)
		printf("<ELF>");
	else
		printf("(%#x,%#x,%#x,%#x)",
		    eh.e_ident[EI_MAG0], eh.e_ident[EI_MAG1],
		    eh.e_ident[EI_MAG2], eh.e_ident[EI_MAG3]);
	printf(", class=");
	switch (eh.e_ident[EI_CLASS]) {
	case ELFCLASSNONE:
		printf("none");
		break;
	case ELFCLASS32:
		printf("class32");
		break;
	case ELFCLASS64:
		printf("class64");
		break;
	default:
		printf("%#x", eh.e_ident[EI_CLASS]);
		break;
	}
	printf(", data=");
	switch (eh.e_ident[EI_DATA]) {
	case ELFDATANONE:
		printf("none");
		break;
	case ELFDATA2LSB:
		printf("2lsb");
		break;
	case ELFDATA2MSB:
		printf("2msb");
		break;
	default:
		printf("%#x", eh.e_ident[EI_CLASS]);
		break;
	}
	printf(", version=%d },\n", eh.e_ident[EI_VERSION]);
	printf("\te_type = %s\n", valtoname(e_types, eh.e_type));
	printf("\te_machine = %s\n", valtoname(e_machines, eh.e_machine));

	/* loop starts at e_version */
	for (i = 3; fields[i].f_name; i++) {
		printf("\t%s = ", fields[i].f_name);
#define GET(ty) (*(ty *)((char *)&eh + fields[i].f_off))
		switch (fields[i].f_type) {
		case EF_HALF:
			printf("%#x\n", (u_int)GET(Elf64_Half));
			break;
		case EF_WORD:
			printf("%#lx\n", (u_long)GET(Elf64_Word));
			break;
		case EF_ADDR:
		case EF_OFF:
		case EF_XWORD:
			printf("%#qx\n", (u_quad_t)GET(Elf64_Xword));
			break;
		case EF_SXWORD:
			printf("%qd\n", (quad_t)GET(Elf64_Sxword));
			break;
		default:
			errx(1, "panic: bad field type %d\n",
			    (int)fields[i].f_type);
		}
	}
}

void
patch(const char *file, int nargs, char **args)
{
	int fd, i, fnum, v;
	u_quad_t uq;
	char *p1, *p2, *endp2;
	Elf64_Ehdr eh;

	if ((fd = open(file, O_RDWR)) < 0 || readhdr(fd, &eh))
		err(1, "%s", file);

	for (i = 0; i < nargs; i++) {
		p1 = args[i];
		p2 = strchr(p1, '=');
		if (p2 == NULL)
			errx(1, "assignments must be of the form name=val");
		*p2++ = 0;
		for (fnum = 0; fields[fnum].f_name; fnum++)
			if (strcasecmp(fields[fnum].f_name, p1) == 0)
				break;
		if (fields[fnum].f_name == NULL)
			errx(1, "don't know what `%s' is", p1);
		if (fields[fnum].f_type == EF_IDENT)
			errx(1, "don't know how to operate on ident");
		v = -1;
		if (fnum == IX_TYPE)
			v = nametoval(e_types, p2);
		else if (fnum == IX_MACHINE)
			v = nametoval(e_machines, p2);
		if (v == -1) {
			uq = strtouq(p2, &endp2, 0);
			if (endp2 == p2 || *endp2)
				errx(1, "don't understand %s for %s", p2, p1);
		} else
			uq = v;
#define SET(ty) (*(ty *)((char *)&eh + fields[fnum].f_off)) = uq
		switch (fields[fnum].f_type) {
		case EF_HALF:
			SET(Elf64_Half);
			break;
		case EF_WORD:
			SET(Elf64_Word);
			break;
		case EF_ADDR:
		case EF_OFF:
		case EF_XWORD:
		case EF_SXWORD:
			SET(Elf64_Xword);
			break;
		default:
			errx(1, "panic: bad field type %d\n",
			    (int)fields[fnum].f_type);
		}
	}
	(void)lseek(fd, (off_t)0, 0);
	if (write(fd, &eh, sizeof eh) != sizeof eh)
		err(1, "%s: write");
}

int
readhdr(int fd, Elf64_Ehdr *eh)
{

	if (read(fd, eh, sizeof *eh) != sizeof *eh)
		return -1;
	return 0;
}

const char *
valtoname(struct value *vp, int val)
{
	static char buf[32];

	for (; vp->v_name; vp++)
		if (vp->v_value == val)
			return (vp->v_name);
	snprintf(buf, sizeof buf, "%#x", val);
	return (buf);
}

int
nametoval(struct value *vp, const char *name)
{

	for (; vp->v_name; vp++)
		if (strcasecmp(name, vp->v_name) == 0)
			return (vp->v_value);
	return (-1);
}
