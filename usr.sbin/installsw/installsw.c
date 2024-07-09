/*-
 * Copyright (c) 1993, 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI installsw.c,v 2.23 1999/01/15 20:05:59 jch Exp
 */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <err.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "installsw.h"
#include "pathnames.h"

PKG    *spkg;				/* Script PKG file. */
loc_t	location;			/* Media location (local or remote). */
media_t media		= M_NONE;	/* Media type. */
sel_t	comsel		= S_NONE;	/* Command-line package selection. */
int	devset;				/* If device set. */
int	express;			/* If doing an express install. */
int	do_delete;			/* Delete mode. */
int	do_update;			/* Update mode. */
int	expert;				/* If the user is expert. */
int	tfileno;                	/* Current tape file number. */ 
char   *device		= CD_MOUNTPT;	/* CDROM/floppy mount, tape device. */
char   *rootdir		= DEF_ROOT;	/* Root directory. */
char   *remote_host;			/* Remote host. */
char   *remote_user;			/* Remote user. */

static void	chk_exist __P((PKGH *));
static void	d_mark __P((PKGH *));
static void	get_packages __P((PKGH *));
static void	get_scripts __P((PKGH *));
static void	s_mark __P((PKGH *));
static void	usage __P((void));

/*
 * installsw --
 *
 * Delete or install packages from a distribution tape, floppy or cd-rom.  The
 * user is prompted for necessary information, followed by a curses interface
 * to select packages.  Packages are deleted or installed when the selection is
 * complete.  Both local and remote devices are supported.
 */
int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
 	extern int optind;
	PKGH pkgh;
	int ch, rval;
	char *res;

	/* Avoid partial updates, or confusing error messages. */
	if (geteuid() != 0) {
		errno = EACCES;
		err(1, NULL);
	}

	while ((ch = getopt(argc, argv, "c:Dd:Eh:l:Lm:s:U")) != EOF)
		switch (ch) {
		case 'c':
			devset = 1;
			device = optarg;
			break;
		case 'D':
			do_delete = 1;
			break;
		case 'd':
			rootdir = optarg;
			break;
		case 'E':
			expert = 1;
			break;
		case 'h':
			remote_host = optarg;
			break;
		case 'l':
			remote_user = optarg;
			break;
		case 'L':
			location = L_LOCAL;
			break;
		case 'm':
			if (!strcmp(optarg, "cdrom"))
				media = M_CDROM;
			else if (!strcmp(optarg, "floppy"))
				media = M_FLOPPY;
			else if (!strcmp(optarg, "tape"))
				media = M_TAPE;
			else
				errx(1,
		"unknown media type: use cdrom, floppy or tape");
			break;
		case 's':
			if (optarg[1] != '\0' ||
			    strchr("doru", optarg[0]) == NULL)
			    errx(1,
		    "unknown selection type: use 'd', 'o' 'r' or 'u'");
			express = 1;
			switch (optarg[0]) {
			case 'd':
				comsel = S_DESIRABLE;
				break;
			case 'o':
				comsel = S_OPTIONAL;
				break;
			case 'r':
				comsel = S_REQUIRED;
				break;
			case 'u':
				comsel = S_UPDATE;
				break;
			default:
				abort();
			}
			break;
		case 'U':
			do_update = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 0)
		usage();

	/* -D and -U are incompatible. */
	if (do_delete && do_update)
		errx(1, "The -D and -U options cannot both be specified.");

	/*
	 * -l requires -h.
	 *
	 * XXX
	 * Well, actually, it really doesn't, but it's not worth doing it any
	 * other way, I doubt a user will specify -l and then expect to be
	 * prompted for the host name.
	 */
	if (remote_user != NULL && remote_host == NULL)
		errx(1, "The -l option requires the -h option");

	/* -h implies a remote location. */
	if (remote_host != NULL)
		location = L_REMOTE;

	/*
	 * -s requires -c and -m.
	 * -s assumes local install, if -h not specified.
	 */
	if (express) {
		if (!devset || media == M_NONE)
			errx(1, "The -s option requires the -c and -m options");
		if (remote_host == NULL)
			location = L_LOCAL;
	}

	/* Clean up temporary files and rewind tape on exit, interrupt. */
	clean_add(TMPMAP);
	clean_add(TMPPACK);
	clean_add(STATUS);
	clean_add(SCRIPT_DIR);
	(void)atexit(cur_end);
	(void)atexit(clean_up);
	(void)atexit(tape_rew);
	(void)signal(SIGINT, onintr);

	setup();			/* Get setup information. */

	get_packages(&pkgh);		/* Read the PACKAGES manifest. */

	chk_exist(&pkgh);		/* Check for package existence. */

	get_scripts(&pkgh);		/* Get pre/post/info/delete files. */

					/* Scripts use for their root. */
	(void)setenv("INSTROOT", rootdir, 1);

	if (do_delete)
		d_mark(&pkgh);		/* Pre-process for deletion. */
	else
		s_mark(&pkgh);		/* Automatic selection. */

					/* Interactively select, install. */
	res = (rval = cur_inter(&pkgh)) ? "were NOT" : "were";
	(void)printf("All packages %s %s successfully.\n",
	    res, do_delete ? "deleted" : "installed");
	exit (rval);
}

