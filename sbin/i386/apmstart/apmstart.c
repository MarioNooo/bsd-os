/*
 * WILDBOAR $Wildboar: apmstart.c,v 1.4 1996/02/13 13:01:00 shigeya Exp $
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
#include <errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <machine/apm.h>
#include <machine/apmioctl.h>

main(argc, argv)
int	argc;
char	**argv;
{
	int fd;
	int status;
	int dummy = 0;
	int qflag = 0;

	if (argc > 1 && strcmp(argv[1], "-q") == 0) {
		qflag = 1;
		++argv;
		--argc;
	}
	if ((fd = open(_PATH_DEVAPM, O_RDWR)) < 0) {
		if (qflag == 0 || errno != ENXIO)
			perror(_PATH_DEVAPM);
		exit(1);
	}
	if (argc == 2)
		dummy = atoi(argv[1]);
	if ((status = ioctl(fd, PIOCAPMSTART, &dummy)) != 0) {
		perror("PIOCAPMSTART");
		exit(1);
	}
	printf("APM enabled.\n");
	close(fd);
	exit(0);
}
