/*	BSDI bsdi-thread.c,v 1.4 2003/09/19 18:00:02 donn Exp	*/

/* Low level interface for debugging BSD/OS threads for GDB, the GNU debugger.
   Copyright 1998 Free Software Foundation, Inc.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* This file was patterned after sol-thread.c, which implements
   'half-targets' that inherit from other targets and interfaces
   with gdb's thread support.  In BSD/OS 4.0, the pthread implementation
   is entirely in user mode and process manipulation still uses ptrace(),
   so the actual code below is quite different from the Solaris code,
   but it should work in a similar way.  */

#include "defs.h"

#include "gdbthread.h"
#include "target.h"
#include "inferior.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "gdb_assert.h"

extern struct target_ops bsdi_thread_ops;
extern struct target_ops bsdi_core_ops;

/* A place to store core_ops before we overwrite it.  */
static struct target_ops orig_core_ops;

extern int child_suppress_run;
extern struct target_ops child_ops;
extern struct target_ops core_ops;

static ptid_t bsdi_first_pthread;
static CORE_ADDR addr_thread_allthreads;
static CORE_ADDR addr_thread_run;
static struct pthread_cache *current_pthread;
static int need_thread_attach;
static struct pthread_cache *pthread_cache_list;
static int threads_initialized;
static void (*target_new_objfile_chain) (struct objfile *);

/* These are static variables to make debugging easier.  */
static ptid_t thread_allthreads;
static ptid_t thread_run;

/* Each pthread_cache structure has a stamp.  If that stamp doesn't
   match the current stamp, we must re-fetch the pthread state.  */

static long pthread_cache_stamp = 0;

/* Forward declarations for functions.  */
void _initialize_bsdi_thread (void);
static struct pthread_cache *add_pthread (ptid_t);
static struct bsdi_arch_thread_ops *bsdi_arch_thread_ops (void);
static void bsdi_core_close (int);
static void bsdi_core_detach (char *, int);
static void bsdi_core_files_info (struct target_ops *);
static void bsdi_core_open (char *, int);
static void bsdi_core_fetch_registers (int);
static int bsdi_thread_alive (ptid_t);
static void bsdi_thread_attach (char *, int);
static int bsdi_thread_can_run (void);
static void bsdi_thread_create_inferior (char *, char *, char **);
static void bsdi_thread_detach (char *, int);
static void bsdi_thread_fetch_registers (int);
static void bsdi_thread_files_info (struct target_ops *);
static void bsdi_thread_kill_inferior (void);
static void bsdi_thread_mourn_inferior (void);
static void bsdi_thread_new_objfile (struct objfile *);
static void bsdi_thread_open (char *, int);
static char *bsdi_thread_pid_to_str (ptid_t);
static void bsdi_thread_prepare_to_store (void);
static void bsdi_thread_reset (void);
static void bsdi_thread_resume (ptid_t, int, enum target_signal);
static void bsdi_thread_stop (void);
static void bsdi_thread_store_registers (int);
static ptid_t bsdi_thread_wait (ptid_t ptid, struct target_waitstatus *);
static int bsdi_thread_xfer_memory (CORE_ADDR, char *, int, int,
				    struct mem_attrib *, struct target_ops *);
static void get_all_pthreads (void);
static struct pthread_cache *get_new_pthread (void);
static struct pthread_cache *get_current_pthread (void);
static int get_thread_allthreads (void);
static int get_thread_run (void);
static struct pthread_cache *get_current_pthread (void);
static struct pthread_cache *get_new_pthread (void);
static int ignore (CORE_ADDR, char *);
static void info_bsdithreads (char *, int);
void init_bsdi_thread_ops (void);
void init_bsdi_core_ops (void);
static struct pthread_cache *lookup_pthread_cache (ptid_t);
static void ptrace_me (void);
static void ptrace_him (int pid);
static int read_pthread (struct pthread_cache *);
static int read_thread_var (CORE_ADDR addr, void *varp, int size);
static void remove_pthread_cache (void);
static struct pthread_cache *update_pthread_cache (ptid_t);