/*
 * get_packages --
 *	Fill in a list of package structures by reading a manifest file.
 */
static void
get_packages(pkghp)
	PKGH *pkghp;
{
	PKG *pkg;
	FILE *fp;
	int linenum, rval;
	char *ep, *p;
	char buf[1024], locpart[1024], pkgfile[MAXPATHLEN], rempart[1024];

	/*
	 * If reading from a tape or a remote machine, need a local copy
	 * of the packages file.
	 */
	if (media == M_TAPE || location == L_REMOTE)
		(void)strcpy(buf, TMPPACK);

	switch (media) {
	case M_CDROM:
	case M_FLOPPY:
		switch (location) {
		case L_REMOTE:
			(void)snprintf(pkgfile, sizeof(pkgfile),
			    "%s/%s", PACKAGEDIR, PACKAGEFILE);
			if (get_remote_file(pkgfile, buf))
				errx(1,
				    "%s: unable to copy manifest", pkgfile);
			if ((fp = fopen(buf, "r")) == NULL)
				err(1, "%s", buf);
			break;
		case L_LOCAL:
			(void)snprintf(pkgfile, sizeof(pkgfile),
			    "%s/%s/%s", device, PACKAGEDIR, PACKAGEFILE);
			if ((fp = fopen(pkgfile, "r")) == NULL)
				err(1, "%s", pkgfile);
			break;
		default:
			abort();
		}
		break;
	case M_TAPE:
		(void)snprintf(pkgfile, sizeof(pkgfile),
		    "%s/%s", PACKAGEDIR, PACKAGEFILE);
		switch (location) {
		case L_REMOTE:
			(void)snprintf(rempart, sizeof(rempart),
			    "%s if=%s bs=10k", _PATH_DD, device);
			(void)snprintf(locpart, sizeof(locpart), " > %s", buf);
			if (remote_local_command(rempart, locpart, 0))
				errx(1, "unable to copy manifest");
			++tfileno;
			break;
		case L_LOCAL:
			(void)snprintf(locpart, sizeof(locpart),
			    "%s if=%s bs=10k > %s", _PATH_DD, device, buf);
			rval = system(locpart);
			if (rval == -1 || rval == 127 ||
			    !WIFEXITED(rval) || WEXITSTATUS(rval))
				errx(1, "unable to copy manifest");
			++tfileno;
			break;
		default:
			abort();
		}
		if ((fp = fopen(buf, "r")) == NULL)
			err(1, "%s", buf);
		break;
	default:
		abort();
	}

	/* Parse the packages file. */
	TAILQ_INIT(pkghp);
	for (linenum = 1; fgets(buf, sizeof(buf), fp) != NULL; ++linenum) {
		if (buf[0] == '\0')
			continue;
		if ((p = strchr(buf, '\n')) == NULL)
			errx(1, "%s: line %d: line too long", pkgfile, linenum);
		*p = '\0';

		if ((p = strtok(buf, " \t")) == NULL)	/* Blank line. */
			continue;
		if (*p == '#')				/* Comment line. */
			continue;

		if ((pkg = calloc(1, sizeof(PKG))) == NULL)
			err(1, NULL);

		/*
		 * XXX
		 * I have no idea what F_NOFILE has to do with anything,
		 * but it was in the old code.  Nothing in the previous
		 * implementation of installsw used a file of -1 as a
		 * special case, either.
		 */
		if (*p == F_NOFILE)			/* File number. */
			pkg->file = -1;
		else {
			pkg->file = strtol(p, &ep, 10);
			if (*ep != '\0')
				errx(1, "%s: line %d: illegal file number: %s",
				    pkgfile, linenum, p);
		}
		/* Ignore lines of the form "0 TOC". */
		if (pkg->file == 0) {
			free(pkg);
			continue;
		}

		if ((p = strtok(NULL, " \t")) == NULL)
			goto err;
		if ((pkg->name = strdup(p)) == NULL)	/* File name. */
			err(1, NULL);

		if ((p = strtok(NULL, " \t")) == NULL)
			goto err;
		pkg->size = strtoq(p, &ep, 10);		/* File size. */
		if (*ep != '\0')
			errx(1, "%s: line %d: illegal size: %s",
			    pkgfile, linenum, p);

		if ((p = strtok(NULL, " \t")) == NULL)
			goto err;
		pkg->type = *p;				/* File type. */
		if (pkg->type != T_COMPRESSED &&
		    pkg->type != T_GZIPPED && pkg->type != T_NORMAL)
			errx(1, "%s: line %d: unknown file type %c",
			    pkgfile, linenum, pkg->type);

		if ((p = strtok(NULL, " \t")) == NULL)
			goto err;
		switch (pkg->pref = *p) {		/* File status. */
		case P_DESIRABLE:
		case P_HIDDEN:
		case P_OPTIONAL:
		case P_REQUIRED:
			break;
		case P_INITIAL:
			/*
			 * Initial packages are flagged as "already installed"
			 * if we're doing an update, and required otherwise.
			 */
			if (do_update)
				pkg->present = 1;
			else
				pkg->pref = P_REQUIRED;
			break;
		default:
			errx(1, "%s: line %d: unknown file preference %c",
			    pkgfile, linenum, pkg->pref);
		}

		if ((p = strtok(NULL, " \t")) == NULL)
			goto err;
		if ((pkg->root = strdup(p)) == NULL)	/* File install root. */
			err(1, NULL);

		if ((p = strtok(NULL, " \t")) == NULL)
			goto err;
		if ((pkg->sentinel = strdup(p)) == NULL)/* File sentinel. */
			err(1, NULL);

		if ((p = strtok(NULL, "")) == NULL)
err:			errx(1, "%s: line %d: malformed line",
			    pkgfile, linenum);		/* File description */
		for (; *p == ' ' || *p == '\t'; ++p);
		if ((pkg->desc = strdup(p)) == NULL)
			err(1, NULL);

		TAILQ_INSERT_TAIL(pkghp, pkg, q);
	}
	(void)fclose(fp);
}

