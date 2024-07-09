/*
 * WILDBOAR $Wildboar: apmbatt.c,v 1.4 1996/02/13 13:01:10 shigeya Exp $
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
/*
 * apmc cmd
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>

#include <machine/apmioctl.h>
#include <machine/apm.h>

main(ac, av)
	int ac ;
	char *av[] ;
{
	int fd ;
	struct apmreq ar ;

	if ((fd = open(_PATH_DEVAPM, O_RDONLY)) < 0) {
		perror(_PATH_DEVAPM) ;
		exit(1) ;
	}

	ar.func = APM_GET_POWER_STATUS ;
	ar.dev = APM_DEV_ALL ;

	if (ioctl(fd, PIOCAPMREQ, &ar) < 0) {
		printf("PIOCAPMREQ: APM_GET_POWER_STATUS error 0x%x\n", ar.err);
		exit(1);
	}

	{
		int astat ;
		int bstat ;
		int blife ;
		static char *astr[] = { "off-line", "on-line" } ;
		static char *bstr[] = { "high", "low", "critical", "charging"};

		astat = (ar.bret >> 8) & 0xff ;
		bstat = ar.bret & 0xff ;
		blife = ar.cret & 0xff ;

		printf("AC ") ;
		if (astat > 1)
			printf("status unknown, ") ;
		else
			printf("%s, ", astr[astat]) ;

		printf("Battery ") ;
		if (bstat > 3)
			printf("status unknown ") ;
		else if (bstat == 3)
			printf("charging ") ;
		else
			printf("level %s ", bstr[bstat]) ;

		if (blife > 100)
			printf(", charge level unknown (%d)\n", blife) ;
		else
			printf("(%d%%)\n", blife) ;
	}

	exit(0);
}
