/* Run time dynamic linker.
   Copyright (C) 1995-1999, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>		/* Check if MAP_ANON is defined.  */
#include <elf/ldsodefs.h>
#include <stdio-common/_itoa.h>
#include <entry.h>
#include <fpu_control.h>
#include <hp-timing.h>
#include "dynamic-link.h"
#include "dl-librecon.h"

#include <assert.h>

/* System-specific function to do initial startup for the dynamic linker.
   After this, file access calls and getenv must work.  This is responsible
   for setting __libc_enable_secure if we need to be secure (e.g. setuid),
   and for setting _dl_argc and _dl_argv, and then calling _dl_main.  */
extern ElfW(Addr) _dl_sysdep_start (void **start_argptr,
				    void (*dl_main) (const ElfW(Phdr) *phdr,
						     ElfW(Half) phent,
						     ElfW(Addr) *user_entry));
extern void _dl_sysdep_start_cleanup (void);

/* This function is used to unload the cache file if necessary.  */
extern void _dl_unload_cache (void);

/* System-dependent function to read a file's whole contents
   in the most convenient manner available.  */
extern void *_dl_sysdep_read_whole_file (const char *filename,
					 size_t *filesize_ptr,
					 int mmap_prot);

/* Helper function to handle errors while resolving symbols.  */
static void print_unresolved (int errcode, const char *objname,
			      const char *errsting);

/* Helper function to handle errors when a version is missing.  */
static void print_missing_version (int errcode, const char *objname,
				   const char *errsting);

/* Print the various times we collected.  */
static void print_statistics (void);

/* This is a list of all the modes the dynamic loader can be in.  */
enum mode { normal, list, verify, trace };

/* Process all environments variables the dynamic linker must recognize.
   Since all of them start with `LD_' we are a bit smarter while finding
   all the entries.  */
static void process_envvars (enum mode *modep, int *lazyp);

int _dl_argc;
char **_dl_argv;
unsigned int _dl_skip_args;	/* Nonzero if we were run directly.  */
int _dl_verbose;
const char *_dl_platform;
size_t _dl_platformlen;
unsigned long _dl_hwcap;
fpu_control_t _dl_fpu_control = _FPU_DEFAULT;
struct r_search_path *_dl_search_paths;
const char *_dl_profile;
const char *_dl_profile_output;
struct link_map *_dl_profile_map;
int _dl_lazy;
int _dl_debug_libs;
int _dl_debug_impcalls;
int _dl_debug_bindings;
int _dl_debug_symbols;
int _dl_debug_versions;
int _dl_debug_reloc;
int _dl_debug_files;
int _dl_debug_statistics;
const char *_dl_inhibit_rpath;		/* RPATH values which should be
					   ignored.  */
const char *_dl_origin_path;

/* This is a pointer to the map for the main object and through it to
   all loaded objects.  */
struct link_map *_dl_loaded;
/* Pointer to the l_searchlist element of the link map of the main object.  */
struct r_scope_elem *_dl_main_searchlist;
/* Copy of the content of `_dl_main_searchlist'.  */
struct r_scope_elem _dl_initial_searchlist;
/* Array which is used when looking up in the global scope.  */
struct r_scope_elem *_dl_global_scope[2];

/* Set nonzero during loading and initialization of executable and
   libraries, cleared before the executable's entry point runs.  This
   must not be initialized to nonzero, because the unused dynamic
   linker loaded in for libc.so's "ld.so.1" dep will provide the
   definition seen by libc.so's initializer; that value must be zero,
   and will be since that dynamic linker's _dl_start and dl_main will
   never be called.  */
int _dl_starting_up;


static void dl_main (const ElfW(Phdr) *phdr,
		     ElfW(Half) phent,
		     ElfW(Addr) *user_entry);

struct link_map _dl_rtld_map;
struct libname_list _dl_rtld_libname;
struct libname_list _dl_rtld_libname2;

/* Variable for statistics.  */
static hp_timing_t rtld_total_time;
static hp_timing_t relocate_time;
static hp_timing_t load_time;
extern unsigned long int _dl_num_relocations;	/* in dl-lookup.c */

static ElfW(Addr) _dl_start_final (void *arg, struct link_map *bootstrap_map_p,
				   hp_timing_t start_time);

#ifdef RTLD_START
RTLD_START
#else
#error "sysdeps/MACHINE/dl-machine.h fails to define RTLD_START"
#endif

static ElfW(Addr)
_dl_start (void *arg)
{
  struct link_map bootstrap_map;
  hp_timing_t start_time;
  size_t cnt;

  /* This #define produces dynamic linking inline functions for
     bootstrap relocation instead of general-purpose relocation.  */
#define RTLD_BOOTSTRAP
#define RESOLVE(sym, version, flags) \
  ((*(sym))->st_shndx == SHN_UNDEF ? 0 : bootstrap_map.l_addr)
#include "dynamic-link.h"

  if (HP_TIMING_INLINE && HP_TIMING_AVAIL)
    HP_TIMING_NOW (start_time);

  /* Partly clean the `bootstrap_map' structure up.  Don't use `memset'
     since it might nor be built in or inlined and we cannot make function
     calls at this point.  */
  for (cnt = 0;
       cnt < sizeof (bootstrap_map.l_info) / sizeof (bootstrap_map.l_info[0]);
       ++cnt)
    bootstrap_map.l_info[cnt] = 0;

  /* Figure out the run-time load address of the dynamic linker itself.  */
  bootstrap_map.l_addr = elf_machine_load_address ();

  /* Read our own dynamic section and fill in the info array.  */
  bootstrap_map.l_ld = (void *) bootstrap_map.l_addr + elf_machine_dynamic ();
  elf_get_dynamic_info (bootstrap_map.l_ld, bootstrap_map.l_addr,
			bootstrap_map.l_info);

#ifdef ELF_MACHINE_BEFORE_RTLD_RELOC
  ELF_MACHINE_BEFORE_RTLD_RELOC (bootstrap_map.l_info);
#endif

  /* Relocate ourselves so we can do normal function calls and
     data access using the global offset table.  */

  ELF_DYNAMIC_RELOCATE (&bootstrap_map, 0, 0);
  /* Please note that we don't allow profiling of this object and
     therefore need not test whether we have to allocate the array
     for the relocation results (as done in dl-reloc.c).  */

  /* Now life is sane; we can call functions and access global data.
     Set up to use the operating system facilities, and find out from
     the operating system's program loader where to find the program
     header table in core.  Put the rest of _dl_start into a separate
     function, that way the compiler cannot put accesses to the GOT
     before ELF_DYNAMIC_RELOCATE.  */
  return _dl_start_final (arg, &bootstrap_map, start_time);
}


