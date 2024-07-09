/*
 * WILDBOAR $Wildboar: kernio.c,v 1.2 1996/03/17 08:35:15 shigeya Exp $
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


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>

#include "csctl.h"

#include <i386/pcmcia/cs_ioctl.h>

void
kern_config(csspec_t **headp)
{
	int fd;
	int n;
	csspecconf_t cssc;
	int ntry = 0;

	fd = open(_PATH_CS, O_RDONLY);
	if (fd < 0) {
		if (!qflag || errno != ENXIO)
			perror(_PATH_CS);
		exit(1);
	}
	cssc.cssc_buf = NULL;
	cssc.cssc_buflen = 0;
	if (ioctl(fd, CSIOCLISTSPEC, &cssc) < 0) {
		perror("CSIOCLISTSPEC");
		exit(1);
	}
  again:
	if (cssc.cssc_buflen == 0) {
		close(fd);
		return;
	}
	if ((cssc.cssc_buf = malloc(cssc.cssc_buflen)) == NULL) {
		fprintf(stderr, "no more memory\n");
		exit(1);
	}
	if (ioctl(fd, CSIOCLISTSPEC, &cssc) < 0) {
		if (errno == EINVAL && ntry++ < 3) {
			/* is someone add new entry? */
			n = cssc.cssc_buflen;
			free(cssc.cssc_buf);
			cssc.cssc_buf = NULL;
			cssc.cssc_buflen = 0;
			if (ioctl(fd, CSIOCLISTSPEC, &cssc) == 0 &&
			    cssc.cssc_buflen != n)
				goto again;
		}
		perror("CSIOCLISTSPEC");
		exit(1);
	}
	close(fd);
	*headp = cssc.cssc_buf;
}

void
kern_add(csspec_t *head)
{
	csspec_t *cp;
	int fd;

	fd = open(_PATH_CS, O_RDONLY);
	if (fd < 0) {
		if (!qflag || errno != ENXIO)
			perror(_PATH_CS);
		exit(1);
	}
	for (cp = head; cp; cp = cp->css_next) {
		if (ioctl(fd, CSIOCADDSPEC, cp) < 0) {
			perror(cp->css_dname);
		}
	}
	close(fd);
}

void
kern_reset(void)
{
	int fd;

	fd = open(_PATH_CS, O_RDONLY);
	if (fd < 0) {
		if (!qflag || errno != ENXIO)
			perror(_PATH_CS);
		exit(1);
	}
	if (ioctl(fd, CSIOCRESETSPEC) < 0) {
		perror("CSIOCRESETSPEC");
		exit(1);
	}
	close(fd);
}
