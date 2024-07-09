/*-
 * Copyright (c) 1993, 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI cur_inter.c,v 2.23 2003/07/08 21:58:42 polk Exp
 */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <ctype.h>
#include <curses.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include "installsw.h"

static void	 cur_adjsize __P((PKG *, char *));
static int	 cur_delete __P((PKGH *));
static void	 cur_display __P((PKGH *, int));
static int	 cur_exec __P((PKGH *, PKG *));
static void	 cur_graph __P((int));
static void	 cur_info __P((PKGH *));
static void	 cur_init __P((void));
static int	 cur_install __P((PKGH *));
static int	 cur_iscript __P((PKGH *, char *, char *, int));
static void	 cur_lable __P((void));
static PKG	*cur_lookup __P((PKGH *, int));
static int	 cur_msg __P((int, const char *, ...));
static int	 cur_needpaint __P((void));
static void	 cur_scroll __P((void));
static void	 cur_suspend __P((void));
static void	 ontstp __P((int));

static int	bottom;
static off_t	current, total;
static size_t	cur_row;
static int	curses_init;
static int	current_count, package_count;
static int	desirable_state, required_state, update_state;
static int	sig_tstp;		/* SIGTSTP arrived. */
static int	spc_header = 1;		/* 0/1: if header. */
static int	spc_packages;		/*   N: lines for packages. */
static int	spc_typeline = 1;	/* 0/1: if space after packages. */
static int	spc_commands = 1;	/* 0/1: if space after type line. */
static int	spc_prompt = 1;		/* 0/1: if space after commands. */
static int	spc_scrollbox = 1;	/* 0/1: if using a scroll box. */

#define	COMMAND	(spc_header + spc_packages + spc_typeline)
#define MSGLINE	(COMMAND + 1 + (do_delete ? 2 : 4) + spc_commands + spc_prompt)

#define	I_TITLE		(COMMAND + spc_scrollbox)
#define	I_PCNT		(I_TITLE + 1)
#define	I_DIVIDE	(I_TITLE + (do_delete ? 1 : 2))
#define	I_SCROLL	(I_TITLE + (do_delete ? 2 : 3))

#define	PC_SIDE_LEFT	0		/* Scrolling box left side offset. */
#define	PC_SIDE_RIGHT	(COLS - 1)	/* Scrolling box right side offset. */
static int pc_scr_off;			/* Scrolling text offset. */

static struct termios orig;		/* Original termios values. */

/*
 * cur_inter --
 *	Run the interactive screen to query users about which packages
 *	they wish to load.
 */