static void
bsdi_thread_reset ()
{
  remove_pthread_cache ();
  addr_thread_allthreads = 0;
  addr_thread_run = 0;
  bsdi_first_pthread = null_ptid;
  current_pthread = NULL;
  need_thread_attach = 0;
  threads_initialized = 0;
}

static int
read_thread_var (CORE_ADDR addr, void *varp, int size)
{
  int ret;

  if (target_has_execution)
    ret = child_ops.to_xfer_memory (addr, varp, size, 0, NULL, NULL);
  else
    ret = orig_core_ops.to_xfer_memory (addr, varp, size, 0, NULL, &core_ops);
  if (ret != size)
    {
      addr = 0; /* XXX */
      return 0;
    }

  return ret;
}

static int
get_thread_allthreads ()
{
  long tid;
  static int stamp = -1;

  if (stamp != pthread_cache_stamp)
    {
      if (read_thread_var (addr_thread_allthreads, &tid, sizeof tid) == 0)
	return 0;
      thread_allthreads = ptid_build (PIDGET (inferior_ptid), 0, tid);
      stamp = pthread_cache_stamp;
    }
  return 1;
}

static int
get_thread_run ()
{
  static int stamp = -1;
  long tid;

  if (stamp != pthread_cache_stamp)
    {
      if (read_thread_var (addr_thread_run, &tid, sizeof tid) == 0)
	return 0;
      thread_run = ptid_build (PIDGET (inferior_ptid), 0, tid);
      stamp = pthread_cache_stamp;
    }
  return 1;
}

static int
read_pthread (struct pthread_cache *ptc)
{
  CORE_ADDR tid = ptid_get_tid (ptc->id);
  int ret;

  if (ptc->stamp == pthread_cache_stamp)
    return sizeof (struct pthread);

  /* XXX CONVERT FROM TARGET TO HOST FORMAT??? */
  if (target_has_execution)
    ret = child_ops.to_xfer_memory (tid, (char *)&ptc->pthread, 
				    sizeof (struct pthread), 0, NULL, NULL);
  else
    ret = orig_core_ops.to_xfer_memory (tid, (char *)&ptc->pthread, 
					sizeof (struct pthread), 0, NULL,
					&core_ops);

  if (ret != sizeof (struct pthread))
    {
      /* We're hosed -- revert to non-pthread state.  */
      bsdi_thread_reset ();
      error ("can't read pthread data");
      return 0;
    }

  ptc->stamp = pthread_cache_stamp;
  return sizeof (struct pthread);
}

static struct pthread_cache *
add_pthread (ptid_t ptid)
{
  struct pthread_cache *ptc;

  ptc = xmalloc (sizeof *ptc);
  ptc->id = ptid;
  ptc->stamp = -1;

  if (read_pthread (ptc) == 0)
    {
      free (ptc);
      return NULL;
    }

  ptc->next = pthread_cache_list;
  pthread_cache_list = ptc;

  return ptc;
}

