/*	BSDI ldconfig.c,v 1.11 2000/08/23 20:21:20 polk Exp	*/

/*
 * ldconfig - update shared library symlinks
 *
 * usage: ldconfig [-DvnNX] [-f conf] [-C cache] [-r root] dir ...
 *        ldconfig -l [-Dv] lib ...
 *        ldconfig -p
 *        -D: debug mode, don't update links
 *        -v: verbose mode, print things as we go
 *        -n: don't process standard directories
 *        -N: don't update the library cache
 *        -X: don't update the library links
 *        -l: library mode, manually link libraries
 *        -p: print the current library cache
 *        -f conf: use conf instead of /etc/ld.so.conf
 *        -C cache: use cache instead of /etc/ld.so.cache
 *        -r root: first, do a chroot to the indicated directory
 *        dir ...: directories to process
 *        lib ...: libraries to link
 *
 * Copyright 1994-1996 David Engel and Mitch D'Souza
 *
 * This program may be used for any purpose as long as this
 * copyright notice is kept.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <dirent.h>
#include <unistd.h>
#include <a.out.h>
#include <linux/elf.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

#include "../config.h"

char *___strtok = NULL;

/* For SunOS */
#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX _POSIX_PATH_MAX
#endif

/* For SunOS */
#ifndef N_MAGIC
#define N_MAGIC(exec) ((exec).a_magic & 0xffff)
#endif

#ifndef QMAGIC
#define QMAGIC ZMAGIC
#endif

#define EXIT_OK    0
#define EXIT_FATAL 128

char *prog = NULL;
int debug = 0;			/* debug mode */
int verbose = 0;		/* verbose mode */
int libmode = 0;		/* library mode */
int nocache = 0;		/* don't build cache */
int nolinks = 0;		/* don't update links */

char *conffile = LDSO_CONF;	/* default cache file */
char *cachefile = LDSO_CACHE;	/* default cache file */
void cache_print(void);
void cache_dolib(char *dir, char *so, char *fname, int libtype);
void cache_write(void);

char *readsoname(char *name, FILE *file, int *type);

void warn(char *fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s: warning: ", prog);

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fprintf(stderr, "\n");

    return;
}

void error(char *fmt, ...)
{
    va_list ap;

    fprintf(stderr, "%s: ", prog);

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fprintf(stderr, "\n");

    exit(EXIT_FATAL);
}

void *xmalloc(size_t size)
{
    void *ptr;
    if ((ptr = malloc(size)) == NULL)
	error("out of memory");
    return ptr;
}

char *xstrdup(char *str)
{
    char *ptr;
    char *strdup(const char *);
    if ((ptr = strdup(str)) == NULL)
	error("out of memory");
    return ptr;
}

/* if shared library, return a malloced copy of the soname and set the
   type, else return NULL */