int
cur_inter(pkghp)
	PKGH *pkghp;
{
	PKG *pkg;
	int c, i;
	char *imessage;

	if (tcgetattr(STDIN_FILENO, &orig) == -1)
		err(1, "tcgetattr");

	/*
	 * Assign selection letters.  Note, there's a chance there's
	 * nothing to display.
	 *
	 * XXX
	 * Assume 'a' to 'z', '1' to '9', contiguous in the character
	 * set.
	 */
#define	MAX_LETTER	'z'
	for (c = 'a', i = 0,
	    pkg = pkghp->tqh_first; pkg != NULL; pkg = pkg->q.tqe_next) {
		if (pkg->pref == P_HIDDEN)
			continue;
		if (c > MAX_LETTER)
			break;
		++i;
		pkg->letter = c++;
	}
#define	MAX_DIGIT	'9'
	for (c = '1'; pkg != NULL; pkg = pkg->q.tqe_next) {
		if (pkg->pref == P_HIDDEN)
			continue;
		if (c > MAX_DIGIT)
			errx(1, "too many selectable packages");
		++i;
		pkg->letter = c++;
	}
	if (i == 0)
		errx(1,
	"there are no packages in this archive that can be installed");

	/* Initialize curses. */
	cur_init();

	/*
	 * If doing an automatic install, do it.
	 *
	 * Automatic install has a simpler screen, 5 lines of box/headers,
	 * 7 lines of scroll.  We put 2/3 of the remaining lines at the
	 * bottom, 1/3 at the top.
	 */
	if (express) {
		if (LINES - 1 < 12) {
			cur_end();
			errx(1, "screen too small to install packages");
		}
		spc_header = spc_typeline = 0;
		spc_packages = (LINES - 12 - 1) / 3;
		bottom = spc_packages + 12;
		return (cur_install(pkghp));
	}

	/* Initialize the start of the commands and messages. */
	spc_packages = (i + 1) / 2;

	/*
	 * We can delete some of the lines of whitespace.
	 *
	 * XXX
	 * We don't play games with columns -- if there aren't at least
	 * 80, we lose.
	 */
	if (LINES - 1 < MSGLINE)	/* ... between commands and prompt. */
		spc_prompt = 0;
	if (LINES - 1 < MSGLINE)	/* ... between type and commands. */
		spc_commands = 0;
	if (LINES - 1 < MSGLINE)	/* Header. */
		spc_header = 0;
	if (LINES - 1 < MSGLINE)	/* ... between packages and type. */
		spc_typeline = 0;
	if (LINES - 1 < MSGLINE || COLS < 80) {
		cur_end();
		errx(1, "screen too small to display %d packages", i);
	}
	bottom = LINES - 1;

	/* Check to see if we've pre-selected packages. */
	for (pkg = pkghp->tqh_first; pkg != NULL; pkg = pkg->q.tqe_next)
		if (pkg->selected) {
			update_state = 1; 
			break;
		}
	/* If so, tell the user. */
	if (update_state)
		imessage = do_update ?
		    "Update packages pre-selected." :
		    "Required and desirable packages pre-selected.";
	else
		imessage = NULL;

	cur_lable();			/* Labels. */
	move(MSGLINE, 0);		/* Lower left corner. */

	/*
	 * If the user didn't select somehow on the command line, we defaulted
	 * to selecting the desirable and required packages.  This is the only
	 * case we check, because all other values of comsel caused express to
	 * be set, and us to branch directly to installation, above.
	 */
	if (comsel == S_DESIRABLE)
		desirable_state = required_state = 1;

	for (;;) {
		cur_display(pkghp, 1);

		if (imessage != NULL) {
			cur_msg(0, imessage);
			imessage = NULL;
		}

		refresh();
		c = getchar();

		(void)move(MSGLINE, 0);
		(void)clrtoeol();
		refresh();

		switch (c) {
		case '\014':			/* ^L: refresh. */
			clearok(curscr, 1);
			continue;
		case 'A':			/* Add. */
			for (pkg = pkghp->tqh_first;
			    pkg != NULL; pkg = pkg->q.tqe_next)
				if (!do_update || pkg->pref != P_INITIAL)
					pkg->selected = 1;
			if (!desirable_state ||
			    !required_state || !update_state) {
				desirable_state =
				    required_state = update_state = 1;
				cur_lable();
			}
			break;
		case 'C':			/* Clear selections. */
			for (pkg = pkghp->tqh_first;
			    pkg != NULL; pkg = pkg->q.tqe_next)
				pkg->selected = 0;
			if (desirable_state ||
			    required_state || update_state) {
				desirable_state =
				    required_state = update_state = 0;
				cur_lable();
			}
			break;
		case 'D':			/* Desired. */
			if (do_delete)
				goto not_special;
			desirable_state = !desirable_state;
			for (i = 0, pkg = pkghp->tqh_first;
			    pkg != NULL; pkg = pkg->q.tqe_next)
				if (!pkg->present && pkg->pref == P_DESIRABLE) {
					i = 1;
					pkg->selected = desirable_state;
				}
			if (i)
				cur_lable();
			else if (desirable_state) {
				desirable_state = !desirable_state;
				cur_msg(0,
			    "No desirable packages not already installed.");
			}
			break;
		case 'N':			/* Install. */
			for (pkg = pkghp->tqh_first;
			    pkg != NULL; pkg = pkg->q.tqe_next)
				if (pkg->selected)
					break;
			if (pkg == NULL) {
				cur_msg(0, "No packages selected for %s",
				    do_delete ? "deletion" : "installation");
				break;
			}
			if (!expert &&
			    !cur_msg(1, "Confirm %s of selected packages?",
			    do_delete ? "deletion" : "installation"))
				break;

			return (do_delete ?
			    cur_delete(pkghp) : cur_install(pkghp));
		case 'R':			/* Required. */
			if (do_delete)
				goto not_special;
			required_state = !required_state;
			for (i = 0, pkg = pkghp->tqh_first;
			    pkg != NULL; pkg = pkg->q.tqe_next)
				if (!pkg->present && pkg->pref == P_REQUIRED) {
					i = 1;
					pkg->selected = required_state;
				}
			if (i)
				cur_lable();
			else if (required_state) {
				required_state = !required_state;
				cur_msg(0,
			    "No required packages not already installed.");
			}
			break;
		case 'U':			/* Update. */
			if (do_delete)
				goto not_special;
			update_state = !update_state;
			for (i = 0, pkg = pkghp->tqh_first;
			    pkg != NULL; pkg = pkg->q.tqe_next)
				if (pkg->update) {
					i = 1;
					pkg->selected = update_state;
				}
			if (i)
				cur_lable();
			else if (update_state) {
				update_state = !update_state;
				cur_msg(0, "No packages to update.");
			}
			break;
		case 'X':			/* Exit. */
			cur_end();
			exit (0);
		case '?':			/* Info. */
			cur_info(pkghp);
			break;
		default:			/* Toggle single item. */
not_special:		if ((pkg = cur_lookup(pkghp, c)) == NULL)
				break;
			if (pkg->selected != 0) {
				pkg->selected = !pkg->selected;
				break;
			}
			if (do_delete) {
				if (!pkg->present && !cur_msg(1,
    "Package `%c' appears not to be installed -- delete anyway?", c))
					break;
			} else {
				if (do_update) {
					if (pkg->pref == P_INITIAL &&
					    !cur_msg(1,
    "Package `%c' should not be installed when updating -- install anyway?", c))
						break;
				}
				if (pkg->present && !cur_msg(1,
    "Package `%c' appears to already be installed -- install anyway?", c))
					break;
			}
			pkg->selected = !pkg->selected;
			break;
		}
	}
	/* NOTREACHED */

	abort();
}