static void
get_all_pthreads ()
{
  struct pthread_cache *ptc;
  struct pthread_cache **pptc;
  struct pthread_cache *curp = NULL;
  pthread_t pt;
  ptid_t ptid;
  int new_threads = 0;

  if (!threads_initialized)
    return;

  if (! get_thread_allthreads ())
    return;

  /* Collect all live threads.  */
  pt = (pthread_t)ptid_get_tid (thread_allthreads);
  for (; pt; pt = ptc->pthread.nxt)
    {
      ptid = ptid_build (PIDGET (inferior_ptid), 0, (long)pt);
      ptc = lookup_pthread_cache (ptid);
      if (ptc == NULL)
	{
	  ptc = add_pthread (ptid);
	  ++new_threads;
	}
      else if (read_pthread (ptc) == 0)
	error ("get_all_pthreads: can't sync pthread");
    }

  /* Add any new threads to the cache.  */
  for (ptc = pthread_cache_list;
       ptc && new_threads > 0;
       ptc = ptc->next, --new_threads)
    {
      delete_thread (ptc->id);	/* Avoid duplicates.  */
      add_thread (ptc->id);
      if (ptc->pthread.state == PS_RUNNING)
	curp = ptc;
    }

  /* Discard all dead threads from the cache.  */
  for (pptc = &pthread_cache_list; (ptc = *pptc); )
    if (ptc->stamp != pthread_cache_stamp)
      {
	*pptc = ptc->next;
	free (ptc);
      }
    else
      pptc = &ptc->next;

  if (curp)
    current_pthread = curp;
  else
    get_current_pthread ();
}

static struct pthread_cache *
get_current_pthread ()
{
  ptid_t ptid;
  struct pthread_cache *ptc;

  /* The cached pthread state may be stale, but _thread_run is current.  */
  if (! get_thread_run ())
    return NULL;

  /* Use current_pthread as a hint.  */
  if (current_pthread != NULL && ptid_equal (current_pthread->id, thread_run))
    return (current_pthread);

  /* XXX Use db routines instead of linear search?  */
  ptc = lookup_pthread_cache (thread_run);

  if (ptc != NULL)
    current_pthread = ptc;

  return ptc;

}

static struct pthread_cache *
get_new_pthread ()
{
  struct pthread_cache *ptc;
  struct pthread_cache **pptc;

  if (! get_thread_allthreads ())
    return NULL;

  if (ptid_get_tid (thread_allthreads) == 0)
    return NULL;
  ptc = lookup_pthread_cache (thread_allthreads);
  if (ptc != NULL)
    {
      /* A reincarnated pthread.  Delete the old thread.  */
      for (pptc = &pthread_cache_list;
	   *pptc != NULL && *pptc != ptc;
	   pptc = &(*pptc)->next)
	;
      if (*pptc == ptc)
	{
	  *pptc = ptc->next;
	  free (ptc);
	}
    }

  return add_pthread (thread_allthreads);
}

static void
remove_pthread_cache ()
{
  struct pthread_cache *ptc;
  struct pthread_cache *nptc;

  for (ptc = pthread_cache_list; ptc; ptc = nptc)
    {
      nptc = ptc->next;
      free (ptc);
    }

  pthread_cache_list = NULL;
}

static struct pthread_cache *
lookup_pthread_cache (ptid_t ptid)
{
  struct pthread_cache *ptc;

  /* XXX Use db routines instead of linear search?  */
  for (ptc = pthread_cache_list; ptc; ptc = ptc->next)
    if (ptid_equal (ptc->id, ptid))
      return ptc;

  return NULL;
}

/* XXX Easy to understand, but inefficient.  */
static struct pthread_cache *
update_pthread_cache (ptid_t ptid)
{
  struct pthread_cache *ptc;

  if (ptid_get_tid (ptid) == 0 && current_pthread != NULL)
    ptid = current_pthread->id;

  ptc = lookup_pthread_cache (ptid);
  if (ptc == NULL || ptc->stamp == pthread_cache_stamp)
    return (ptc);

  get_all_pthreads ();
  return lookup_pthread_cache (ptid);
}

/* This normally just produces an error...  */
static void
bsdi_thread_open (char *arg, int from_tty)
{
  child_ops.to_open (arg, from_tty);
}

static void
bsdi_thread_attach (char *args, int from_tty)
{
  child_ops.to_attach (args, from_tty);
  push_target (&bsdi_thread_ops);

  need_thread_attach = 1;
}

static void
bsdi_thread_detach (char *args, int from_tty)
{
  unpush_target (&bsdi_thread_ops);
  bsdi_thread_reset ();

  child_ops.to_detach (args, from_tty);
}

