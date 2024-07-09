/* SLIP-style 'kgdb' protocol for remote BSD/OS kernels.
   Copyright (C) 1988, 1989, 1991 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/*
 * Copyright (c) 1990, 1991, 1992 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Lawrence Berkeley Laboratory,
 * Berkeley, CA.  The name of the University may not be used to
 * endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <stdio.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/file.h>

#include "defs.h"
#include "frame.h"
#include "inferior.h"
#include "symfile.h"
#include "target.h"
#include "gdb_wait.h"
#include "gdbcmd.h"
#include "remote-kgdb.h"
#include "remote-kgdb-sl.h"
#include "kgdb_proto.h"
#include "bsdi-kernel.h"

extern struct target_ops remote_kgdb_ops;

/* Count attaches and resumes, so that other code can find out whether
   its data is stale.  */
int remote_kgdb_generation = 1;

static int remote_kgdb_debugging;
static int remote_kgdb_quiet;

static unsigned long to_clamp = 8000;
static int to_base = 1000;

/* Exponentially back off a timer value.  Clamp the value at to_clamp.  */
#define BACKOFF(to)	((2 * (to) < to_clamp) ? 2 * (to) : to_clamp)

static FILE *kiodebug;
static int icache = 1;

static int remote_kgdb_cache_valid;
static int remote_kgdb_instub;

static struct remote_kgdb_fn remote_kgdb_fn;

static unsigned char *inbuffer;
static unsigned char *outbuffer;

static void init_remote_kgdb_ops (void);
static void m_recv (int, unsigned char *, int *);
static void m_send (int, unsigned char *, int);
static int m_xchg (int, unsigned char *, int, unsigned char *, int *);
static void print_msg (int, unsigned char *, int, int);
static void remote_kgdb_bail (void *);
static void remote_kgdb_close (int);
static void remote_kgdb_debug (char *, ...);
static void remote_kgdb_debug_command (char *, int);
static void remote_kgdb_detach (char *, int);
static void remote_kgdb_fetch_registers (int);
static void remote_kgdb_files_info (struct target_ops *);
static int remote_kgdb_insert_bkpt (CORE_ADDR, char *);
static void remote_kgdb_kill (void);
static void remote_kgdb_mourn (void);
static void remote_kgdb_open (char *, int);
static void remote_kgdb_prepare_to_store (void);
static int remote_kgdb_read (CORE_ADDR, unsigned char *, int);
static int remote_kgdb_read_text (CORE_ADDR, char *, int, struct target_ops *);
static int remote_kgdb_remove_bkpt (CORE_ADDR, char *);
static void remote_kgdb_resume (ptid_t, int, enum target_signal);
static void remote_kgdb_signal (void);
static void remote_kgdb_signal_command (char *, int);
static void remote_kgdb_store_registers (int);
static ptid_t remote_kgdb_wait (ptid_t, struct target_waitstatus *);
static int remote_kgdb_write (CORE_ADDR, unsigned char *, int);
static int remote_kgdb_xfer_memory (CORE_ADDR, char *, int, int,
     struct mem_attrib *, struct target_ops *);

/*
 * Statistics.
 */
static int remote_kgdb_ierrs;
static int remote_kgdb_oerrs;
static int remote_kgdb_seqerrs;
static int remote_kgdb_spurious;

#define PUTCMD(cmd)	m_xchg (cmd, 0, 0, 0, 0)

/* Send an outbound message to the remote machine and read the reply.
   Either or both message buffers may be NULL.  */