/*
 * cur_delete --
 *	Process the package list and delete selected entries.
 */
static int
cur_delete(pkghp)
	PKGH *pkghp;
{
	PKG *pkg;

	/*
	 * !!!
	 * The delete scripts can be interactive.  Clear the screen,
	 * and quit curses.
	 */
	clear();
	cur_end();

	for (pkg = pkghp->tqh_first; pkg != NULL; pkg = pkg->q.tqe_next)
		if (pkg->selected) {
			if (run_script(pkg, SCRIPT_DELETE, "delete")) {
				log(pkg, LOG_DFAILED);
				return (1);
			}
			log(pkg, LOG_DELETE);
		}
	return (0);
}

/*
 * cur_install --
 *	Process the package list and install selected entries.
 */
static int
cur_install(pkghp)
	PKGH *pkghp;
{
	PKG *pkg;
	sigset_t set;
	size_t row;
	int i, rval;

	/*
	 * Run the before installation scripts.  The select flag is toggled
	 * as packages are installed, so set the script flag first.
	 */
	for (pkg = pkghp->tqh_first; pkg != NULL; pkg = pkg->q.tqe_next)
		if (pkg->selected)
			pkg->script = 1;
	if (cur_iscript(pkghp, SCRIPT_BEGIN, "pre-install", 1))
		goto err;

	/*
	 * XXX
	 * Curses doesn't like being reentered, so handle stop signals locally.
	 * We don't try to protect most of this code since we spend very little
	 * time in curses, and it's going to be difficult and complex to get it
	 * right, especially around the getchar() calls.  However, if we get a
	 * SIGTSTP signal after pax starts spewing output to the screen, it will
	 * almost certainly cause curses to reenter itself.  Block the signal
	 * while we paint the screen, we'll reenable it in a minute.
	 */
	(void)sigemptyset(&set);
	(void)sigaddset(&set, SIGTSTP);
	sigprocmask(SIG_BLOCK, &set, NULL);

	/* Clear the command section of the screen. */
	for (row = COMMAND; row < LINES; ++row) {
		(void)move(row, 0);
		(void)clrtoeol();
	}

	/* Initialize the package count. */
	if (package_count == 0)
		for (pkg = pkghp->tqh_first; pkg != NULL; pkg = pkg->q.tqe_next)
			if (pkg->selected)
				++package_count;

	/* If on a PC and have sufficient space, build the scroll box. */
	if (express ||
	    (spc_header && spc_typeline && spc_commands && spc_prompt)) {
		(void)mvaddch(COMMAND, PC_SIDE_LEFT, ACS_ULCORNER);
		for (i = PC_SIDE_LEFT; i < PC_SIDE_RIGHT - 1; ++i)
			(void)addch(ACS_HLINE);
		(void)addch(ACS_URCORNER);
		for (i = COMMAND + 1; i < bottom; ++i) {
			(void)mvaddch(i, PC_SIDE_LEFT, ACS_VLINE);
			(void)mvaddch(i, PC_SIDE_RIGHT, ACS_VLINE);
		}
		(void)mvaddch(bottom, PC_SIDE_LEFT, ACS_LLCORNER);
		for (i = PC_SIDE_LEFT; i < PC_SIDE_RIGHT - 1; ++i)
			(void)addch(ACS_HLINE);
		(void)addch(ACS_LRCORNER);
		pc_scr_off = PC_SIDE_LEFT + 2;
	} else
		pc_scr_off = spc_scrollbox = 0;

	/* If installing, provide percent complete information. */
	/* Compute the total number of bytes we're going to install. */
	for (current = total = 0,
	    pkg = pkghp->tqh_first; pkg != NULL; pkg = pkg->q.tqe_next)
		if (pkg->selected)
			total += pkg->size;
	total *= 1024;

#define	PCT	60
#define	PCT_END	pc_scr_off + 15
	(void)mvaddstr(I_PCNT, pc_scr_off, "   % complete: ");
#ifdef	notdef
	standout();
#endif
	for (i = 0; i < PCT; ++i)
		(void)addch(ACS_BOARD);
#ifdef	notdef
	standend();
#endif

	/* Put up the dividing line. */
	(void)move(I_DIVIDE, PC_SIDE_LEFT);
	(void)addch(ACS_LTEE);
	for (i = PC_SIDE_LEFT; i < PC_SIDE_RIGHT - 1; ++i)
		(void)addch(ACS_HLINE);
	(void)addch(ACS_RTEE);

	/*
	 * Set current row to last line before the start of the
	 * scrolling region, gets incremented right before use.
	 */
	cur_row = I_SCROLL - 1;

	move(I_PCNT, pc_scr_off);
	refresh();

	/*
	 * Install the packages.  If we fail to install a package
	 * and the sentinel file didn't already exist when we started running,
	 * delete it.
	 */
	rval = 0;
	for (pkg = pkghp->tqh_first; pkg != NULL; pkg = pkg->q.tqe_next)
		if (pkg->selected && cur_exec(pkghp, pkg)) {
			if (!pkg->previous)
				(void)unlink(pkg->sentinel);
			rval = 1;
			break;
		}

	if (rval == 0) {
		/* Our accounting isn't exact.  Cheat. */
		cur_graph(1);
	} else {
		/*
		 * If an error occurred, leave everything as it is so users
		 * can read the error messages to customer support.
		 */
		cur_scroll();
		standout();
		mvprintw(cur_row, pc_scr_off,
"Installation was UNSUCCESSFUL for a package; press any key to continue:");
		standend();
		refresh();
		(void)getchar();

		/*
		 * Don't run SCRIPT_END scripts for any package that failed
		 * or wasn't installed.
		 */
		for (; pkg != NULL; pkg = pkg->q.tqe_next)
			pkg->script = 0;
	}

	/* End curses. */
	cur_end();

	/* Run the after installation scripts. */
	return (cur_iscript(pkghp, SCRIPT_END, "post-install", 0) || rval);

err:	cur_end();
	return (1);
}