static ElfW(Addr)
_dl_start_final (void *arg, struct link_map *bootstrap_map_p,
		 hp_timing_t start_time)
{
  /* The use of `alloca' here looks ridiculous but it helps.  The goal
     is to avoid the function from being inlined.  There is no official
     way to do this so we use this trick.  gcc never inlines functions
     which use `alloca'.  */
  ElfW(Addr) *start_addr = alloca (sizeof (ElfW(Addr)));

  if (HP_TIMING_AVAIL)
    {
      /* If it hasn't happen yet record the startup time.  */
      if (! HP_TIMING_INLINE)
	HP_TIMING_NOW (start_time);

      /* Initialize the timing functions.  */
      HP_TIMING_DIFF_INIT ();
    }

  /* Transfer data about ourselves to the permanent link_map structure.  */
  _dl_rtld_map.l_addr = bootstrap_map_p->l_addr;
  _dl_rtld_map.l_ld = bootstrap_map_p->l_ld;
  _dl_rtld_map.l_opencount = 1;
  memcpy (_dl_rtld_map.l_info, bootstrap_map_p->l_info,
	  sizeof _dl_rtld_map.l_info);
  _dl_setup_hash (&_dl_rtld_map);

/* Don't bother trying to work out how ld.so is mapped in memory.  */
  _dl_rtld_map.l_map_start = ~0;
  _dl_rtld_map.l_map_end = ~0;

  /* Call the OS-dependent function to set up life so we can do things like
     file access.  It will call `dl_main' (below) to do all the real work
     of the dynamic linker, and then unwind our frame and run the user
     entry point on the same stack we entered on.  */
  *start_addr =  _dl_sysdep_start (arg, &dl_main);

  if (HP_TIMING_AVAIL)
    {
      hp_timing_t end_time;

      /* Get the current time.  */
      HP_TIMING_NOW (end_time);

      /* Compute the difference.  */
      HP_TIMING_DIFF (rtld_total_time, start_time, end_time);
    }

  if (_dl_debug_statistics)
    print_statistics ();

  return *start_addr;
}

/* Now life is peachy; we can do all normal operations.
   On to the real work.  */

void ENTRY_POINT (void);

/* Some helper functions.  */

/* Arguments to relocate_doit.  */
struct relocate_args
{
  struct link_map *l;
  int lazy;
};

struct map_args
{
  /* Argument to map_doit.  */
  char *str;
  /* Return value of map_doit.  */
  struct link_map *main_map;
};

/* Arguments to version_check_doit.  */
struct version_check_args
{
  int doexit;
};

static void
relocate_doit (void *a)
{
  struct relocate_args *args = (struct relocate_args *) a;

  _dl_relocate_object (args->l, args->l->l_scope,
		       args->lazy, 0);
}

static void
map_doit (void *a)
{
  struct map_args *args = (struct map_args *) a;
  args->main_map = _dl_map_object (NULL, args->str, 0, lt_library, 0);
}

static void
version_check_doit (void *a)
{
  struct version_check_args *args = (struct version_check_args *) a;
  if (_dl_check_all_versions (_dl_loaded, 1) && args->doexit)
    /* We cannot start the application.  Abort now.  */
    _exit (1);
}


static inline struct link_map *
find_needed (const char *name)
{
  unsigned int n = _dl_loaded->l_searchlist.r_nlist;

  while (n-- > 0)
    if (_dl_name_match_p (name, _dl_loaded->l_searchlist.r_list[n]))
      return _dl_loaded->l_searchlist.r_list[n];

  /* Should never happen.  */
  return NULL;
}

static int
match_version (const char *string, struct link_map *map)
{
  const char *strtab = (const void *) map->l_info[DT_STRTAB]->d_un.d_ptr;
  ElfW(Verdef) *def;

#define VERDEFTAG (DT_NUM + DT_PROCNUM + DT_VERSIONTAGIDX (DT_VERDEF))
  if (map->l_info[VERDEFTAG] == NULL)
    /* The file has no symbol versioning.  */
    return 0;

  def = (ElfW(Verdef) *) ((char *) map->l_addr
			  + map->l_info[VERDEFTAG]->d_un.d_ptr);
  while (1)
    {
      ElfW(Verdaux) *aux = (ElfW(Verdaux) *) ((char *) def + def->vd_aux);

      /* Compare the version strings.  */
      if (strcmp (string, strtab + aux->vda_name) == 0)
	/* Bingo!  */
	return 1;

      /* If no more definitions we failed to find what we want.  */
      if (def->vd_next == 0)
	break;

      /* Next definition.  */
      def = (ElfW(Verdef) *) ((char *) def + def->vd_next);
    }

  return 0;
}

static const char *library_path;	/* The library search path.  */
static const char *preloadlist;		/* The list preloaded objects.  */
static int version_info;		/* Nonzero if information about
					   versions has to be printed.  */