static int
m_xchg (int type, unsigned char *out, int outlen, unsigned char *in, int *inlen)
{
  int err, nrecv;
  int (*send) (unsigned char, unsigned char *, int) = remote_kgdb_fn.send;
  int (*recv) (int *, unsigned char *, int *, int) = remote_kgdb_fn.recv;
  int ack;
  unsigned long to = to_base;
  static int seqbit = 0;

  if (! remote_kgdb_instub)
    {
      remote_kgdb_instub = 1;
      PUTCMD (KGDB_EXEC);
    }

  seqbit ^= KGDB_SEQ;
  for (;;)
    {
      nrecv = 0;
      err = (*send) (type | seqbit, out, outlen);
      if (err)
	{
	  ++remote_kgdb_oerrs;
	  if (kiodebug)
	    remote_kgdb_debug ("send error %d\n", err);
	  /* XXX shouldn't we retry the !@#$^*ing send? */
	}
      if (kiodebug)
	print_msg (type | seqbit, out, outlen, 'O');

again:
      err = (*recv) (&ack, in, inlen, to);
      if (err != 0)
	{
	  ++remote_kgdb_ierrs;
	  if (kiodebug)
	    remote_kgdb_debug ("recv error %d\n", err);
	  remote_kgdb_cache_valid = 0;
	  if (err == EKGDB_TIMEOUT)
	    {
	      to = BACKOFF (to);
	      if (to >= to_clamp)
		error ("remote host not responding");
	      continue;
	    }
	  if (++nrecv < 5)
	    /* Try to receive five times before retransmitting.  */
	    goto again;
	  continue;
	}
      else
	if (kiodebug)
	  print_msg (ack, in, inlen ? *inlen : 0, 'I');

      if ((ack & KGDB_ACK) == 0 || KGDB_CMD (ack) != KGDB_CMD (type))
	{
	  ++remote_kgdb_spurious;
	  continue;
	}
      if ((ack & KGDB_SEQ) ^ seqbit)
	{
	  ++remote_kgdb_seqerrs;
	  goto again;
	}
      return ack;
    }
}

/* Wait for the specified message type.  Discard anything else.
   This is used by 'remote-signal' to help us resync with other side. */

static void
m_recv (int type, unsigned char *in, int *inlen)
{
  int reply, err;

  for (;;)
    {
      err = (*remote_kgdb_fn.recv) (&reply, in, inlen, -1);
      if (err)
	{
	  ++remote_kgdb_ierrs;
	  if (kiodebug)
	    remote_kgdb_debug ("recv error %d\n", err);
	}
      else
	if (kiodebug)
	  print_msg (reply, in, inlen ? *inlen : 0, 'I');

      if (KGDB_CMD (reply) == type)
	return;
      ++remote_kgdb_spurious;
    }
}

/* Send a message.  Do not wait for *any* response from the other side.
   Some other thread of control will pick up the ack that will be generated.  */

static void
m_send (int type, unsigned char *buf, int len)
{
  int err;

  if (! remote_kgdb_instub)
    {
      remote_kgdb_instub = 1;
      PUTCMD (KGDB_EXEC);
    }

  err = (*remote_kgdb_fn.send) (type, buf, len);
  if (err)
    {
      ++remote_kgdb_ierrs;
      if (kiodebug)
	remote_kgdb_debug ("[send error %d] ", err);
    }
  if (kiodebug)
    print_msg (type, buf, len, 'O');
}

static void
remote_kgdb_bail (void *arg)
{
  printf ("Remote attach interrupted.\n");
  remote_kgdb_quiet = 1;
  pop_target ();
}

/* Open a connection to a remote debugger.
   NAME is the filename used for communication.  */

static void
remote_kgdb_open (char *name, int from_tty)
{
  struct cleanup *chain;

  if (name == 0)
    error ("To open a remote debug connection, "
	   "you need to specify what serial\n"
	   "device is attached to the remote system (e.g. /dev/ttya).");
  if (remote_kgdb_debugging) 
    error ("Already remote debugging.  Detach first.");

  target_preopen (from_tty);

  memset (&remote_kgdb_fn, '\0', sizeof remote_kgdb_fn);
  kgdb_sl_open (name, &remote_kgdb_fn);

  if (from_tty)
    printf ("Remote debugging using %s\n", name);
  remote_kgdb_debugging = 1;

  ++remote_kgdb_generation;

  remote_kgdb_cache_valid = 0;

  /* Remote protocol limits data length size to one byte.  */
  if (remote_kgdb_fn.maxdata > 255)
    remote_kgdb_fn.maxdata = 255;

  /* Allocate msg buffers.  */
  inbuffer = xmalloc (remote_kgdb_fn.rpcsize);
  outbuffer = xmalloc (remote_kgdb_fn.rpcsize);

  chain = make_cleanup (remote_kgdb_bail, 0);
  push_target (&remote_kgdb_ops);

  remote_kgdb_ierrs = 0;
  remote_kgdb_oerrs = 0;
  remote_kgdb_spurious = 0;

  /* Signal the remote kernel and set things up
     so gdb views it as a process.  */
  remote_kgdb_signal ();
  start_remote ();

  discard_cleanups (chain);
}

/* Take the remote kernel out of debugging mode.  */

