/*	BSDI dlib.c,v 1.6 1998/09/09 06:11:06 torek Exp	*/

/*
 * libdl.c
 * 
 * Functions required for dlopen et. al.
 */

#include "dlfcn.h"
#include "link.h"
#include <stdlib.h>
#include <linux/mman.h>
#ifdef IBCS_COMPATIBLE
#include <ibcs/unistd.h>
#else
#include <linux/unistd.h>
#endif
#include "sysdep.h"
#include "syscall.h"
#ifdef __bsdi__
#include <linux/elf.h>
#endif
#include "hash.h"
#include "string.h"
#include "linuxelf.h"

#ifndef IBCS_COMPATIBLE
#define	USE_WEAK
#endif

extern int _dl_error_number;
extern struct r_debug * _dl_debug_addr;

extern void * (*_dl_malloc_function)(size_t size);

static int do_fixup(struct elf_resolve * tpnt, int flag);
static int do_dlclose(void *, int need_fini);

void * _dlopen(char * filename, int flag);
const char * _dlerror(void);
void * _dlsym(void *, char *);
int _dlclose(void *);

static const char * dl_error_names[] = {
	"",
	"File not found",
	"Unable to open /dev/zero",
	"Not an ELF file",
#if defined (__i386__)
 	"Not i386 binary",
#elif defined (__sparc__)
	"Not sparc binary",
#elif defined (__mc68000__)
	"Not m68k binary",
#else
	"Unrecognized binary type",
#endif
	"Not an ELF shared library",
	"Unable to mmap file",
	"No dynamic section",
#ifdef ELF_USES_RELOCA
	"Unable to process REL relocs",
#else
	"Unable to process RELA relocs",
#endif
	"Bad handle",
	"Unable to resolve symbol"
};
	
static void dl_cleanup(void) __attribute__ ((destructor));

static void dl_cleanup(void)
{
	struct dyn_elf*		d;
	
	for (d = _dl_handles; d; d = d->next_handle)
		if (d->dyn->libtype == loaded_file && d->dyn->dynamic_info[DT_FINI])
		{
			(*((int(*)(void))(d->dyn->loadaddr + d->dyn->dynamic_info[DT_FINI])))();
			d->dyn->dynamic_info[DT_FINI] = 0;
		}
}