static void
dl_main (const ElfW(Phdr) *phdr,
	 ElfW(Half) phent,
	 ElfW(Addr) *user_entry)
{
  const ElfW(Phdr) *ph;
  enum mode mode;
  struct link_map **preloads;
  unsigned int npreloads;
  size_t file_size;
  char *file;
  int has_interp = 0;
  unsigned int i;
  int rtld_is_main = 0;
  hp_timing_t start;
  hp_timing_t stop;
  hp_timing_t diff;

  /* Process the environment variable which control the behaviour.  */
  process_envvars (&mode, &_dl_lazy);

  /* Set up a flag which tells we are just starting.  */
  _dl_starting_up = 1;

  if (*user_entry == (ElfW(Addr)) &ENTRY_POINT)
    {
      /* Ho ho.  We are not the program interpreter!  We are the program
	 itself!  This means someone ran ld.so as a command.  Well, that
	 might be convenient to do sometimes.  We support it by
	 interpreting the args like this:

	 ld.so PROGRAM ARGS...

	 The first argument is the name of a file containing an ELF
	 executable we will load and run with the following arguments.
	 To simplify life here, PROGRAM is searched for using the
	 normal rules for shared objects, rather than $PATH or anything
	 like that.  We just load it and use its entry point; we don't
	 pay attention to its PT_INTERP command (we are the interpreter
	 ourselves).  This is an easy way to test a new ld.so before
	 installing it.  */
      rtld_is_main = 1;

      /* Note the place where the dynamic linker actually came from.  */
      _dl_rtld_map.l_name = _dl_argv[0];

      while (_dl_argc > 1)
	if (! strcmp (_dl_argv[1], "--list"))
	  {
	    mode = list;
	    _dl_lazy = -1;	/* This means do no dependency analysis.  */

	    ++_dl_skip_args;
	    --_dl_argc;
	    ++_dl_argv;
	  }
	else if (! strcmp (_dl_argv[1], "--verify"))
	  {
	    mode = verify;

	    ++_dl_skip_args;
	    --_dl_argc;
	    ++_dl_argv;
	  }
	else if (! strcmp (_dl_argv[1], "--library-path") && _dl_argc > 2)
	  {
	    library_path = _dl_argv[2];

	    _dl_skip_args += 2;
	    _dl_argc -= 2;
	    _dl_argv += 2;
	  }
	else if (! strcmp (_dl_argv[1], "--inhibit-rpath") && _dl_argc > 2)
	  {
	    _dl_inhibit_rpath = _dl_argv[2];

	    _dl_skip_args += 2;
	    _dl_argc -= 2;
	    _dl_argv += 2;
	  }
	else
	  break;

      /* If we have no further argument the program was called incorrectly.
	 Grant the user some education.  */
      if (_dl_argc < 2)
	_dl_sysdep_fatal ("\
Usage: ld.so [OPTION]... EXECUTABLE-FILE [ARGS-FOR-PROGRAM...]\n\
You have invoked `ld.so', the helper program for shared library executables.\n\
This program usually lives in the file `/lib/ld.so', and special directives\n\
in executable files using ELF shared libraries tell the system's program\n\
loader to load the helper program from this file.  This helper program loads\n\
the shared libraries needed by the program executable, prepares the program\n\
to run, and runs it.  You may invoke this helper program directly from the\n\
command line to load and run an ELF executable file; this is like executing\n\
that file itself, but always uses this helper program from the file you\n\
specified, instead of the helper program file specified in the executable\n\
file you run.  This is mostly of use for maintainers to test new versions\n\
of this helper program; chances are you did not intend to run this program.\n\
\n\
  --list                list all dependencies and how they are resolved\n\
  --verify              verify that given object really is a dynamically linked\n\
                        object we get handle\n\
  --library-path PATH   use given PATH instead of content of the environment\n\
                        variable LD_LIBRARY_PATH\n\
  --inhibit-rpath LIST  ignore RPATH information in object names in LIST\n",
			  NULL);

      ++_dl_skip_args;
      --_dl_argc;
      ++_dl_argv;

      /* Initialize the data structures for the search paths for shared
	 objects.  */
      _dl_init_paths (library_path);

      if (__builtin_expect (mode, normal) == verify)
	{
	  char *err_str = NULL;
	  struct map_args args;

	  args.str = _dl_argv[0];
	  (void) _dl_catch_error (&err_str, map_doit, &args);
	  if (err_str != NULL)
	    {
	      free (err_str);
	      _exit (EXIT_FAILURE);
	    }
	}
      else
	{
	  HP_TIMING_NOW (start);
	  _dl_map_object (NULL, _dl_argv[0], 0, lt_library, 0);
	  HP_TIMING_NOW (stop);

	  HP_TIMING_DIFF (load_time, start, stop);
	}

      phdr = _dl_loaded->l_phdr;
      phent = _dl_loaded->l_phnum;
      /* We overwrite here a pointer to a malloc()ed string.  But since
	 the malloc() implementation used at this point is the dummy
	 implementations which has no real free() function it does not
	 makes sense to free the old string first.  */
      _dl_loaded->l_name = (char *) "";
      *user_entry = _dl_loaded->l_entry;
    }
  else
    {
      /* Create a link_map for the executable itself.
	 This will be what dlopen on "" returns.  */
      _dl_new_object ((char *) "", "", lt_executable, NULL);
      if (_dl_loaded == NULL)
	_dl_sysdep_fatal ("cannot allocate memory for link map\n", NULL);
      _dl_loaded->l_phdr = phdr;
      _dl_loaded->l_phnum = phent;
      _dl_loaded->l_entry = *user_entry;
      _dl_loaded->l_opencount = 1;

      /* At this point we are in a bit of trouble.  We would have to
	 fill in the values for l_dev and l_ino.  But in general we
	 do not know where the file is.  We also do not handle AT_EXECFD
	 even if it would be passed up.

	 We leave the values here defined to 0.  This is normally no
	 problem as the program code itself is normally no shared
	 object and therefore cannot be loaded dynamically.  Nothing
	 prevent the use of dynamic binaries and in these situations
	 we might get problems.  We might not be able to find out
	 whether the object is already loaded.  But since there is no
	 easy way out and because the dynamic binary must also not
	 have an SONAME we ignore this program for now.  If it becomes
	 a problem we can force people using SONAMEs.  */

      /* We delay initializing the path structure until we got the dynamic
	 information for the program.  */
    }

  /* It is not safe to load stuff after the main program.  */
  _dl_loaded->l_map_end = ~0;
  /* Perhaps the executable has no PT_LOAD header entries at all.  */
  _dl_loaded->l_map_start = ~0;

  /* Scan the program header table for the dynamic section.  */
  for (ph = phdr; ph < &phdr[phent]; ++ph)
    switch (ph->p_type)
      {
      case PT_PHDR:
	/* Find out the load address.  */
	_dl_loaded->l_addr = (ElfW(Addr)) phdr - ph->p_vaddr;
	break;
      case PT_DYNAMIC:
	/* This tells us where to find the dynamic section,
	   which tells us everything we need to do.  */
	_dl_loaded->l_ld = (void *) _dl_loaded->l_addr + ph->p_vaddr;
	break;
      case PT_INTERP:
	/* This "interpreter segment" was used by the program loader to
	   find the program interpreter, which is this program itself, the
	   dynamic linker.  We note what name finds us, so that a future
	   dlopen call or DT_NEEDED entry, for something that wants to link
	   against the dynamic linker as a shared library, will know that
	   the shared object is already loaded.  */
	_dl_rtld_libname.name = ((const char *) _dl_loaded->l_addr
				 + ph->p_vaddr);
	_dl_rtld_libname.next = NULL;
	_dl_rtld_map.l_libname = &_dl_rtld_libname;

	/* Ordinarilly, we would get additional names for the loader from
	   our DT_SONAME.  This can't happen if we were actually linked as
	   a static executable (detect this case when we have no DYNAMIC).
	   If so, assume the filename component of the interpreter path to
	   be our SONAME, and add it to our name list.  */
	if (_dl_rtld_map.l_ld == NULL)
	  {
	    char *p = strrchr (_dl_rtld_libname.name, '/');
	    if (p)
	      {
		_dl_rtld_libname2.name = p+1;
		_dl_rtld_libname2.next = NULL;
		_dl_rtld_libname.next = &_dl_rtld_libname2;
	      }
	  }

	has_interp = 1;
	break;
      case PT_LOAD:
	/* Remember where the main program starts in memory.  */
	{
	  ElfW(Addr) mapstart;
	  mapstart = _dl_loaded->l_addr + (ph->p_vaddr & ~(ph->p_align - 1));
	  if (_dl_loaded->l_map_start > mapstart)
	    _dl_loaded->l_map_start = mapstart;
	}
	break;
      }
  if (! _dl_rtld_map.l_libname && _dl_rtld_map.l_name)
    {
      /* We were invoked directly, so the program might not have a
	 PT_INTERP.  */
      _dl_rtld_libname.name = _dl_rtld_map.l_name;
      _dl_rtld_libname.next = NULL;
      _dl_rtld_map.l_libname =  &_dl_rtld_libname;
    }
  else
    assert (_dl_rtld_map.l_libname); /* How else did we get here?  */

  if (! rtld_is_main)
    {
      /* Extract the contents of the dynamic section for easy access.  */
      elf_get_dynamic_info (_dl_loaded->l_ld, _dl_loaded->l_addr,
			    _dl_loaded->l_info);
      if (_dl_loaded->l_info[DT_HASH])
	/* Set up our cache of pointers into the hash table.  */
	_dl_setup_hash (_dl_loaded);
    }

  if (__builtin_expect (mode, normal) == verify)
    {
      /* We were called just to verify that this is a dynamic
	 executable using us as the program interpreter.  Exit with an
	 error if we were not able to load the binary or no interpreter
	 is specified (i.e., this is no dynamically linked binary.  */
      if (_dl_loaded->l_ld == NULL)
	_exit (1);

      /* We allow here some platform specific code.  */
#ifdef DISTINGUISH_LIB_VERSIONS
      DISTINGUISH_LIB_VERSIONS;
#endif
      _exit (has_interp ? 0 : 2);
    }

  if (! rtld_is_main)
    /* Initialize the data structures for the search paths for shared
       objects.  */
    _dl_init_paths (library_path);

  /* Put the link_map for ourselves on the chain so it can be found by
     name.  Note that at this point the global chain of link maps contains
     exactly one element, which is pointed to by _dl_loaded.  */
  if (! _dl_rtld_map.l_name)
    /* If not invoked directly, the dynamic linker shared object file was
       found by the PT_INTERP name.  */
    _dl_rtld_map.l_name = (char *) _dl_rtld_map.l_libname->name;
  _dl_rtld_map.l_type = lt_library;
  _dl_loaded->l_next = &_dl_rtld_map;
  _dl_rtld_map.l_prev = _dl_loaded;

  /* We have two ways to specify objects to preload: via environment
     variable and via the file /etc/ld.so.preload.  The latter can also
     be used when security is enabled.  */
  preloads = NULL;
  npreloads = 0;

  if (preloadlist)
    {
      /* The LD_PRELOAD environment variable gives list of libraries
	 separated by white space or colons that are loaded before the
	 executable's dependencies and prepended to the global scope
	 list.  If the binary is running setuid all elements
	 containing a '/' are ignored since it is insecure.  */
      char *list = strdupa (preloadlist);
      char *p;

      HP_TIMING_NOW (start);

      while ((p = strsep (&list, " :")) != NULL)
	if (p[0] != '\0'
	    && (! __libc_enable_secure || strchr (p, '/') == NULL))
	  {
	    struct link_map *new_map = _dl_map_object (_dl_loaded, p, 1,
						       lt_library, 0);
	    if (new_map->l_opencount == 1)
	      /* It is no duplicate.  */
	      ++npreloads;
	  }

      HP_TIMING_NOW (stop);
      HP_TIMING_DIFF (diff, start, stop);
      HP_TIMING_ACCUM_NT (load_time, diff);
    }

  /* Read the contents of the file.  */
  file = _dl_sysdep_read_whole_file ("/etc/ld.so.preload", &file_size,
				     PROT_READ | PROT_WRITE);
  if (file)
    {
      /* Parse the file.  It contains names of libraries to be loaded,
	 separated by white spaces or `:'.  It may also contain
	 comments introduced by `#'.  */
      char *problem;
      char *runp;
      size_t rest;

      /* Eliminate comments.  */
      runp = file;
      rest = file_size;
      while (rest > 0)
	{
	  char *comment = memchr (runp, '#', rest);
	  if (comment == NULL)
	    break;

	  rest -= comment - runp;
	  do
	    *comment = ' ';
	  while (--rest > 0 && *++comment != '\n');
	}

      /* We have one problematic case: if we have a name at the end of
	 the file without a trailing terminating characters, we cannot
	 place the \0.  Handle the case separately.  */
      if (file[file_size - 1] != ' ' && file[file_size - 1] != '\t'
	  && file[file_size - 1] != '\n' && file[file_size - 1] != ':')
	{
	  problem = &file[file_size];
	  while (problem > file && problem[-1] != ' ' && problem[-1] != '\t'
		 && problem[-1] != '\n' && problem[-1] != ':')
	    --problem;

	  if (problem > file)
	    problem[-1] = '\0';
	}
      else
	{
	  problem = NULL;
	  file[file_size - 1] = '\0';
	}

      HP_TIMING_NOW (start);

      if (file != problem)
	{
	  char *p;
	  runp = file;
	  while ((p = strsep (&runp, ": \t\n")) != NULL)
	    if (p[0] != '\0')
	      {
		struct link_map *new_map = _dl_map_object (_dl_loaded, p, 1,
							   lt_library, 0);
		if (new_map->l_opencount == 1)
		  /* It is no duplicate.  */
		  ++npreloads;
	      }
	}

      if (problem != NULL)
	{
	  char *p = strndupa (problem, file_size - (problem - file));
	  struct link_map *new_map = _dl_map_object (_dl_loaded, p, 1,
						     lt_library, 0);
	  if (new_map->l_opencount == 1)
	    /* It is no duplicate.  */
	    ++npreloads;
	}

      HP_TIMING_NOW (stop);
      HP_TIMING_DIFF (diff, start, stop);
      HP_TIMING_ACCUM_NT (load_time, diff);

      /* We don't need the file anymore.  */
      __munmap (file, file_size);
    }


  /*
   * Modifications by Red Hat Software
   *
   * Deal with the broken binaries from the non-versioned ages of glibc.
   * If a binary does not have version information enebled, we assume that
   * it is a glibc 2.0 binary andwe load a compatibility library to try to
   * overcome binary incompatibilities.
   *			Blame: gafton@redhat.com
   */
#define LIB_NOVERSION "/lib/libNoVersion.so.1"
  
  if (_dl_loaded->l_info[DT_NUM + DT_PROCNUM + DT_VERSIONTAGIDX (DT_VERNEED)] == NULL) {
      struct link_map *new_map = NULL;
      struct stat test_st;
      int test_fd;
      int can_load;
      
      HP_TIMING_NOW (start);
	   
/*       _dl_sysdep_message("Loading compatibility library... ", NULL); */

      can_load = 1;
      test_fd = __open (LIB_NOVERSION, O_RDONLY);
      if (test_fd < 0) {
	  can_load = 0;
/* 	  _dl_sysdep_message(" Can't find " LIB_NOVERSION "\n",  NULL); */
      } else {
	  if (__fxstat (_STAT_VER, test_fd, &test_st) < 0 || test_st.st_size == 0) {
	      can_load = 0;
/* 	      _dl_sysdep_message(" Can't stat " LIB_NOVERSION "\n",  NULL); */
	  }
      }
      
      if (test_fd >= 0) /* open did no fail.. */
	  __close(test_fd); /* avoid fd leaks */

      if (can_load != 0) {
	  new_map = _dl_map_object (_dl_loaded, LIB_NOVERSION,
				    1, lt_library, 0);
	  if (new_map->l_opencount == 1) {
	      /* It is no duplicate.  */
	      ++npreloads;
/* 	      _dl_sysdep_message(" DONE\n", NULL); */
	  } else {
/* 	      _dl_sysdep_message(" FAILED\n", NULL); */
	  }
      }
	  
      HP_TIMING_NOW (stop);
      HP_TIMING_DIFF (diff, start, stop);
      HP_TIMING_ACCUM_NT (load_time, diff);
  }
      
  if (npreloads != 0)
    {
      /* Set up PRELOADS with a vector of the preloaded libraries.  */
      struct link_map *l;
      preloads = __alloca (npreloads * sizeof preloads[0]);
      l = _dl_rtld_map.l_next; /* End of the chain before preloads.  */
      i = 0;
      do
	{
	  preloads[i++] = l;
	  l = l->l_next;
	} while (l);
      assert (i == npreloads);
    }

  /* Load all the libraries specified by DT_NEEDED entries.  If LD_PRELOAD
     specified some libraries to load, these are inserted before the actual
     dependencies in the executable's searchlist for symbol resolution.  */
  HP_TIMING_NOW (start);
  _dl_map_object_deps (_dl_loaded, preloads, npreloads, mode == trace, 0);
  HP_TIMING_NOW (stop);
  HP_TIMING_DIFF (diff, start, stop);
  HP_TIMING_ACCUM_NT (load_time, diff);

  /* Mark all objects as being in the global scope.  */
  for (i = _dl_loaded->l_searchlist.r_nlist; i > 0; )
    _dl_loaded->l_searchlist.r_list[--i]->l_global = 1;

#ifndef MAP_ANON
  /* We are done mapping things, so close the zero-fill descriptor.  */
  __close (_dl_zerofd);
  _dl_zerofd = -1;
#endif

  /* Remove _dl_rtld_map from the chain.  */
  _dl_rtld_map.l_prev->l_next = _dl_rtld_map.l_next;
  if (_dl_rtld_map.l_next)
    _dl_rtld_map.l_next->l_prev = _dl_rtld_map.l_prev;

  if (__builtin_expect (_dl_rtld_map.l_opencount, 2) > 1)
    {
      /* Some DT_NEEDED entry referred to the interpreter object itself, so
	 put it back in the list of visible objects.  We insert it into the
	 chain in symbol search order because gdb uses the chain's order as
	 its symbol search order.  */
      i = 1;
      while (_dl_loaded->l_searchlist.r_list[i] != &_dl_rtld_map)
	++i;
      _dl_rtld_map.l_prev = _dl_loaded->l_searchlist.r_list[i - 1];
      if (__builtin_expect (mode, normal) == normal)
	_dl_rtld_map.l_next = (i + 1 < _dl_loaded->l_searchlist.r_nlist
			       ? _dl_loaded->l_searchlist.r_list[i + 1]
			       : NULL);
      else
	/* In trace mode there might be an invisible object (which we
	   could not find) after the previous one in the search list.
	   In this case it doesn't matter much where we put the
	   interpreter object, so we just initialize the list pointer so
	   that the assertion below holds.  */
	_dl_rtld_map.l_next = _dl_rtld_map.l_prev->l_next;

      assert (_dl_rtld_map.l_prev->l_next == _dl_rtld_map.l_next);
      _dl_rtld_map.l_prev->l_next = &_dl_rtld_map;
      if (_dl_rtld_map.l_next)
	{
	  assert (_dl_rtld_map.l_next->l_prev == _dl_rtld_map.l_prev);
	  _dl_rtld_map.l_next->l_prev = &_dl_rtld_map;
	}
    }

  /* Now let us see whether all libraries are available in the
     versions we need.  */
  {
    struct version_check_args args;
    args.doexit = mode == normal;
    _dl_receive_error (print_missing_version, version_check_doit, &args);
  }

  if (__builtin_expect (mode, normal) != normal)
    {
      /* We were run just to list the shared libraries.  It is
	 important that we do this before real relocation, because the
	 functions we call below for output may no longer work properly
	 after relocation.  */
      if (! _dl_loaded->l_info[DT_NEEDED])
	_dl_sysdep_message ("\t", "statically linked\n", NULL);
      else
	{
	  struct link_map *l;

	  for (l = _dl_loaded->l_next; l; l = l->l_next)
	    if (l->l_opencount == 0)
	      /* The library was not found.  */
	      _dl_sysdep_message ("\t", l->l_libname->name, " => not found\n",
				  NULL);
	    else
	      {
		char buf[20], *bp;
		buf[sizeof buf - 1] = '\0';
		bp = _itoa_word (l->l_addr, &buf[sizeof buf - 1], 16, 0);
		while ((size_t) (&buf[sizeof buf - 1] - bp)
		       < sizeof l->l_addr * 2)
		  *--bp = '0';
		_dl_sysdep_message ("\t", l->l_libname->name, " => ",
				    l->l_name, " (0x", bp, ")\n", NULL);
	      }
	}

      if (__builtin_expect (mode, trace) != trace)
	for (i = 1; i < _dl_argc; ++i)
	  {
	    const ElfW(Sym) *ref = NULL;
	    ElfW(Addr) loadbase = _dl_lookup_symbol (_dl_argv[i], &ref,
						     _dl_loaded->l_scope,
						     "argument",
						     ELF_MACHINE_JMP_SLOT);
	    char buf[20], *bp;
	    buf[sizeof buf - 1] = '\0';
	    bp = _itoa_word (ref->st_value, &buf[sizeof buf - 1], 16, 0);
	    while ((size_t) (&buf[sizeof buf - 1] - bp) < sizeof loadbase * 2)
	      *--bp = '0';
	    _dl_sysdep_message (_dl_argv[i], " found at 0x", bp, NULL);
	    buf[sizeof buf - 1] = '\0';
	    bp = _itoa_word (loadbase, &buf[sizeof buf - 1], 16, 0);
	    while ((size_t) (&buf[sizeof buf - 1] - bp) < sizeof loadbase * 2)
	      *--bp = '0';
	    _dl_sysdep_message (" in object at 0x", bp, "\n", NULL);
	  }
      else
	{
	  if (_dl_lazy >= 0)
	    {
	      /* We have to do symbol dependency testing.  */
	      struct relocate_args args;
	      struct link_map *l;

	      args.lazy = _dl_lazy;

	      l = _dl_loaded;
	      while (l->l_next)
		l = l->l_next;
	      do
		{
		  if (l != &_dl_rtld_map && l->l_opencount > 0)
		    {
		      args.l = l;
		      _dl_receive_error (print_unresolved, relocate_doit,
					 &args);
		    }
		  l = l->l_prev;
		} while (l);
	    }

#define VERNEEDTAG (DT_NUM + DT_PROCNUM + DT_VERSIONTAGIDX (DT_VERNEED))
	  if (version_info)
	    {
	      /* Print more information.  This means here, print information
		 about the versions needed.  */
	      int first = 1;
	      struct link_map *map = _dl_loaded;

	      for (map = _dl_loaded; map != NULL; map = map->l_next)
		{
		  const char *strtab;
		  ElfW(Dyn) *dyn = map->l_info[VERNEEDTAG];
		  ElfW(Verneed) *ent;

		  if (dyn == NULL)
		    continue;

		  strtab = (const void *) map->l_info[DT_STRTAB]->d_un.d_ptr;
		  ent = (ElfW(Verneed) *) (map->l_addr + dyn->d_un.d_ptr);

		  if (first)
		    {
		      _dl_sysdep_message ("\n\tVersion information:\n", NULL);
		      first = 0;
		    }

		  _dl_sysdep_message ("\t", (map->l_name[0]
					     ? map->l_name : _dl_argv[0]),
				      ":\n", NULL);

		  while (1)
		    {
		      ElfW(Vernaux) *aux;
		      struct link_map *needed;

		      needed = find_needed (strtab + ent->vn_file);
		      aux = (ElfW(Vernaux) *) ((char *) ent + ent->vn_aux);

		      while (1)
			{
			  const char *fname = NULL;

			  _dl_sysdep_message ("\t\t",
					      strtab + ent->vn_file,
					      " (", strtab + aux->vna_name,
					      ") ",
					      (aux->vna_flags
					       & VER_FLG_WEAK
					       ? "[WEAK] " : ""),
					      "=> ", NULL);

			  if (needed != NULL
			      && match_version (strtab+aux->vna_name, needed))
			    fname = needed->l_name;

			  _dl_sysdep_message (fname ?: "not found", "\n",
					      NULL);

			  if (aux->vna_next == 0)
			    /* No more symbols.  */
			    break;

			  /* Next symbol.  */
			  aux = (ElfW(Vernaux) *) ((char *) aux
						   + aux->vna_next);
			}

		      if (ent->vn_next == 0)
			/* No more dependencies.  */
			break;

		      /* Next dependency.  */
		      ent = (ElfW(Verneed) *) ((char *) ent + ent->vn_next);
		    }
		}
	    }
	}

      _exit (0);
    }

  {
    /* Now we have all the objects loaded.  Relocate them all except for
       the dynamic linker itself.  We do this in reverse order so that copy
       relocs of earlier objects overwrite the data written by later
       objects.  We do not re-relocate the dynamic linker itself in this
       loop because that could result in the GOT entries for functions we
       call being changed, and that would break us.  It is safe to relocate
       the dynamic linker out of order because it has no copy relocs (we
       know that because it is self-contained).  */

    struct link_map *l;
    int consider_profiling = _dl_profile != NULL;
    hp_timing_t start;
    hp_timing_t stop;
    hp_timing_t add;

    /* If we are profiling we also must do lazy reloaction.  */
    _dl_lazy |= consider_profiling;

    l = _dl_loaded;
    while (l->l_next)
      l = l->l_next;

    HP_TIMING_NOW (start);
    do
      {
	if (l != &_dl_rtld_map)
	  _dl_relocate_object (l, l->l_scope, _dl_lazy, consider_profiling);

	l = l->l_prev;
      }
    while (l);
    HP_TIMING_NOW (stop);

    HP_TIMING_DIFF (relocate_time, start, stop);

    /* Do any necessary cleanups for the startup OS interface code.
       We do these now so that no calls are made after rtld re-relocation
       which might be resolved to different functions than we expect.
       We cannot do this before relocating the other objects because
       _dl_relocate_object might need to call `mprotect' for DT_TEXTREL.  */
    _dl_sysdep_start_cleanup ();

    if (_dl_rtld_map.l_opencount > 1)
      {
	/* There was an explicit ref to the dynamic linker as a shared lib.
	   Re-relocate ourselves with user-controlled symbol definitions.  */
	HP_TIMING_NOW (start);
	_dl_relocate_object (&_dl_rtld_map, _dl_loaded->l_scope, 0, 0);
	HP_TIMING_NOW (stop);
	HP_TIMING_DIFF (add, start, stop);
	HP_TIMING_ACCUM_NT (relocate_time, add);
      }
  }

  /* Now set up the variable which helps the assembler startup code.  */
  _dl_main_searchlist = &_dl_loaded->l_searchlist;
  _dl_global_scope[0] = &_dl_loaded->l_searchlist;

  /* Safe the information about the original global scope list since
     we need it in the memory handling later.  */
  _dl_initial_searchlist = *_dl_main_searchlist;

  {
    /* Initialize _r_debug.  */
    struct r_debug *r = _dl_debug_initialize (_dl_rtld_map.l_addr);
    struct link_map *l;

    l = _dl_loaded;

#ifdef ELF_MACHINE_DEBUG_SETUP

    /* Some machines (e.g. MIPS) don't use DT_DEBUG in this way.  */

    ELF_MACHINE_DEBUG_SETUP (l, r);
    ELF_MACHINE_DEBUG_SETUP (&_dl_rtld_map, r);

#else

    if (l->l_info[DT_DEBUG])
      /* There is a DT_DEBUG entry in the dynamic section.  Fill it in
	 with the run-time address of the r_debug structure  */
      l->l_info[DT_DEBUG]->d_un.d_ptr = (ElfW(Addr)) r;

    /* Fill in the pointer in the dynamic linker's own dynamic section, in
       case you run gdb on the dynamic linker directly.  */
    if (_dl_rtld_map.l_info[DT_DEBUG])
      _dl_rtld_map.l_info[DT_DEBUG]->d_un.d_ptr = (ElfW(Addr)) r;

#endif

    /* Notify the debugger that all objects are now mapped in.  */
    r->r_state = RT_ADD;
    _dl_debug_state ();
  }

#ifndef MAP_COPY
  /* We must munmap() the cache file.  */
  _dl_unload_cache ();
#endif

  /* Now enable profiling if needed.  */
  if (_dl_profile_map != NULL)
    /* We must prepare the profiling.  */
    _dl_start_profile (_dl_profile_map, _dl_profile_output);

  /* Once we return, _dl_sysdep_start will invoke
     the DT_INIT functions and then *USER_ENTRY.  */
}