/*
 * cur_exec --
 *	Set up the processes, and do the installation.
 */
static int
cur_exec(pkghp, pkg)
	PKGH *pkghp;
	PKG *pkg;
{
	pid_t pid;
	struct sigaction act;
	sigset_t set;
	size_t len;
	int nr, rval, std_output[2];
	char *p, *t, buf[256], path[8 * 1024];

	/* Display the installation header. */
	standout();
	(void)mvprintw(I_TITLE, pc_scr_off, "Installing %s...", pkg->desc);
	standend();
	(void)printw(" (package %d of %d)", ++current_count, package_count);
	clrtoeol();
	if (pc_scr_off)
		(void)mvaddch(I_TITLE, PC_SIDE_RIGHT, ACS_VLINE);
	/*
	 * The installer writes standard output and standard error output.  The
	 * parent reads the standard output and standard error output, copying
	 * them onto the screen.  The parent reads std_output[0] and [2], the
	 * installer writes std_output[1] and [3].
	 */
	std_output[0] = std_output[1] = -1;
	if (pipe(std_output) < 0) {
		cur_end();
		err(1, NULL);
	}

	switch (pid = fork()) {
	case -1:				/* Error. */
		(void)close(std_output[0]);
		(void)close(std_output[1]);
		cur_end();
		err(1, NULL);
	case 0:					/* Utility. */
		/* Default behavior for signals. */
		(void)signal(SIGINT, SIG_DFL);
		(void)signal(SIGTSTP, SIG_DFL);

		/* Redirect stdout/stderr to the write end of the pipe. */
		(void)dup2(std_output[1], STDOUT_FILENO);
		(void)dup2(std_output[1], STDERR_FILENO);

		/* Close the utility's file descriptors. */
		(void)close(std_output[0]);
		(void)close(std_output[1]);

		/* Install packages. */
		rval = install(pkg, rootdir, 1);

		/* Flush any queued output. */
		(void)fflush(stdout);
		(void)fflush(stderr);
		_exit(rval);
	default:				/* Parent. */
		/* Close the pipe ends the parent won't use. */
		(void)close(std_output[1]);
		break;
	}

	/* Interrupt the read call, if the user wants to suspend. */
	act.sa_handler = ontstp;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	(void)sigaction(SIGTSTP, &act, NULL);

	(void)sigemptyset(&set);
	(void)sigaddset(&set, SIGTSTP);
	sigprocmask(SIG_UNBLOCK, &set, NULL);

	/*
	 * While there's input, put it on the screen.  Every time we're
	 * about to write past the end of the screen, scroll it a line.
	 */
#define	MAXLEN	((PC_SIDE_RIGHT - pc_scr_off) - 1)
	for (t = path;;) {
		move(I_PCNT, pc_scr_off);
		if (sig_tstp) {
			cur_suspend();
			clearok(stdscr, 1);
		}
		if (cur_needpaint())
			clearok(stdscr, 1);
		refresh();
		if ((nr = read(std_output[0], buf, sizeof(buf))) <= 0) {
			if (nr == -1 && errno == EINTR)
				continue;
			break;
		}
		for (p = buf; nr > 0; --nr) {
			if ((*t++ = *p++) != '\n')
				continue;
			*--t = '\0';
			for (t = path, len = strlen(path);;) {
				cur_scroll();
				(void)move(cur_row, pc_scr_off);
				if (len == 0)
					break;
				else if (len > MAXLEN) {
					addnstr(t, MAXLEN);
					t += MAXLEN;
					len -= MAXLEN;
				} else {
					addnstr(t, len);
					break;
				}
			}
			clrtoeol();
			if (pc_scr_off)
				(void)mvaddch(cur_row,
				    PC_SIDE_RIGHT, ACS_VLINE);
			cur_adjsize(pkg, path);
			t = path;
		}

		/* Update the current report; lower bound is one entry. */
		cur_graph(0);
	}

	/* Close last open fd. */
	(void)close(std_output[0]);

	/* Print any I/O error messages. */
	if (nr == -1) {
		cur_scroll();
		standout();
		mvprintw(cur_row, 0, "Read error: %s", strerror(errno));
		standend();
		refresh();
	}

	/* Wait for the child and return its status. */
	(void)waitpid(pid, &rval, 0);
	rval = WIFEXITED(rval) ? WEXITSTATUS(rval) : 1;

	/* I/O errors cause error exit, too. */
	if (nr == -1)
		rval = 1;

	/* If interactive, turn off the package highlight in the menu. */
	if (!express) {
		pkg->present = 1;
		pkg->selected = 0;
		cur_display(pkghp, 0);
	}

	return (rval);
}