char *is_shlib(char *dir, char *name, int *type, int *islink, int *isalias)
{
    char *good = NULL;
    char *cp, *ep;
    FILE *file;
    struct exec exec;
    struct elfhdr *elf_hdr;
    struct stat statbuf;
    char buff[PATH_MAX];

    *islink = *isalias = *type = 0;

    /* see if name is of the form libZ.so* */
    if ((strncmp(name, "lib", 3) == 0 || strncmp(name, "ld-", 3) == 0) && \
	name[strlen(name)-1] != '~' && (ep = strstr(name, ".so")))
    {
	/* find the start of the Vminor part, if any */
	ep += 3;
	if (*ep != '.' || (cp = strchr(ep + 1, '.')) == NULL)
	    cp = ep + strlen(ep);

	/* construct the full path name */
	sprintf(buff, "%s%s%s", dir, (*dir && strcmp(dir, "/")) ?
		"/" : "", name);

	/* first, make sure it's a regular file */
	if (lstat(buff, &statbuf))
	    warn("can't lstat %s (%s), skipping", buff, strerror(errno));
	else if (!S_ISREG(statbuf.st_mode) && !S_ISLNK(statbuf.st_mode))
	    warn("%s is not a regular file or symlink, skipping", 
		 buff, strerror(errno));
	else
	{
	    /* is it a regular file or a symlink */
	    *islink = S_ISLNK(statbuf.st_mode);

	    /* then try opening it */
	    if (!(file = fopen(buff, "rb")))
		warn("can't open %s (%s), skipping", buff, strerror(errno));
	    else
	    {
		/* now make sure it's a shared library */
		if (fread(&exec, sizeof exec, 1, file) < 1)
		    warn("can't read header from %s, skipping", buff);
		else if (N_MAGIC(exec) != ZMAGIC && N_MAGIC(exec) != QMAGIC)
		{
		    elf_hdr = (struct elfhdr *) &exec;
		    if (elf_hdr->e_ident[0] != 0x7f ||
			strncmp(&elf_hdr->e_ident[1], "ELF",3) != 0)
		    {
		        /* silently ignore linker scripts */
		        if (strncmp((char *)&exec, "/* GNU ld", 9) != 0)
			    warn("%s is not a shared library, skipping", buff);
		    }
		    else
		    {
		        /* always call readsoname to update type */
		        *type = LIB_ELF;
		        good = readsoname(buff, file, type);
			if (good == NULL)
			{
			    good = xstrdup(name);
			}
			else
			{
			    /* if the soname does not match the filename,
			       this is an alias */
			    if (strncmp(good, name, ep - name) != 0)
			    {
				if (debug)
				    warn("%s has inconsistent soname (%s)",
					buff, good);
				*isalias = 1;
			    }
			}
		    }
		}
		else
		{
		    if (*islink)
		        good = xstrdup(name);
		    else
		    {
		        good = xmalloc(cp - name + 1);
			strncpy(good, name, cp - name);
			good[cp - name] = '\0';
		    }
		    *type = LIB_DLL;
		}
		fclose(file);
	    }
	}
    }

    return good;
}

/* update the symlink to new library */
void link_shlib(char *dir, char *file, char *so)
{
    int change = -1;
    char libname[1024];
    char linkname[1024];
    struct stat libstat;
    struct stat linkstat;

    /* construct the full path names */
    sprintf(libname, "%s/%s", dir, file);
    sprintf(linkname, "%s/%s", dir, so);

    /* see if a link already exists */
    if (!stat(linkname, &linkstat))
    {
	/* now see if it's the one we want */
	if (stat(libname, &libstat))
	    warn("can't stat %s (%s)", libname, strerror(errno));
	else if (libstat.st_dev == linkstat.st_dev &&
	    libstat.st_ino == linkstat.st_ino)
	    change = 0;
    }

    /* then update the link, if required */
    if (change && !nolinks)
    {
	if (lstat(linkname, &linkstat) == 0 &&
	    (!S_ISLNK(linkstat.st_mode) || remove(linkname) == -1)) {
	    if (S_ISLNK(linkstat.st_mode))
		warn("can't unlink %s (%s)", linkname, strerror(errno));
	    else
		warn("%s: can't replace a non-symlink", linkname);
	} else if (symlink(file, linkname))
	    warn("can't link %s to %s (%s)", linkname, file, strerror(errno));
	else
	    change = 1;
    }

    /* some people like to know what we're doing */
    if (verbose)
	printf("\t%s => %s%s\n", so, file,
	       change < 0 ? " (SKIPPED)" :
	       (change > 0 ? " (changed)" : ""));

    return;
}

/* figure out which library is greater */
int libcmp(char *p1, char *p2)
{
    while (*p1)
    {
	if (isdigit(*p1) && isdigit(*p2))
	{
	    /* must compare this numerically */
	    int v1, v2;
	    v1 = strtoul(p1, &p1, 10);
	    v2 = strtoul(p2, &p2, 10);
	    if (v1 != v2)
		return v1 - v2;
	}
	else if (isdigit(*p1) && !isdigit(*p2))
	    return 1;
	else if (!isdigit(*p1) && isdigit(*p2))
	    return -1;
	else if (*p1 != *p2)
	    return *p1 - *p2;
	else
	    p1++, p2++;
    }

    return *p1 - *p2;
}

struct soname
{
    char *so_name;			/* Library name */
    char *so_fname;			/* Best filename match */
    struct prefix *so_spfx;		/* Pointer to the soname prefix */
    int so_islink;			/* Is a symbolic link */
    int so_isalias;			/* Is an alias */
    int so_libtype;			/* Library type */
    struct soname *so_next;		/* next soname in list */
};

struct prefix
{
    char *pfx_name;			/* Library prefix (no major version) */
    struct soname *pfx_max_so;		/* Highest soname version */
    struct soname *pfx_max_alias;	/* Highest alias version */
    struct prefix *pfx_next;		/* next prefix in list */
};