/*
 * chk_exist --
 *	Check to see if the packages are currently installed.
 */
static void
chk_exist(pkghp)
	PKGH *pkghp;
{
	struct stat sb;
	FILE *fp;
	PKG *pkg;
	size_t ladd, ldel, len;
	int added, deleted;
	char *p, add[1024], del[1024], buf[MAXPATHLEN];

	/*
	 * If both the sentinel file and the log say it's installed, it's
	 * installed, otherwise it's not installed.  If it's an initial
	 * package, we may already have flagged it as installed.
	 */
	for (pkg = pkghp->tqh_first; pkg != NULL; pkg = pkg->q.tqe_next) {
		if (pkg->present)
			continue;
		(void)snprintf(add, sizeof(add),
		    "%s%s", rootdir, pkg->sentinel);

		/*
		 * If the sentinel file exists, set the update flag as well.
		 * If the log entry is then found, the update bit should be
		 * cleared.
		 */
		pkg->present =
		    pkg->previous = pkg->update = lstat(add, &sb) == 0;
	}

	(void)snprintf(buf, sizeof(buf), "%s/%s", rootdir, _PATH_LOG);
	fp = fopen(buf, "r");

	/* N**2, but probably not worth fixing. */
	for (pkg = pkghp->tqh_first; pkg != NULL; pkg = pkg->q.tqe_next) {
		/*
		 * If sentinel not present, the log message isn't sufficient
		 * to change that.
		 */
		if (!pkg->present)
			continue;
		/*
		 * If we're upgrading, initial packages are treated as present
		 * regardless of the existence of a log record.
		 */
		if (do_update && pkg->pref == P_INITIAL)
			continue;
		if (fp == NULL) {
			pkg->present = 0;
			continue;
		}
		added = deleted = 0;
		ladd = snprintf(add, sizeof(add),
		    "package %s %s", pkg->name, LOG_INSTALL);
		ldel = snprintf(del, sizeof(del),
		    "package %s %s", pkg->name, LOG_DELETE);
		for (rewind(fp); (p = fgetln(fp, &len)) != NULL;)
			if (len >= ladd && !memcmp(add, p, ladd)) {
				added = 1;
				deleted = 0;
			}
			else if (len >= ldel && !memcmp(del, p, ldel)) {
				added = 0;
				deleted = 1;
			}
		if (added)
			pkg->update = 0;
		else
			pkg->present = 0;
	}
	if (fp != NULL)
		(void)fclose(fp);
}