/*
 * cur_adjsize --
 *	Adjust the current size and update the remainder.
 */
static void
cur_adjsize(pkg, path)
	PKG *pkg;
	char *path;
{
	struct stat sb;
	char buf[MAXPATHLEN];

	if (lstat(path, &sb)) {
		(void)snprintf(buf, sizeof(buf), "%s/%s", rootdir, pkg->root);
		if (chdir(buf) || lstat(path, &sb))
			return;
	}
	current += sb.st_size / sb.st_nlink;
}

/*
 * cur_graph --
 *	Update the graph.
 */
static void
cur_graph(cheat)
	int cheat;
{
	static int graph_begin;
	double pct;
	int graph_end;

	/* Our accouting isn't exact; cheat. */
	if (cheat)
		current = total;

	pct = (double)(current + 1) / (double)total;

	/*
	 * The PACKAGE accounting isn't exact, either.  Don't hit 100% until
	 * we finish.
	 */
	if (!cheat && pct > .99)
		pct = .99;

	/* Update the percentage. */
	(void)mvprintw(I_PCNT, pc_scr_off, "%3d", (int)(pct * 100));

	/* Bound the graph at 1. */
	if ((graph_end = PCT * pct) == 0)
		graph_end = 1;
	if (graph_begin == graph_end)
		return;

#ifdef	notdef
	standout();
#endif
	for (; graph_begin < graph_end; ++graph_begin)
		mvaddch(I_PCNT, graph_begin + PCT_END, ACS_CKBOARD);
#ifdef	notdef
	standend();
#endif
}