/* This is a little helper function for resolving symbols while
   tracing the binary.  */
static void
print_unresolved (int errcode __attribute__ ((unused)), const char *objname,
		  const char *errstring)
{
  if (objname[0] == '\0')
    objname = _dl_argv[0] ?: "<main program>";
  _dl_sysdep_error (errstring, "	(", objname, ")\n", NULL);
}

/* This is a little helper function for resolving symbols while
   tracing the binary.  */
static void
print_missing_version (int errcode __attribute__ ((unused)),
		       const char *objname, const char *errstring)
{
  _dl_sysdep_error (_dl_argv[0] ?: "<program name unknown>", ": ",
		    objname, ": ", errstring, "\n", NULL);
}

/* Nonzero if any of the debugging options is enabled.  */
static int any_debug;

/* Process the string given as the parameter which explains which debugging
   options are enabled.  */
static void
process_dl_debug (const char *dl_debug)
{
  size_t len;
#define separators " ,:"
  do
    {
      len = 0;
      /* Skip separating white spaces and commas.  */
      dl_debug += strspn (dl_debug, separators);
      if (*dl_debug != '\0')
	{
	  len = strcspn (dl_debug, separators);

	  switch (len)
	    {
	    case 3:
	      /* This option is not documented since it is not generally
		 useful.  */
	      if (memcmp (dl_debug, "all", 3) == 0)
		{
		  _dl_debug_libs = 1;
		  _dl_debug_impcalls = 1;
		  _dl_debug_reloc = 1;
		  _dl_debug_files = 1;
		  _dl_debug_symbols = 1;
		  _dl_debug_bindings = 1;
		  _dl_debug_versions = 1;
		  any_debug = 1;
		  continue;
		}
	      break;

	    case 4:
	      if (memcmp (dl_debug, "help", 4) == 0)
		{
		  _dl_sysdep_message ("\
Valid options for the LD_DEBUG environment variable are:\n\
\n\
  bindings  display information about symbol binding\n\
  files     display processing of files and libraries\n\
  help      display this help message and exit\n\
  libs      display library search paths\n\
  reloc     display relocation processing\n\
  symbols   display symbol table processing\n\
  versions  display version dependencies\n\
\n\
To direct the debugging output into a file instead of standard output\n\
a filename can be specified using the LD_DEBUG_OUTPUT environment variable.\n",
				  NULL);
		  _exit (0);
		}

	      if (memcmp (dl_debug, "libs", 4) == 0)
		{
		  _dl_debug_libs = 1;
		  _dl_debug_impcalls = 1;
		  any_debug = 1;
		  continue;
		}
	      break;

	    case 5:
	      if (memcmp (dl_debug, "reloc", 5) == 0)
		{
		  _dl_debug_reloc = 1;
		  _dl_debug_impcalls = 1;
		  any_debug = 1;
		  continue;
		}

	      if (memcmp (dl_debug, "files", 5) == 0)
		{
		  _dl_debug_files = 1;
		  _dl_debug_impcalls = 1;
		  any_debug = 1;
		  continue;
		}
	      break;

	    case 7:
	      if (memcmp (dl_debug, "symbols", 7) == 0)
		{
		  _dl_debug_symbols = 1;
		  _dl_debug_impcalls = 1;
		  any_debug = 1;
		  continue;
		}
	      break;

	    case 8:
	      if (memcmp (dl_debug, "bindings", 8) == 0)
		{
		  _dl_debug_bindings = 1;
		  _dl_debug_impcalls = 1;
		  any_debug = 1;
		  continue;
		}

	      if (memcmp (dl_debug, "versions", 8) == 0)
		{
		  _dl_debug_versions = 1;
		  _dl_debug_impcalls = 1;
		  any_debug = 1;
		  continue;
		}
	      break;

	    case 10:
	      if (memcmp (dl_debug, "statistics", 10) == 0)
		{
		  _dl_debug_statistics = 1;
		  continue;
		}
	      break;

	    default:
	      break;
	    }

	  {
	    /* Display a warning and skip everything until next separator.  */
	    char *startp = strndupa (dl_debug, len);
	    _dl_sysdep_error ("warning: debug option `", startp,
			      "' unknown; try LD_DEBUG=help\n", NULL);
	    break;
	  }
	}
    }
  while (*(dl_debug += len) != '\0');
}

