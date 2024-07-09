/*
 * Copyright (c) 1994-1996 Eric Youngdale, Peter MacDonald, David Engel,
 * Hongjiu Lu and Mitch D'Souza
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the above contributors may not be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Notice of general intent:
 *
 * The linux operating system generally contains large amounts of code
 * that fall under the GNU General Public License, or GPL for short.
 * This file contains source code that by it's very nature would always
 * be linked with an application program, and because of this a GPL type
 * of copyright on this file would place restrictions upon the
 * distribution of binary-only commercial software.  Since the goal of the
 * Linux project as a whole is not to discourage the development and
 * distribution of commercial software for Linux, this file has been placed
 * under a more relaxed BSD-style of copyright.
 *
 * It is the general understanding of the above contributors that a
 * program executable linked to a library containing code that falls
 * under the GPL or GLPL style of license is not subject to the terms of
 * the GPL or GLPL license if the program executable(s) that are supplied
 * are linked to a shared library form of the GPL or GLPL library, and as
 * long as the form of the shared library is such that it is possible for
 * the end user to modify and rebuild the library and use it in
 * conjunction with the program executable.
 */

#include <alloca.h>
#include <unistd.h>
#include <syscall.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sharedlib.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include "../config.h"
#include "fixups.h"
#include <sys/mman.h>

#define EXIT_FATAL 128

#define VMAJOR(x) (((x) & MAJOR_MASK) >> 16)
#define VMINOR(x) (((x) & MINOR_MASK) / 100)
#define VPATCH(x) (((x) & MINOR_MASK) % 100)

#define TOTAL_COMPATIBLE	0
#define MAJOR_COMPATIBLE	1
#define NOT_COMPATIBLE		2

int errno = 0;
char **__environ = 0;
#pragma weak environ = __environ
/*char *___strtok=NULL;*/
extern int _start;
extern int _edata;

int fdprintf(int fd, const char *fmt, ...);

#define SYS_dl_exit SYS_exit
_syscall1(int,dl_exit,int,code)

static caddr_t cache_init (char *, size_t *);

static int
incompatible(int in_core, int linked)
{
    return (VMAJOR(in_core) == VMAJOR(linked))
	? ((VMINOR(in_core) >= VMINOR(linked)) 
	   ? TOTAL_COMPATIBLE : MAJOR_COMPATIBLE)
	    : NOT_COMPATIBLE;
}

/* parse a list of directories and add them to the search list */
static int
add_dirs(char *argv0, int ndirs, char *libdir[], char *path)
{
    static int toomany = 0;

    for (path = strtok(path, DIR_SEP); path && ndirs < MAX_DIRS;
	 path = strtok(NULL, DIR_SEP))
	libdir[ndirs++] = path;

    /* how can you keep track of this many directories? */
    if (path && !toomany++)
	fdprintf(2, "%s: too many directories in library path\n", argv0);

    return ndirs;
}

static int try_lib(char *argv0, char *buffer, char *dir, char *lib)
{
    int found;

    strcpy(buffer, dir);
    if (lib != NULL)
    {
        strcat(buffer, "/");
	strcat(buffer, lib);
    }

    if (!(found = !uselib(buffer)))
    {
	if (errno != ENOENT)
	{
	    fdprintf(2, "%s: can't load library '%s'\n", argv0, buffer);
	    fdprintf(2, "\t%s\n", strerror(errno));
	}
    }

    return found;
}

#ifndef DEBUG_LIB
/* store our address information so we can be unmapped */
static void
save_mapinfo(unsigned *mapinfo)
{
    if (mapinfo)
    {
	mapinfo[0] = (unsigned)&_start-0x20;
	mapinfo[1] = ((unsigned)&_edata-(unsigned)&_start+0xfff) & ~0xfff;
    }
}
#endif

