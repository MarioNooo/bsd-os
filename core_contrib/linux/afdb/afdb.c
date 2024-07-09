/*	BSDI afdb.c,v 1.1.1.1 1998/12/23 19:51:06 prb Exp	*/

/*
 * Build function name to address and address to function name databases
 * for the template transform program.  We do this because it is complex
 * and error-prone to make the transform sources specify the real names
 * and weak aliases for the different libraries, since the libraries don't
 * follow reliable naming patterns.
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/elf.h>
#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int do_file(const char *, int);
void do_sht(caddr_t, Elf32_Shdr *, size_t, size_t, size_t);
void do_dynsym(Elf32_Sym *, size_t, size_t, const char *);
size_t atof_prefix(const DBT *, const DBT *);

char *ftoa_db_name;
DB *ftoa_db;
char *atof_db_name;
DB *atof_db;
BTREEINFO bt = { R_DUP, 0, 0, 0, 0, NULL, atof_prefix, 0 };

#define	N_BADMAG(e) \
	((e)->e_ident[EI_MAG0] != ELFMAG0 || \
	 (e)->e_ident[EI_MAG1] != ELFMAG1 || \
	 (e)->e_ident[EI_MAG2] != ELFMAG2 || \
	 (e)->e_ident[EI_MAG3] != ELFMAG3 || \
	 ((e)->e_ident[EI_DATA] != ELFDATA2LSB && \
	  BYTE_ORDER == LITTLE_ENDIAN) || \
	 ((e)->e_ident[EI_DATA] != ELFDATA2MSB && \
	  BYTE_ORDER == BIG_ENDIAN))

int
main(int argc, char **argv)
{
	int r = 0;
	int f;

	while (*++argv)
		if ((f = open(*argv, O_RDONLY)) != -1) {
			r |= do_file(*argv, f);
			close(f);
		} else {
			r |= 1;
			warn("%s", *argv);
		}

	return (r);
}

int
do_file(const char *name, int f)
{
	Elf32_Ehdr *e;
	struct stat st;
	size_t size;
	caddr_t base, top;
	char *s;
	int save_errno;

	/*
	 * Map the file.
	 * First we check whether the file is too big to map...
	 */
	if (fstat(f, &st) < 0) {
		warn("%s", name);
		return (1);
	}
	if (st.st_size > SIZE_T_MAX) {
		warnx("%s: %s", name, strerror(EFBIG));
		return (1);
	}
	size = (size_t)st.st_size;

	if ((base = mmap(NULL, size, PROT_READ, 0, f, 0)) == (caddr_t)-1) {
		warn("%s", name);
		return (1);
	}
	top = base + size;

	/*
	 * Perform some sanity checking on the file header.
	 */
	e = (Elf32_Ehdr *)base;
	if (size < sizeof (*e) ||
	    N_BADMAG(e)) {
		warnx("%s: %s", name, strerror(EFTYPE));
		munmap(base, size);
		return (1);
	}

	/*
	 * Create the database output files.
	 */
	if ((s = strrchr(name, '/')) != NULL)
		++s;
	else
		s = (char *)name;
	if ((ftoa_db_name = malloc(strlen(s) + sizeof (".ftoa.db"))) == NULL)
		err(1, NULL);
	strcpy(ftoa_db_name, s);
	strcat(ftoa_db_name, ".ftoa.db");
	if ((atof_db_name = malloc(strlen(s) + sizeof (".atof.db"))) == NULL)
		err(1, NULL);
	strcpy(atof_db_name, s);
	strcat(atof_db_name, ".atof.db");
	ftoa_db = dbopen(ftoa_db_name, O_RDWR|O_CREAT|O_TRUNC|O_EXLOCK,
	    0666, DB_HASH, NULL);
	if (ftoa_db != NULL)
		atof_db = dbopen(atof_db_name, O_RDWR|O_CREAT|O_TRUNC|O_EXLOCK,
		    0666, DB_BTREE, &bt);
	if (ftoa_db == NULL || atof_db == NULL) {
		save_errno = errno;
		if (ftoa_db != NULL)
			ftoa_db->close(ftoa_db);
		unlink(ftoa_db_name);
		unlink(atof_db_name);
		errno = save_errno;
		warn("%s", ftoa_db == NULL ? ftoa_db_name : atof_db_name);
		return (1);
	}

	/*
	 * Process the file, looking for the dynamic linking symbols.
	 */
	if (e->e_shnum)
		do_sht(base, (Elf32_Shdr *)(base + e->e_shoff),
		    e->e_shentsize, e->e_shnum, e->e_shstrndx);
	else
		warnx("%s: no section header table", name);

	/*
	 * Clean up.
	 */
	munmap(base, size);
	ftoa_db->close(ftoa_db);
	atof_db->close(atof_db);
	free(ftoa_db_name);
	free(atof_db_name);

	return (0);
}