/* Process all environments variables the dynamic linker must recognize.
   Since all of them start with `LD_' we are a bit smarter while finding
   all the entries.  */
static void
process_envvars (enum mode *modep, int *lazyp)
{
  char **runp = NULL;
  char *envline;
  enum mode mode = normal;
  int bind_now = 0;
  char *debug_output = NULL;

  /* This is the default place for profiling data file.  */
  _dl_profile_output = "/var/tmp";

  while ((envline = _dl_next_ld_env_entry (&runp)) != NULL)
    {
      size_t len = strcspn (envline, "=") - 3;

      switch (len)
	{
	case 4:
	  /* Warning level, verbose or not.  */
	  if (memcmp (&envline[3], "WARN", 4) == 0)
	    _dl_verbose = envline[8] != '\0';
	  break;

	case 5:
	  /* Debugging of the dynamic linker?  */
	  if (memcmp (&envline[3], "DEBUG", 5) == 0)
	    process_dl_debug (&envline[9]);
	  break;

	case 7:
	  /* Print information about versions.  */
	  if (memcmp (&envline[3], "VERBOSE", 7) == 0)
	    {
	      version_info = envline[11] != '\0';
	      break;
	    }

	  /* List of objects to be preloaded.  */
	  if (memcmp (&envline[3], "PRELOAD", 7) == 0)
	    {
	      preloadlist = &envline[11];
	      break;
	    }

	  /* Which shared object shall be profiled.  */
	  if (memcmp (&envline[3], "PROFILE", 7) == 0)
	    {
	      _dl_profile = &envline[11];
	      if (*_dl_profile == '\0')
		_dl_profile = NULL;
	    }
	  break;

	case 8:
	  /* Do we bind early?  */
	  if (memcmp (&envline[3], "BIND_NOW", 8) == 0)
	    bind_now = envline[12] != '\0';
	  break;

	case 9:
	  /* Test whether we want to see the content of the auxiliary
	     array passed up from the kernel.  */
	  if (memcmp (&envline[3], "SHOW_AUXV", 9) == 0)
	    _dl_show_auxv ();
	  break;

	case 10:
	  /* Mask for the important hardware capabilities.  */
	  if (memcmp (&envline[3], "HWCAP_MASK", 10) == 0)
	    _dl_hwcap_mask = strtoul (&envline[14], NULL, 0);
	  break;

	case 11:
	  /* Path where the binary is found.  */
	  if (!__libc_enable_secure
	      && memcmp (&envline[3], "ORIGIN_PATH", 11) == 0)
	    _dl_origin_path = &envline[15];
	  break;

	case 12:
	  /* Where to place the profiling data file.  */
	  if (memcmp (&envline[3], "DEBUG_OUTPUT", 12) == 0)
	    {
	      debug_output = &envline[16];
	      break;
	    }

	  /* The library search path.  */
	  if (memcmp (&envline[3], "LIBRARY_PATH", 12) == 0)
	    library_path = &envline[16];
	  break;

	case 14:
	  /* Where to place the profiling data file.  */
	  if (!__libc_enable_secure
	      && memcmp (&envline[3], "PROFILE_OUTPUT", 14) == 0)
	    {
	      _dl_profile_output = &envline[18];
	      if (*_dl_profile_output == '\0')
		_dl_profile_output = "/var/tmp";
	    }
	  break;

	case 20:
	  /* The mode of the dynamic linker can be set.  */
	  if (memcmp (&envline[3], "TRACE_LOADED_OBJECTS", 20) == 0)
	    mode = trace;
	  break;

	  /* We might have some extra environment variable to handle.  This
	     is tricky due to the pre-processing of the length of the name
	     in the switch statement here.  The code here assumes that added
	     environment variables have a different length.  */
#ifdef EXTRA_LD_ENVVARS
	  EXTRA_LD_ENVVARS
#endif
	}
    }

  /* Extra security for SUID binaries.  Remove all dangerous environment
     variables.  */
  if (__libc_enable_secure)
    {
      static const char *unsecure_envvars[] =
      {
#ifdef EXTRA_UNSECURE_ENVVARS
	EXTRA_UNSECURE_ENVVARS
#endif
      };
      size_t cnt;

      if (preloadlist != NULL)
	unsetenv ("LD_PRELOAD");
      if (library_path != NULL)
	unsetenv ("LD_LIBRARY_PATH");

      for (cnt = 0;
	   cnt < sizeof (unsecure_envvars) / sizeof (unsecure_envvars[0]);
	   ++cnt)
	unsetenv (unsecure_envvars[cnt]);
    }

  /* If we have to run the dynamic linker in debugging mode and the
     LD_DEBUG_OUTPUT environment variable is given, we write the debug
     messages to this file.  */
  if (any_debug && debug_output != NULL && !__libc_enable_secure)
    {
      size_t name_len = strlen (debug_output);
      char buf[name_len + 12];
      char *startp;

      buf[name_len + 11] = '\0';
      startp = _itoa_word (__getpid (), &buf[name_len + 11], 10, 0);
      *--startp = '.';
      startp = memcpy (startp - name_len, debug_output, name_len);

      _dl_debug_fd = __open (startp, O_WRONLY | O_APPEND | O_CREAT, 0666);
      if (_dl_debug_fd == -1)
	/* We use standard output if opening the file failed.  */
	_dl_debug_fd = STDOUT_FILENO;
    }

  /* LAZY is determined by the environment variable LD_WARN and
     LD_BIND_NOW if we trace the binary.  */
  if (__builtin_expect (mode, normal) == trace)
    *lazyp = _dl_verbose ? !bind_now : -1;
  else
    *lazyp = !bind_now;

  *modep = mode;
}