static void
remote_kgdb_kill ()
{
  /* Clear all breakpoints.
     It's a very, very bad idea to go away leaving
     breakpoints in a remote kernel or to leave it
     stopped at a breakpoint.  */
  remove_breakpoints ();

  /* Take remote machine out of debug mode.  */
  PUTCMD (KGDB_KILL);
  remote_kgdb_debugging = 0;

  target_mourn_inferior ();
}

static void
remote_kgdb_mourn ()
{
  unpush_target (&remote_kgdb_ops);
  generic_mourn_inferior ();
}

/* Close the open connection to the remote debugger.  Use this when you want
   to detach and do something else with your gdb.  */

static void
remote_kgdb_detach (char *args, int from_tty)
{
  if (args)
    {
      if (args[0] == 'q')
	remote_kgdb_quiet = 1;
      else
	error ("Bad argument to remote \"detach\".");
    }	
  if (remote_kgdb_quiet)
    /* This will invoke remote_kgdb_close().  */
    pop_target ();
  else
    target_close (0);
  if (from_tty)
    printf ("Ending remote debugging.\n");
}

static void
remote_kgdb_close (int quitting)
{
  if (! remote_kgdb_debugging)
    return;
  if (! remote_kgdb_quiet)
    {
      remote_kgdb_quiet = 0;
      remote_kgdb_kill ();
    }
  else
    remote_kgdb_debugging = 0;

  (*remote_kgdb_fn.close) ();

  free (inbuffer);
  free (outbuffer);
}

/* Tell the remote machine to resume.  */

static void
remote_kgdb_resume (ptid_t ptid, int step, enum target_signal sig)
{
  ++remote_kgdb_generation;

  if (! step)
    {
      PUTCMD (KGDB_CONT);
      remote_kgdb_instub = 0;
    }
  else
    {
      if (SOFTWARE_SINGLE_STEP_P ())
	SOFTWARE_SINGLE_STEP (0, 0);
      else
	PUTCMD (KGDB_STEP);
    }
}

/* Wait until the remote machine stops, then return, storing status in STATUS
   just as `wait' would.  */

static ptid_t
remote_kgdb_wait (ptid_t ptid, struct target_waitstatus *tstatus)
{
  int len;

  tstatus->kind = TARGET_WAITKIND_EXITED;
  tstatus->value.integer = 0;

  /* When the machine stops, it will send us a KGDB_SIGNAL message,
     so we wait for one of these.  */
  m_recv (KGDB_SIGNAL, inbuffer, &len);
  tstatus->kind = TARGET_WAITKIND_STOPPED;
  tstatus->value.sig = (enum target_signal)inbuffer[0];

  /* Let the machine dependent module have a chance to 
     look up current process context and so on.  */
  bsdi_set_curproc ();

  return null_ptid;
}

/* Register context as of last remote_kgdb_fetch_registers().  */
static char reg_cache[REGISTER_BYTES];

/* Read the remote registers into the block REGS.  */

static void
remote_kgdb_fetch_registers (int unused_regno)
{
  int regno, len, rlen, ack;
  unsigned char *cp, *ep;

  regno = -1;
  do
    {
      outbuffer[0] = regno + 1;
      ack = m_xchg ((remote_kgdb_cache_valid ? KGDB_DELTA : 0) | KGDB_REG_R,
		    outbuffer, 1, inbuffer, &len);
      cp = inbuffer;
      ep = cp + len;
      while (cp < ep)
	{
	  regno = *cp++;
	  rlen = REGISTER_RAW_SIZE (regno);
	  memcpy (&reg_cache[REGISTER_BYTE (regno)], cp, rlen);
	  cp += rlen;
	}
    }
  while (ack & KGDB_MORE);

  remote_kgdb_cache_valid = 1;
  for (regno = 0; regno < NUM_REGS; ++regno)
    supply_register (regno, &reg_cache[REGISTER_BYTE (regno)]);
}

