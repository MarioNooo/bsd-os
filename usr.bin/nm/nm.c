/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Hans Huebner.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)nm.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <a.out.h>
#include <stab.h>
#include <ar.h>
#include <ranlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int (*nlist_callback_fn) __P((const struct nlist *,
	const char *, void *));
extern int _nlist_apply __P((nlist_callback_fn, void *, const void *, size_t));

int callback __P((const struct nlist *, const char *, void *));
void *emalloc __P((size_t));
void *erealloc __P((void *, size_t));
void format_symbols __P((char *));
void print_symbol __P((char *, struct nlist *));
int process_file __P((char *));
int show_archive __P((char *, const void *, size_t));
int show_objfile __P((char *, const void *, size_t));
char *typestring __P((u_char));
char typeletter __P((u_char));
void usage __P((void));

#define	START_SYMBOLS	8192
#define	START_STRINGS	65536

struct nlist *symbols;
int any_symbols;
size_t next_symbol;
size_t last_symbol;
char *strings;
size_t last_string;
size_t next_string;

int ignore_bad_archive_entries = 1;
int print_only_external_symbols;
int print_only_undefined_symbols;
int print_all_symbols;
int print_file_each_line;
int fcount;

int rev;
int fname __P((const void *, const void *));
int rname __P((const void *, const void *));
int value __P((const void *, const void *));
int (*sfunc) __P((const void *, const void *)) = fname;

/* some macros for symbol type (nlist.n_type) handling */
#define	IS_DEBUGGER_SYMBOL(x)	((x) & N_STAB)
#define	IS_EXTERNAL(x)		((x) & N_EXT)
#define	SYMBOL_TYPE(x)		((x) & (N_TYPE | N_STAB))

/*
 * main()
 *	parse command line, execute process_file() for each file
 *	specified on the command line.
 */
int
main(argc, argv)
	int argc;
	char **argv;
{
	int ch, errors;

	while ((ch = getopt(argc, argv, "agnopruw")) != EOF) {
		switch (ch) {
		case 'a':
			print_all_symbols = 1;
			break;
		case 'g':
			print_only_external_symbols = 1;
			break;
		case 'n':
			sfunc = value;
			break;
		case 'o':
			print_file_each_line = 1;
			break;
		case 'p':
			sfunc = NULL;
			break;
		case 'r':
			rev = 1;
			break;
		case 'u':
			print_only_undefined_symbols = 1;
			break;
		case 'w':
			ignore_bad_archive_entries = 0;
			break;
		case '?':
		default:
			usage();
		}
	}
	fcount = argc - optind;
	argv += optind;

	if (rev && sfunc == fname)
		sfunc = rname;

	if (!fcount)
		errors = process_file("a.out");
	else {
		errors = 0;
		do {
			errors |= process_file(*argv);
		} while (*++argv);
	}
	exit(errors);
}

/*
 * process_file()
 *	show symbols in the file given as an argument.  Accepts archive and
 *	object files as input.
 */
int
process_file(fname)
	char *fname;
{
	int f;
	int retval;
	void *v;
	size_t size;
	struct stat st;
    
	if ((f = open(fname, O_RDONLY)) == -1) {
		(void)fprintf(stderr, "nm: cannot read %s: %s.\n", fname,
		    strerror(errno));
		return(1);
	}
	if (fstat(f, &st)) {
		(void)fprintf(stderr, "nm: stat(%s): %s.\n", fname,
		    strerror(errno));
		return(1);
	}
	if (st.st_size > SIZE_T_MAX) {
		(void)fprintf(stderr, "nm: %s: %s.\n", fname, strerror(EFBIG));
		return(1);
	}
	size = st.st_size;
	if ((v = mmap(NULL, size, PROT_READ, MAP_SHARED, f, 0)) == MAP_FAILED) {
		(void)fprintf(stderr, "nm: mmap(%s): %s.\n", fname,
		    strerror(errno));
		return(1);
	}

	if (fcount > 1)
		(void)printf("\n%s:\n", fname);

	if (size >= SARMAG && memcmp(v, ARMAG, SARMAG) == 0)
		retval = show_archive(fname, v, size);
	else
		retval = show_objfile(fname, v, size);
	munmap(v, size);
	close(f);
	return (retval);
}

/*
 * show_archive()
 *	show symbols in the given archive file
 */