void * _dlopen(char * libname, int flag)
{
	struct elf_resolve * tpnt, *tfrom;
	struct dyn_elf * rpnt;
	struct dyn_elf * dyn_chain;
	struct dyn_elf * dpnt;
	static int dl_init = 0;
	char *  from;
	void (*dl_brk)(void);
	int (*dl_elf_init)(void);

	from = __builtin_return_address(0);

	/* Have the dynamic linker use the regular malloc function now */
	if (!dl_init) {
		dl_init++;
		_dl_malloc_function = malloc;
	}

	/* Cover the trivial case first */
	if (!libname) return _dl_symbol_tables;

#ifdef USE_CACHE
	_dl_map_cache();
#endif

	/*
	 * Try and locate the module we were called from - we
	 * need this so that we get the correct RPATH.  Note that
	 * this is the current behavior under Solaris, but the
	 * ABI+ specifies that we should only use the RPATH from
	 * the application.  Thus this may go away at some time
	 * in the future.
	 */
	tfrom = NULL;
	for(dpnt = _dl_symbol_tables; dpnt; dpnt = dpnt->next)
	  {
	    tpnt = dpnt->dyn;
	    if(   tpnt->loadaddr < from 
	       && (   tfrom == NULL
		   || tfrom->loadaddr < tpnt->loadaddr))
	      tfrom = tpnt;
	  }

	if(!(tpnt = _dl_load_shared_library(tfrom, libname)))
	{
#ifdef USE_CACHE
	  _dl_unmap_cache();
#endif
	  return NULL;
	}

	tpnt->usage_count++;
	dyn_chain = rpnt = 
	   (struct dyn_elf *) malloc(sizeof(struct dyn_elf));
	_dl_memset (rpnt, 0, sizeof(*rpnt));
	rpnt->dyn = tpnt;
	rpnt->flags = flag;
	if (!tpnt->symbol_scope) tpnt->symbol_scope = dyn_chain;
	
	rpnt->next_handle = _dl_handles;
	_dl_handles = rpnt;

	/*
	 * OK, we have the requested file in memory.  Now check for
	 * any other requested files that may also be required.
	 */
	  {
	    struct elf_resolve *tcurr;
	    struct elf_resolve * tpnt1;
	    struct dynamic * dpnt;
	    char * lpnt;

	    tcurr = tpnt;
	    do{
	      for(dpnt = (struct dynamic *) tcurr->dynamic_addr; dpnt->d_tag; dpnt++)
		{
	  
		  if(dpnt->d_tag == DT_NEEDED)
		    {
		      lpnt = tcurr->loadaddr + tcurr->dynamic_info[DT_STRTAB] + 
			dpnt->d_un.d_val;
		      if(!(tpnt1 = _dl_load_shared_library(tcurr, lpnt)))
			goto oops;

		      rpnt->next = 
		         (struct dyn_elf *) malloc(sizeof(struct dyn_elf));
		      _dl_memset (rpnt->next, 0, sizeof (*(rpnt->next)));
		      rpnt = rpnt->next;
		      tpnt1->usage_count++;
		      if (!tpnt1->symbol_scope) tpnt1->symbol_scope = dyn_chain;
		      rpnt->dyn = tpnt1;
		    };
		}
	      
	      tcurr = tcurr->next;
	    } while(tcurr);
	  }
	 
	/*
	 * OK, now attach the entire chain at the end
	 */

	rpnt->next = _dl_symbol_tables;

	if (do_fixup(tpnt, flag)) {
	  _dl_error_number = DL_NO_SYMBOL;
	  goto oops;
	}

	dl_brk = (void (*)()) _dl_debug_addr->r_brk;
	if( dl_brk != NULL )
	  {
	    _dl_debug_addr->r_state = RT_ADD;
	    (*dl_brk)();
	    
	    _dl_debug_addr->r_state = RT_CONSISTENT;
	    (*dl_brk)();
	  }

	for(rpnt = dyn_chain; rpnt; rpnt = rpnt->next)
    	{
		tpnt = rpnt->dyn;
	      /* Apparently crt1 for the application is responsible for handling this.
	       * We only need to run the init/fini for shared libraries
	       */
 	      if (tpnt->libtype == elf_executable) continue;
	      if (tpnt->init_flag & INIT_FUNCS_CALLED) continue;
	      tpnt->init_flag |= INIT_FUNCS_CALLED;
      
	      if(tpnt->dynamic_info[DT_INIT]) {
		dl_elf_init = (int (*)(void)) (tpnt->loadaddr + 
					    tpnt->dynamic_info[DT_INIT]);
		(*dl_elf_init)();
	      }
   	}

#ifdef USE_CACHE
	_dl_unmap_cache();
#endif
	return (void *) dyn_chain;

oops:
	/* Something went wrong.  Clean up and return NULL. */
#ifdef USE_CACHE
	_dl_unmap_cache();
#endif
	do_dlclose (dyn_chain, 0);
	return NULL;
}

static int do_fixup(struct elf_resolve * tpnt, int flag)
{
  int goof = 0;
  if(tpnt->next) goof += do_fixup(tpnt->next, flag);

  if(tpnt->dynamic_info[DT_REL]) {
#ifdef ELF_USES_RELOCA
    goof++;
#else
    if (tpnt->init_flag & RELOCS_DONE) return goof;
    tpnt->init_flag |= RELOCS_DONE;
   
    goof += _dl_parse_relocation_information(tpnt, tpnt->dynamic_info[DT_REL],
					     tpnt->dynamic_info[DT_RELSZ], 0);
#endif
  }
  if(tpnt->dynamic_info[DT_RELA]) {
#ifdef ELF_USES_RELOCA
    if (tpnt->init_flag & RELOCS_DONE) return goof;
    tpnt->init_flag |= RELOCS_DONE;
   
    goof += _dl_parse_relocation_information(tpnt, tpnt->dynamic_info[DT_RELA],
					     tpnt->dynamic_info[DT_RELASZ], 0);
#else
    goof++;
#endif
  }
  if(tpnt->dynamic_info[DT_JMPREL])
    {
      if (tpnt->init_flag & JMP_RELOCS_DONE) return goof;
      tpnt->init_flag |= JMP_RELOCS_DONE;
      
      if(flag == RTLD_LAZY)
	_dl_parse_lazy_relocation_information(tpnt, tpnt->dynamic_info[DT_JMPREL],
					      tpnt->dynamic_info[DT_PLTRELSZ], 0);
      else
	goof +=  _dl_parse_relocation_information(tpnt,
						  tpnt->dynamic_info[DT_JMPREL],
						  tpnt->dynamic_info[DT_PLTRELSZ], 0);
    };
  return goof;
}