/* Store the remote registers from the contents of the block REGS.  */
static void
remote_kgdb_store_registers (int unused_regno)
{
  char *regs = registers;
  unsigned char *cp, *ep;
  int regno, off, rlen;

  cp = outbuffer;
  ep = cp + remote_kgdb_fn.maxdata;

  for (regno = 0; regno < NUM_REGS; ++regno)
    {
      off = REGISTER_BYTE (regno);
      rlen = REGISTER_RAW_SIZE (regno);
      if (! remote_kgdb_cache_valid ||
	  memcmp (&regs[off], &reg_cache[off], rlen) != 0)
	{
	  if (cp + rlen + 1 >= ep)
	    {
	      m_xchg (KGDB_REG_W, outbuffer, cp - outbuffer, 0, 0);
	      cp = outbuffer;
	    }
	  *cp++ = regno;
	  memcpy (cp, &regs[off], rlen);
	  cp += rlen;
	}
    }
  if (cp != outbuffer)
    m_xchg (KGDB_REG_W, outbuffer, cp - outbuffer, 0, 0);

  memcpy (reg_cache, regs, REGISTER_BYTES);
}

/* XXX DOES THE NEW PROTOCOL NEED THIS?
   Prepare to store registers.  Since we send them all, we have to
   read out the ones we don't want to change first.  */

static void 
remote_kgdb_prepare_to_store ()
{
  remote_kgdb_fetch_registers (-1);
}

/* Store a chunk of memory into the remote host.
   'remote_addr' is the address in the remote memory space.
   'cp' is the address of the buffer in our space, and 'len' is
   the number of bytes.  Returns an errno status.  */

static int
remote_kgdb_write (CORE_ADDR addr, unsigned char *cp, int len)
{
  const size_t kaddr_size = TARGET_PTR_BIT / TARGET_CHAR_BIT;
  int cnt;

  while (len > 0)
    {
      cnt = min (len, remote_kgdb_fn.maxdata - kaddr_size);
      store_address (outbuffer, kaddr_size, addr);
      memcpy (outbuffer + kaddr_size, cp, cnt);
      m_xchg (KGDB_MEM_W, outbuffer, cnt + kaddr_size, inbuffer, &len);

      if (inbuffer[0])
	return inbuffer[0];

      addr += cnt;
      cp += cnt;
      len -= cnt;
    }
  return 0;
}

/* Read memory data directly from the remote machine.
   'addr' is the address in the remote memory space.
   'cp' is the address of the buffer in our space, and 'len' is
   the number of bytes.  Returns an errno status.  */

static int
remote_kgdb_read (CORE_ADDR addr, unsigned char *cp, int len)
{
  const size_t kaddr_size = TARGET_PTR_BIT / TARGET_CHAR_BIT;
  int cnt, inlen;

  while (len > 0)
    {
      cnt = min (len, remote_kgdb_fn.maxdata);
      outbuffer[0] = cnt;
      store_address (&outbuffer[1], kaddr_size, addr);

      m_xchg (KGDB_MEM_R, outbuffer, kaddr_size + 1, inbuffer, &inlen);
      if (inlen < 1)
	error ("remote_kgdb_read: remote protocol botch");

      /* Return errno from remote side.  */
      if (inbuffer[0])
	return inbuffer[0];

      --inlen;

      if (inlen <= 0)
	error ("remote_kgdb_read: inlen too small (%d)", inlen);

      if (inlen > cnt)
	{
	  printf ("remote_kgdb_read: warning: asked for %d, got %d\n",
		  cnt, inlen);
	  inlen = cnt;
	}

      memcpy (cp, &inbuffer[1], inlen);

      addr += inlen;
      cp += inlen;
      len -= inlen;
    }
  return 0;
}

static int
remote_kgdb_read_text (CORE_ADDR addr, char *cp, int len,
     struct target_ops *target)
{
  struct target_stack_item *tsi;
  int cc;

  /* Look down the target stack (we're on top) for the text file.
     If it can transfer the whole chunk, let it.  Otherwise,
     revert to the remote call.  */
  for (tsi = target_stack; tsi != 0; tsi = tsi->next)
    if (tsi->target_ops == target)
      break;
  for (; tsi != 0; tsi = tsi->next)
    {
      if (tsi->target_ops->to_stratum != file_stratum)
	continue;
      cc = tsi->target_ops->to_xfer_memory (addr, cp, len, 0, NULL, target);
      if (cc == len)
	return 0;
    }		
  return remote_kgdb_read (addr, cp, len);
}

/* Read or write LEN bytes from inferior memory at MEMADDR, transferring
   to or from debugger address MYADDR.  Write to inferior if SHOULD_WRITE is
   nonzero.  Returns length of data written or read; 0 for error.  */
static int
remote_kgdb_xfer_memory (CORE_ADDR addr, char *cp, int len, int should_write,
     struct mem_attrib *attrib, struct target_ops *target)
{
  int st;

