/*
 *  Shamelessly stolen/adapted from nm.c - binutils-1.9.
 *  Mitch DSouza - m.dsouza@mrc-apu.cam.ac.uk 
 * 
 *  1994-11-26: '_preload_main_' feature added.
 *  Tristan Gingold - C/O gingold@amoko.saclay.cea.fr
 */

/* Describe symbol table of a rel file.
   Copyright (C) 1986, 1988, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "../config.h"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/file.h>
#include <a.out.h>
#include <strings.h>
#include <alloca.h>
#include <stdlib.h>
#include <sharedlib.h>

#define min(x,y) ((x)<(y)?(x):(y))
int fdprintf(int fd, const char *fmt, ...);

/* Always use the GNU version of debugging symbol type codes, if possible.  */
#include "stab.h"
struct exec hdr;

/* Current file's name.  */
char *input_name;

/* The __environ array is declared in ld.so.c */
extern char **__environ;

/* Function to call when the file has just been linked */
/* Arguments:	VER: version number
 *		NLIBS: number of libraries
 *	     	LIBS: array of the linked libraries path
 *		NMOD: number of modules (preloaded binaries)
 *		MODS: array of the modules name
 *		ARGV0: argv[0] of the program
 *		ENVIRON: n.c.
 */
typedef void (*callmefunc)(int ver, int nlibs, char **libs,
		 int nmod, char **mods, char *argv0, char **environ);
callmefunc call_me;
#define CALL_ME_VER 1	/* The argument version */

/* Name of the function */
#define CALL_ME_NAME "__preload_main_"

static void do_one_rel_file ();
static char preload_str[]="preload_";
static unsigned preload_str_len = sizeof(preload_str)-1;

/* Like malloc but get fatal error if memory is exhausted.  */
#if 0
static char *
xmalloc (size)
     unsigned size;
{
  char *result = alloca (size);
  if (!result) {
    fdprintf (2,"%s: virtual memory exhausted (attempt to alloc %d bytes)\n", LDSO_IMAGE,size);
    exit (1);
  }
  return result;
}
#else
#define xmalloc(size) alloca(size)
#endif

typedef struct {
	long txtoff;
	long txtend;
} libtext_t;
libtext_t libn[10];
int nlibs;

void
ldpreload (char *name, int ldd, char **libs, int num, char *argv0)
{
  int desc, err, fd, i;
  unsigned nchars;
  struct exec h;


  if (!name || !*name)
     return;

  desc = open (name, O_RDONLY, 0);

  if (desc < 0)
    {
      fdprintf(2,"%s: %s for `%s'\n", LDSO_IMAGE, strerror(errno), name);
      return;
    }

  input_name = name;
  call_me = (callmefunc)0;
  nchars = read (desc, &hdr, sizeof(struct exec));
  if (nchars != sizeof(struct exec) || !hdr.a_entry) {
	fdprintf(2,"%s: `%s' not a relocatable object\n",LDSO_IMAGE, name);
	return;
  }
  else {
    if ((err=uselib(name))) {
	fdprintf(2,"%s: loading failure for `%s' (%s)\n",LDSO_IMAGE,name,
		 strerror(errno));
	return;
    } 
   for (nlibs=num,i=0; i < nlibs; i++) {
	/* No checking because this has already succeeded when being mmaped */

	fd = open (libs[i], O_RDONLY);
	read (fd, &h, sizeof(struct exec));
	libn[i].txtoff=h.a_entry;
	libn[i].txtend=h.a_entry+h.a_text;
	close (fd);
    }
#ifdef DEBUG
  for (i=0; i<nlibs; i++)
	fdprintf(2,"%2d: 0x%x - 0x%x (%s)\n",i,libn[i].txtoff,libn[i].txtend,libs[i]);
#endif
   if (ldd)
     fdprintf(2, "\t%s\n", name);
   do_one_rel_file (desc, ldd);
   if (call_me)
     (*call_me)(CALL_ME_VER, num, libs, 1, &name, argv0, __environ);
  }
  close (desc);
}

/* Read a file's header and fill in various pieces of information.
   Return 0 on failure.  */


static int
read_header_info (desc, syms_offset, syms_size, strs_offset, strs_size)
     int desc;
     long int *syms_offset;
     unsigned int *syms_size;
     long int *strs_offset;
     unsigned int *strs_size;
{
  unsigned int len;

    lseek (desc, 0, SEEK_SET);
    len = read (desc, (char *) &hdr, sizeof (struct exec));
    if (len == sizeof (struct exec) && !N_BADMAG (hdr))
      {
	*syms_offset = N_SYMOFF(hdr);
	*syms_size = hdr.a_syms;
	*strs_offset = N_STROFF(hdr);
	lseek(desc, N_STROFF(hdr) , SEEK_SET);
	if ((unsigned)read (desc, (char *) strs_size, sizeof *strs_size) != sizeof *strs_size)
	  {
	    fdprintf (2, "%s: cannot read string table size of `%s'\n", LDSO_IMAGE, input_name);
	    return 0;
	  }
	return 1;
      }
  return 0;
}

static void lookup_symbol ();
struct nlist *symbols_and_strings;
int symcount;