void * _dlsym(void * vhandle, char * name)
{	
	struct elf_resolve * tpnt, *tfrom;
	struct dyn_elf * handle;
	char *  from;
	struct dyn_elf * rpnt;
	void *ret;

	handle = (struct dyn_elf *) vhandle;

	/* First of all verify that we have a real handle
	of some kind.  Return NULL if not a valid handle. */

	if (handle == NULL)
		handle = _dl_symbol_tables;
	else if (handle != RTLD_NEXT && handle != _dl_symbol_tables) {
		for(rpnt = _dl_handles; rpnt; rpnt = rpnt->next_handle)
			if (rpnt == handle) break;
		if (!rpnt) {
			_dl_error_number = DL_BAD_HANDLE;
			return NULL;
		}
	}
	else if (handle == RTLD_NEXT )
	  {
	    /*
	     * Try and locate the module we were called from - we
	     * need this so that we know where to start searching
	     * from.  We never pass RTLD_NEXT down into the actual
	     * dynamic loader itself, as it doesn't know
	     * how to properly treat it.
	     */
	    from = __builtin_return_address(0);

	    tfrom = NULL;
	    for(rpnt = _dl_symbol_tables; rpnt; rpnt = rpnt->next)
	      {
		tpnt = rpnt->dyn;
		if(   tpnt->loadaddr < from 
		      && (   tfrom == NULL
			     || tfrom->loadaddr < tpnt->loadaddr))
		  {
		    tfrom = tpnt;
		    handle = rpnt->next;
		  }
	      }
	  }

	ret = _dl_find_hash(name, handle, 1, NULL, 1);

       /*
        * Nothing found.
        */
	if (!ret)
		_dl_error_number = DL_NO_SYMBOL;
	return ret;
}

int _dlclose(void * vhandle)
{
  return do_dlclose(vhandle, 1);
}

static int do_dlclose(void * vhandle, int need_fini)
{
	struct dyn_elf * rpnt, *rpnt1;
	struct dyn_elf *spnt, *spnt1;
	struct elf_phdr * ppnt;
	struct elf_resolve * tpnt;
	int (*dl_elf_fini)(void);
	void (*dl_brk)(void);
	struct dyn_elf * handle;
	unsigned int end;
	int i = 0;

	handle = (struct dyn_elf *) vhandle;
	rpnt1 = NULL;
	for(rpnt = _dl_handles; rpnt; rpnt = rpnt->next_handle)
	{
		if(rpnt == handle) {
			break;
		}
		rpnt1 = rpnt;
	}
	
	if (!rpnt) {
		_dl_error_number = DL_BAD_HANDLE;
		return 1;
	}

	/* OK, this is a valid handle - now close out the file.
	 * We check if we need to call fini () on the handle. */
	spnt = need_fini ? handle : handle->next;
	for(; spnt; spnt = spnt1)
	{
	    spnt1 = spnt->next;

	    /* We appended the module list to the end - when we get back here, 
	     quit. The access counts were not adjusted to account for being here. */
	    if (spnt == _dl_symbol_tables) break;
	    if(spnt->dyn->usage_count==1 && spnt->dyn->libtype == loaded_file) {
		tpnt = spnt->dyn;
		/* Apparently crt1 for the application is responsible for handling this.
		 * We only need to run the init/fini for shared libraries
		 */

		if(tpnt->dynamic_info[DT_FINI]) {
		    dl_elf_fini = (int (*)(void)) (tpnt->loadaddr + 
						   tpnt->dynamic_info[DT_FINI]);
		    (*dl_elf_fini)();
		}	
	    }
	}
	if(rpnt1)
	    rpnt1->next_handle = rpnt->next_handle;
	else
	    _dl_handles = rpnt->next_handle;
	
	/* OK, this is a valid handle - now close out the file */
	for(rpnt = handle; rpnt; rpnt = rpnt1)
	  {
		rpnt1 = rpnt->next;

		/* We appended the module list to the end - when we get back here, 
		   quit. The access counts were not adjusted to account for being here. */
		if (rpnt == _dl_symbol_tables) break;

		rpnt->dyn->usage_count--;
		if(rpnt->dyn->usage_count == 0 && rpnt->dyn->libtype == loaded_file)
		{
			tpnt = rpnt->dyn;
		      /* Apparently crt1 for the application is responsible for handling this.
		       * We only need to run the init/fini for shared libraries
		       */
#if 0
/* We have to do this above, before we start closing objects.
Otherwise when the needed symbols for _fini handling are
resolved a coredump would occur. Rob Ryan (robr@cmu.edu)*/
		      if(tpnt->dynamic_info[DT_FINI]) {
			dl_elf_fini = (int (*)(void)) (tpnt->loadaddr + 
						    tpnt->dynamic_info[DT_FINI]);
			(*dl_elf_fini)();
		      }	
#endif
		      end = 0;
			for(i = 0, ppnt = rpnt->dyn->ppnt; 
				i < rpnt->dyn->n_phent; ppnt++, i++) {
				if (ppnt->p_type != PT_LOAD) continue;
				if (end < ppnt->p_vaddr + ppnt->p_memsz)
					end = ppnt->p_vaddr + ppnt->p_memsz;
			}
			_dl_munmap (rpnt->dyn->loadaddr, end);
			/* Next, remove rpnt->dyn from the loaded_module list */
			if (_dl_loaded_modules == rpnt->dyn)
			{
			  _dl_loaded_modules = rpnt->dyn->next;
			  if (_dl_loaded_modules)
			    _dl_loaded_modules->prev = 0;
			}
			else
			  for (tpnt = _dl_loaded_modules; tpnt; tpnt = tpnt->next)
				if (tpnt->next == rpnt->dyn) {
				  tpnt->next = tpnt->next->next;
				  if (tpnt->next) 
				    tpnt->next->prev = tpnt;
				  break;
				}
			free(rpnt->dyn);
		      }
		free(rpnt);
	  }


	dl_brk = (void (*)()) _dl_debug_addr->r_brk;
	if( dl_brk != NULL )
	  {
	    _dl_debug_addr->r_state = RT_DELETE;
	    (*dl_brk)();
	    
	    _dl_debug_addr->r_state = RT_CONSISTENT;
	    (*dl_brk)();
	  }

	return 0;
}

