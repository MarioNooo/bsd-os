/*	BSDI solib-bsdi.c,v 1.5 2003/08/09 07:01:08 donn Exp	*/

/* Support for debugging BSD/OS statically linked shared libraries.
   Rudimentary, but then so are the shared libraries!
   Donn Seeley, 12/30/1998  */

#include "defs.h"
#include "symtab.h"
#include "bfd.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "target.h"
#include "inferior.h"
#include "solist.h"

#include <sys/param.h>
#include <machine/shlib.h>
#include <string.h>

static void bsdi_clear_solib (void);
static struct so_list * bsdi_current_sos (void);
static void bsdi_free_so (struct so_list *);
static int bsdi_in_dynsym_resolve_code (CORE_ADDR);
static int bsdi_open_symbol_file_object (void *);
static void bsdi_relocate_section_addresses (struct so_list *,
					     struct section_table *);
static void bsdi_solib_create_inferior_hook (void);
static void bsdi_special_symbol_handling (void);
CORE_ADDR find_bsdi_solib_trampoline_target (CORE_ADDR);

struct target_so_ops bsdi_so_ops;

/* The static library loader doesn't generate a link_map structure.
   This declaration is really for documentation.  */

struct lm_info
{
  int dummy;
};

/* Since the libraries are statically linked, we don't need to relocate
   them!  */

static void
bsdi_relocate_section_addresses (struct so_list *so, struct section_table *sec)
{
}

/* We don't have a link map, so we don't have to free it.  */

static void
bsdi_free_so (struct so_list *so)
{
}

/* We don't maintain any static information, so this is also a no-op.  */

static void
bsdi_clear_solib (void)
{
}

/* We don't have a breakpoint on loading new libraries, so we must
   update the library list here.  */

static void
bsdi_solib_create_inferior_hook (void)
{
  SOLIB_ADD (NULL, 0, NULL, auto_solib_add);
}

/* Nothing to do here either.  */

static void
bsdi_special_symbol_handling (void)
{
}

/* Read a list of shared library objects out of the executable.
   We don't actually read the shared libraries themselves;
   the machine-independent code is responsible for that.  */

static struct so_list *
bsdi_current_sos ()
{
  /* XXX This code assumes that all ldtab structures have the same layout
     on all architectures, although they are declared in a machdep header.  */
  const bfd_size_type ptr_bytes = TARGET_PTR_BIT / TARGET_CHAR_BIT;

  asection *data;
  char libtable_external[ptr_bytes];
  bfd_vma libtable;
  struct so_list *head = NULL;
  struct so_list **link_ptr = &head;

  /* A pointer to the shared library table is the first word of
     the data section in an executable that uses these libraries.  */
  data = bfd_get_section_by_name (exec_bfd, ".data");
  if (data == NULL
      || (bfd_get_section_flags (exec_bfd, data) & SEC_LOAD) != SEC_LOAD)
    return NULL;

  read_memory (bfd_section_vma (exec_bfd, data), libtable_external, ptr_bytes);
  libtable = (bfd_vma) extract_address (libtable_external, ptr_bytes);

  /*  Loop over the libtable array.  */

  for (; ; libtable += ptr_bytes * 2)
    {
      char ldtab_name_external[ptr_bytes];
      bfd_vma ldtab_name;
      struct so_list *sol;
      struct cleanup *old_chain;
      char *buffer;
      int errcode;

      /* Pull out the next ldtab structure and internalize it.
	 Stop if the name pointer is NULL.  */
      read_memory (libtable, ldtab_name_external, ptr_bytes);
      ldtab_name = (bfd_vma) extract_address (ldtab_name_external, ptr_bytes);
      if (ldtab_name == 0)
	break;

      sol = xmalloc (sizeof *sol);
      old_chain = make_cleanup (xfree, sol);
      memset (sol, '\0', sizeof *sol);

      /* Fetch the library name, which is the only interesting information.  */
      target_read_string (ldtab_name, &buffer, SO_NAME_MAX_PATH_SIZE - 1,
			  &errcode);
      if (errcode != 0)
	warning ("current_sos: Can't read pathname: %s",
		 safe_strerror (errcode));

      if (buffer[0] == '\0')
	{
	  xfree (sol);
	  discard_cleanups (old_chain);
	  continue;
	}

      strncpy (sol->so_original_name, buffer, SO_NAME_MAX_PATH_SIZE - 1);
      sol->so_original_name[SO_NAME_MAX_PATH_SIZE - 1] = '\0';
      strncpy (sol->so_name, buffer, SO_NAME_MAX_PATH_SIZE - 1);
      sol->so_name[SO_NAME_MAX_PATH_SIZE - 1] = '\0';
      xfree (buffer);

      *link_ptr = sol;
      link_ptr = &sol->next;

      discard_cleanups (old_chain);
    }

  return head;
}

/* We can't provide access to the inferior's symbol table
   without the inferior's executable file.  But if we say
   we can't do this, then attaches won't work, and there's
   no good reason for that.  So we fake success!  */

static int
bsdi_open_symbol_file_object (void *v)
{
  return 1;
}

/* We don't link dynamically, so this is always false.
   I suppose we could return true if we were executing an instruction
   from the library stub trampoline code; would this be useful?  */

static int
bsdi_in_dynsym_resolve_code (CORE_ADDR pc)
{
  return 0;
}

/* If the PC is in a BSD/OS shared library trampoline stub, return the
   address of the `real' function belonging to the stub.  If this isn't
   a BSD/OS shared library trampoline stub, try a dynamic library
   trampoline stub instead.  This is common code for SKIP_TRAMPOLINE_CODE()
   across different BSD/OS architectures.  */