struct soname *
so_lookup(struct soname *so_list, char *soname)
{
    struct soname *sp;

    for (sp = so_list; sp != NULL; sp = sp->so_next)
    {
	if (strcmp(soname, sp->so_name) == 0)
	    break;
    }

    return (sp);
}

struct soname *
so_create(struct soname **so_list, char *soname)
{
    struct soname *sp;

    sp = xmalloc(sizeof(*sp));
    memset(sp, 0, sizeof(*sp));
    sp->so_name = xstrdup(soname);
    sp->so_next = *so_list;
    *so_list = sp;

    return (sp);
}

struct prefix *
pfx_lookup(struct prefix *pfx_list, char *prefix)
{
    struct prefix *pp;

    for (pp = pfx_list; pp != NULL; pp = pp->pfx_next)
    {
	if (strcmp(prefix, pp->pfx_name) == 0)
	    break;
    }

    return (pp);
}

struct prefix *
pfx_create(struct prefix **pfx_list, char *prefix)
{
    struct prefix *pp;

    pp = xmalloc(sizeof(*pp));
    memset(pp, 0, sizeof(*pp));
    pp->pfx_name = xstrdup(prefix);
    pp->pfx_next = *pfx_list;
    *pfx_list = pp;

    return (pp);
}

void scan_dir(char *name)
{
    DIR *dir;
    struct dirent *ent;
    struct prefix *pp, *spfx_list = NULL;
    struct soname *sp, *so_list = NULL;
    int libtype, islink, isalias;
    char *cp, *so;

    /* let 'em know what's going on */
    if (verbose)
	printf("%s:\n", name);

    /* if we can't open it, we can't do anything */
    if ((dir = opendir(name)) == NULL)
    {
#ifdef NOTDEF
	warn("can't open %s (%s), skipping", name, strerror(errno));
#endif
	return;
    }

    /* yes, we have to look at every single file */
    while ((ent = readdir(dir)) != NULL)
    {

	/* if it's not a shared library, don't bother */
	if ((so = is_shlib(name, ent->d_name, &libtype, &islink, &isalias))
	    == NULL)
	    continue;

	/* aliases sonames match their filename, we ignore other links */
	if (isalias)
	{
	    free(so);
	    so = xstrdup(ent->d_name);
	}
	else if (islink)
	{
	    free(so);
	    continue;
	}

	/* Does this soname exist? */
	sp = so_lookup(so_list, so);
	if (sp == NULL)
	{
	    sp = so_create(&so_list, so);
	    sp->so_fname = xstrdup(ent->d_name);
	    sp->so_islink = islink;
	    sp->so_isalias = isalias;
	    sp->so_libtype = libtype;

	    /* Find or create a prefix entry */
	    if ((cp = strstr(so, ".so")) == NULL)
		sp->so_spfx = NULL;
	    else
	    {
		cp[3] = 0;
		pp = pfx_lookup(spfx_list, so);
		if (pp == NULL)
		{
		    pp = pfx_create(&spfx_list, so);
		    if (sp->so_isalias)
			pp->pfx_max_alias = sp;
		    else
			pp->pfx_max_so = sp;
		}
		else
		{
		    if (sp->so_isalias)
		    {
			if (pp->pfx_max_alias == NULL ||
			    libcmp(sp->so_name, pp->pfx_max_alias->so_name) > 0)
			    pp->pfx_max_alias = sp;
		    }
		    else 
		    {
			if (pp->pfx_max_so == NULL ||
			    libcmp(sp->so_name, pp->pfx_max_so->so_name) > 0)
			    pp->pfx_max_so = sp;
		    }
		}
		sp->so_spfx = pp;
	    }
	}
	else
	{
	    /* check for a higher version number of an existing soname */
	    if (libcmp(ent->d_name, sp->so_fname) > 0)
	    {
		free(sp->so_fname);
		sp->so_fname = xstrdup(ent->d_name);
		sp->so_islink = islink;
		sp->so_isalias = isalias;
		sp->so_libtype = libtype;
		pp = sp->so_spfx;
		if ((cp = strstr(sp->so_name, ".so")) != NULL)
		{
		    if (sp->so_isalias)
		    {
			if (libcmp(sp->so_name, pp->pfx_max_alias->so_name) > 0)
			    pp->pfx_max_alias = sp;
		    }
		    else
		    {
			if (pp->pfx_max_so == NULL ||
			    libcmp(sp->so_name, pp->pfx_max_so->so_name) > 0)
			    pp->pfx_max_so = sp;
		    }
		}
	    }
	}
	free(so);
    }

    /* link each prefix to it's greatest major version */
    for (pp = spfx_list; pp != NULL; pp = pp->pfx_next)
    {
	if (pp->pfx_max_alias != NULL &&
	    strcmp(pp->pfx_max_alias->so_name, pp->pfx_name) != 0)
	    link_shlib(name, pp->pfx_max_alias->so_name, pp->pfx_name);
	else if (pp->pfx_max_so != NULL &&
	    strcmp(pp->pfx_max_so->so_name, pp->pfx_name) != 0)
	    link_shlib(name, pp->pfx_max_so->so_name, pp->pfx_name);
    }

    /* Link each soname to it's filename */
    for (sp = so_list; sp != NULL; sp = sp->so_next)
    {
	/* don't link something to itself */
	if (strcmp(sp->so_name, sp->so_fname) == 0)
	    continue;

	/* don't link if we match the prefix name */
	if (strcmp(sp->so_name, sp->so_spfx->pfx_name) == 0)
	    continue;

	/* don't link if there is an alias for our root */
	if (sp->so_spfx != NULL && sp->so_spfx->pfx_max_alias != NULL)
	    continue;

        link_shlib(name, sp->so_fname, sp->so_name);
    }

    /* create dynamic loader cache entries */
    if (!nocache)
	for (sp = so_list; sp != NULL; sp = sp->so_next)
	{
	    char *target;

	    /* aliases never make it into the cache */
	    if (sp->so_isalias)
		continue;

	    /* target is filename if there is an alias or we have no
	       version in our soname */
	    if (sp->so_spfx->pfx_max_alias != NULL ||
		    strcmp(sp->so_name, sp->so_spfx->pfx_name) == 0)
		    target = sp->so_fname;
	    else
		    target = sp->so_name;

	    /* create entry, use file name if there is an alias */
	    cache_dolib(name, sp->so_name, target, sp->so_libtype);
	}

    /* always try to clean up after ourselves */
    while ((sp = so_list))
    {
	so_list = sp->so_next;
	free(sp->so_name);
	free(sp->so_fname);
    }

    while ((pp = spfx_list))
    {
	spfx_list = pp->pfx_next;
	free(pp->pfx_name);
    }
}