/*
 * get_scripts --
 *	Retrieve the pre/post/delete scripts and information files.
 */
static void
get_scripts(pkghp)
	PKGH *pkghp;
{
	PKG *pkg;

	/*
	 * Find and install script entry from the package.  We ignore
	 * what the PACKAGE file builder entered for the file, we want
	 * it in /var/tmp, under a specific name.
	 */
	for (pkg = pkghp->tqh_first; pkg != NULL; pkg = pkg->q.tqe_next)
		if (strcmp(pkg->name, SCRIPTFILE) == 0)
			break;

	if ((spkg = pkg) == NULL)
		return;

	/* Install the script directory. */
	pkg->root = SCRIPT_DIR;
	if (install(pkg, "/", 0))
		exit (1);

	pkg->pref = P_HIDDEN;
}

/*
 * d_mark --
 *	Pre-process the packages lists for deletion.
 */
static void
d_mark(pkghp)
	PKGH *pkghp;
{
	struct stat sb;
	PKG *pkg;
	int found;
	char buf[MAXPATHLEN];

	/* If no delete scripts, we're done. */
	if (spkg == NULL)
		goto nodelete;

	/*
	 * If we're doing deletion, only present the user with the entries
	 * which can be deleted.
	 */
	for (found = 0,
	    pkg = pkghp->tqh_first; pkg != NULL; pkg = pkg->q.tqe_next) {
		if (pkg->pref == P_HIDDEN)
			continue;
		(void)snprintf(buf, sizeof(buf),
		    "%s/%s/%s", spkg->root, pkg->name, SCRIPT_DELETE);
		if (stat(buf, &sb) == 0)
			found = 1;
		else
			pkg->pref = P_HIDDEN;
	}
	if (!found)
nodelete:	errx(1,
	    "there are no packages in this archive that can be deleted");
}

/*
 * s_mark --
 *	Pre-process the packages lists for command-line selection.
 */
static void
s_mark(pkghp)
	PKGH *pkghp;
{
	PKG *pkg;
	int found;

	found = 0;

	/* If no command-line selection... */
	if (comsel == S_NONE) {
		/* The -U option selects all of the update packages. */
		if (do_update) {
			for (pkg = pkghp->tqh_first;
			    pkg != NULL; pkg = pkg->q.tqe_next)
				if (pkg->update)
					pkg->selected = 1;
			return;
		}

		/*
		 * By default, select the desirable and required packages.
		 * But don't bail if we can't find any uninstalled
		 * desirable packages
		 */
		comsel = S_DESIRABLE;
		found = 1;
	}
		
	/* Select based on the comsel value. */
	for (pkg = pkghp->tqh_first; pkg != NULL; pkg = pkg->q.tqe_next)
		switch (pkg->pref) {
		case P_HIDDEN:
		case P_INITIAL:
			continue;
		case P_DESIRABLE:
			if ((comsel == S_DESIRABLE || comsel == S_OPTIONAL) &&
			    !pkg->present ||
			    comsel == S_UPDATE && pkg->update) {
				found = 1;
				pkg->selected = 1;
			}
			break;
		case P_OPTIONAL:
			if (comsel == S_OPTIONAL && !pkg->present ||
			    comsel == S_UPDATE && pkg->update) {
				found = 1;
				pkg->selected = 1;
			}
			break;
		case P_REQUIRED:
			if ((comsel == S_DESIRABLE || comsel == S_OPTIONAL ||
			    comsel == S_REQUIRED) && !pkg->present ||
			    comsel == S_UPDATE && pkg->update) {
				found = 1;
				pkg->selected = 1;
			}
			break;
		}
	if (!found)
		errx(1,
	"there are no packages in this archive in the selected categories");
}

/*
 * install --
 *	Install the named package.
 */