int
show_archive(fname, v, size)
	char *fname;
	const void *v;
	size_t size;
{
	const char *base = v;
	const char *arhdr;
	const char *member_data;
	const struct ar_hdr *ar;
	const char *ar_name;
	const char *dict = NULL;
	size_t dictlen = 0;
	u_long ar_size;
	size_t namelen;
	size_t fullnamelen;
	unsigned long lnameoff;
	int rval;
	char *p, *name;
	size_t flen = strlen(fname);
	const size_t rllen = sizeof (RANLIBMAG) - 1;
	const size_t ar_namelen = sizeof (ar->ar_name);
#ifdef _STRICT_ALIGN
	void *copy;
#endif

	rval = 0;
	for (arhdr = base + SARMAG;
	    (member_data = arhdr + sizeof *ar) <= base + size;
	    arhdr += sizeof *ar + ar_size) {
		ar = (const struct ar_hdr *)arhdr;

		/* bad archive entry - stop processing this archive */
		if (memcmp(ar->ar_fmag, ARFMAG, sizeof(ar->ar_fmag))) {
			(void)fprintf(stderr,
			    "nm: %s: bad format archive header.\n", fname);
			rval = 1;
			break;
		}

		ar_size = roundup(strtoul(ar->ar_size, NULL, 10), 2);
		if (member_data + ar_size > base + size) {
			(void)fprintf(stderr, "nm: %s: bad archive\n", fname);
			rval = 1;
			break;
		}
		ar_name = ar->ar_name;

		/* Skip the table of contents.  */
		if (memcmp(ar_name, RANLIBMAG, rllen) == 0 &&
		    ar_name[rllen] == ' ')
			continue;
		if (ar_name[0] == '/' && ar_name[1] == ' ')
			continue;

		/* Handle special SVR4 members. */
		if (ar_name[0] == '/') {
			if (ar_name[1] == '/' && ar_name[2] == ' ') {
				/* This is the long name dictionary.  */
				dict = member_data;
				dictlen = ar_size;
				continue;
			}

			/* Handle a long name.  */
			if (dict == NULL) {
				(void)fprintf(stderr,
    "nm: %s: long archive member name without long name dictionary\n", fname);
				rval = 1;
				continue;
			}
			lnameoff = strtoul(&ar_name[1], NULL, 10);
			if (lnameoff >= dictlen)
				goto bad_long_name;
			ar_name = &dict[lnameoff];
			p = memchr(ar_name, '/', dictlen - lnameoff);
			if (p == NULL || p == ar_name) {
		bad_long_name:
				(void)fprintf(stderr,
    "nm: %s: corrupted long name dictionary in archive\n", fname);
				rval = 1;
				continue;
			}
			namelen = p - ar_name;
		} else {
			/*
			 * Normal archive member.
			 * XXX We don't support BSD-style long names.
			 */
			p = memchr(ar_name, '/', ar_namelen);
			if (p != NULL)
				namelen = p - ar_name;
			else {
				p = memchr(ar_name, ' ', ar_namelen);
				namelen = p != NULL ? p - ar_name : ar_namelen;
			}
		}

		/* Build a nul-terminated name string.  */
		fullnamelen = namelen;
		if (print_file_each_line)
			fullnamelen += flen + 1;
		p = name = emalloc(fullnamelen + 1);
		if (print_file_each_line) {
			memcpy(p, fname, flen);
			p += flen;
			*p++ = ':';
		}
		memcpy(p, ar_name, namelen);
		p[namelen] = '\0';

#ifdef _STRICT_ALIGN
		/*
		 * Enforce machine alignment on archive member data.
		 * Sometimes this may be overkill (e.g., 8-byte or 16-byte
		 * alignment on a 4-byte format), but there is no easy
		 * way to tell.
		 */
		copy = NULL;
		if (member_data != (const char *)ALIGN(member_data)) {
			if ((copy = malloc(ar_size)) == NULL) {
				(void)fprintf(stderr, "nm: %s(%s): %s.\n",
				    fname, p, strerror(errno));
				rval = 1;
				free(name);
				continue;	/* or break? */
			}
			memcpy(copy, member_data, ar_size);
			member_data = copy;
		}
#endif
		next_symbol = 0;
		next_string = 0;
		if (_nlist_apply(callback, NULL, member_data, ar_size) == -1) {
			if (!ignore_bad_archive_entries) {
				(void)fprintf(stderr, "nm: %s: bad format.\n",
				    name);
				rval = 1;
			}
		} else {
			if (!print_file_each_line)
				(void)printf("\n%s:\n", name);
			format_symbols(name);
		}
#ifdef _STRICT_ALIGN
		if (copy != NULL)
			free(copy);
#endif

		free(name);
	}
	return(rval);
}

