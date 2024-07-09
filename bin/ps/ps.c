/*	BSDI	ps.c,v 2.8 2003/08/20 23:17:06 polk Exp	*/

/*-
 * Copyright (c) 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1990, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)ps.c	8.4 (Berkeley) 4/2/94";
#endif /* not lint */

/*
 * We need the normally kernel-only struct tty definition (which requires
 * the normally kernel-only struct selinfo definition.
 */
#define _NEED_STRUCT_SELINFO
#define _NEED_STRUCT_TTY

#include <sys/param.h>
#include <sys/user.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/tty.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

#include "ps.h"

KINFO *kinfo;
struct varent *vhead, *vtail;

int	eval;			/* exit value */
int	rawcpu;			/* -C */
int	sumrusage;		/* -S */
int	termwidth;		/* width of screen (0 == infinity) */
int	totwidth;		/* calculated width of requested variables */

static int needuser, needcomm, needenv;

enum sort { DEFAULT, SORTMEM, SORTCPU } sortby = DEFAULT;

static char	*fmt __P((char **(*)(kvm_t *, const struct kinfo_proc *, int),
		    KINFO *, char *, int));
static char	*kludge_oldps_options __P((char *));
static int	 pscomp __P((const void *, const void *));
static void	 saveuser __P((KINFO *));
static void	 scanvars __P((void));
static void	 show __P((int, int, int, int));
static void	 usage __P((void));

char dfmt[] = "pid tt state time command";
char jfmt[] = "user pid ppid pgid sess jobc state tt time command";
char lfmt[] = "uid pid ppid cpu pri nice vsz rss wchan state tt time command";
char   o1[] = "pid";
char   o2[] = "tt state time command";
char ufmt[] = "user pid %cpu %mem vsz rss tt state start time command";
char vfmt[] = "pid state time sl re pagein vsz rss lim tsiz %cpu %mem command";