int
install(pkg, root, userpackage)
	PKG *pkg;
	char *root;
	int userpackage;
{
	struct stat sb;
	pid_t pid;
	FILE *fp;
	int rval;
	char *cmd, *rempart, *tail, buf[MAXPATHLEN], pkgname[MAXPATHLEN];

	/* Run pre-install script. */
	if (run_script(pkg, SCRIPT_PRE, "pre-package install"))
		return (1);

	/*
	 * Build and move to the root of the installation.
	 *
	 * XXX
	 * Previous implementations of this code returned to the original
	 * directory between packages.  We don't bother.
	 */
	(void)snprintf(buf, sizeof(buf), "%s/%s", root, pkg->root);
	mkdirp(buf);
	if (chdir(buf)) {
		warn("chdir: %s", buf);
		return (1);
	}

	switch (media) {
	case M_CDROM:
	case M_FLOPPY:
		switch(pkg->type) {
		case T_NORMAL:
			tail = ".tar";
			break;
		case T_COMPRESSED:
			tail = ".tar.Z";
			break;
		case T_GZIPPED:
			tail = ".tar.gz";
			break;
		default:
			abort();
		}
		(void)snprintf(pkgname, sizeof(pkgname),
		    "%s/%s/%s%s", device, PACKAGEDIR, pkg->name, tail);
		switch (location) {
		case L_LOCAL:
			if (stat(pkgname, &sb) != 0) {
				warn("%s", pkgname);
				return (1);
			}
			cmd = build_extract_command(pkg,
			    pkgname, NULL, !userpackage);
			break;
		case L_REMOTE:
			cmd = build_extract_command(pkg,
			    pkgname, &rempart, !userpackage);
			break;
		default:
			abort();
		}
		break;
	case M_TAPE:
		(void)tape_setup(pkg->file, 1);
		cmd = build_extract_command(pkg,
		    device, &rempart, !userpackage);
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	/* Make sure everything's flushed out before forking. */
	(void)fflush(stdout);
	(void)fflush(stderr);

	/* Execute the installation command. */
	if (location == L_REMOTE)
		rval = remote_local_command(rempart, cmd, !userpackage);
	else
		switch (pid = vfork()) {
		case -1:
			warn(NULL);
			_exit (1);
			/* NOTREACHED */
		case 0:
			execl(_PATH_BSHELL, "sh", "-c", cmd, NULL);
			_exit(1);
		default:
			(void)waitpid(pid, &rval, 0);
			rval = WIFEXITED(rval) ? WEXITSTATUS(rval) : 1;

			/* If a user package, write out status file messages. */
			if (userpackage && (fp = fopen(STATUS, "r")) != NULL) {
				while (fgets(buf, sizeof(buf), fp) != NULL)
					(void)printf("%s", buf);
				(void)fclose(fp);
				(void)fflush(stdout);
			}
			break;
		}

	/*
	 * If we installed from tape, we've moved the tape forward a file.
	 * Just in case we're a lot faster than we think we are, wait a few
	 * seconds.
	 */
	if (media == M_TAPE) {
		++tfileno;
		(void)sleep(TAPESLEEP);
	}

	/* Log the installation of user packages. */
	if (userpackage)
		log(pkg, rval ? LOG_IFAILED : LOG_INSTALL);

	if (rval) {
		warnx("failed to extract package \"%s\"", pkg->desc);
		return (1);
	}

	/* Run post-install script. */
	return (run_script(pkg, SCRIPT_POST, "post-package install"));
}

/*
 * run_script --
 *	Run a pre/post install script.
 */
int
run_script(pkg, script_path, script_name)
	PKG *pkg;
	char *script_path, *script_name;
{
	struct stat sb;
	int fd, rval;
	char buf[MAXPATHLEN];

	/* Check for the script. */
	if (spkg == NULL)
		return (0);
	(void)snprintf(buf, sizeof(buf),
	    "%s/%s/%s", spkg->root, pkg->name, script_path);
	if (stat(buf, &sb) != 0)
		return (0);

	/* Move to the script directory so that it can find other files. */
	if ((fd = open(".", O_RDONLY)) < 0) {
		warn(".");
		return (1);
	}
	if (chdir(spkg->root)) {
		warn(spkg->root);
		goto err;
	}
	if (chdir(pkg->name)) {
		warn(pkg->name);
		goto err;
	}

	(void)snprintf(buf, sizeof(buf),
	    "chmod 555 %s && ./%s", script_path, script_path);
	rval = system(buf);
	if (rval == -1 || rval == 127 ||
	    !WIFEXITED(rval) || WEXITSTATUS(rval)) {
		(void)printf("The \"%s\" %s script FAILED.\n",
		    pkg->desc, script_name);
		goto err;
	}
	rval = 0;
	if (0)
err:		rval = 1;
	(void)fchdir(fd);
	close(fd);
	return (rval);
}

/*
 * usage --
 *	Display usage message and die.
 */
static void
usage()
{
	(void)fprintf(stderr, "usage: installsw %s\n\t%s\n",
"[-DELU] [-c device] [-d root_dir]",
"[-h rhost [-l ruser]] [-m cdrom | floppy | tape] [-s d | o | r]");
	exit(1);
}