/*
 * show_objfile()
 *	show symbols from the object file pointed to by fp.  The current
 *	file pointer for fp is expected to be at the beginning of an a.out
 *	header.
 */
int
show_objfile(objname, base, size)
	char *objname;
	const void *base;
	size_t size;
{

	any_symbols = 0;
	next_symbol = 0;
	next_string = 0;
	if (_nlist_apply(callback, NULL, base, size) == -1) {
		(void)fprintf(stderr, "nm: %s: %s\n", objname, strerror(errno));
		return(1);
	}

	if (!any_symbols) {
		(void)fprintf(stderr, "nm: %s: no name list.\n", objname);
		return(1);
	}

	format_symbols(objname);
	return (0);
}

/*
 * Process one symbol; called from nlist() routines.
 * The symbol and name may be synthetic, so we must copy them.
 * We don't use the callback argument because we don't need to be re-entrant.
 */
int
callback(np, name, unused)
	const struct nlist *np;
	const char *name;
	void *unused;
{
	struct nlist *p;
	size_t namelen;

	any_symbols = 1;
	if (!print_all_symbols && (IS_DEBUGGER_SYMBOL(np->n_type) ||
	    name == 0 || *name == '\0'))
		return (0);
	if (print_only_external_symbols && !IS_EXTERNAL(np->n_type))
		return (0);
	if (print_only_undefined_symbols && SYMBOL_TYPE(np->n_type) != N_UNDF)
		return (0);

	if (name)
		namelen = strlen(name) + 1;
	else {
		name = "";
		namelen = 1;
	}

	if (symbols == NULL) {
		last_symbol = START_SYMBOLS;
		symbols = emalloc(last_symbol * sizeof (struct nlist));
	}
	if (next_symbol == last_symbol) {
		last_symbol *= 2;
		symbols = erealloc(symbols,
		    last_symbol * sizeof (struct nlist));
	}

	if (strings == NULL) {
		last_string = START_STRINGS;
		strings = emalloc(last_string);
	}
	while (next_string + namelen > last_string) {
		last_string *= 2;
		strings = erealloc(strings, last_string);
	}

	p = &symbols[next_symbol++];
	p->n_un.n_strx = next_string;
	p->n_type = np->n_type;
	p->n_other = np->n_other;
	p->n_desc = np->n_desc;
	p->n_value = np->n_value;

	/*
	 * common symbols are characterized by a n_type of N_UNDF and a
	 * non-zero n_value -- change n_type to N_COMM for all such
	 * symbols to make life easier later.
	 */
	if (SYMBOL_TYPE(np->n_type) == N_UNDF && np->n_value)
		p->n_type = N_COMM | (np->n_type & N_EXT);

	memcpy(&strings[next_string], name, namelen);
	next_string += namelen;

	return (0);
}

void
format_symbols(objname)
	char *objname;
{
	struct nlist *np;
	size_t i;

	if (symbols == NULL)
		return;

	/* sort the symbol table if applicable */
	if (sfunc)
		qsort(symbols, next_symbol, sizeof (*symbols), sfunc);

	/* print out symbols */
	for (np = symbols, i = 0; i < next_symbol; np++, i++)
		print_symbol(objname, np);
}

/*
 * print_symbol()
 *	show one symbol
 */
void
print_symbol(objname, sym)
	char *objname;
	register struct nlist *sym;
{

	if (print_file_each_line)
		(void)printf("%s:", objname);

	/*
	 * handle undefined-only format seperately (no space is
	 * left for symbol values, no type field is printed)
	 */
	if (print_only_undefined_symbols) {
		puts(&strings[sym->n_un.n_strx]);
		return;
	}

	/* print symbol's value */
	if (SYMBOL_TYPE(sym->n_type) == N_UNDF)
		(void)printf("        ");
	else
		(void)printf("%08lx", sym->n_value);

	/* print type information */
	if (IS_DEBUGGER_SYMBOL(sym->n_type))
		(void)printf(" - %02x %04x %5s ", sym->n_other,
		    sym->n_desc&0xffff, typestring(sym->n_type));
	else
		(void)printf(" %c ", typeletter(sym->n_type));

	/* print the symbol's name */
	puts(&strings[sym->n_un.n_strx]);
}

