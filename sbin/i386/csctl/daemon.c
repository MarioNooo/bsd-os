/*
 * WILDBOAR $Wildboar: daemon.c,v 1.2 1996/03/17 08:35:14 shigeya Exp $
 */
/*
 *  Portions or all of this file are Copyright(c) 1994,1995,1996
 *  Yoichi Shinoda, Yoshitaka Tokugawa, WIDE Project, Wildboar Project
 *  and Foretune.  All rights reserved.
 *
 *  This code has been contributed to Berkeley Software Design, Inc.
 *  by the Wildboar Project and its contributors.
 *
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE WILDBOAR PROJECT AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 *  WILDBOAR PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 *  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "csctl.h"

#include <i386/pcmcia/cs_ioctl.h>

struct slotinfo {
	int		si_alive;
	int		si_flags;
	cs_cfg_t	si_cfg;
	struct timeval	si_lattach;
};

static void sighandler(int s);
static void cs_init(void);
static int getslotinfo(int slot, struct slotinfo *si);
static void cs_checkstatus(void);

static int cs_fd;
static int nfds;
static fd_set readfds;
static int nslot;
static struct slotinfo *slotinfo;

static volatile int got_sighup;

void
start_daemon(void)
{
	fd_set fds;

	daemon(0, 0);
	cs_init();
#if 0
	apm_init();
#endif
	signal(SIGHUP, sighandler);

	for (;;) {
		if (got_sighup) {
			got_sighup = 0;
			reset_config();
		}
		fds = readfds;
		if (select(nfds, &fds, NULL, NULL, NULL) < 0) {
			if (errno == EINTR)
				continue;
			perror("select");
			exit(1);
		}
		if (got_sighup)
			continue;	/* paranoia */
		if (FD_ISSET(cs_fd, &fds))
			cs_checkstatus();
#if 0
		if (FD_ISSET(apm_fd, &fds))
			apm_checkstatus();
#endif
	}
}

static void
sighandler(int sig)
{
	if (sig == SIGHUP)
		got_sighup = 1;
}

static void
cs_init()
{
	int i;

	if ((cs_fd = CS_Open()) < 0) {
		perror("CS_Open");
		exit(1);
	}
	FD_SET(cs_fd, &readfds);
	if (cs_fd >= nfds)
		nfds = cs_fd + 1;
	if ((nslot = CS_GetMaxSlot(cs_fd)) < 0) {
		perror("CS_GetMaxSlot");
		exit(1);
	}
	slotinfo = calloc(sizeof(struct slotinfo), nslot);
	if (slotinfo == NULL) {
		fprintf(stderr, "no more memory\n");
		exit(1);
	}
	for (i = 0; i < nslot; i++)
		(void)getslotinfo(i, &slotinfo[i]);
}

static int
getslotinfo(int slot, struct slotinfo *si)
{
	int flags;

	memset(si, 0, sizeof(*si));
	flags = CS_GetSlotFlags(cs_fd, slot);
	si->si_flags = flags;
	if (flags >= 0) {
		si->si_alive = (flags & (CSF_SLOT_UP|CSF_SLOT_EMPTY)) ==
			       CSF_SLOT_UP;
		(void)CS_GetClientInfo(cs_fd, slot, &si->si_cfg);
	}
	(void)CS_GetLastAttach(cs_fd, slot, &si->si_lattach);
	if (flags < 0)
		return -1;
	return 0;
}

static void
cs_checkstatus(void)
{
	int i;
	struct slotinfo *si, cur;

	for (i = 0, si = slotinfo; i < nslot; i++, si++) {
		if (getslotinfo(i, &cur) < 0)
				continue;
		if (!si->si_alive && cur.si_alive) {
			insert_event(i, &cur.si_cfg);
		} else if (si->si_alive && !cur.si_alive) {
			remove_event(i, &si->si_cfg);
		} else if (cur.si_alive && si->si_alive) {
			if (cur.si_lattach.tv_sec != si->si_lattach.tv_sec ||
			    cur.si_lattach.tv_usec != si->si_lattach.tv_usec) {
				/* changed or removed&inserted */
				remove_event(i, &si->si_cfg);
				insert_event(i, &cur.si_cfg);
			}
		}
		*si = cur;
	}
}