static void
bsdi_thread_resume (ptid_t ptid, int step, enum target_signal signo)
{
  ++pthread_cache_stamp;
  bsdi_arch_thread_ops ()->resume ();

  child_ops.to_resume (minus_one_ptid, step, signo);
}

static ptid_t
bsdi_thread_wait (ptid_t ptid, struct target_waitstatus *statusp)
{
  struct pthread_cache *ptc;
  CORE_ADDR addr_thread_event;
  int new_thread = 0;
  int need_all_pthreads = 0;
  ptid_t ret;

  ret = child_ops.to_wait (minus_one_ptid, statusp);

  if (statusp->kind != TARGET_WAITKIND_STOPPED)
    {
      /* The child is probably dead.  Return something useful.  */
      if (threads_initialized)
	{
	  if (current_pthread != NULL)
	    return current_pthread->id;
	  return bsdi_first_pthread;
	}
      return ret;
    }

  if (! threads_initialized)
    {
      if (need_thread_attach)
	{
	  need_thread_attach = 0;
	  if (bsdi_arch_thread_ops ()->attach_event (&addr_thread_event) != 0
	      && addr_thread_event != 0)
	    {
	      threads_initialized = 1;
	      need_all_pthreads = 1;
	    }
	}
      else if (bsdi_arch_thread_ops ()->event (&addr_thread_event) != 0)
	threads_initialized = 1;

      if (threads_initialized)
	{
	  if (child_ops.to_xfer_memory (addr_thread_event,
					(char *)&addr_thread_run,
					sizeof addr_thread_run, 0, NULL, NULL)
	      != sizeof addr_thread_run)
	    error ("Can't read thread information");
	  if (child_ops.to_xfer_memory (addr_thread_event
					+ sizeof addr_thread_run,
					(char *)&addr_thread_allthreads,
					sizeof addr_thread_allthreads, 0, NULL,
					NULL)
	      != sizeof addr_thread_allthreads)
	    error ("Can't read thread information");
	}
    }

  if (bsdi_arch_thread_ops ()->event (NULL) != 0)
    {
      new_thread = 1;
      statusp->kind = TARGET_WAITKIND_SPURIOUS;
    }

  /* We must have the current pthread before we check registers.  */
  if (threads_initialized)
    {
      if (need_all_pthreads)
	get_all_pthreads ();
      get_current_pthread ();
    }

  if (new_thread)
    {
      ptc = get_new_pthread ();
      if (ptc != NULL)
	{
	  if (ptid_equal (bsdi_first_pthread, null_ptid))
	    {
	      current_pthread = ptc;
	      bsdi_first_pthread = ptc->id;
	    }
	  delete_thread (ptc->id);	/* Avoid duplicates.  */
	  if (ptid_equal (inferior_ptid, ptc->id) && current_pthread != NULL)
	    /* Try to avoid matching ptc->id and inferior_ptid.  */
	    inferior_ptid = current_pthread->id;

	  add_thread (ptc->id);
	  target_terminal_ours_for_output ();
	  printf_filtered ("[New %s]\n", target_pid_to_str (ptc->id));
	  target_terminal_inferior ();

	  return ptc->id;
	}
    }

  if (! threads_initialized)
    return ret;

  if (current_pthread != NULL)
    return current_pthread->id;
  return inferior_ptid;
}

static void
bsdi_thread_fetch_registers (int regno)
{
  struct pthread_cache *ptc;

  if (!threads_initialized
      || (current_pthread != NULL
	  && ptid_equal (inferior_ptid, current_pthread->id)))
    {
      child_ops.to_fetch_registers (regno);
      return;
    }

  /* We need to recover register state from a thread that isn't running.  */
  ptc = update_pthread_cache (inferior_ptid);
  if (ptc == NULL)
    error ("can't fetch registers: thread %#x died", inferior_ptid);

  bsdi_arch_thread_ops ()->fetch_registers (ptc);
}

