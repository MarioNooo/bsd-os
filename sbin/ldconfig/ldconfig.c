/*	BSDI ldconfig.c,v 1.2 2002/11/19 22:43:31 fmayhar Exp	*/

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <db.h>
#include <dirent.h>
#include <elf.h>
#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pathnames.h"

struct directory_list {
	TAILQ_ENTRY(directory_list) next;
	char *directory;
};
TAILQ_HEAD(directory_head, directory_list) directory_head;

struct file_info {
	mode_t mode;
	char link_name[PATH_MAX + 1];
};

struct target {
	char *link_name;
	char *expansion;
};

int debug = 0;
DB *file_hash_db = NULL;
DB *root_library_db = NULL;
DB *soname_db = NULL;
int suppress_linking = 0;
int verbose = 0;

void add_target(struct target *, const char *, DB *);
void create_link(const char *, const char *, const char *);
char *dir_prefix(const char *, const char *);
char *expand(const char *);
char *get_soname(const char *, const char *);
void init_databases(void);
struct file_info *lookup_file(const char *);
struct target *lookup_target(const char *, DB *);
void process_directory(DIR *, const char *);
void read_argument_list(const char *, char **);
void read_configure_file(const char *, const char *);
int record_file(const char *, const char *);
void usage(void);

void
usage()
{

	fprintf(stderr, "usage: ldconfig -[DXnv] [-f configure-file] [-r root-dir]\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	DIR *dirp;
	struct directory_list *dlp;
	char *configure_file = _PATH_LD_SO_CONF;
	char *root_directory = "";
	int only_command_line_directories = 0;
	int c;

	while ((c = getopt(argc, argv, "C:DNXf:lnpr:v")) != -1)
		switch (c) {

		case 'C':
			warnx("-C flag not supported");
			/* usage(); */
			break;

		case 'D':
			++debug;
			verbose = 1;
			suppress_linking = 1;
			break;

		case 'N':
			warnx("-N flag not supported");
			/* usage(); */
			break;

		case 'X':
			suppress_linking = 1;
			break;

		case 'f':
			configure_file = optarg;
			break;

		case 'l':
			warnx("-l flag not supported");
			usage();
			break;

		case 'n':
			only_command_line_directories = 1;
			break;

		case 'p':
			warnx("-p flag not supported");
			usage();
			break;

		case 'r':
			root_directory = optarg;
			break;

		case 'v':
			verbose = 1;
			break;

		default:
			usage();
			break;
		}
	argv += optind;

	if (debug)
		setlinebuf(stdout);

	TAILQ_INIT(&directory_head);
	if (!only_command_line_directories)
		read_configure_file(root_directory, configure_file);
	read_argument_list(root_directory, argv);

	TAILQ_FOREACH(dlp, &directory_head, next) {
		if ((dirp = opendir(dlp->directory)) == NULL) {
			if (verbose)
				warn("%s", dlp->directory);
			continue;
		}
		process_directory(dirp, dlp->directory);
		closedir(dirp);
	}

	return (0);
}

char *
dir_prefix(const char *prefix, const char *dir_name)
{
	char *buf;

	if (*prefix == '\0')
		return (strdup(dir_name));

	if ((buf = malloc(strlen(prefix) + strlen(dir_name) + 2)) == NULL)
		err(1, NULL);

	strcpy(buf, prefix);
	strcat(buf, "/");
	strcat(buf, dir_name);
	return (buf);
}

void
read_configure_file(const char *root_directory, const char *configure_file)
{
	struct directory_list *dlp;
	FILE *f;
	char *s;
	size_t len;

	if ((f = fopen(configure_file, "r")) == NULL)
		err(1, "%s", configure_file);

	while ((s = fgetln(f, &len)) != NULL) {
		if (len > 0 && s[len - 1] == '\n')
			--len;
		s[len] = '\0';

		if ((dlp = malloc(sizeof (*dlp))) == NULL)
			err(1, NULL);
		dlp->directory = dir_prefix(root_directory, s);
		TAILQ_INSERT_TAIL(&directory_head, dlp, next);
		if (debug > 0)
			warnx("added directory %s", dlp->directory);
	}

	if (ferror(f))
		err(1, "%s", configure_file);
	fclose(f);
}

void
read_argument_list(const char *root_directory, char **argv)
{
	struct directory_list *dlp;

	for (; *argv != NULL; ++argv) {
		if ((dlp = malloc(sizeof (*dlp))) == NULL)
			err(1, NULL);
		dlp->directory = dir_prefix(root_directory, *argv);
		TAILQ_INSERT_TAIL(&directory_head, dlp, next);
		if (debug > 0)
			warnx("added directory %s", dlp->directory);
	}
}

void
process_directory(DIR *dirp, const char *dir_name)
{
	DBT key;
	DBT data;
	struct dirent *dp;
	const char *cp;
	char *root_library;
	char *soname;
	char *expansion;
	struct target *st;
	struct target *rt;
	int r;

	if (verbose || suppress_linking)
		printf("%s:\n", dir_name);

	if (chdir(dir_name) == -1) {
		if (verbose)
			warn("%s", dir_name);
		return;
	}

	init_databases();

	for (dp = readdir(dirp); dp; dp = readdir(dirp)) {

		/* Discard names that don't look like shared libraries.  */
		if (dp->d_namlen < strlen("lib") + sizeof (".so") ||
		    strncmp(dp->d_name, "lib", strlen("lib")) != 0 ||
		    (cp = strstr(dp->d_name, ".so")) == NULL) {
			if (debug)
				warnx("ignoring %s/%s",
				    dir_name, dp->d_name);
			continue;
		}

		/* Record any name that looks like a shared library.  */
		if (record_file(dp->d_name, dir_name) == -1)
			continue;

		/* Extract the shared object name from the file.  */
		if ((soname = get_soname(dp->d_name, dir_name)) == NULL)
			continue;

		/* Compute the 'root library name'.  */
		cp += strlen (".so");
		if ((root_library = malloc(cp + 1 - dp->d_name)) == NULL)
			err(1, NULL);
		memcpy(root_library, dp->d_name, cp - dp->d_name);
		root_library[cp - dp->d_name] = '\0';

		/* If the soname doesn't match the root name, skip it.  */
		if (strncmp(soname, root_library, cp - dp->d_name) != 0) {
			if (debug)
				warnx("%s/%s: soname doesn't match library name",
					dir_name, dp->d_name);
			free(root_library);
			continue;
		}

		/*
		 * If we've seen this soname before,
		 * compare this filename to the 'highest ranking' one
		 * that we've seen so far for this soname.
		 * 'Highest ranking' compares the strings numerically
		 * on decimal numeric substrings, lexicographically otherwise.
		 * If the new filename is higher ranking,
		 * replace the old values and continue.
		 */
		if ((st = lookup_target(soname, soname_db)) != NULL) {
			expansion = expand(dp->d_name);
			if (strcmp(expansion, st->expansion) > 0) {
				if (debug)
					warnx("replacing soname target %s with %s",
					    st->link_name, dp->d_name);
				free(st->link_name);
				if ((st->link_name = strdup(dp->d_name)) == NULL)
					err(1, NULL);
				free(st->expansion);
				st->expansion = expansion;
			} else
				free(expansion);
			free(root_library);
			continue;
		}

		if (debug)
			warnx("creating soname record %s => %s", soname,
			    dp->d_name);

		/* Create a new soname record.  */
		if (strcmp(soname, dp->d_name) != 0) {
			if ((st = malloc(sizeof (*st))) == NULL)
				err(1, NULL);
			if ((st->link_name = strdup(dp->d_name)) == NULL)
				err(1, NULL);
			st->expansion = expand(dp->d_name);
			add_target(st, soname, soname_db);
		}

		/*
		 * Update the root library link information.
		 * The comparison is similar to the file name comparison above.
		 */
		if ((rt = lookup_target(root_library, root_library_db)) != NULL) {
			expansion = expand(soname);
			if (strcmp(expansion, rt->expansion) > 0) {
				if (debug)
					warnx("replacing root library target %s with %s",
					    st->link_name, soname);
				free(rt->link_name);
				if ((rt->link_name = strdup(soname)) == NULL)
					err(1, NULL);
				free(rt->expansion);
				rt->expansion = expansion;
			} else
				free(expansion);
			free(root_library);
			continue;
		}

		if (debug)
			warnx("creating root library record %s => %s",
			    root_library, soname);

		/* Create a new root library record.  */
		if (strcmp(root_library, soname) != 0) {
			if ((rt = malloc(sizeof (*rt))) == NULL)
				err(1, NULL);
			if ((rt->link_name = strdup(soname)) == NULL)
				err(1, NULL);
			rt->expansion = expand(soname);
			add_target(rt, root_library, root_library_db);
		}

		free(root_library);
	}

	/* Generate the symlinks for the root library names.  */
	for (r = root_library_db->seq(root_library_db, &key, &data, R_FIRST);
	    r == 0;
	    r = root_library_db->seq(root_library_db, &key, &data, R_NEXT)) {
		rt = (struct target *)data.data;
		create_link(key.data, rt->link_name, dir_name);
		free(rt->link_name);
		free(rt->expansion);
	}

	/* Generate the symlinks for the sonames.  */
	for (r = soname_db->seq(soname_db, &key, &data, R_FIRST);
	    r == 0;
	    r = soname_db->seq(soname_db, &key, &data, R_NEXT)) {
		if (lookup_target(key.data, root_library_db) != NULL)
			continue;
		st = (struct target *)data.data;
		create_link(key.data, st->link_name, dir_name);
		free(st->link_name);
		free(st->expansion);
	}
}

void
init_databases()
{

	if (file_hash_db != NULL)
		file_hash_db->close(file_hash_db);
	if ((file_hash_db = dbopen(NULL, O_RDWR, 0, DB_HASH, NULL)) == NULL)
		err(1, "db error");

	/* We use btrees here simply to make the verbose output look pretty.  */
	if (root_library_db != NULL)
		root_library_db->close(root_library_db);
	if ((root_library_db = dbopen(NULL, O_RDWR, 0, DB_BTREE, NULL)) == NULL)
		err(1, "db error");

	if (soname_db != NULL)
		soname_db->close(soname_db);
	if ((soname_db = dbopen(NULL, O_RDWR, 0, DB_BTREE, NULL)) == NULL)
		err(1, "db error");
}

int
record_file(const char *name, const char *dir_name)
{
	DBT key;
	DBT data;
	struct file_info fi;
	struct stat st;
	int len;

	if (debug)
		warnx("recording %s/%s", dir_name, name);

	if (lstat(name, &st) == -1) {
		if (verbose)
			warn("%s/%s", dir_name, name);
		return (-1);
	}

	if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode)) {
		if (debug)
			warnx("rejecting %s/%s, neither regular file nor link",
			    dir_name, name);
		return (-1);
	}

	fi.mode = st.st_mode;

	if (S_ISLNK(st.st_mode)) {
		if ((len = readlink(name, fi.link_name, PATH_MAX)) == -1) {
			if (verbose)
				warn("%s/%s", dir_name, name);
			return (-1);
		}
		fi.link_name[len++] = '\0';
	} else {
		fi.link_name[0] = '\0';
		len = 1;
	}

	key.data = (char *)name;
	key.size = strlen(name) + 1;
	data.data = &fi;
	data.size = (caddr_t)&fi.link_name[len] - (caddr_t)&fi;
	if (file_hash_db->put(file_hash_db, &key, &data, 0) == -1)
		err(1, NULL);

	return (S_ISREG(st.st_mode) ? 0 : -1);
}