kvm_t *kd;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct winsize ws;
	struct passwd *p;
	dev_t ttydev;
	pid_t *pid;
	uid_t uid, euid;
	size_t npid;
	int all, ch, flag, fmt;
	int prtheader, wflag, xflg;
	char *end,  *nlistf, *memf, *swapf, errbuf[_POSIX2_LINE_MAX];

	if ((ioctl(STDOUT_FILENO, TIOCGWINSZ, (char *)&ws) == -1 &&
	     ioctl(STDERR_FILENO, TIOCGWINSZ, (char *)&ws) == -1 &&
	     ioctl(STDIN_FILENO,  TIOCGWINSZ, (char *)&ws) == -1) ||
	     ws.ws_col == 0)
		termwidth = 79;
	else
		termwidth = ws.ws_col - 1;

	if (argc > 1)
		argv[1] = kludge_oldps_options(argv[1]);

	npid = 0;
	all = fmt = prtheader = wflag = xflg = 0;
	uid = euid = (uid_t)-1;
	ttydev = NODEV;
	pid = NULL;
	memf = nlistf = swapf = NULL;
	while ((ch = getopt(argc, argv,
	    "aCE:eghjLlM:mN:O:o:p:rSTt:U:uvW:wx")) != EOF)
		switch (ch) {
		case 'a':
			all = 1;
			break;
		case 'E':
			if (uid != (uid_t)-1)
				errx(1, "-%c may not be used with -U", ch);
			if ((p = getpwnam(optarg)) != NULL)
				euid = p->pw_uid;
			else {
				errno = 0;
				euid = strtoul(optarg, &end, 10);
				if (euid == ULONG_MAX && errno == ERANGE)
					err(1, "%s", optarg);
				if (!isdigit(*optarg) || *end != 0)
					errx(1, "%s: not a valid userid",
					    optarg);
			}
			xflg = 1;
			break;
		case 'e':			/* XXX set ufmt */
			needenv = 1;
			break;
		case 'C':
			rawcpu = 1;
			break;
		case 'g':
			break;			/* no-op */
		case 'h':
			prtheader = ws.ws_row > 5 ? ws.ws_row : 22;
			break;
		case 'j':
			parsefmt(jfmt);
			fmt = 1;
			jfmt[0] = '\0';
			break;
		case 'L':
			showkey();
			exit(0);
		case 'l':
			parsefmt(lfmt);
			fmt = 1;
			lfmt[0] = '\0';
			break;
		case 'M':
			memf = optarg;
			break;
		case 'm':
			sortby = SORTMEM;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 'O':
			parsefmt(o1);
			parsefmt(optarg);
			parsefmt(o2);
			o1[0] = o2[0] = '\0';
			fmt = 1;
			break;
		case 'o':
			parsefmt(optarg);
			fmt = 1;
			break;
		case 'p':
			if (pid == NULL) {
				if ((pid =
				    malloc(argc * sizeof(pid_t))) == NULL)
					err(1, NULL);
				npid = 0;
			}
			pid[npid++] = atol(optarg);
			xflg = 1;
			break;
		case 'r':
			sortby = SORTCPU;
			break;
		case 'S':
			sumrusage = 1;
			break;
		case 'T':
			if ((optarg = ttyname(STDIN_FILENO)) == NULL)
				errx(1, "stdin: not a terminal");
			/* FALLTHROUGH */
		case 't': {
			struct stat sb;
			char *ttypath, pathbuf[MAXPATHLEN];

			if (strcmp(optarg, "co") == 0 ||
			    strcmp(optarg, "con") == 0)
				ttypath = _PATH_CONSOLE;
			else if (*optarg != '/')
				(void)snprintf(ttypath = pathbuf,
				    sizeof(pathbuf), "%s%s", _PATH_TTY, optarg);
			else
				ttypath = optarg;
			if (stat(ttypath, &sb) == -1)
				err(1, "%s", ttypath);
			if (!S_ISCHR(sb.st_mode))
				errx(1, "%s: not a terminal", ttypath);
			ttydev = sb.st_rdev;
			break;
		}
		case 'U':
			if (euid != (uid_t)-1)
				errx(1, "-%c may not be used with -E", ch);
			if ((p = getpwnam(optarg)) != NULL)
				uid = p->pw_uid;
			else {
				errno = 0;
				uid = strtoul(optarg, &end, 10);
				if (uid == ULONG_MAX && errno == ERANGE)
					err(1, "%s", optarg);
				if (!isdigit(*optarg) || *end != 0)
					errx(1, "%s: not a valid userid",
					    optarg);
			}
			xflg = 1;
			break;
		case 'u':
			parsefmt(ufmt);
			sortby = SORTCPU;
			fmt = 1;
			ufmt[0] = '\0';
			break;
		case 'v':
			parsefmt(vfmt);
			sortby = SORTMEM;
			fmt = 1;
			vfmt[0] = '\0';
			break;
		case 'W':
			swapf = optarg;
			break;
		case 'w':
			if (wflag)
				termwidth = UNLIMITED;
			else if (termwidth < 131)
				termwidth = 131;
			wflag++;
			break;
		case 'x':
			xflg = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

#define	BACKWARD_COMPATIBILITY
#ifdef	BACKWARD_COMPATIBILITY
	if (*argv) {
		nlistf = *argv;
		if (*++argv) {
			memf = *argv;
			if (*++argv)
				swapf = *argv;
		}
	}
#endif
	/*
	 * Discard setgid privileges if not the running kernel so that bad
	 * guys can't print interesting stuff from kernel memory.
	 */
	if (nlistf != NULL || memf != NULL || swapf != NULL)
		setgid(getgid());

	kd = kvm_openfiles(nlistf, memf, swapf, O_RDONLY, errbuf);
	if (kd == 0)
		errx(1, "%s", errbuf);

	if (!fmt)
		parsefmt(dfmt);

	if (euid == (uid_t)-1 && uid == (uid_t)-1 &&
	    !all && ttydev == NODEV && pid == NULL)
		euid = getuid();

	/*
	 * scan requested variables, noting what structures are needed,
	 * and adjusting header widths as appropiate.
	 */
	scanvars();

	/*
	 * print header
	 */
	printheader();

	/*
	 * get proc list
	 */
	if (euid != (uid_t) -1)
		show(KERN_PROC_UID, euid, xflg, prtheader);
	else if (uid != (uid_t) -1)
		show(KERN_PROC_RUID, uid, xflg, prtheader);
	else if (ttydev != NODEV) {
#if defined(hp300) || defined(i386)
		dev_t c2rdev(dev_t);

		flag = c2rdev(ttydev);
#else
		flag = ttydev;
#endif
		show(KERN_PROC_TTY, flag, xflg, prtheader);
	} else if (pid != NULL)
		while (npid--)
			show(KERN_PROC_PID, *pid++, xflg, prtheader);
	else
		show(KERN_PROC_ALL, 0, xflg, prtheader);
	exit (eval);
}