static void
bsdi_thread_store_registers (int regno)
{
  struct pthread_cache *ptc;

  if (!threads_initialized
      || (current_pthread != NULL
	  && ptid_equal (inferior_ptid, current_pthread->id)))
    {
      child_ops.to_store_registers (regno);
      return;
    }

  ptc = update_pthread_cache (inferior_ptid);
  if (ptc == NULL)
    error ("can't store registers: thread %#x died",
	   ptid_get_tid (inferior_ptid));

  bsdi_arch_thread_ops ()->store_registers (ptc);
}

static void
bsdi_thread_prepare_to_store ()
{
  child_ops.to_prepare_to_store ();
}

static int
bsdi_thread_xfer_memory (CORE_ADDR memaddr, char *myaddr, int len, int dowrite,
			 struct mem_attrib *attrib, struct target_ops *target)
{
  int ret;

  if (target_has_execution)
    ret = child_ops.to_xfer_memory (memaddr, myaddr, len, dowrite, attrib,
				    target);
  else
    ret = orig_core_ops.to_xfer_memory (memaddr, myaddr, len, dowrite, attrib,
					target);

  return ret;
}

static void
bsdi_thread_files_info (struct target_ops *ignore)
{
  child_ops.to_files_info (ignore);
}

static void
bsdi_thread_kill_inferior ()
{
  child_ops.to_kill ();
}

static void
ptrace_me ()
{
  call_ptrace (0, 0, (PTRACE_ARG3_TYPE) 0, 0);
}

static void
ptrace_him (int pid)
{
  push_target (&bsdi_thread_ops);

#ifdef START_INFERIOR_TRAPS_EXPECTED
  startup_inferior (START_INFERIOR_TRAPS_EXPECTED);
#else
  /* One trap to exec the shell, one to exec the program being debugged.  */
  startup_inferior (2);
#endif

  /* XXX Do we want to support target_post_startup_inferior()?  */
}

static void
bsdi_thread_create_inferior (char *exec_file, char *allargs, char **env)
{
  fork_inferior (exec_file, allargs, env, ptrace_me, ptrace_him, NULL, NULL);
  proceed ((CORE_ADDR) -1, TARGET_SIGNAL_0, 0);
}

static void
bsdi_thread_mourn_inferior ()
{
  remove_pthread_cache ();
  unpush_target (&bsdi_thread_ops);
  bsdi_thread_reset ();
  child_ops.to_mourn_inferior ();
}

/* We return the opposite sense from child_can_run().  */
static int
bsdi_thread_can_run ()
{
  return child_suppress_run;
}

static int
bsdi_thread_alive (ptid_t ptid)
{
  struct pthread_cache *ptc;

  if (! threads_initialized)
    return 1;

  /* Sometimes we get asked about the main process.  */
  if (ptid_get_tid (ptid) == 0)
    return 1;

  /* Prune dead pthreads.  */
  get_all_pthreads ();

  for (ptc = pthread_cache_list; ptc; ptc = ptc->next)
    if (ptid_equal (ptc->id, ptid))
      return ptc->pthread.state != PS_DEAD;

  return 0;
}

static void
bsdi_thread_stop ()
{
  child_ops.to_stop ();
}

static void
bsdi_core_open (char *filename, int from_tty)
{
  orig_core_ops.to_open (filename, from_tty);
}

static void
bsdi_core_close (int quitting)
{
  orig_core_ops.to_close (quitting);
}

static void
bsdi_core_detach (char *args, int from_tty)
{
  unpush_target (&core_ops);
  core_ops = orig_core_ops;
  bsdi_thread_reset ();
  core_ops.to_detach (args, from_tty);
}

static void
bsdi_core_files_info (struct target_ops *ops)
{
  orig_core_ops.to_files_info (ops);
}