static void
do_one_rel_file (desc, ldd)
     int desc;
     int ldd;
{
  int totalsize;
  char *strings;
  long int syms_offset, strs_offset;
  unsigned int syms_size, strs_size;
  int i;

  if (!read_header_info (desc, &syms_offset, &syms_size, &strs_offset, &strs_size))
    {
      fdprintf (2,"%s: malformed preload file `%s' (not a rel file)\n", LDSO_IMAGE, input_name);
      return;
    }

  /* Number of symbol entries in the file.  */
  symcount = syms_size / sizeof (struct nlist);

  if (!symcount) {
	fdprintf(2,"%s: can't preload stripped object `%s'\n", LDSO_IMAGE, input_name);
	return;
  }
  totalsize = strs_size + syms_size;

  /* Allocate space for symbol entries and string table.  */
  symbols_and_strings = (struct nlist *) xmalloc (totalsize*sizeof(struct nlist *));
  strings = (char *) symbols_and_strings + syms_size;

#if 0
  fdprintf(2,"symsiz=%ld, strsize=%ld\n",syms_size,strs_size);
  fdprintf(2,"symoff=%ld, stroff=%ld\n",syms_offset,strs_offset);
#endif

  /* Read them both in.  */
  lseek (desc, syms_offset , SEEK_SET);
  if (syms_size != (unsigned) read (desc, (char *) symbols_and_strings, syms_size))
    {
      fdprintf (2,"%s: premature end of file while preloading `%s'\n", LDSO_IMAGE, input_name);
      return;
    }

  lseek (desc, strs_offset, SEEK_SET);
  if (strs_size != (unsigned) read (desc, (char *) strings, strs_size))
    {
      fdprintf (2,"%s: premature end of file while preloading `%s'\n", LDSO_IMAGE,input_name);
      return;
    }
  /* Modify each symbol entry to point directly at the symbol name.
     This is so the sort routine does not need to be passed
     the value of `strings' separately.  */

  {
    struct nlist *p = symbols_and_strings;
    struct nlist *end = symbols_and_strings + symcount;

    for (; p < end; p++)
      {
	/* A zero index means there is no string.  */
	if (p->n_un.n_strx != 0)
	  {
	    if (p->n_un.n_strx > 0 && (unsigned) p->n_un.n_strx < strs_size)
	      p->n_un.n_name = strings + p->n_un.n_strx;
	    else
	      {
		fdprintf (2, "%s: invalid string table offset for preload file `%s'\n", LDSO_IMAGE, input_name);
		return;
	      }
	  }
      }
  }

  /* Print the symbols in the order they are now in.  */

  for (i = 0; i < symcount; i++)
    {
      if (symbols_and_strings[i].n_type == (N_ABS|N_EXT) &&
	  strlen (symbols_and_strings[i].n_un.n_name) > preload_str_len &&
	  strncmp(symbols_and_strings[i].n_un.n_name, preload_str, preload_str_len) == 0)
        {
	  lookup_symbol (&symbols_and_strings[i],ldd);
        } else {
	  /* Is it the CALL_ME function ? */
          if (!call_me &&
	      symbols_and_strings[i].n_type == (N_TEXT|N_EXT) &&
	      strcmp(symbols_and_strings[i].n_un.n_name, CALL_ME_NAME) == 0)
	    call_me = (callmefunc)(symbols_and_strings[i].n_value);
	}
    }
}

static long
find_func(sym)
    struct nlist *sym;
{
int i;

  for (i = 0; i < symcount; i++) {
  /* __PLT_'s are always N_ABS|N_EXT */

    if (symbols_and_strings[i].n_type != (N_TEXT|N_EXT) ||
	symbols_and_strings[i].n_un.n_name == NULL ||
	symbols_and_strings[i].n_value == sym->n_value ||
	strlen(symbols_and_strings[i].n_un.n_name) != strlen(sym->n_un.n_name)-preload_str_len+1)
		continue;

    if (strcmp(symbols_and_strings[i].n_un.n_name,
	       sym->n_un.n_name + preload_str_len -1)==0) {
#ifdef DEBUG
fdprintf(2,"returning hit %s == %s at 0%x\n",
	 symbols_and_strings[i].n_un.n_name,
	 sym->n_un.n_name,
	 symbols_and_strings[i].n_value);
#endif
	return symbols_and_strings[i].n_value;
    }
  }
return 0;
}

static void
lookup_symbol (sym, ldd)
     struct nlist *sym;
     int ldd;
{
long oldaddr;			/* The original address */
long newaddr;			/* The new address to jump to */
int i;

  if (!(newaddr=find_func(sym)))
	return;

  oldaddr = sym->n_value;
  /* See whether this memory location has been mmap()'ed before fixup */
  for (i=0;i<nlibs ; i++) {
	if (oldaddr > (long)libn[i].txtoff && oldaddr < (long)libn[i].txtend)
		goto fixup;
  }
  return;

fixup:

#if defined (i386)
  /* Calculate the new offset minus the jump instruction and offset */
  oldaddr=newaddr-sym->n_value-0x5;
#elif defined (mc68000)
  /* Use the new address (absolut) */
  oldaddr = sym->n_value;
#else
#error "Architecture not supported"
#endif

if (ldd)
	fdprintf(1, "\t\tpreload (%s)\n", sym->n_un.n_name+preload_str_len);

#ifdef DEBUG
  fdprintf(2,"preload fixup %s 0x%x -> 0x%x (jmp offset=0x%x)\n",
	   sym->n_un.n_name,  sym->n_value, newaddr, oldaddr);
#endif

#if 0
  ((char *)oldaddr)[0]=0xe9;	/* The jmp instruction in hex */
#endif

  /* Copy the new address into the old lib pointer location */
#if defined (i386)
  bcopy(&oldaddr, &((char *)sym->n_value)[1], sizeof(oldaddr));
#elif defined (mc68000)
  bcopy(&oldaddr, &((char *)sym->n_value)[2], sizeof(oldaddr));
#endif

}