  if (should_write)
    st = remote_kgdb_write (addr, cp, len);
  else
    {
#ifdef NEED_TEXT_START_END
      extern CORE_ADDR text_start, text_end;
      if (icache && addr >= text_start && addr + len <= text_end)
	st = remote_kgdb_read_text (addr, cp, len, target);
      else
#endif
      st = remote_kgdb_read (addr, cp, len);
    }
  return (st == 0) ? len : 0;
}

static int
remote_kgdb_insert_bkpt (CORE_ADDR addr, char *save)
{
  int r;
  unsigned char *b;
  int size_b;

  b = BREAKPOINT_FROM_PC (&addr, &size_b);
  if (b == NULL || size_b <= 0)
    error ("bad breakpoint in remote_kgdb_insert_bkpt");

  if ((r = remote_kgdb_read (addr, save, size_b)) != 0)
    return r;

  return remote_kgdb_write (addr, (char *)b, size_b);
}

static int
remote_kgdb_remove_bkpt (CORE_ADDR addr, char *save)
{
  int size_b;

  BREAKPOINT_FROM_PC (&addr, &size_b);
  if (size_b <= 0)
    error ("bad breakpoint size in remote_kgdb_remove_bkpt");

  return remote_kgdb_write (addr, save, size_b);
}

/* Signal the remote machine.  The remote end might be idle or it might
   already be in debug mode -- we need to handle both case.  Thus, we use
   the framing character as the wakeup byte, and send a SIGNAL packet.
   If the remote host is idle, the framing character will wake it up.
   If it is in the kgdb stub, then we will get a SIGNAL reply.  */

static void
remote_kgdb_signal ()
{
  if (! remote_kgdb_debugging)
    printf ("Remote agent not active.\n");
  else
    {
      remote_kgdb_instub = 0;
      m_send (KGDB_SIGNAL, 0, 0);
    }
}

static void
remote_kgdb_signal_command (char *unused1, int unused2)
{
  struct target_waitstatus t;

  if (! remote_kgdb_debugging)
    error ("Remote agent not active.");
  remote_kgdb_cache_valid = 0;
  remote_kgdb_signal ();
  target_wait (pid_to_ptid (-1), &t);
  remove_breakpoints ();
  remote_kgdb_signal ();
  start_remote ();
}

/* Print a message for debugging.  */
static void
print_msg (int type, unsigned char *buf, int len, int dir)
{
  int i;
  char *s;

  switch (KGDB_CMD (type))
    {
    case KGDB_MEM_R:	s = "memr"; break;
    case KGDB_MEM_W:	s = "memw"; break;
    case KGDB_REG_R:	s = "regr"; break;
    case KGDB_REG_W:	s = "regw"; break;
    case KGDB_CONT:	s = "cont"; break;
    case KGDB_STEP:	s = "step"; break;
    case KGDB_KILL:	s = "kill"; break;
    case KGDB_SIGNAL:	s = "sig "; break;
    case KGDB_EXEC:	s = "exec"; break;
    default:		s = "unk "; break;
    }
  remote_kgdb_debug ("%c %c%c%c%c %s (%02x): ", dir,
		(type & KGDB_ACK) ? 'A' : '.',
		(type & KGDB_DELTA) ? 'D' : '.',
		(type & KGDB_MORE) ? 'M' : '.',
		(type & KGDB_SEQ) ? '-' : '+',
		s, type);
  if (buf)
    for (i = 0; i < len; ++i)
      remote_kgdb_debug ("%02x", buf[i]);
  remote_kgdb_debug ("\n");
}

static void
remote_kgdb_debug_command (char *arg, int from_tty)
{
  char *name;

  if (kiodebug != 0 && kiodebug != stderr)
    fclose (kiodebug);

  if (arg == 0)
    {
      kiodebug = 0;
      printf ("Remote debugging off.\n");
      return;
    }
  if (arg[0] == '-')
    {
      kiodebug = stderr;
      name = "stderr";
    }
  else
    {
      kiodebug = fopen (arg, "w");
      if (kiodebug == 0)
	{
	  printf ("Cannot open '%s'.\n", arg);
	  return;
	}
      name = arg;
    }
  printf ("Remote debugging output routed to %s.\n", name);
}