/*
 * typestring()
 *	return the a description string for an STAB entry
 */
char *
typestring(type)
	register u_char type;
{
	switch(type) {
	case N_BCOMM:
		return("BCOMM");
	case N_ECOML:
		return("ECOML");
	case N_ECOMM:
		return("ECOMM");
	case N_ENTRY:
		return("ENTRY");
	case N_FNAME:
		return("FNAME");
	case N_FUN:
		return("FUN");
	case N_GSYM:
		return("GSYM");
	case N_LBRAC:
		return("LBRAC");
	case N_LCSYM:
		return("LCSYM");
	case N_LENG:
		return("LENG");
	case N_LSYM:
		return("LSYM");
	case N_PC:
		return("PC");
	case N_PSYM:
		return("PSYM");
	case N_RBRAC:
		return("RBRAC");
	case N_RSYM:
		return("RSYM");
	case N_SLINE:
		return("SLINE");
	case N_SO:
		return("SO");
	case N_SOL:
		return("SOL");
	case N_SSYM:
		return("SSYM");
	case N_STSYM:
		return("STSYM");
	}
	return("???");
}

/*
 * typeletter()
 *	return a description letter for the given basic type code of an
 *	symbol table entry.  The return value will be upper case for
 *	external, lower case for internal symbols.
 */
char
typeletter(type)
	u_char type;
{
	switch(SYMBOL_TYPE(type)) {
	case N_ABS:
		return(IS_EXTERNAL(type) ? 'A' : 'a');
	case N_BSS:
		return(IS_EXTERNAL(type) ? 'B' : 'b');
	case N_COMM:
		return(IS_EXTERNAL(type) ? 'C' : 'c');
	case N_DATA:
		return(IS_EXTERNAL(type) ? 'D' : 'd');
	case N_RODATA:
		return(IS_EXTERNAL(type) ? 'R' : 'r');
	case N_FN:
		return(IS_EXTERNAL(type) ? 'F' : 'f');
	case N_TEXT:
		return(IS_EXTERNAL(type) ? 'T' : 't');
	case N_UNDF:
		return(IS_EXTERNAL(type) ? 'U' : 'u');
	}
	return('?');
}

int
fname(a0, b0)
	const void *a0, *b0;
{
	const struct nlist *a = a0, *b = b0;

	return (strcmp(&strings[a->n_un.n_strx], &strings[b->n_un.n_strx]));
}

int
rname(a0, b0)
	const void *a0, *b0;
{
	const struct nlist *a = a0, *b = b0;

	return (strcmp(&strings[b->n_un.n_strx], &strings[a->n_un.n_strx]));
}

int
value(a0, b0)
	const void *a0, *b0;
{
	const struct nlist *a = a0, *b = b0;

	if (SYMBOL_TYPE(a->n_type) == N_UNDF)
		if (SYMBOL_TYPE(b->n_type) == N_UNDF)
			return(0);
		else
			return(-1);
	else if (SYMBOL_TYPE(b->n_type) == N_UNDF)
		return(1);
	if (rev) {
		if (a->n_value == b->n_value)
			return(rname(a0, b0));
		return(b->n_value > a->n_value ? 1 : -1);
	} else {
		if (a->n_value == b->n_value)
			return(fname(a0, b0));
		return(a->n_value > b->n_value ? 1 : -1);
	}
}

void *
emalloc(size)
	size_t size;
{
	char *p;

	if ((p = malloc(size)) != NULL)
		return(p);
	(void)fprintf(stderr, "nm: %s\n", strerror(errno));
	exit(1);
}

void *
erealloc(p, size)
	void *p;
	size_t size;
{

	if ((p = realloc(p, size)) != NULL)
		return(p);
	(void)fprintf(stderr, "nm: %s\n", strerror(errno));
	exit(1);
}

void
usage()
{
	(void)fprintf(stderr, "usage: nm [-agnopruw] [file ...]\n");
	exit(1);
}