/*
 * cur_needpaint --
 *	Check to see if the user wants a repaint.
 */
static int
cur_needpaint()
{
	struct timeval poll;
	fd_set rdfd;
	ssize_t nr;
	char buf[32];

	FD_ZERO(&rdfd);
	poll.tv_sec = 0;
	poll.tv_usec = 0;
	FD_SET(STDIN_FILENO, &rdfd);
	if (select(STDIN_FILENO + 1, &rdfd, NULL, NULL, &poll) != 1)
		return (0);

	if ((nr = read(STDIN_FILENO, buf, sizeof(buf))) <= 0)
		return (0);

	while (--nr >= 0)
		if (buf[nr] == '\014')		/* ^L: refresh. */
			return (1);
	return (0);
}

/*
 * cur_scroll --
 *	Scroll the output messages.
 */
static void
cur_scroll()
{
	if (cur_row == bottom - (pc_scr_off ? 1 : 0)) {
		move(I_SCROLL, 0);
		deleteln();
		if (pc_scr_off) {
			move(bottom - 1, 0);
			insertln();
			(void)mvaddch(cur_row, PC_SIDE_LEFT, ACS_VLINE);
			(void)mvaddch(cur_row, PC_SIDE_RIGHT, ACS_VLINE);
		}
	} else
		++cur_row;
}

/*
 * cur_display --
 *	Display packages with a few of their attributes.
 */
static void
cur_display(pkghp, update_space)
	PKGH *pkghp;
	int update_space;
{
	PKG *pkg;
	int oldy, oldx;
	int i, len, row, spaceused;

	row = 0;
	getyx(stdscr, oldy, oldx);

	if (spc_header) {
		(void)move(0, 0);
		(void)addch(ACS_ULCORNER);
		(void)addch(ACS_HLINE);
		addch(' ');
		standout();
		(void)addstr("BSD/OS Installation");
		standend();
		getyx(stdscr, row, i);
		(void)addch(' ');
		for (; i < COLS - 2; ++i)
			(void)addch(ACS_HLINE);
		(void)addch(ACS_URCORNER);
		++row;
	}
	for (i = spaceused = 0,
	    pkg = pkghp->tqh_first; pkg != NULL; pkg = pkg->q.tqe_next) {
		if (pkg->pref == P_HIDDEN)
			continue;
		if (i >= spc_packages)
			(void)move(row, 40);
		else {
			(void)move(row, 0);
			if (spc_header && spc_typeline) {
				(void)addch(ACS_VLINE);
				addch(' ');
			}
		}
		(void)addch(pkg->letter);

		if (pkg->selected) {
			spaceused += pkg->size;
			addch('+');
		} else
			addch(' ');

		if (pkg->present)
			addch('*');
		else
			switch (pkg->pref) {
			case P_DESIRABLE:
				addch('D');
				break;
			case P_OPTIONAL:
				addch(' ');
				break;
			case P_REQUIRED:
				addch('R');
				break;
			default:
				abort();
			}
		printw(" %6dK ", (int)pkg->size);

		if (pkg->selected)
			standout();

#define	REMAINING	28
		if ((len = strlen(pkg->desc)) >= REMAINING)
			len = REMAINING - 1;
		addnstr(pkg->desc, len);

		if (pkg->selected)
			standend();

		if (spc_header && spc_typeline)
			mvaddch(row, COLS - 1, ACS_VLINE);

		if (++i == spc_packages)
			row = spc_header;
		else
			++row;
	}

	if (spc_header && spc_typeline) {
		(void)move(spc_header + spc_packages, 0);
		(void)addch(ACS_LLCORNER);
		for (i = 1; i < COLS - 1; ++i)
			(void)addch(ACS_HLINE);
		(void)addch(ACS_LRCORNER);
		++row;
	}

	if (update_space) {
		(void)mvprintw(COMMAND, 63, "           ");
		(void)mvprintw(COMMAND, 63, " %dK", spaceused);
	}
	(void)move(oldy, oldx);
}