void
shared_loader(int func, ...)
{
    int ndirs = 0;
    char *libdir[MAX_DIRS];
    struct libentry **ptr;
    char *cp;
    int i, ldd = 0;
    va_list ap;
    struct libentry **__SHARED_LIBRARIES__ = NULL;
    struct fixuplist *_SHARABLE_CONFLICTS__ = NULL;
    char *argv0 = NULL;
#ifndef DEBUG_LIB
    unsigned *mapinfo = NULL;
#endif
    char *libs[32];
    int nlibs=0;
    int nowarn;
    int keepdir;
    char *preload = NULL;
    caddr_t cache_addr;
    size_t cache_size;
    callbackptr callback = (callbackptr)0;

#ifdef TIMER
#include <time.h>
    long clk;

    clk = clock();
#endif

    switch (func)
    {
    case FUNC_VERS:
	/* tell 'em who we are */
	fdprintf(1, "%s: version %s\n", LDSO_IMAGE, VERSION);
#ifndef DEBUG_LIB
	va_start(ap, func);
	mapinfo = va_arg(ap, unsigned *);
	save_mapinfo(mapinfo);
	va_end(ap);
#endif
	return;
    case FUNC_LDD:
	/* just going through the motions */
	ldd = 1;
	/* fall through */;
    case FUNC_LINK_AND_CALLBACK:
    case FUNC_LINK:
	/* this is why we're all here */
	va_start(ap, func);
#ifndef DEBUG_LIB
	mapinfo = va_arg(ap, unsigned *);
	save_mapinfo(mapinfo);
#endif
	argv0 = va_arg(ap, char *);
	__environ = va_arg(ap, char **);
	__SHARED_LIBRARIES__ = va_arg(ap, struct libentry **);
	_SHARABLE_CONFLICTS__ = va_arg(ap, struct fixuplist *);
	if (func == FUNC_LINK_AND_CALLBACK)
	  callback = va_arg(ap, callbackptr);
	va_end(ap);
	break;
    default:
	/* you want me to do what? */
	fdprintf(2, "%s: invalid dynamic linker option %d\n",
		 LDSO_IMAGE, func);
	dl_exit(EXIT_FATAL);
    }

    /* read any environmental options */
    nowarn = getenv("LD_NOWARN") != NULL;
    keepdir = getenv("LD_KEEPDIR") != NULL;

    /* find out who we are, in case somebody wants to know */
    if (!argv0 && !(argv0 = getenv(LDD_ARGV0)))
	argv0 = LDSO_IMAGE;

    /* hmm, you want your own configuration, do you? */
    if (getuid() == geteuid() && getgid() == getegid())
    {
      /* Well OK, I'll allow it, but just this time */
      preload = getenv("LD_AOUT_PRELOAD");
      cp = getenv("LD_AOUT_LIBRARY_PATH");
      if (cp && *cp)
      {
	char *temp = alloca(strlen(cp) + 1);
	ndirs = add_dirs(argv0, ndirs, libdir, strcpy(temp, cp));
      }
    }
    else
    {
      /* sorry, Charlie, I can't let you do that */
      unsetenv("LD_PRELOAD");
      unsetenv("LD_AOUT_PRELOAD");
      preload = NULL;
      unsetenv("LD_LIBRARY_PATH");
      unsetenv("LD_AOUT_LIBRARY_PATH");
    }

    /* attempt to read and initialize the cache */
    cache_addr = cache_init(LDSO_CACHE, &cache_size);

    /* OK, enough formalities, which libs do you want? */
    for (ptr = __SHARED_LIBRARIES__+2; *ptr; ptr++)
    {
	char buffer[PATH_MAX+1];
	unsigned *vers = (unsigned *)(*ptr)->addr;
	int found;

	/* ignore directory part unless told not to */
	if ((cp = strrchr((*ptr)->name, '/')) && !keepdir)
	    cp++;
	else
	    cp = (*ptr)->name;

	/* OK, I'll see if I can find it for you */
	if (*cp == '/')
	{
	    /* an absolute name, this one is easy */
	    strcpy(buffer, cp);
	    found = !access(buffer, F_OK);
	}
	else
	{
	    /* a relative name, guess I'll have to search */

	    /* let's try the normal directory list first */
	    for (found = 0, i = 0; !found && i < ndirs; i++)
		found = try_lib(argv0, buffer, libdir[i], cp);

	    /* not there, maybe it's in the cache library list */
	    if (!found && cache_addr)
	    {
		header_t *header = (header_t *)cache_addr;
		libentry_t *libent = (libentry_t *)&header[1];
		char *strs = (char *)&libent[header->nlibs];

		for (i = 0; !found && i < header->nlibs; i++)
		{
		    if (libent[i].flags == LIB_DLL &&
			strcmp(cp, strs + libent[i].sooffset) == 0)
		        found = try_lib(argv0, buffer, 
					strs + libent[i].liboffset,
					NULL);
		}
	    }

	    /* if not found yet, then try /usr/lib and /lib */
	    if (!found)
		found = try_lib(argv0, buffer, "/usr/lib", cp);
	    if (!found)
		found = try_lib(argv0, buffer, "/lib", cp);
	}

	/* OK, let's take a look and see what we've found */
	if (!found)
	{
	    /* methinks somebody's pulling our leg */
	    if (ldd)
		fdprintf(1, "\t%s (%s) => not found\n", cp, (*ptr)->avers);
	    else
	    {
		fdprintf(2, "%s: can't find library '%s'\n", argv0, cp);
		dl_exit(EXIT_FATAL);
	    }
	}
	else
	{
	     /* for preloading we need to know the offsets and length of the
		.text section. This is passed to ldpreload() below */
	    if (preload || callback)
	    {
	    	if (nlibs >= (sizeof libs / sizeof libs[0]))
	    	{
	    		fdprintf(2, "%s: too many preloads\n",argv0);
	    		exit(EXIT_FATAL);
	    	}
	        libs[nlibs] = alloca(strlen(buffer)+1);
	        strcpy(libs[nlibs], buffer);
	        nlibs++;
	    }

	    /* hey, I think I've got a live one here */
	    if (ldd)
	    {
		/* no thanks, I'm just browsing */
		fdprintf(1, "\t%s (%s) => %s\n", cp,
			 (*ptr)->avers, buffer);
	    }
	    else
	    {
		/* so far, so good, but is it the real thing? */
		switch (incompatible(*vers, (*ptr)->vers))
		{
		case MAJOR_COMPATIBLE:
		    if (!nowarn)
		    {
			/* don't you keep your system up to date? */
			fdprintf(2, "%s: using incompatible library '%s'\n",
				 argv0, buffer);
			fdprintf(2, "\tDesire minor version >= %d and found %d\n",
				 VMINOR((*ptr)->vers), VMINOR(*vers));
		    }
		    break;
		case NOT_COMPATIBLE:
		    /* methinks somebody is lying to us */
		    fdprintf(2, "%s: incompatible library '%s'\n", argv0,
			     buffer);
		    fdprintf(2, "\tRequire major version %d and found %d\n",
			     VMAJOR((*ptr)->vers), VMAJOR(*vers));
		    dl_exit(EXIT_FATAL);
		    break;
		}
	    }
	}
    }

    /* we can unmap ld.so.cache if it exists */
    if (cache_addr)
	munmap(cache_addr, cache_size);

#ifdef TIMER
    fdprintf(1, "%ld CPU seconds spent in ld.so\n", clock()-clk);
#endif

    /* the rest of this program is brought to you by Eric Youngdale */
    if (!ldd)
    {
	static char err[] = "dynamic linker error in";

	/* Subtract curr addr from .long _foo */
	if (__do_fixups(_SHARABLE_CONFLICTS__, 0))
	{
	    fdprintf(2, "%s: %s fixup pass 1\n", argv0, err);
	    dl_exit(EXIT_FATAL);
	}

	if (__dynamic_resolve (_SHARABLE_CONFLICTS__))
	{
	    fdprintf(2, "%s: %s resolve pass\n", argv0, err);
	    dl_exit(EXIT_FATAL);
	}

	/* Add (possibly) new addr to .long _foo */
	if (__do_fixups(_SHARABLE_CONFLICTS__, 1))
	{
	    fdprintf(2, "%s: %s fixup pass 2\n", argv0, err);
	    dl_exit(EXIT_FATAL);
	}

	/* Set the magic numbers back to the starting values for emacs */
	if (__reset_magic(_SHARABLE_CONFLICTS__))
	{
	    fdprintf(2, "%s: %s reset pass\n", argv0, err);
	    dl_exit(EXIT_FATAL);
	}
    }
    if (preload)
      ldpreload(preload, ldd, libs, nlibs, argv0);
    if (callback)
      (*callback)(CALLBACK_VER, nlibs, libs, preload ? 1 : 0, &preload);
}