static void
remote_kgdb_files_info (struct target_ops *unused)
{
  printf ("Using %s for text references.\n",
	  icache ? "local executable" : "remote");
  printf ("Protocol debugging is %s.\n", kiodebug? "on" : "off");
  printf ("%d spurious input messages.\n", remote_kgdb_spurious);
  printf ("%d input errors; %d output errors; %d sequence errors.\n",
	  remote_kgdb_ierrs, remote_kgdb_oerrs, remote_kgdb_seqerrs);
}

static void
remote_kgdb_debug (char *fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);

	vfprintf (kiodebug, fmt, ap);
	va_end (ap);
	fflush (kiodebug);
}

/* Define the target subroutine names.  */

struct target_ops remote_kgdb_ops;

static void
init_remote_kgdb_ops ()
{
  remote_kgdb_ops.to_shortname = "kgdb";
  remote_kgdb_ops.to_longname
    = "Remote serial target using LBL/4.4BSD kgdb protocol";
  remote_kgdb_ops.to_doc
    = "Debug a remote host, using LBL/4.4BSD kgdb protocol.\n"
      "Specify the target as an argument (e.g. /dev/tty01 for a serial link).\n"
      "Use \"detach\" or \"quit\" to end remote debugging gracefully, which\n"
      "removes breakpoints from and resumes the remote kernel.\n"
      "Use \"detach quiet\" to end remote debugging with no negotiation.\n"
      "This latter method is desirable, for instance, when the remote kernel\n"
      "has crashed and messages from gdb could wreak havoc.\n";
  remote_kgdb_ops.to_open = remote_kgdb_open;
  remote_kgdb_ops.to_close = remote_kgdb_close;
  remote_kgdb_ops.to_detach = remote_kgdb_detach;
  remote_kgdb_ops.to_resume = remote_kgdb_resume;
  remote_kgdb_ops.to_wait = remote_kgdb_wait;
  remote_kgdb_ops.to_fetch_registers = remote_kgdb_fetch_registers;
  remote_kgdb_ops.to_store_registers = remote_kgdb_store_registers;
  remote_kgdb_ops.to_prepare_to_store = remote_kgdb_prepare_to_store;
  remote_kgdb_ops.to_xfer_memory = remote_kgdb_xfer_memory;
  remote_kgdb_ops.to_files_info = remote_kgdb_files_info;
  remote_kgdb_ops.to_insert_breakpoint = remote_kgdb_insert_bkpt;
  remote_kgdb_ops.to_remove_breakpoint = remote_kgdb_remove_bkpt;
  remote_kgdb_ops.to_kill = remote_kgdb_kill;
  remote_kgdb_ops.to_load = generic_load;
  remote_kgdb_ops.to_mourn_inferior = remote_kgdb_mourn;
  remote_kgdb_ops.to_stratum = process_stratum;
  remote_kgdb_ops.to_has_all_memory = 1;
  remote_kgdb_ops.to_has_memory = 1;
  remote_kgdb_ops.to_has_stack = 1;
  remote_kgdb_ops.to_has_registers = 1;
  remote_kgdb_ops.to_has_execution = 1;
  remote_kgdb_ops.to_magic = OPS_MAGIC;
}

void
_initialize_remote_kgdb ()
{
  struct cmd_list_element *c;
  extern struct cmd_list_element *setlist;

  init_remote_kgdb_ops ();

  add_com ("remote-signal", class_run, remote_kgdb_signal_command,
	   "If remote debugging, send interrupt signal to remote.");

  c = add_set_cmd ("remote-text-refs", class_support, var_boolean, &icache,
		   "Set use of local executable for text segment references.\n"
		   "If on, all memory read/writes go to remote.\n"
		   "If off, text segment reads use the local executable.",
		   &setlist);
  add_show_from_set (c, &showlist);

  c = add_set_cmd ("remote-timeout", class_support, var_uinteger, &to_base,
		   "Set remote timeout interval (in msec).  "
		   "The gdb remote protocol\n"
		   "uses an exponential backoff retransmit timer "
		   "that begins initialized\n"
		   "to this value (on each transmission).",
		   &setlist);
  add_show_from_set (c, &showlist);

  add_com ("remote-debug", class_run, remote_kgdb_debug_command,
	   "With a file name argument, enables output "
	   "of remote protocol debugging\n"
	   "messages to said file.  If file is `-', stderr is used.\n"
	   "With no argument, remote debugging is disabled.");

#ifdef notdef
  add_info ("remote-kgdb", remote_kgdb_info,
	    "Show current settings of remote debugging options.");
#endif
  add_target (&remote_kgdb_ops);
}