#define	CMSG0 \
"*=Already installed  D=Desirable  R=Required      Space req'd:            "
#define	CMSG0D \
"*=Already installed                               Space freed:            "
#define	CMSG1 \
"a-z1-9: Toggle a single package       ?: Package description"
#define	CMSG2A \
"     A: Add all packages              C: Clear selections"
#define	CMSG2B \
"     A: Add all non-initial packages  C: Clear selections"
#define	CMSG3A \
"     D: Clear `Desirable' packages    R: Clear `Required' packages"
#define	CMSG3B \
"     D: Add `Desirable' packages      R: Clear `Required' packages"
#define	CMSG3C \
"     D: Clear `Desirable' packages    R: Add `Required' packages  "
#define	CMSG3D \
"     D: Add `Desirable' packages      R: Add `Required' packages  "
#define	CMSG4A \
"     U: Clear `Update' packages       N: Install packages         X: Quit"
#define	CMSG4B \
"     U: Add `Update' packages         N: Install packages         X: Quit"
#define	CMSG4C \
"                                      N: Delete packages          X: Quit"

/*
 * cur_lable --
 *	Display command labels.
 */
static void
cur_lable()
{
	size_t row;

	row = COMMAND;
	(void)mvprintw(row, 0, do_delete ? CMSG0D : CMSG0);
	row += spc_commands;
	(void)mvprintw(++row, 0, CMSG1);
	(void)mvprintw(++row, 0, do_update ? CMSG2B : CMSG2A);
	if (do_delete)
		(void)mvprintw(++row, 0, CMSG4C);
	else {
		if (required_state)
			if (desirable_state)
				(void)mvprintw(++row, 0, CMSG3A);
			else
				(void)mvprintw(++row, 0, CMSG3B);
		else
			if (desirable_state)
				(void)mvprintw(++row, 0, CMSG3C);
			else
				(void)mvprintw(++row, 0, CMSG3D);
		(void)mvprintw(++row, 0, update_state ? CMSG4A : CMSG4B);
	}

	(void)move(MSGLINE, 0);
	(void)clrtoeol();
}

/*
 * cur_info --
 *	Display information about a package.
 */
static void
cur_info(pkghp)
	PKGH *pkghp;
{
	PKG *pkg;
	FILE *fp;
	size_t len, row;
	int ch;
	char *lp, buf[MAXPATHLEN];

	if (spkg == NULL) {
		cur_msg(0,
"There is no additional information available for any of these packages.");
		return;
	}

	move(MSGLINE, 0);
	(void)clrtoeol();
	standout();
	addstr("Information about which package? ");
	standend();
	refresh();
	if ((pkg = cur_lookup(pkghp, ch = getchar())) == NULL)
		return;

	(void)snprintf(buf, sizeof(buf),
	    "%s/%s/%s", spkg->root, pkg->name, SCRIPT_INFO);
	if ((fp = fopen(buf, "r")) == NULL) {
		cur_msg(0, "No available information about package %c.", ch);
		return;
	}

	clear();
	for (row = 0; (lp = fgetln(fp, &len)) != NULL; ++row) {
		(void)move(row, 0);
		addnstr(lp, len - 1);
	}
	move(++row, 0);
	standout();
	addstr("Press any key to continue:");
	standend();
	refresh();
	(void)getchar();
	clear();
	cur_lable();
}

/*
 * cur_iscript --
 *	Run a begin/end install script.
 */