const char * _dlerror()
{
	const char * retval;
	if(!_dl_error_number) return NULL;
	retval = dl_error_names[_dl_error_number];
	_dl_error_number = 0;
	return retval;
}

/* Generate the correct symbols that we need. */
#ifdef USE_WEAK
#if 1
#pragma weak dlopen = _dlopen
#pragma weak dlerror = _dlerror
#pragma weak dlclose = _dlclose
#pragma weak dlsym = _dlsym
#pragma weak dladdr = _dladdr
#else
__asm__(".weak dlopen;dlopen=_dlopen");
__asm__(".weak dlerror;dlerror=_dlerror");
__asm__(".weak dlclose;dlclose=_dlclose");
__asm__(".weak dlsym;dlsym=_dlsym");
__asm__(".weak dladdr;dladdr=_dladdr");
#endif
#endif

/* This is a real hack.  We need access to the dynamic linker, but we
also need to make it possible to link against this library without any
unresolved externals.  We provide these weak symbols to make the link
possible, but at run time the normal symbols are accessed. */

static void foobar()
{
	_dl_fdprintf(2,"libdl library not correctly linked\n");
	_dl_exit(1);
}

static int foobar1 = (int)foobar; /* Use as pointer */

#ifdef USE_WEAK
#if 1
#pragma weak _dl_find_hash = foobar
#pragma weak _dl_symbol_tables = foobar1
#pragma weak _dl_handles = foobar1
#pragma weak _dl_loaded_modules = foobar1
#pragma weak _dl_debug_addr = foobar1
#pragma weak _dl_error_number = foobar1
#pragma weak _dl_load_shared_library = foobar
#ifdef USE_CACHE
#pragma weak _dl_map_cache = foobar
#pragma weak _dl_unmap_cache = foobar
#endif
#pragma weak _dl_malloc_function = foobar1
#pragma weak _dl_parse_relocation_information = foobar
#pragma weak _dl_parse_lazy_relocation_information = foobar
#pragma weak _dl_fdprintf = foobar
#else
__asm__(".weak _dl_find_hash; _dl_find_hash = foobar");
__asm__(".weak _dl_symbol_tables; _dl_symbol_tables = foobar1");
__asm__(".weak _dl_handles; _dl_handles = foobar1");
__asm__(".weak _dl_loaded_modules; _dl_loaded_modules = foobar1");
__asm__(".weak _dl_debug_addr; _dl_debug_addr = foobar1");
__asm__(".weak _dl_error_number; _dl_error_number = foobar1");
__asm__(".weak _dl_load_shared_library; _dl_load_shared_library = foobar");
#ifdef USE_CACHE
__asm__(".weak _dl_map_cache; _dl_map_cache = foobar");
__asm__(".weak _dl_unmap_cache; _dl_unmap_cache = foobar");
#endif
__asm__(".weak _dl_malloc_function; _dl_malloc_function = foobar1");
__asm__(".weak _dl_parse_relocation_information; _dl_parse_relocation_information = foobar");
__asm__(".weak _dl_parse_lazy_relocation_information; _dl_parse_lazy_relocation_information = foobar");
__asm__(".weak _dl_fdprintf; _dl_fdprintf = foobar");
#endif
#endif

