/*
 * WILDBOAR $Wildboar: csctl.c,v 1.2 1996/03/17 08:35:14 shigeya Exp $
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
#include <string.h>

#include "csctl.h"

char *cmd;

static void show_config();

static int aflag, cflag, dflag, lflag, rflag;
static csspec_t *css_file, *css_kern;
static char *conffile = _PATH_CARDCONF;
int qflag;

/*
 * csctl
 *	-a	add
 *	-c	check
 *	-d	daemon
 *	-l	load
 *	-r	reset
 *	-q	be quiet on device not configured errors
 *	(none)	show
 */

void
main(int argc, char **argv)
{
	int c;

	if ((cmd = strrchr(argv[0], '/')) != NULL)
		cmd++;
	else
		cmd = argv[0];

	while ((c = getopt(argc, argv, "acdlqr")) != EOF) {
		switch (c) {
		case 'a':	aflag++;	break;
		case 'c':	cflag++;	break;
		case 'd':	dflag++;	break;
		case 'l':	lflag++;	break;
		case 'r':	rflag++;	break;
		case 'q':	qflag++;	break;
		default:
  usage:
			fprintf(stderr, "Usage: %s [-acdlr] [conffile]\n", cmd);
			exit(1);
		}
	}
	if (argc > optind) {
		if (argc == optind + 1)
			conffile = argv[optind];
		else
			goto usage;
	}
	load_config(conffile, &css_file);
	if (cflag) {
		show_config(css_file);
		exit(0);
	}
	if (rflag || lflag)
		kern_reset();
	if (aflag || lflag)
		kern_add(css_file);
	if (optind <= 1) {
		kern_config(&css_kern);
		show_config(css_kern);
	}
	if (dflag && have_event()) {
		start_daemon();
		/*NOTREACHED*/
	}
	exit(0);
}

void
reset_config(void)
{
	load_config(conffile, &css_file);
	kern_reset();
	kern_add(css_file);
}

static void
show_config(csspec_t *head)
{
	csspec_t *cp;
	int i, j;

	for (cp = head; cp; cp = cp->css_next) {
		printf("%s\t", cp->css_dname);
		show_quote(cp->css_str, cp->css_len);
		printf("\t");
		for (i = 0; i < CSS_MAXPARM; i++) {
			for (j = i; j < CSS_MAXPARM; j++) {
				if (cp->css_parm[j])
					break;
			}
			if (j == CSS_MAXPARM)
				break;
			printf("0x%x ", cp->css_parm[i]);
		}
		printf("\n");
		show_event(cp->css_dname, cp->css_str, cp->css_len);
	}
}