CORE_ADDR
find_bsdi_solib_trampoline_target (CORE_ADDR pc)
{
  struct objfile *o;
  struct minimal_symbol *m;
  struct minimal_symbol *t = lookup_minimal_symbol_by_pc (pc);

  /* If this address corresponds to a jump table slot, look for a symbol
     in the same object file whose name matches the trailing part of
     the name of the jump table slot.  */
  if (t != NULL)
    {
      if (STREQN (SYMBOL_NAME (t), "__JUMP_", 7))
	{
	  ALL_MSYMBOLS (o, m)
	    {
	      if (MSYMBOL_TYPE (m) == mst_text
		  && STREQ (SYMBOL_NAME (m), SYMBOL_NAME (t)))
		{
		  m = lookup_minimal_symbol (SYMBOL_NAME (t) + 7, NULL, o);
		  if (m != NULL)
		    return SYMBOL_VALUE_ADDRESS (m);
		  break;
		}
	    }
	}
    }

  return find_solib_trampoline_target (pc);
}

/* Common code for IN_SOLIB_CALL_TRAMPOLINE().  */

int
bsdi_in_solib_call_trampoline (CORE_ADDR pc, char *name)
{
  extern int in_plt_section (CORE_ADDR, char *);

  if (STREQN (name, "__JUMP_", 7))
    return 1;
  return in_plt_section (pc, name);
}

/* Determine which flavor of shared library ops to use.  There
   are two flavors -- SVr4-compatible dynamically linked shared
   libraries, and BSD/OS-unique statically linked shared libraries.
   If it's a dynamic object or if the interpreter name starts
   with '/shlib/ld-bsdi.so', it's dynamically linked.

   Currently this is a hack.  There should be an so_op that checks
   whether the current executable uses the given type of shared
   library, and a hook in exec_file_attach() that calls this op for
   each shared library type until the correct type is found.
   Note that we have to check both at solib_create_inferior_hook()
   time and at current_sos(), depending on whether we have a live
   target or a core file target.
   FIXME  */

struct target_so_ops generic_bsdi_so_ops;

static void
reset_bsdi_solib_ops ()
{
  extern struct target_so_ops svr4_so_ops;

  /* We re-check every time that we have to identify new shared libraries.
     We really only need to re-check when the exec file changes...
     We default to statically linked shared libraries.  */
  generic_bsdi_so_ops = bsdi_so_ops;
  if (exec_bfd == NULL)
    return;
  if (exec_bfd->flags & DYNAMIC)
    generic_bsdi_so_ops = svr4_so_ops;
  else
    {
      asection *interp_sect;

      interp_sect = bfd_get_section_by_name (exec_bfd, ".interp");
      if (interp_sect == NULL)
	interp_sect = bfd_get_section_by_name (exec_bfd, "interp0");
      if (interp_sect != NULL)
	{
	  char *buf = alloca (interp_sect->_raw_size);
	  static const char ld_bsdi[] = "/shlib/ld-bsdi.so";

	  if (bfd_get_section_contents (exec_bfd, interp_sect, buf, 0,
					interp_sect->_raw_size))
	    {
	      if (interp_sect->_raw_size >= sizeof ld_bsdi
		  && memcmp (buf, ld_bsdi, sizeof ld_bsdi - 1) == 0)
		generic_bsdi_so_ops = svr4_so_ops;
	    }
	}
    }
}

static struct so_list *generic_bsdi_current_sos (void);

static void
generic_bsdi_solib_create_inferior_hook ()
{
  reset_bsdi_solib_ops ();

  generic_bsdi_so_ops.solib_create_inferior_hook ();

  generic_bsdi_so_ops.current_sos = generic_bsdi_current_sos;
  generic_bsdi_so_ops.solib_create_inferior_hook
    = generic_bsdi_solib_create_inferior_hook;
}

static struct so_list *
generic_bsdi_current_sos ()
{
  struct so_list *ret;

  reset_bsdi_solib_ops ();

  ret = generic_bsdi_so_ops.current_sos ();

  generic_bsdi_so_ops.current_sos = generic_bsdi_current_sos;
  generic_bsdi_so_ops.solib_create_inferior_hook
    = generic_bsdi_solib_create_inferior_hook;

  return ret;
}

void
_initialize_bsdi_solib ()
{
  bsdi_so_ops.relocate_section_addresses = bsdi_relocate_section_addresses;
  bsdi_so_ops.free_so = bsdi_free_so;
  bsdi_so_ops.clear_solib = bsdi_clear_solib;
  bsdi_so_ops.solib_create_inferior_hook = bsdi_solib_create_inferior_hook;
  bsdi_so_ops.special_symbol_handling = bsdi_special_symbol_handling;
  bsdi_so_ops.current_sos = bsdi_current_sos;
  bsdi_so_ops.open_symbol_file_object = bsdi_open_symbol_file_object;
  bsdi_so_ops.in_dynsym_resolve_code = bsdi_in_dynsym_resolve_code;

  /* BSD/OS supplies both statically and dynamically linked libraries.
     There should be an exec hook that lets us set the shared
     library type based on the exec file, but for now, there is a
     hack in i386bsdi-tdep.c that handles the situation.  FIXME  */

  reset_bsdi_solib_ops ();
  generic_bsdi_so_ops.current_sos = generic_bsdi_current_sos;
  generic_bsdi_so_ops.solib_create_inferior_hook
    = generic_bsdi_solib_create_inferior_hook;
}