static void
bsdi_core_fetch_registers (int regno)
{
  struct pthread_cache *ptc;
  CORE_ADDR addr_thread_event;

  /* Core_open() calls us cold, so we need to initialize some values.  */
  if (addr_thread_run == NULL)
    {
      if (bsdi_arch_thread_ops ()->core_event (&addr_thread_event) != 0)
	{
	  threads_initialized = 1;
	  if (orig_core_ops.to_xfer_memory (addr_thread_event,
					    (char *)&addr_thread_run,
					    sizeof addr_thread_run, 0, NULL,
					    &core_ops)
	      != sizeof addr_thread_run)
	    error ("Can't read thread information");
	  if (orig_core_ops.to_xfer_memory (addr_thread_event
					    + sizeof addr_thread_run,
					    (char *)&addr_thread_allthreads,
					    sizeof addr_thread_allthreads, 0,
					    NULL, &core_ops)
	      != sizeof addr_thread_allthreads)
	    error ("Can't read thread information");
	}
    }

  if (threads_initialized && pthread_cache_list == NULL)
    {
      init_thread_list ();	/* Brutal, but effective.  */
      get_all_pthreads ();
      if (current_pthread != NULL)
	inferior_ptid = current_pthread->id;
    }

  if (!threads_initialized
      || (current_pthread != NULL
	  && ptid_equal (inferior_ptid, current_pthread->id)))
    {
      orig_core_ops.to_fetch_registers (regno);
      return;
    }

  /* We need to recover register state from a thread that isn't running.  */
  ptc = lookup_pthread_cache (inferior_ptid);
  if (ptc == NULL)
    error ("bsdi_thread_fetch_registers: can't locate thread");
  if (read_pthread (ptc) == 0)
    error ("bsdi_thread_fetch_registers: can't read thread data");

  bsdi_arch_thread_ops ()->fetch_registers (ptc);
}

static void
bsdi_thread_new_objfile (struct objfile *objfile)
{
  if (objfile == NULL)
    /* XXX Too big a hammer?  */
    bsdi_thread_reset ();
  else
    ; /* ??? */

  /* Call predecessor on chain, if any. */
  if (target_new_objfile_chain)
    target_new_objfile_chain (objfile);
}

/* In theory, we return 1 if we stopped at a breakpoint and the
   selected thread isn't the current thread, and 0 otherwise.
   Currently we don't worry about checking for the breakpoint.  */
int
bsdi_thread_prepare_to_proceed (int unused)
{
  if (!threads_initialized || current_pthread == NULL)
    return 0;

  if (ptid_equal (inferior_ptid, current_pthread->id))
    return 0;

  /* Stolen from switch_to_thread() in thread.c.  */
  inferior_ptid = current_pthread->id;
  flush_cached_frames ();
  registers_changed ();
  stop_pc = read_pc();
  select_frame (get_current_frame (), 0);

  return 1;
}

static char *
bsdi_thread_pid_to_str (ptid_t ptid)
{
  static char buf[100];

  if (! threads_initialized || ptid_get_tid (ptid) == 0)
    sprintf (buf, "process %d    ", PIDGET (ptid));
  else
    sprintf (buf, "thread %#lx", ptid_get_tid (ptid));

  return buf;
}

static void
bsdi_thread_find_new_threads ()
{
  get_all_pthreads ();
}

struct gdbarch_data *bsdi_arch_thread_ops_handle;

static struct bsdi_arch_thread_ops *
bsdi_arch_thread_ops ()
{
  struct bsdi_arch_thread_ops *specific_bsdi_arch_thread_ops;

  gdb_assert (bsdi_arch_thread_ops_handle != NULL);
  specific_bsdi_arch_thread_ops = gdbarch_data (bsdi_arch_thread_ops_handle);
  gdb_assert (specific_bsdi_arch_thread_ops != NULL);
  return specific_bsdi_arch_thread_ops;
}