/* return the list of system-specific directories */
char *get_extpath(void)
{
    char *res = NULL, *cp;
    FILE *file;
    struct stat stat;

    if ((file = fopen(conffile, "r")) != NULL)
    {
	fstat(fileno(file), &stat);
	res = xmalloc(stat.st_size + 1);
	fread(res, 1, stat.st_size, file);
	fclose(file);
	res[stat.st_size] = '\0';

	/* convert comments fo spaces */
	for (cp = res; *cp; /*nada*/) {
	  if (*cp == '#') {
	    do
	      *cp++ = ' ';
	    while (*cp && *cp != '\n');
	  } else {
	    cp++;
	  }
	}	  
    }

    return res;
}

int main(int argc, char **argv)
{
    int i, c;
    int nodefault = 0;
    int printcache = 0;
    char *cp, *dir, *so;
    char *extpath;
    int libtype, islink, isalias;
    char *chroot_dir = NULL;

    prog = argv[0];
    opterr = 0;

    while ((c = getopt(argc, argv, "DvnNXlpf:C:r:")) != EOF)
	switch (c)
	{
	case 'D':
	    debug = 1;		/* debug mode */
	    nocache = 1;
	    nolinks = 1;
	    verbose = 1;
	    break;
	case 'v':
	    verbose = 1;	/* verbose mode */
	    break;
	case 'n':
	    nodefault = 1;	/* no default dirs */
	    nocache = 1;
	    break;
	case 'N':
	    nocache = 1;	/* don't build cache */
	    break;
	case 'X':
	    nolinks = 1;	/* don't update links */
	    break;
	case 'l':
	    libmode = 1;	/* library mode */
	    break;
	case 'p':
	    printcache = 1;	/* print cache */
	    break;
	case 'f':
	    conffile = optarg;	/* alternate conf file */
	    break;
	case 'C':
	    cachefile = optarg;	/* alternate cache file */
	    break;
	case 'r':
	    chroot_dir = optarg;
	    break;
	default:
	    fprintf(stderr, "usage: %s [-DvnNX] [-f conf] [-C cache] [-r root] dir ...\n", prog);
	    fprintf(stderr, "       %s -l [-Dv] lib ...\n", prog);
	    fprintf(stderr, "       %s -p\n", prog);
	    exit(EXIT_FATAL);
	    break;

	/* THE REST OF THESE ARE UNDOCUMENTED AND MAY BE REMOVED
	   IN FUTURE VERSIONS. */
	}

    if (chroot_dir && *chroot_dir) {
	if (chroot(chroot_dir) < 0)
	    error("couldn't chroot to %s (%s)", chroot_dir, strerror(errno));
        if (chdir("/") < 0)
	    error("couldn't chdir to / (%s)", strerror(errno));
    }

    /* allow me to introduce myself, hi, my name is ... */
    if (verbose)
	printf("%s: version %s\n", argv[0], VERSION);

    if (printcache)
    {
	/* print the cache -- don't you trust me? */
	cache_print();
	exit(EXIT_OK);
    }
    else if (libmode)
    {
	/* so you want to do things manually, eh? */

	/* ok, if you're so smart, which libraries do we link? */
	for (i = optind; i < argc; i++)
	{
	    /* split into directory and file parts */
	    if (!(cp = strrchr(argv[i], '/')))
	    {
		dir = "";	/* no dir, only a filename */
		cp = argv[i];
	    }
	    else
	    {
		if (cp == argv[i])
		    dir = "/";	/* file in root directory */
		else
		    dir = argv[i];
		*cp++ = '\0';	/* neither of the above */
	    }

	    /* we'd better do a little bit of checking */
	    if ((so = is_shlib(dir, cp, &libtype, &islink, &isalias)) == NULL)
		error("%s%s%s is not a shared library", dir,
		      (*dir && strcmp(dir, "/")) ? "/" : "", cp);

	    /* so far, so good, maybe he knows what he's doing */
	    link_shlib(dir, cp, so);
	}
    }
    else
    {
	/* the lazy bum want's us to do all the work for him */

	/* don't cache dirs on the command line */
	int nocache_save = nocache;
	nocache = 1;

	/* OK, which directories should we do? */
	for (i = optind; i < argc; i++)
	    scan_dir(argv[i]);

	/* restore the desired caching state */
	nocache = nocache_save;

	/* look ma, no defaults */
	if (!nodefault)
	{
	    /* I guess the defaults aren't good enough */
	    if ((extpath = get_extpath()))
	    {
		for (cp = strtok(extpath, DIR_SEP); cp;
		     cp = strtok(NULL, DIR_SEP))
		    scan_dir(cp);
		free(extpath);
	    }

#ifndef __bsdi__
	    /* everybody needs these, don't they? */
	    scan_dir("/usr/lib");
	    scan_dir("/lib");
#endif
	}

	if (!nocache)
	    cache_write();
    }

    exit(EXIT_OK);
}

