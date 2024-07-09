/*	BSDI sort.c,v 2.4 1997/02/07 19:53:01 bostic Exp	*/

/* Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 * 
 * Derived from software contributed to Berkeley by Peter McIlroy.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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

#include <sys/param.h>

#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sort.h"
#include "fsort.h"
#include "pathnames.h"

u_char REC_D = '\n';
u_char d_mask[NBINS];		/* flags for rec_d, field_d, <blank> */

/*
 * weight tables.  Gweights is one of ascii, Rascii..
 * modified to weight rec_d = 0 (or 255) 
 */
extern u_char gweights[NBINS];
u_char ascii[NBINS], Rascii[NBINS], RFtable[NBINS], Ftable[NBINS];

/*
 * masks of ignored characters.  Alltable is 256 ones 
 */
u_char dtable[NBINS], itable[NBINS], alltable[NBINS];
int SINGL_FLD = 0, SEP_FLAG = 0, UNIQUE = 0;
struct coldesc clist[(ND+1)*2];
int ncols = 0;
extern struct coldesc clist[(ND+1)*2];
extern int ncols;

char devstdin[] = _PATH_STDIN;
char toutpath[MAXPATHLEN];

static void clean_exit __P((void));
static void clean_sig __P((int));
static void usage __P((char *));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern int optind;
	extern char *optarg;
	struct field fldtab[ND+2], *ftpos, *p;
	union f_handle filelist;
	enum { NONE, OLD_K_ARG, NEW_K_ARG } argstate;
	FILE *outfd;
	int ch, cflag, i, mflag, stdinflag;
	int (*get)();
	char *outfile, *outpath;

	bzero(fldtab, (ND+2) * sizeof(struct field));
	bzero(d_mask, NBINS);
	d_mask[REC_D = '\n'] = REC_D_F;
	SINGL_FLD = SEP_FLAG = 0;
	d_mask['\t'] = d_mask[' '] = BLANK | FLD_D;
	fixit(&argc, argv);

	argstate = NONE;
	outpath = NULL;
	cflag = mflag = 0;
	ftpos = fldtab;
	while ((ch = getopt(argc, argv, "\01:bcdfik:mHno:rR:t:T:ux")) != EOF)
		switch (ch) {
		case 'b': 
			if (argstate == OLD_K_ARG)
				ftpos->flags |= BI | BT;
			else {
				if (argstate == NEW_K_ARG)
warnx("-b option after -k option: treated as global option");
				fldtab->flags |= BI | BT;
			}
			break;
		case 'd':
		case 'f':
		case 'i':
		case 'n':
		case 'r': 
			if (argstate == OLD_K_ARG)
				ftpos->flags |= optval(ch);
			else {
				if (argstate == NEW_K_ARG)
warnx("-%c option after -k option: treated as global option", ch);
				fldtab->flags |= optval(ch);
			}
			break;
		case 'o':
			outpath = optarg;
			break;
		case '\01':			/* +pos1, -pos2 */
			argstate = OLD_K_ARG;
			(++ftpos)->optarg = optarg;
			break;
		case 'k':			/* save field name. */
			argstate = NEW_K_ARG;
			(++ftpos)->optarg = optarg;
			break;
		case 't':
			if (SEP_FLAG)
				usage("multiple field delimiters");
			SEP_FLAG = 1;
			d_mask[' '] &= ~FLD_D;
			d_mask['\t'] &= ~FLD_D;
			d_mask[*optarg] |= FLD_D;
			if (d_mask[*optarg] & REC_D_F)
				errx(2, "record/field delimiter clash.");
			break;
		case 'R':
			if (REC_D != '\n')
				usage("multiple record delimiters");
			if ('\n' == (REC_D = *optarg))
				break;
			d_mask['\n'] = d_mask[' '];
			d_mask[REC_D] = REC_D_F;
			break;
		case 'T':
			tmpdir = optarg;
			break;
		case 'u':
			UNIQUE = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		case 'm':
			mflag = 1;
			break;
		case 'H':
			PANIC = 0;
			break;
		case '?':
		default:
			usage(NULL);
		}

	if (cflag && argc > optind + 1)
		errx(2, "too many input files for -c option");
	if (argc - 2 > optind && !strcmp(argv[argc-2], "-o")) {
		outpath = argv[argc-1];
		argc -= 2;
	}
	if (mflag && argc - optind > MAXFCT - (16 + 1))
		errx(2, "too many input files for -m option");

	stdinflag = 0;
	for (i = optind; i < argc; i++)
		/*
		 * Allow exactly one occurrence of /dev/stdin.
		 *
		 * XXX
		 * This is probably wrong.
		 */
		if (!strcmp(argv[i], "-") || !strcmp(argv[i], devstdin)) {
			if (stdinflag)
				warnx("ignoring extra \"%s\" in file list",
				    argv[i]);
			else {
				stdinflag = 1;
				argv[i] = devstdin;
			}
		}
		else if (ch = access(argv[i], R_OK))
			err(2, "%s", argv[i]);

	/*
	 * All flags parsed; initialize fields and set max column.  According
	 * to POSIX 1003.2-1992, page 432, the -bdfinr options, when following
	 * +pos/-pos options, only apply to those options.  Otherwise, they're
	 * global in scope.  We've already printed out a warning message if
	 * they followed -k options (that's a POSIX no-no).
	 */
	if (ftpos > fldtab)
		for (p = fldtab + 1; p <= ftpos; ++p)
			setfield(p->optarg,
			    p, p->flags ? p->flags : fldtab->flags);

	if (fldtab->flags & F)
		if (fldtab->flags & R)
			fldtab->weights = RFtable;
		else
			fldtab->weights = Ftable;
	else if (fldtab->flags & R)
		fldtab->weights = Rascii;

	if (fldtab->flags & (I | D | N) || fldtab[1].icol.num) {
		if (!fldtab[1].icol.num) {
			if (fldtab->flags & (I | D))
				fldtab[0].flags &= ~(BI | BT);
			setfield("1", ++ftpos, fldtab->flags);
		}
		fldreset(fldtab);
		fldtab[0].flags &= ~F;
	} else {
		SINGL_FLD = 1;
		fldtab[0].icol.num = 1;
	}
	settables(fldtab[0].flags);
	num_init();
	fldtab->weights = gweights;

	if (optind == argc) 
		argv[--optind] = devstdin;
	filelist.names = argv+optind;

	if (SINGL_FLD) 
		get = makeline;
	else 
		get = makekey;

	if (cflag) {
		order(filelist, get, fldtab);
		/* NOT REACHED */
	}

	if (!outpath) 
		outfile = outpath = "/dev/stdout";
	else if (!(ch = access(outpath, 0))) {
		struct sigaction act = {0, SIG_BLOCK, 6};
		int sigtable[] = {
		    SIGHUP, SIGINT, SIGPIPE, SIGXCPU,
		    SIGXFSZ, SIGVTALRM, SIGPROF, 0
		};

		errno = 0;
		if (access(outpath, W_OK))
			err(2, "%s", outpath);
		act.sa_handler = clean_sig;
		outfile = toutpath;
		sprintf(outfile, "%sXXXX", outpath);
		outfile = mktemp(outfile);
		if (!outfile) 
			err(2, "mktmp failed: %s");

		/* Always unlink toutpath. */
		atexit(clean_exit);
		for (i = 0; sigtable[i]; ++i)
			(void)sigaction(sigtable[i], &act, 0);
	} else 
		outfile = outpath;

	if (!(outfd = fopen(outfile, "w"))) 
		err(2, "%s", outfile);

	if (mflag) 
		fmerge(-1, filelist, argc-optind, get, outfd, putline, fldtab);
	else 
		fsort(-1, 0, filelist, argc-optind, outfd, fldtab);

	if (outfile != outpath) {
		if (access(outfile, 0))
			err(2, "%s", outfile);
		(void)unlink(outpath);
		if (link(outfile, outpath))
			err(2, "cannot link %s; output left in %s",
			    outpath, outfile);
		(void)unlink(outfile);
	}
	exit(0);
}

static void
usage(msg)
	char *msg;
{
	if (msg != NULL)
		(void)fprintf(stderr, "sort: %s\n", msg);

	(void)fprintf(stderr,
	    "usage: sort [-bcdfimnru] [-k keydef] [-o output]\n");
	(void)fprintf(stderr,
	    "\t[-R char] [-T directory] [-t char] [files ...]\n");

	exit(2);
}

static void
clean_exit()
{
	if (toutpath[0])
		(void)unlink(toutpath);
}

static void 
clean_sig(signo)
	int signo;
{
	if (toutpath[0])
		(void)unlink(toutpath);
	exit(2);			/* return 2 on error/interrupt */
}