struct file_info *
lookup_file(const char *name)
{
	DBT key;
	DBT data;
	struct file_info *fi;

	key.data = (char *)name;
	key.size = strlen(name) + 1;

	switch (file_hash_db->get(file_hash_db, &key, &data, 0)) {

	case 1:
		fi = NULL;
		break;

	case 0:
		fi = data.data;
		break;

	default:
		err(1, "db fault");
		fi = NULL;
		break;
	}

	return (fi);
}

#define N_BADMAG(e) \
        ((e)->e_ident[EI_MAG0] != ELFMAG0 || \
         (e)->e_ident[EI_MAG1] != ELFMAG1 || \
         (e)->e_ident[EI_MAG2] != ELFMAG2 || \
         (e)->e_ident[EI_MAG3] != ELFMAG3 || \
         ((e)->e_ident[EI_DATA] != ELFDATA2LSB && \
          BYTE_ORDER == LITTLE_ENDIAN) || \
         ((e)->e_ident[EI_DATA] != ELFDATA2MSB && \
          BYTE_ORDER == BIG_ENDIAN))

char *
get_soname(const char *name, const char *dir_name)
{
	struct stat st;
	Elf32_Ehdr *e;
	Elf32_Shdr *s;
	Elf32_Dyn *d;
	void *v;
	Elf32_Word soname_offset = 0;
	Elf32_Addr strtab_ptr = 0;
	char *soname, *sn;
	int f;
	int len;


	if ((f = open(name, O_RDONLY)) == -1) {
		if (verbose)
			warn("%s/%s", dir_name, name);
		return (NULL);
	}
	if (fstat(f, &st) == -1) {
		if (verbose)
			warn("%s/%s", dir_name, name);
		close(f);
		return (NULL);
	}
	if (st.st_size < sizeof (*e) || st.st_size >= SIZE_T_MAX) {
		if (verbose)
			warnx("%s/%s: bad size", dir_name, name);
		close(f);
		return (NULL);
	}
	if ((v = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_SHARED, f, 0)) ==
	    MAP_FAILED) {
		if (verbose)
			warn("%s/%s", dir_name, name);
		close(f);
		return (NULL);
	}
	close(f);

	e = v;
	if (N_BADMAG(e) || e->e_type != ET_DYN) {
		if (verbose)
			warnx("%s/%s: wrong file type", dir_name, name);
		munmap(v, (size_t)st.st_size);
		return (NULL);
	}
	if (e->e_shoff + e->e_shnum * e->e_shentsize > st.st_size) {
		if (verbose)
			warnx("%s/%s: bad size", dir_name, name);
		munmap(v, (size_t)st.st_size);
		return (NULL);
	}
	for (s = (Elf32_Shdr *)((char *)v + e->e_shoff);
	    s < (Elf32_Shdr *)((char *)v + e->e_shoff +
		e->e_shnum * e->e_shentsize);
	    s = (Elf32_Shdr *)((char *)s + e->e_shentsize))
		if (s->sh_type == SHT_DYNAMIC)
			break;
	if (s >= (Elf32_Shdr *)((char *)v + e->e_shoff +
	    e->e_shnum * e->e_shentsize)) {
		if (verbose)
			warnx("%s/%s: bad format", dir_name, name);
		munmap(v, (size_t)st.st_size);
		return (NULL);
	}
	if (s->sh_offset + s->sh_size > st.st_size) {
		if (verbose)
			warnx("%s/%s: bad size", dir_name, name);
		munmap(v, (size_t)st.st_size);
		return (NULL);
	}
	for (d = (Elf32_Dyn *)((char *)v + s->sh_offset);
	    d < (Elf32_Dyn *)((char *)v + s->sh_offset + s->sh_size);
	    ++d) {
		if (d->d_tag == DT_SONAME)
			soname_offset = d->d_un.d_val;
		if (d->d_tag == DT_STRTAB)
			strtab_ptr = d->d_un.d_ptr;
		if (soname_offset != 0 && strtab_ptr != 0)
			break;
	}
	if (soname_offset == 0 || strtab_ptr == 0) {
		if (verbose)
			warnx("%s/%s: no soname", dir_name, name);
		/*
		 * No soname in this library, so we fake one of the form
		 * "<libraryname.so>.0".
		 */
		munmap(v, (size_t)st.st_size);
		len = (strstr(name, ".so") - name) + 5;
		soname = malloc(len + 1);
		bcopy(name, soname, len - 2);
		bcopy(".0", soname + len - 2, 3);
		return(soname);
	}
	for (s = (Elf32_Shdr *)((char *)v + e->e_shoff);
	    s < (Elf32_Shdr *)((char *)v + e->e_shoff +
		e->e_shnum * e->e_shentsize);
	    s = (Elf32_Shdr *)((char *)s + e->e_shentsize))
		if (s->sh_type == SHT_STRTAB && s->sh_addr == strtab_ptr)
			break;
	if (s >= (Elf32_Shdr *)((char *)v + e->e_shoff +
	    e->e_shnum * e->e_shentsize) ||
	    s->sh_offset + soname_offset > st.st_size) {
		if (verbose)
			warnx("%s/%s: bad format", dir_name, name);
		munmap(v, (size_t)st.st_size);
		return (NULL);
	}
	soname = (char *)v + s->sh_offset + soname_offset;
	if (memchr(soname, '\0', st.st_size - (s->sh_offset + soname_offset)) ==
	    NULL) {
		if (verbose)
			warnx("%s/%s: bad format", dir_name, name);
		munmap(v, (size_t)st.st_size);
		return (NULL);
	}

	/*
	 * If the soname ends in ".so" it doesn't have a major number.  We
	 * need to fake a major of ".0" for it, so our hack in ld.so to look
	 * for <libraryname.so>.0 dynamic shared libraries will work.
	 */
	sn = soname + (strlen(soname) - 3);
	if (strstr(sn, ".so") != NULL) {
		if (verbose)
			warnx("%s/%s: no major", dir_name, name);
		sn = malloc(strlen(soname) + 3);
		strcpy(sn, soname);
		strcat(sn, ".0");
		munmap(v, (size_t)st.st_size);
		return(sn);
	}

	if ((soname = strdup(soname)) == NULL)
		err(1, NULL);

	munmap(v, (size_t)st.st_size);

	return (soname);
}

