/*
 * Copyright (c) 1994-1996 Eric Youngdale, Peter MacDonald, David Engel,
 * Hongjiu Lu and Mitch D'Souza.
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

#include <unistd.h>
#include <syscall.h>
#include <stdarg.h>
#include <sharedlib.h>
#include <errno.h>
#include <sys/mman.h>
#include "../config.h"
#include "../ld-so/fixups.h"

static int errno = 0;

#define SYS_dl_exit SYS_exit
#define SYS_dl_uselib SYS_uselib
#define SYS_dl_write SYS_write
#define SYS_dl_munmap SYS_munmap

static
_syscall1(int,dl_exit,int,code)

static
_syscall1(int,dl_uselib,const char *,filename)

static
_syscall3(ssize_t,dl_write,int,fd,const void *,buf,size_t,count)

static
_syscall2(int,dl_munmap,caddr_t,addr,size_t,len)

extern struct libentry * __SHARED_LIBRARIES__[];
extern struct fixuplist  _SHARABLE_CONFLICTS__;

void
__load_shared_libraries (int argc, char **argv, char **envp)
{
    if (__SHARED_LIBRARIES__[2])
    {
#ifndef DEBUG_LIB
	unsigned mapinfo[2];
	/* this must be volatile to work with -O, GCC bug? */
	volatile loadptr loader = (loadptr)LDSO_ADDR;

	if (dl_uselib(LDSO_IMAGE))
	{
	    char *p = argv[0];
	    static char errmsg[] =
		": can't load dynamic linker '" LDSO_IMAGE "'\n";

	    if (p)
	    {
		while (*p) p++;
		dl_write(2, argv[0], p-argv[0]);
	    }

	    dl_write(2, errmsg, (sizeof errmsg) - 1);
	    while (1) dl_exit(128);
	}
	loader((argc <= 0) ? FUNC_LDD : FUNC_LINK, mapinfo, argv[0],
	       envp, __SHARED_LIBRARIES__, &_SHARABLE_CONFLICTS__);
	dl_munmap((caddr_t)mapinfo[0], mapinfo[1]);
#else
	void shared_loader(int func, ...);
        shared_loader((argc <= 0) ? FUNC_LDD : FUNC_LINK, argv[0],
               envp,__SHARED_LIBRARIES__, &_SHARABLE_CONFLICTS__);
#endif
    }
    else 
    {
	static char msg[] = "\tstatically linked\n";

	if (argc<=0)
		dl_write(2, msg, (sizeof msg) - 1);
    }
    if (argc <= 0)
	while (1) dl_exit(0);
}
