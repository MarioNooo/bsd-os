/*	BSDI readelf.c,v 1.3 1998/09/09 06:11:10 torek Exp	*/

/* adapted from Eric Youngdale's readelf program */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/elf.h>
#include "../config.h"
#include <sys/types.h>

#ifndef ELF32_R_SYM
#define ELF32_R_SYM(x) ((x) >> 8)
#endif
#ifndef ELF32_R_TYPE
#define ELF32_R_TYPE(x) ((x) & 0xff)
#endif

void warn(char *fmt, ...);
char *xstrdup(char *);

struct needed_tab
{
  char *soname;
  int type;
};

struct needed_tab needed_tab[] = {
  { "libc.so.5",    LIB_ELF_LIBC5 },
  { "libm.so.5",    LIB_ELF_LIBC5 },
  { "libdl.so.1",   LIB_ELF_LIBC5 },
  { "libc.so.6",    LIB_ELF_LIBC6 },
  { "libm.so.6",    LIB_ELF_LIBC6 },
  { "libdl.so.2",   LIB_ELF_LIBC6 },
  { NULL,           LIB_ELF }
};

char *readsoname(char *name, FILE *infile, int *type)
{
  struct elfhdr *epnt;
  struct elf_phdr *ppnt;
  int i, j;
  char *header;
  unsigned int dynamic_addr = 0;
  unsigned int dynamic_size = 0;
  int strtab_val = 0;
  int needed_val;
  int loadaddr = -1;
  struct dynamic *dpnt;
  struct stat st;
  char *needed;
  char *soname = NULL;
  int multi_libcs = 0;
  unsigned int page_mask = getpagesize() - 1;

  *type = LIB_ELF;

  if (fstat(fileno(infile), &st))
    return NULL;
  header = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fileno(infile), 0);
  if (header == (caddr_t)-1)
    return NULL;

  epnt = (struct elfhdr *)header;
  if ((int)(epnt+1) > (int)(header + st.st_size))
    goto skip;

  ppnt = (struct elf_phdr *)&header[epnt->e_phoff];
  if ((int)ppnt < (int)header ||
      (int)(ppnt+epnt->e_phnum) > (int)(header + st.st_size))
    goto skip;

  for(i = 0; i < epnt->e_phnum; i++)
  {
    if (loadaddr == -1 && ppnt->p_type == PT_LOAD) 
      loadaddr = (ppnt->p_vaddr & ~page_mask) -
	(ppnt->p_offset & ~page_mask);
    if(ppnt->p_type == 2)
    {
      dynamic_addr = ppnt->p_offset;
      dynamic_size = ppnt->p_filesz;
    };
    ppnt++;
  };
    
  dpnt = (struct dynamic *) &header[dynamic_addr];
  dynamic_size = dynamic_size / sizeof(struct dynamic);
  if ((int)dpnt < (int)header ||
      (int)(dpnt+dynamic_size) > (int)(header + st.st_size))
    goto skip;
  
  for (i = 0; i < dynamic_size; i++)
  {
    if (dpnt->d_tag == DT_STRTAB)
      strtab_val = dpnt->d_un.d_val;
    dpnt++;
  };

  if (!strtab_val)
    goto skip;

  dpnt = (struct dynamic *) &header[dynamic_addr];
  for (i = 0; i < dynamic_size; i++)
  {
    if (dpnt->d_tag == DT_SONAME || dpnt->d_tag == DT_NEEDED)
    {
      needed_val = dpnt->d_un.d_val;
      if (needed_val + strtab_val - loadaddr >= 0 ||
	  needed_val + strtab_val - loadaddr < st.st_size)
      {
	needed = (char *) (header - loadaddr + strtab_val + needed_val);

	if (dpnt->d_tag == DT_SONAME)
	  soname = xstrdup(needed);

	for (j = 0; needed_tab[j].soname != NULL; j++)
	{
	  if (strcmp(needed, needed_tab[j].soname) == 0)
	  {
	    if (*type != LIB_ELF && *type != needed_tab[j].type)
	      multi_libcs = 1;
	    *type = needed_tab[j].type;
	  }
	}
      }
    }
    dpnt++;
  };
  
  if (multi_libcs)
    warn("%s appears to be for multiple libc's", name);

 skip:
  munmap(header, st.st_size);

  return soname;
}
