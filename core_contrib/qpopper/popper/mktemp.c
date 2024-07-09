/*
 * Copyright (c) 2003 QUALCOMM Incorporated. All rights reserved.
 * See License.txt file for terms and conditions for modification and
 * redistribution.
 *
 * Revisions: 
 *
 * 06/16/00  [rcg]
 *         - Fix compiler warnings.
 */



/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *       This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#if HAVE_UNISTD_H
#  include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if HAVE_SYS_UNISTD_H
#  include <sys/unistd.h>
#endif /* HAVE_SYS_UNISTD_H */

#include "popper.h"

#ifndef S_ISDIR
#  define S_ISDIR(mode)   ((mode&0xF000) == 0x4000)
#endif

static int _gettemp();

int
mkstemp(path)
    char *path;
{
    int fd;

    return (_gettemp(path, &fd) ? fd : -1);
}

char *
mktemp(path)
    char *path;
{
    return(_gettemp(path, (int *)NULL) ? path : (char *)NULL);
}

static
int
_gettemp(path, doopen)
    char *path;
    register int *doopen;
{
    extern int errno;
    register char *start, *trv;
    struct stat sbuf;

    PID_T pid;

    pid = getpid();
    for (trv = path; *trv; ++trv);      /* extra X's get set to 0's */
    while (*--trv == 'X') {
        *trv = (pid % 10) + '0';
        pid /= 10;
    }

    /*
     * check the target directory; if you have six X's and it
     * doesn't exist this runs for a *very* long time.
     */
    for (start = trv + 1;; --trv) {
        if (trv <= path)
            break;
        if (*trv == '/') {
            *trv = '\0';
            if (stat(path, &sbuf))
                return(0);
            if (!S_ISDIR(sbuf.st_mode)) {
                errno = ENOTDIR;
                return(0);
            }
            *trv = '/';
            break;
        }
    }

    for (;;) {
        if (doopen) {
            if ((*doopen =
                open(path, O_CREAT|O_EXCL|O_RDWR, 0600)) >= 0)
                return(1);
            if (errno != EEXIST)
                return(0);
        }
        else if (stat(path, &sbuf))
            return(errno == ENOENT ? 1 : 0);

        /* tricky little algorithm for backward compatibility */
        for (trv = start;;) {
            if (!*trv)
                return(0);
            if (*trv == 'z')
                *trv++ = 'a';
            else {
                if ( isdigit ( (int) *trv ) )
                    *trv = 'a';
                else
                    ++*trv;
                break;
            }
        }
    }
    /*NOTREACHED*/
    return 0;
}