typedef struct liblist
{
    int flags;
    int sooffset;
    int liboffset;
    char *soname;
    char *libname;
    struct liblist *next;
} liblist_t;

static header_t magic = { LDSO_CACHE_MAGIC, LDSO_CACHE_VER, 0 };
static liblist_t *lib_head = NULL;

static int liblistcomp(liblist_t *x, liblist_t *y)
{
    int res;

    if ((res = libcmp(x->soname, y->soname)) == 0)
    {
        res = libcmp(strrchr(x->libname, '/') + 1,
		     strrchr(y->libname, '/') + 1);
    }

    return res;
}

void cache_dolib(char *dir, char *so, char *name, int libtype)
{
    char fullpath[PATH_MAX];
    liblist_t *new_lib, *cur_lib;

    magic.nlibs++;
    sprintf(fullpath, "%s/%s", dir, name);
    new_lib = xmalloc(sizeof (liblist_t));
    new_lib->flags = libtype;
    new_lib->soname = xstrdup(so);
    new_lib->libname = xstrdup(fullpath);

    if (lib_head == NULL || liblistcomp(new_lib, lib_head) > 0)
    {
        new_lib->next = lib_head;
	lib_head = new_lib;
    }
    else
    {
        for (cur_lib = lib_head; cur_lib->next != NULL &&
	     liblistcomp(new_lib, cur_lib->next) <= 0;
	     cur_lib = cur_lib->next)
	    /* nothing */;
	new_lib->next = cur_lib->next;
	cur_lib->next = new_lib;
    }
}