struct target *
lookup_target(const char *name, DB *db)
{
	DBT key;
	DBT data;
	struct target *t;

	key.data = (char *)name;
	key.size = strlen(name) + 1;

	switch (db->get(db, &key, &data, 0)) {

	case 1:
		t = NULL;
		break;

	case 0:
		t = data.data;
		break;

	default:
		err(1, "db fault");
		t = NULL;
		break;
	}

	return (t);
}

void
add_target(struct target *t, const char *name, DB *db)
{
	DBT key;
	DBT data;

	key.data = (char *)name;
	key.size = strlen(name) + 1;
	data.data = t;
	data.size = sizeof (*t);

	if (db->put(db, &key, &data, 0) == -1)
		err(1, "db fault");
}

/*
 * Expand a string containing version numbers in such a way
 * that it will sort numerically rather than lexicographically
 * on any numeric substrings.  We simply replace all (decimal)
 * numbers after the '.so' with 8-character hex expansions.
 */
char *
expand(const char *s0)
{
	const char *s;
	char *s_end;
	char *e0;
	char *e;
	size_t n;
	size_t len;
	unsigned long ul;

	/* First pass: compute the size of the buffer.  */
	if ((s = strstr(s0, ".so")) == NULL)
		errx(1, "%s: bad target expansion", s0);
	s += strlen(".so");
	for (len = s - s0; *s != '\0'; ) {
		n = strcspn(s, "0123456789");
		len += n;
		s += n;
		n = strspn(s, "0123456789");
		len += n > 0 ? sizeof (ul) * 2 : 0;
		s += n;
	}

	if ((e0 = malloc(len + 1)) == NULL)
		err(1, NULL);

	/* Second pass: fill in the buffer.  */
	if ((s = strstr(s0, ".so")) == NULL)
		errx(1, "%s: bad target expansion", s0);
	s += strlen(".so");
	memcpy(e0, s0, s - s0);
	for (e = e0 + (s - s0); *s != '\0'; ) {
		n = strcspn(s, "0123456789");
		memcpy(e, s, n);
		s += n;
		e += n;
		if (*s == '\0')
			break;
		ul = strtoul(s, &s_end, 10);
		sprintf(e, "%0*lx", sizeof (ul) * 2, ul);
		s = s_end;
		e += sizeof (ul) * 2;
	}
	*e = '\0';

	return (e0);
}

void
create_link(const char *name, const char *target, const char *dir_name)
{
	struct file_info *fi;

	if ((fi = lookup_file(name)) != NULL) {
		if (!S_ISLNK(fi->mode) || strcmp(target, fi->link_name) == 0) {
			if (verbose)
				printf("\t%s => %s (SKIPPED)\n", name, target);
			return;
		}
		if (!suppress_linking && unlink(name) == -1) {
			warn("%s/%s", dir_name, name);
			return;
		}
	}

	if (suppress_linking) {
		printf("\t%s => %s (SUPPRESSED)\n", name, target);
		return;
	}

	if (symlink(target, name) == -1) {
		warn("%s/%s", dir_name, name);
		return;
	}

	if (verbose)
		printf("\t%s => %s\n", name, target);
}