#ifdef MAINTENANCE_CMDS

/* Implement 'info threads'.  */
static void
info_bsdithreads (char *args, int from_tty)
{
  struct pthread_cache *ptc;
  struct minimal_symbol *msym;
  char *s;

  /* Prune dead pthreads.  */
  get_all_pthreads ();

  /* XXX Sort by historical order?  */
  for (ptc = pthread_cache_list; ptc; ptc = ptc->next)
    {
      if (ptc->pthread.state == PS_DEAD)
	continue;
      printf_filtered ("%c%2d thread %#x ",
		       current_pthread == ptc ? '*' : ' ',
		       pid_to_thread_id ((int)ptc->id),
		       (int)ptc->id);
      switch (ptc->pthread.state)
	{
	case PS_RUNNING:	s = "(run)"; break;
	case PS_RUNNABLE:	s = "(active)"; break;
	case PS_SUSPENDED:	s = "(suspended)"; break;
	case PS_MUTEX_WAIT:	s = "(mutex)"; break;
	case PS_COND_WAIT:	s = "(cond)"; break;
	case PS_IO_WAIT:	s = "(i/o)"; break;
	case PS_SELECT_WAIT:	s = "(select)"; break;
	case PS_SLEEP_WAIT:	s = "(sleep)"; break;
	case PS_WAIT_WAIT:	s = "(wait)"; break;
	case PS_SIG_WAIT:	s = "(sigwait)"; break;
	case PS_JOIN_WAIT:	s = "(join)"; break;
	default:		s = "<unknown>"; break;
	}
      printf_filtered ("%-11s", s);

      if (ptc->pthread.start_routine != NULL)
	if (msym = lookup_minimal_symbol_by_pc ((CORE_ADDR)ptc->pthread.start_routine))
	  printf_filtered ("   startfunc: %s", SYMBOL_NAME (msym));
	else
	  printf_filtered ("   startfunc: %#010x",
			   ((CORE_ADDR)ptc->pthread.start_routine));

      printf_filtered ("\n");
    }
}

#endif

static int
ignore (CORE_ADDR addr, char *contents)
{
  return 0;
}

struct target_ops bsdi_thread_ops;

void
init_bsdi_thread_ops ()
{
  bsdi_thread_ops.to_shortname = "bsdi-threads";
  bsdi_thread_ops.to_longname = "BSD/OS pthread.";
  bsdi_thread_ops.to_doc = "BSD/OS pthread support.";
  bsdi_thread_ops.to_open = bsdi_thread_open;
  bsdi_thread_ops.to_attach = bsdi_thread_attach;
  bsdi_thread_ops.to_detach = bsdi_thread_detach;
  bsdi_thread_ops.to_resume = bsdi_thread_resume;
  bsdi_thread_ops.to_wait = bsdi_thread_wait;
  bsdi_thread_ops.to_fetch_registers = bsdi_thread_fetch_registers;
  bsdi_thread_ops.to_store_registers = bsdi_thread_store_registers;
  bsdi_thread_ops.to_prepare_to_store = bsdi_thread_prepare_to_store;
  bsdi_thread_ops.to_xfer_memory = bsdi_thread_xfer_memory;
  bsdi_thread_ops.to_files_info = bsdi_thread_files_info;
  bsdi_thread_ops.to_insert_breakpoint = memory_insert_breakpoint;
  bsdi_thread_ops.to_remove_breakpoint = memory_remove_breakpoint;
  bsdi_thread_ops.to_terminal_init = terminal_init_inferior;
  bsdi_thread_ops.to_terminal_inferior = terminal_inferior;
  bsdi_thread_ops.to_terminal_ours_for_output = terminal_ours_for_output;
  bsdi_thread_ops.to_terminal_ours = terminal_ours;
  bsdi_thread_ops.to_terminal_info = child_terminal_info;
  bsdi_thread_ops.to_kill = bsdi_thread_kill_inferior;
  bsdi_thread_ops.to_create_inferior = bsdi_thread_create_inferior;
  bsdi_thread_ops.to_mourn_inferior = bsdi_thread_mourn_inferior;
  bsdi_thread_ops.to_can_run = bsdi_thread_can_run;
  bsdi_thread_ops.to_thread_alive = bsdi_thread_alive;
  bsdi_thread_ops.to_pid_to_str = bsdi_thread_pid_to_str;
  bsdi_thread_ops.to_find_new_threads = bsdi_thread_find_new_threads;
  bsdi_thread_ops.to_stop = bsdi_thread_stop;
  bsdi_thread_ops.to_stratum = process_stratum;
  bsdi_thread_ops.to_has_all_memory = 1;
  bsdi_thread_ops.to_has_memory = 1;
  bsdi_thread_ops.to_has_stack = 1;
  bsdi_thread_ops.to_has_registers = 1;
  bsdi_thread_ops.to_has_execution = 1;
  bsdi_thread_ops.to_has_thread_control = tc_none;
  /* bsdi_thread_ops.to_find_memory_regions = XXX; */
  /* bsdi_thread_ops.to_make_corefile_notes = XXX; */
  bsdi_thread_ops.to_magic = OPS_MAGIC;
}