/*
 * show --
 *	Display a set of processes.
 */
static void
show(what, flag, xflg, prtheader)
	int what, flag, xflg, prtheader;
{
	static int lineno;
	struct kinfo_proc *kp;
	struct varent *vent;
	int i, nentries;

	/*
	 * select procs
	 */
	if ((kp = kvm_getprocs(kd, what, flag, &nentries)) == 0)
		errx(1, "%s", kvm_geterr(kd));
	if (nentries == 0)
		return;
	if ((kinfo = malloc(nentries * sizeof(*kinfo))) == NULL)
		err(1, NULL);
	for (i = nentries; --i >= 0; ++kp) {
		kinfo[i].ki_p = kp;
		if (needuser)
			saveuser(&kinfo[i]);
	}

	/*
	 * sort proc list
	 */
	qsort(kinfo, nentries, sizeof(KINFO), pscomp);

	/*
	 * for each proc, call each variable output function.
	 */
	for (i = 0; i < nentries; i++) {
		if (xflg == 0 && (KI_EPROC(&kinfo[i])->e_tdev == NODEV ||
		    (KI_PROC(&kinfo[i])->p_flag & P_CONTROLT ) == 0))
			continue;
		for (vent = vhead; vent; vent = vent->next) {
			(vent->var->oproc)(&kinfo[i], vent);
			if (vent->next != NULL)
				(void)putchar(' ');
		}
		(void)putchar('\n');
		if (prtheader && lineno++ == prtheader - 4) {
			(void)putchar('\n');
			printheader();
			lineno = 0;
		}
	}
	free(kinfo);
}

#if defined(hp300) || defined(i386)
/*
 * XXX  If this is the console device, attempt to ascertain
 * the true console device dev_t.
 */

struct nlist c2rnl[] = {
        { "_cn_tty" },
#define X_CNTTY         0
        { "" },
};

dev_t
c2rdev(dev_t tdev)
{
	static dev_t cn_dev;
	struct tty cn_tty, *cn_ttyp;

	if (tdev != 0)
		return (tdev);

	if (cn_dev)
		return (cn_dev);
	if (kvm_nlist(kd, c2rnl))
		return (0);
	if (kvm_read(kd, (u_long)c2rnl[X_CNTTY].n_value,
	    (char *)&cn_ttyp, sizeof(cn_ttyp)) > 0) {
		(void)kvm_read(kd, (u_long)cn_ttyp,
		    (char *)&cn_tty, sizeof (cn_tty));
		cn_dev = cn_tty.t_dev;
	}
	return (cn_dev);
}
#endif

static void
scanvars()
{
	struct varent *vent;
	VAR *v;
	int i;

	for (vent = vhead; vent; vent = vent->next) {
		v = vent->var;
		i = strlen(v->header);
		if (v->width < i)
			v->width = i;
		totwidth += v->width + 1;	/* +1 for space */
		if (v->flag & USER)
			needuser = 1;
		if (v->flag & COMM)
			needcomm = 1;
	}
	totwidth--;
}

static char *
fmt(fn, ki, comm, maxlen)
	char **(*fn) __P((kvm_t *, const struct kinfo_proc *, int));
	KINFO *ki;
	char *comm;
	int maxlen;
{
	char *s;

	if ((s =
	    fmt_argv((*fn)(kd, ki->ki_p, termwidth), comm, maxlen)) == NULL)
		err(1, NULL);
	return (s);
}