void cache_write(void)
{
    int cachefd;
    int stroffset = 0;
    char tempfile[1024];
    liblist_t *cur_lib;

    if (!magic.nlibs)
	return;

    sprintf(tempfile, "%s~", cachefile);

    if (unlink(tempfile) && errno != ENOENT)
        error("can't unlink %s (%s)", tempfile, strerror(errno));

    if ((cachefd = creat(tempfile, 0644)) < 0)
	error("can't create %s (%s)", tempfile, strerror(errno));

    if (write(cachefd, &magic, sizeof (header_t)) != sizeof (header_t))
	error("can't write %s (%s)", tempfile, strerror(errno));

    for (cur_lib = lib_head; cur_lib != NULL; cur_lib = cur_lib->next)
    {
        cur_lib->sooffset = stroffset;
	stroffset += strlen(cur_lib->soname) + 1;
	cur_lib->liboffset = stroffset;
	stroffset += strlen(cur_lib->libname) + 1;
	if (write(cachefd, cur_lib, sizeof (libentry_t)) !=
	    sizeof (libentry_t))
	    error("can't write %s (%s)", tempfile, strerror(errno));
    }

    for (cur_lib = lib_head; cur_lib != NULL; cur_lib = cur_lib->next)
    {
      if (write(cachefd, cur_lib->soname, strlen(cur_lib->soname) + 1)
	  != strlen(cur_lib->soname) + 1)
	  error("can't write %s (%s)", tempfile, strerror(errno));
      if (write(cachefd, cur_lib->libname, strlen(cur_lib->libname) + 1)
	  != strlen(cur_lib->libname) + 1)
	  error("can't write %s (%s)", tempfile, strerror(errno));
    }

    if (close(cachefd))
        error("can't close %s (%s)", tempfile, strerror(errno));

    if (chmod(tempfile, 0644))
	error("can't chmod %s (%s)", tempfile, strerror(errno));

    if (rename(tempfile, cachefile))
	error("can't rename %s (%s)", tempfile, strerror(errno));
}

void cache_print(void)
{
    caddr_t c;
    struct stat st;
    int fd = 0;
    char *strs;
    header_t *header;
    libentry_t *libent;

    if (stat(cachefile, &st) || (fd = open(cachefile, O_RDONLY))<0)
	error("can't read %s (%s)", cachefile, strerror(errno));
    if ((c = mmap(0,st.st_size, PROT_READ, MAP_SHARED ,fd, 0)) == (caddr_t)-1)
	error("can't map %s (%s)", cachefile, strerror(errno));
    close(fd);

    if (memcmp(((header_t *)c)->magic, LDSO_CACHE_MAGIC, LDSO_CACHE_MAGIC_LEN))
	error("%s cache corrupt", cachefile);

    if (memcmp(((header_t *)c)->version, LDSO_CACHE_VER, LDSO_CACHE_VER_LEN))
	error("wrong cache version - expected %s", LDSO_CACHE_VER);

    header = (header_t *)c;
    libent = (libentry_t *)(c + sizeof (header_t));
    strs = (char *)&libent[header->nlibs];

    printf("%d libs found in cache `%s' (version %s)\n",
	    header->nlibs, cachefile, LDSO_CACHE_VER);

    for (fd = 0; fd < header->nlibs; fd++)
    {
	printf("\t%s ", strs + libent[fd].sooffset);
#ifndef __bsdi__
	switch (libent[fd].flags) 
	{
	  case LIB_DLL:
	    printf("(DLL)");
	    break;
	  case LIB_ELF:
	    printf("(ELF)");
	    break;
	  case LIB_ELF_LIBC5:
	  case LIB_ELF_LIBC6:
	    printf("(ELF-libc%d)", libent[fd].flags + 3);
	    break;
	  default:
	    printf("(unknown)");
	    break;
	}
#endif
	printf(" => %s\n", strs + libent[fd].liboffset);
    }

    munmap (c,st.st_size);
}

