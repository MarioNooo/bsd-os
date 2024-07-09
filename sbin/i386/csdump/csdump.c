/*
 * WILDBOAR $Wildboar: csdump.c,v 1.3 1996/02/13 13:01:06 shigeya Exp $
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
#include <ctype.h>
#include <i386/pcmcia/cs_ioctl.h>

main(argc, argv)
int	argc;
char	**argv;
{
	int fd, sock;
	u_char buf[CIS_MAXSIZE];
	u_char *p;
	register int i;

	if (argc != 2 || !isdigit(*argv[1])) {
		fprintf(stderr, "usage: %s slot\n", argv[0]);
		exit(1);
	}
	sock = atoi(argv[1]);

	if ((fd = CS_Open()) < 0) {
		perror("CS_Open");
		exit(1);
	}

	if (CS_GetCIS(fd, sock, buf) < 0) {
		if (errno == EAGAIN)
			fprintf(stderr, "slot %d is empty\n", sock);
		else if (errno == EBUSY)
			fprintf(stderr, "slot %d is busy\n", sock);
		else
			perror("CS_GetCIS");
		exit(1);
	}
	CS_Close(fd);

	printf("socket%d dumping CIS\n", sock);
	for (i = 0, p = buf; i < CIS_MAXSIZE; i++, p++) {
		if (i && (i % 16) == 0)
			printf("\n") ;
		printf("%02x ", *p) ;
	}
	printf("\n");
	exit(0);
}