/*
 * Dump information to stderrr about the current loaded modules
 */
static char * type[] = {"Lib","Exe","Int","Mod"};

void _dlinfo()
{
	struct elf_resolve * tpnt;
	struct dyn_elf * rpnt, *hpnt;
	_dl_fdprintf(2, "List of loaded modules\n");
	/* First start with a complete list of all of the loaded files. */
	for (tpnt = _dl_loaded_modules; tpnt; tpnt = tpnt->next)
		_dl_fdprintf(2, "\t%8.8x %8.8x %8.8x %s %d %s\n",
			(unsigned)tpnt->loadaddr, (unsigned)tpnt,
			(unsigned)tpnt->symbol_scope,
			type[tpnt->libtype],
			tpnt->usage_count, 
			tpnt->libname);

	/* Next dump the module list for the application itself */
	_dl_fdprintf(2, "\nModules for application (%x):\n",
		(unsigned)_dl_symbol_tables);
	for (rpnt = _dl_symbol_tables; rpnt; rpnt = rpnt->next)
		_dl_fdprintf(2, "\t%8.8x %s\n", (unsigned)rpnt->dyn,
			rpnt->dyn->libname);

	for (hpnt = _dl_handles; hpnt; hpnt = hpnt->next_handle)
	{
		_dl_fdprintf(2, "Modules for handle %x\n", (unsigned)hpnt);
		for(rpnt = hpnt; rpnt; rpnt = rpnt->next)
			_dl_fdprintf(2, "\t%8.8x %s\n", (unsigned)rpnt->dyn,
				rpnt->dyn->libname);
	}
}

int
_dladdr(void *address, Dl_info *dlip)
{
	const struct elf_resolve *erp;
	const struct elf_resolve *cerp;
	const char *v;
	unsigned int syment;
	const Elf32_Sym *s, *slast, *cs;
	const Elf32_Word *hash;
	const char *strtab;

	/*
	 * First we find a candidate elf_resolve entry.
	 * Look for the entry with the highest load address
	 * that is less than or equal to the given address.
	 */
	cerp = NULL;
	for (erp = _dl_loaded_modules; erp != NULL; erp = erp->next) {
		if (erp->loadaddr > (const char *)address)
			continue;
		if (cerp != NULL && erp->loadaddr <= cerp->loadaddr)
			continue;
		cerp = erp;
	}

	if (cerp == NULL)
		return (0);

	/*
	 * We have the module.
	 * Fill in the module information.
	 */
	dlip->dli_fname = cerp->libname;
	dlip->dli_fbase = cerp->loadaddr;
	dlip->dli_sname = NULL;
	dlip->dli_saddr = NULL;

	/*
	 * Loop over symbols and find the symbol with the greatest address
	 * that is less than or equal to the given address.
	 */
	v = cerp->loadaddr;
	strtab = v + cerp->dynamic_info[DT_STRTAB];
	syment = cerp->dynamic_info[DT_SYMENT];
	s = (const Elf32_Sym *)(v + cerp->dynamic_info[DT_SYMTAB]);
	hash = (const Elf32_Word *)(v + cerp->dynamic_info[DT_HASH]);
	slast = (const Elf32_Sym *)((const char *)s + syment * hash[1]);
	cs = NULL;
	for (; s < slast; s = (const Elf32_Sym *)((const char *)s + syment)) {
		/*
		 * Dynamic symbols in executables have absolute addresses,
		 * while those in shared objects have base-relative addresses.
		 * To compensate, the loadaddr is 0 for executables.
		 */
		if (s->st_shndx == SHN_UNDEF || s->st_shndx == SHN_COMMON ||
		    ELF32_ST_BIND(s->st_info) == STB_LOCAL)
			continue;
		if (v + s->st_value > (const char *)address)
			continue;
		if (cs != NULL && s->st_value <= cs->st_value)
			continue;
		cs = s;
	}

	/* If we found a symbol, fill in the symbol information.  */
	if (cs != NULL) {
		dlip->dli_sname = strtab + cs->st_name;
		dlip->dli_saddr = (void *)(v + cs->st_value);
	}

	return (1);
}