/* Print the various times we collected.  */
static void
print_statistics (void)
{
  char buf[200];
  char *cp;
  char *wp;

  /* Total time rtld used.  */
  if (HP_TIMING_AVAIL)
    {
      HP_TIMING_PRINT (buf, sizeof (buf), rtld_total_time);
      _dl_debug_message (1, "\nruntime linker statistics:\n"
			 "  total startup time in dynamic loader: ",
			 buf, "\n", NULL);
    }

  /* Print relocation statistics.  */
  if (HP_TIMING_AVAIL)
    {
      HP_TIMING_PRINT (buf, sizeof (buf), relocate_time);
      _dl_debug_message (1, "            time needed for relocation: ", buf,
			 NULL);
      cp = _itoa_word ((1000 * relocate_time) / rtld_total_time,
		       buf + sizeof (buf), 10, 0);
      wp = buf;
      switch (buf + sizeof (buf) - cp)
	{
	case 3:
	  *wp++ = *cp++;
	case 2:
	  *wp++ = *cp++;
	case 1:
	  *wp++ = '.';
	  *wp++ = *cp++;
	}
      *wp = '\0';
      _dl_debug_message (0, " (", buf, "%)\n", NULL);
    }

  buf[sizeof (buf) - 1] = '\0';
  _dl_debug_message (1, "                 number of relocations: ",
		     _itoa_word (_dl_num_relocations,
				 buf + sizeof (buf) - 1, 10, 0),
		     "\n", NULL);

  /* Time spend while loading the object and the dependencies.  */
  if (HP_TIMING_AVAIL)
    {
      HP_TIMING_PRINT (buf, sizeof (buf), load_time);
      _dl_debug_message (1, "           time needed to load objects: ", buf,
			 NULL);
      cp = _itoa_word ((1000 * load_time) / rtld_total_time,
		       buf + sizeof (buf), 10, 0);
      wp = buf;
      switch (buf + sizeof (buf) - cp)
	{
	case 3:
	  *wp++ = *cp++;
	case 2:
	  *wp++ = *cp++;
	case 1:
	  *wp++ = '.';
	  *wp++ = *cp++;
	}
      *wp = '\0';
      _dl_debug_message (0, " (", buf, "%)\n", NULL);
    }
}