static caddr_t cache_init(char *file, size_t *size)
{
    int fd;
    struct stat st;
    caddr_t cache_addr;
    header_t *header;
    libentry_t *libent;
    int i, strtabsize;

    if (stat(LDSO_CACHE, &st) || (fd = open(LDSO_CACHE, O_RDONLY)) < 0)
    {
	fdprintf(2, "%s: can't open cache '%s'\n", LDSO_IMAGE, LDSO_CACHE);
	return NULL;
    }
    *size = st.st_size;
    if ((cache_addr = mmap(0, *size, PROT_READ, MAP_SHARED, fd, 0)) ==
	(caddr_t)-1)
    {
	fdprintf(2, "%s: can't map cache '%s'\n", LDSO_IMAGE, LDSO_CACHE);
	close(fd);
	return NULL;
    }

    close(fd);

    header = (header_t *)cache_addr;

    if (*size < sizeof (header_t) ||
	memcmp(header->magic, LDSO_CACHE_MAGIC, LDSO_CACHE_MAGIC_LEN) ||
	memcmp(header->version, LDSO_CACHE_VER, LDSO_CACHE_VER_LEN) ||
	*size < (sizeof (header_t) + header->nlibs * sizeof (libentry_t)) ||
	cache_addr[*size-1] != '\0')
    {
	fdprintf(2, "%s: cache '%s' is corrupt\n", LDSO_IMAGE,
                 LDSO_CACHE);
    no_cache:
	munmap(cache_addr, *size);
	return NULL;
    }

    strtabsize = *size - sizeof (header_t) - 
      header->nlibs * sizeof (libentry_t);
    libent = (libentry_t *)&header[1];

    for (i = 0; i < header->nlibs; i++)
    {
      if (libent[i].sooffset >= strtabsize ||
	  libent[i].liboffset >= strtabsize)
      {
	fdprintf(2, "%s: cache '%s' is corrupt\n", LDSO_IMAGE,
                 LDSO_CACHE);
	goto no_cache;
      }
    }

    return cache_addr;
}