static void
saveuser(ki)
	KINFO *ki;
{
	struct pstats pstats;
	struct usave *usp;

	usp = &ki->ki_u;
	if (kvm_read(kd, (u_long)&KI_PROC(ki)->p_addr->u_stats,
	    (char *)&pstats, sizeof(pstats)) == sizeof(pstats)) {
		/*
		 * The u-area might be swapped out, and we can't get
		 * at it because we have a crashdump and no swap.
		 * If it's here fill in these fields, otherwise, just
		 * leave them 0.
		 */
		usp->u_start = pstats.p_start;
		usp->u_ru = pstats.p_ru;
		usp->u_cru = pstats.p_cru;
		usp->u_valid = 1;
	} else
		usp->u_valid = 0;
	/*
	 * save arguments if needed
	 */
	if (needcomm)
		ki->ki_args = fmt(kvm_getargv, ki, KI_PROC(ki)->p_comm,
		    MAXCOMLEN);
	else
		ki->ki_args = NULL;
	if (needenv)
		ki->ki_env = fmt(kvm_getenvv, ki, (char *)NULL, 0);
	else
		ki->ki_env = NULL;
}

static int
pscomp(a, b)
	const void *a, *b;
{
	int i;
#define VSIZE(k) (KI_EPROC(k)->e_vm.vm_dsize + KI_EPROC(k)->e_vm.vm_ssize + \
		  KI_EPROC(k)->e_vm.vm_tsize)

	if (sortby == SORTCPU)
		return (getpcpu((KINFO *)b) - getpcpu((KINFO *)a));
	if (sortby == SORTMEM)
		return (VSIZE((KINFO *)b) - VSIZE((KINFO *)a));
	i =  KI_EPROC((KINFO *)a)->e_tdev - KI_EPROC((KINFO *)b)->e_tdev;
	if (i == 0)
		i = KI_PROC((KINFO *)a)->p_pid - KI_PROC((KINFO *)b)->p_pid;
	return (i);
}

/*
 * ICK (all for getopt), would rather hide the ugliness
 * here than taint the main code.
 *
 *  ps foo -> ps -foo
 *  ps 34 -> ps -p34
 *
 * The old convention that 't' with no trailing tty arg means the users
 * tty, is only supported if argv[1] doesn't begin with a '-'.  This same
 * feature is available with the option 'T', which takes no argument.
 */
static char *
kludge_oldps_options(s)
	char *s;
{
	size_t len;
	char *newopts, *ns, *cp;

	len = strlen(s);

	/* Three extra slots: initial '-', 'p' option char, trailing nul. */
	if ((newopts = ns = malloc(len + 3)) == NULL)
		err(1, NULL);
	/*
	 * options begin with '-'
	 */
	if (*s != '-')
		*ns++ = '-';	/* add option flag */
	/*
	 * gaze to end of argv[1]
	 */
	cp = s + len - 1;
	/*
	 * if last letter is a 't' flag with no argument (in the context
	 * of the oldps options -- option string NOT starting with a '-' --
	 * then convert to 'T' (meaning *this* terminal, i.e. ttyname(0)).
	 */
	if (*cp == 't' && *s != '-')
		*cp = 'T';
	else {
		/*
		 * otherwise check for trailing number, which *may* be a
		 * pid.
		 */
		while (cp >= s && isdigit(*cp))
			--cp;
	}
	cp++;
	memmove(ns, s, (size_t)(cp - s));	/* copy up to trailing number */
	ns += cp - s;
	/*
	 * If there's a trailing number, and not a preceding E (euid), U (uid),
	 * p (pid) or (possibly trailing) t (tty) option, assume it's
	 * a pid and insert a p option.
	 */
	if (isdigit(*cp) &&
	   (cp == s ||
	   cp[-1] != 'E' && cp[-1] != 'U' && cp[-1] != 'p' && cp[-1] != 't' &&
	   (cp - 1 == s || cp[-2] != 't')))
		*ns++ = 'p';
	(void)strcpy(ns, cp);		/* and append the number */

	return (newopts);
}

static void
usage()
{

	(void)fprintf(stderr,
	    "usage:\t%s\n\t   %s\n\t%s\n",
	    "ps [-aChjlmrSTuvwx] [-E user] [-M core] [-N system] [-O fmt] [-o fmt]",
	    "[-p pid] [-t tty] [-U user ] [-W swap]",
	    "ps [-L]");
	exit(1);
}