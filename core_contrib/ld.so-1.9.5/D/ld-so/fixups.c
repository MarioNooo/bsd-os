/*
 * Copyright (c) 1993-1996 Eric Youngdale, Peter MacDonald, David Engel
 * and Hongjiu Lu.
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
 * are linked to a shared library form of the GPL or GLPL library, and as long
 * as the form of the shared library is such that it is possible for
 * the end user to modify and rebuild the library and use it in
 * conjunction with the program executable.
 */

#include <stddef.h>
#include "fixups.h"

/* Here we fix addresses of global variables that are stored in the
   data section (or stored as data in the text section).d  We wait to
   do this until after all of the GOT pointers have been updated by
   __dynamic_resolve because this is where we are copying our numbers
   from */

int __reset_magic(struct fixuplist * xpnt)
{
  int size;
  /* Now go back through and reset the magic numbers.  The only reason we
     need to do this is that we must allow for the possiblility of the
     memory image being dumped to a new executable (i.e. emacs). */

  if(*xpnt->magic == 0xfeeb1ed3) return 0;  /* This means we have been through
							   here before */
  if(*xpnt->magic != 0xfeeb1ed6)
    return -1;
  
  (*xpnt->magic) = 0xfeeb1ed3;  /* Reset back to the original value */

  size = (unsigned int) xpnt->size - 3;
  
  /* First recurse through the various shared libraries we are linked to */
  while(size >= 0)
    if (__reset_magic(**xpnt->shrlib_list[size--]))
      return -1;

  return 0;
}

int __do_fixups(struct fixuplist * xpnt, int flag)
{
  int size;
  struct builtin_fixup * bpnt;
  /* Now we walk through the list of pointers that need to be updated.
     This is essentially our dynamic linking.  Strictly speaking this is
     somewhat simpler, because all we are really doing is fixing the scope
     of variables and functions so that functions are used in the same
     was as they would be if we were linked statically.
     */

  if(*xpnt->magic == 0xfeeb1ed4 + (flag << 1)) return 0;  /* This means we have been through
							   here before */
  
  if(*xpnt->magic != 0xfeeb1ed3 + (flag << 1))
    return -1;
  
  (*xpnt->magic)++;  /* This is so we know if we have been
			through here before */

  size = (unsigned int) xpnt->size - 3;
  
  /* First recurse through the various shared libraries we are linked to */
  while(size >= 0)
    if (__do_fixups(**xpnt->shrlib_list[size--], flag))
      return -1;
      
  /* Now get the pointer to the builtin fixups. */
  bpnt = xpnt->list->fix[xpnt->list->size].un.bifu;
  
  /* If there are fixups, then we scan through the various files that
     have tables, and apply all of the fixups in the table.  Consider that
     an address could be stored as:

     .long _foo+3

     we allow for the offset by first subracting the value of _foo that
     the linker assigned, then using __dynamic_resolve to fix all of the
     GOT entries that need to be fixed, and then adding in the new offset
     of the (possibly) new value of _foo 
*/
  if(bpnt)
    for(size = 0; size < bpnt->len; size++) {
      int i = 0;
      struct file_fixup * fpnt;
      fpnt = bpnt->fixpnt[size];
      while(1==1){
	if(!fpnt->fixup[i].gotaddr) break;
	if(!flag)
	  *fpnt->fixup[i].fixaddr -= *fpnt->fixup[i].gotaddr;
	else
	  *fpnt->fixup[i].fixaddr += *fpnt->fixup[i].gotaddr;
	i++;
      };
    };

  return 0;
}

int __dynamic_resolve(struct fixuplist * xpnt)
{
  /* Now we walk through the list of pointers that need to be updated.
     This is essentially our dynamic linking.  Strictly speaking this is
     somewhat simpler, because all we are really doing is fixing the scope
     of variables and functions so that functions are used in the same
     was as they would be if we were linked statically.
     */
  int size;

  /* Check for the magic number to make sure that we really have a proper
     fixup table */

  if(*xpnt->magic == 0xfeeb1ed5) return 0;  /* This means we have been through
					     here before */

  if(*xpnt->magic != 0xfeeb1ed4)
    return -1;

  (*xpnt->magic)++;  /* This is so we catch circularly linked libraries */

  size = (unsigned int) xpnt->size - 3;
  
  /* Recurse through the shared libraries, one by one */
  while(size >= 0)
    if (__dynamic_resolve(**xpnt->shrlib_list[size--]))
      return -1;
      
  /* Now that this is done, perform the fixups that were generated
     when this image was linked. */

  for(size = 0; size < xpnt->list->size; size++) {
    if(!xpnt->list->fix[size].un.newaddr) break;
    *xpnt->list->fix[size].fixaddr = xpnt->list->fix[size].un.newaddr;
  };

  /* Now handle the linker-identified fixups.  These are listed as a
     pair of GOT addresses - the difference is that these are represent a
     pair of pointers to GOT entries rather than a new address and a GOT
     address.  There are a pair of longword NULL pointers that separate the
     linker identified fixups from the linker identified places where we
     simply copy a pointer.  We are effectively replacing pointers in the
     GOT variables here.  */

  while (++size < xpnt->list->size){
    *xpnt->list->fix[size].fixaddr = 
      *((int *) xpnt->list->fix[size].un.newaddr);
    size++;
  };

  return 0;
}

