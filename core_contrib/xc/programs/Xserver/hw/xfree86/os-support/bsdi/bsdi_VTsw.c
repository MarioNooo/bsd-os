/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/bsd/bsd_VTsw.c,v 3.5 1996/12/23 06:49:35 dawes Exp $ */
/*
 * Derived from bsd_VTsw.c which was derived from VTsw_usl.c which is
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 * by S_ren Schmidt (sos@login.dkuug.dk)
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Wexelblat not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  David Wexelblat makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL DAVID WEXELBLAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: bsd_VTsw.c /main/4 1996/02/21 17:50:57 kaleb $ */

#include "X.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

/*
 * This function is the signal handler for the VT-switching signal.  It
 * is only referenced inside the OS-support layer.
 */
void 
xf86VTRequest(int sig)
{
	xf86Info.vtRequestsPending = TRUE;
	return;
}

Bool 
xf86VTSwitchPending()
{
	return(xf86Info.vtRequestsPending ? TRUE : FALSE);
}

Bool 
xf86VTSwitchAway()
{
	int n;
	xf86Info.vtRequestsPending = FALSE;

	if (ioctl(xf86Info.consoleFd, PCCONIOCCOOK, 0) < 0)
	{
	    FatalError("%s: PCCONIOCCOOK failed (%s)\n", 
		       "xf86VTSwitchAway", strerror(errno));
	}
	close(xf86Info.screenFd);	/* close /dev/vga */
	n = 0;
	if (ioctl(xf86Info.consoleFd, PCCONIOCSWNOW, &n) < 0) {
		if (ioctl(xf86Info.consoleFd, PCCONIOCRAW, 0) < 0)
		{
	    	    FatalError("%s: PCCONIOCRAW failed (%s)\n", 
		       	"xf86VTSwitchAway", strerror(errno));
		}
		if ((xf86Info.screenFd = 
			open("/dev/vga", O_RDWR|O_NDELAY,0)) < 0)
		{
	    	    FatalError("%s: Cannot re-open /dev/vga (%s)\n", 
		       	"xf86VTSwitchAway", strerror(errno));
		}
		return(FALSE);
	}
	close(xf86Info.consoleFd);	/* close /dev/kbd */
	return(TRUE);
}

Bool 
xf86VTSwitchTo()
{
	int n;
	xf86Info.vtRequestsPending = FALSE;

	if ((xf86Info.consoleFd = open("/dev/kbd", O_RDWR|O_NDELAY,0)) < 0)
	{
	    FatalError("%s: Cannot re-open /dev/kbd (%s)\n", 
		"xf86VTSwitchTo", strerror(errno));
	}
	n = 1;
	if (ioctl(xf86Info.consoleFd, PCCONIOCSWNOW, &n) < 0) {
		close(xf86Info.consoleFd);
		return(FALSE);
	}
	if (ioctl(xf86Info.consoleFd, PCCONIOCRAW, 0) < 0)
	{
	    FatalError("%s: PCCONIOCRAW failed (%s)\n", 
		"xf86VTSwitchTo", strerror(errno));
	}
	if ((xf86Info.screenFd = open("/dev/vga", O_RDWR|O_NDELAY,0)) < 0)
	{
	    FatalError("%s: Cannot re-open /dev/vga (%s)\n", 
		"xf86VTSwitchTo", strerror(errno));
	}
	return(TRUE);
}