struct target_ops bsdi_core_ops;

void
init_bsdi_core_ops ()
{
  bsdi_core_ops.to_shortname = "bsdi-core";
  bsdi_core_ops.to_longname = "BSD/OS core pthread.";
  bsdi_core_ops.to_doc = "BSD/OS pthread support for core files.";
  bsdi_core_ops.to_open = bsdi_core_open;
  bsdi_core_ops.to_close = bsdi_core_close;
  bsdi_core_ops.to_attach = bsdi_thread_attach;
  bsdi_core_ops.to_detach = bsdi_core_detach;
  bsdi_core_ops.to_fetch_registers = bsdi_core_fetch_registers;
  bsdi_core_ops.to_xfer_memory = bsdi_thread_xfer_memory;
  bsdi_core_ops.to_files_info = bsdi_core_files_info;
  bsdi_core_ops.to_insert_breakpoint = ignore;
  bsdi_core_ops.to_remove_breakpoint = ignore;
  bsdi_core_ops.to_create_inferior = bsdi_thread_create_inferior;
  bsdi_core_ops.to_stratum = core_stratum;
  bsdi_core_ops.to_has_memory = 1;
  bsdi_core_ops.to_has_stack = 1;
  bsdi_core_ops.to_has_registers = 1;
  bsdi_core_ops.to_has_thread_control = tc_none;
  bsdi_core_ops.to_magic = OPS_MAGIC;
}

/* Core targets are messed up.  Suppress other core targets.  */
int coreops_suppress_target = 1;

void
_initialize_bsdi_thread ()
{
  if (bsdi_kernel_debugging)
    return;

  init_bsdi_thread_ops ();
  init_bsdi_core_ops ();

  add_target (&bsdi_thread_ops);
  child_suppress_run = 1;

#ifdef MAINTENANCE_CMDS
  add_cmd ("bsdi-threads", class_maintenance, info_bsdithreads, 
	    "Show info on BSDI user threads.\n", &maintenanceinfolist);
#endif /* MAINTENANCE_CMDS */

  memcpy(&orig_core_ops, &core_ops, sizeof (struct target_ops));
  memcpy(&core_ops, &bsdi_core_ops, sizeof (struct target_ops));
  add_target (&core_ops);

  /* Hook into new_objfile notification. */
  target_new_objfile_chain = target_new_objfile_hook;
  target_new_objfile_hook  = bsdi_thread_new_objfile;

  /* Set up architecture specific thread ops.  */
  bsdi_arch_thread_ops_handle = register_gdbarch_data (NULL, NULL);
}