void
do_sht(caddr_t base, Elf32_Shdr *sht, size_t entsize, size_t num, size_t strndx)
{
	Elf32_Shdr *sh = sht;
	Elf32_Shdr *shstr = (Elf32_Shdr *)((caddr_t)sh + entsize * strndx);
	size_t i;

	sh = (Elf32_Shdr *)((caddr_t)sh + entsize);	/* skip null section */
	for (i = 1; i < num; ++i, sh = (Elf32_Shdr *)((caddr_t)sh + entsize)) {
		if (sh->sh_type == SHT_DYNSYM)
			do_dynsym((Elf32_Sym *)(base + sh->sh_offset),
			    sh->sh_entsize, sh->sh_size,
			    base + ((Elf32_Shdr *)((caddr_t)sht +
			    sh->sh_link * entsize))->sh_offset);
	}
}

void
do_dynsym(Elf32_Sym *symt, size_t entsize, size_t size, const char *symnames)
{
	DBT key;
	DBT data;
	Elf32_Sym *sym;
	char *s;
	int binding;
	int type;

	for (sym = (Elf32_Sym *)((caddr_t)symt + entsize);
	    (caddr_t)sym < (caddr_t)symt + size;
	    sym = (Elf32_Sym *)((caddr_t)sym + entsize)) {

		binding = ELF32_ST_BIND(sym->st_info);
		if (binding != STB_GLOBAL && binding != STB_WEAK)
			continue;
		type = ELF32_ST_TYPE(sym->st_info);
		if (type != STT_FUNC && type != STT_NOTYPE)
			continue;

		/* Map the function name to the address.  */
		s = (char *)symnames + sym->st_name;
		key.data = s;
		key.size = strlen(s);
		data.data = &sym->st_value;
		data.size = sizeof (sym->st_value);
		if (ftoa_db->put(ftoa_db, &key, &data, 0) != 0) {
			warn("%s", ftoa_db_name);
			break;
		}

		/* Map the address to the binding plus the function name.  */
		data.size = key.size + 1;
		if ((data.data = malloc(data.size)) == NULL) {
			warn(NULL);
			break;
		}
		if (binding == STB_GLOBAL)
			*(char *)data.data = 'G';
		else
			*(char *)data.data = 'W';
		memcpy((char *)data.data + 1, key.data, key.size);
		key.data = &sym->st_value;
		key.size = sizeof (sym->st_value);
		if (atof_db->put(atof_db, &key, &data, 0) != 0) {
			warn("%s", atof_db_name);
			break;
		}
		free(data.data);
	}

	if ((caddr_t)sym < (caddr_t)symt + size) {
		ftoa_db->close(ftoa_db);
		atof_db->close(atof_db);
		unlink(ftoa_db_name);
		unlink(atof_db_name);
		exit(1);
	}
}

size_t
atof_prefix(const DBT *key1, const DBT *key2)
{

	return (sizeof (Elf32_Addr));
}