static int
cur_iscript(pkghp, script_path, script_name, prescript)
	PKGH *pkghp;
	char *script_path, *script_name;
	int prescript;
{
	struct stat sb;
	PKG *pkg;
	int first, rval;
	char buf[MAXPATHLEN];

	if (spkg == NULL)
		return (0);

	rval = 0;
	first = 1;
	for (pkg = pkghp->tqh_first;
	    !rval && pkg != NULL; pkg = pkg->q.tqe_next) {
		if (!pkg->script)
			continue;

		(void)snprintf(buf, sizeof(buf),
		    "%s/%s/%s", spkg->root, pkg->name, script_path);
		if (stat(buf, &sb) != 0)
			continue;

		if (first && prescript) {
			first = 0;

			(void)endwin();
			putp(tigetstr("clear"));
			fflush(stdout);
		}

		if ((rval = run_script(pkg, script_path, script_name)) != 0) {
			(void)printf("Press any key to exit:");
			(void)getchar();
			break;
		}
	}

	if (!rval && !first)
		refresh();
	return (rval);
}

/*
 * cur_lookup --
 *	Search the set of possible choices for the selected character.
 */
static PKG *
cur_lookup(pkghp, c)
	PKGH *pkghp;
	int c;
{
	PKG *pkg;

	for (pkg = pkghp->tqh_first; pkg != NULL; pkg = pkg->q.tqe_next)
		if (pkg->letter == c)
			return (pkg);
	if (isgraph(c))
		cur_msg(0, "No package `%c' listed.", c);
	else
		cur_msg(0, "Unknown command or package identifer.");
	return (NULL);
}

/*
 * cur_init --
 *	Curses initialization routine.
 */
void
cur_init()
{
	if (initscr() == NULL)
		err(1, "curses initialization");
	crmode();
	noecho();
	scrollok(stdscr, FALSE);
	clear();
	curses_init = 1;
}

/*
 * cur_end --
 *	Curses end routine.
 */
void
cur_end()
{
	if (curses_init) {
		(void)move(LINES - 1, 0);
		(void)refresh();
		(void)endwin();
		curses_init = 0;
	}
}

/*
 * cur_suspend --
 *	Suspend the screen.
 */
static void
cur_suspend()
{
	struct termios t;

	sig_tstp = 0;

	/* Move to the lower-left hand corner of the screen. */
	(void)move(LINES - 1, 0);
	(void)clrtoeol();
	(void)refresh();

	/* Save the terminal settings. */
	(void)tcgetattr(STDIN_FILENO, &t);

	/* Restore the original terminal settings. */
	(void)tcsetattr(STDIN_FILENO, TCSASOFT | TCSADRAIN, &orig);

	/* Stop the process group. */
	(void)kill(0, SIGSTOP);

	/* Time passes. */

	/* Restore the terminal settings. */
	(void)tcsetattr(STDIN_FILENO, TCSASOFT | TCSADRAIN, &t);
}

/*
 * cur_msg --
 *	Display message on MSGLINE.
 */
static int
#if __STDC__
cur_msg(int confirm, const char *fmt, ...)
#else
cur_msg(confirm, fmt, va_alist)
	int confirm;
	char *fmt;
	va_dcl
#endif
{
	va_list ap;
	int ch;
	char line[128];

#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	(void)vsnprintf(line, sizeof(line), fmt, ap);
	va_end(ap);

	move(MSGLINE, 0);
	(void)clrtoeol();
	standout();
	addstr(line);
	if (confirm)
		addstr(" [y/n]");
	standend();
	refresh();
	if (confirm)
		for (;;) {
			ch = getchar();
			if (isupper(ch))
				ch = tolower(ch);
			if (ch == 'y' || ch == 'n') {
				move(MSGLINE, 0);
				(void)clrtoeol();
				return (ch == 'y');
			}
			(void)write(STDOUT_FILENO, "\007", 1);	/* '\a' */
		}
	return (0);
}

/*
 * onintr --
 *	Handle SIGINT arrival.
 */
void
onintr(signo)
	int signo;
{
	signo = signo;				/* Shut gcc -Wall up. */

	cur_end();				/* End curses. */
	clean_up();				/* General cleanup. */
	tape_rew();				/* Rewind the tape drive. */

	(void)fprintf(stderr, "\nInterrupted.\n");

	(void)signal(SIGINT, SIG_DFL);
	kill(getpid(), SIGINT);

	/* NOTREACHED */
	exit(1);
}

/*
 * ontstp --
 *	Handle SIGTSTP arrival.
 */
void
ontstp(signo)
	int signo;
{
	signo = signo;				/* Shut gcc -Wall up. */

	sig_tstp = 1;
}
