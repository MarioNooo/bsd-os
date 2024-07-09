/*
 * Copyright (c) 1993
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
"@(#) Copyright (c) 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)pathconf.c	8.1 (Berkeley) 6/6/93";
#endif /* not lint */

#include <sys/param.h>
#include <sys/sysctl.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct _list {
	char	*name;			/* path variable */
	int	 indx;			/* value */
	enum { CS, PC, SC } type;	/* confstr, pathconf, sysconf */
} LIST;

LIST list[] = {
#ifdef _SC_AIO_LISTIO_MAX
	{ "_posix_aio_listio_max", _SC_AIO_LISTIO_MAX, SC },
#endif
#ifdef _SC_AIO_MAX
	{ "_posix_aio_max", _SC_AIO_MAX, SC },
#endif
	{ "_posix_arg_max", _SC_ARG_MAX, SC },
#ifdef _PC_ASYNC_IO
	{ "_posix_async_io", _PC_ASYNC_IO, SC },
#endif
#ifdef _SC_ASYNCHRONOUS_IO
	{ "_posix_asynchronous_io", _SC_ASYNCHRONOUS_IO, SC },
#endif
	{ "_posix_child_max", _SC_CHILD_MAX, SC },
	{ "_posix_chown_restricted", _PC_CHOWN_RESTRICTED, PC },
#ifdef _SC_DELAYTIMER_MAX
	{ "_posix_delaytimer_max", _SC_DELAYTIMER_MAX, SC },
#endif
#ifdef _SC_FSYNC
	{ "_posix_fsync", _SC_FSYNC, SC },
#endif
	{ "_posix_job_control", _SC_JOB_CONTROL, SC },
	{ "_posix_link_max", _PC_LINK_MAX, PC },
#ifdef _SC_MAPPED_FILES
	{ "_posix_mapped_files", _SC_MAPPED_FILES, SC },
#endif
	{ "_posix_max_canon", _PC_MAX_CANON, PC },
	{ "_posix_max_input", _PC_MAX_INPUT, PC },
#ifdef _SC_MEMLOCK
	{ "_posix_memlock", _SC_MEMLOCK, SC },
#endif
#ifdef _SC_MEMLOCK_RANGE
	{ "_posix_memlock_range", _SC_MEMLOCK_RANGE, SC },
#endif
#ifdef _SC_MEMORY_PROTECTION
	{ "_posix_memory_protection", _SC_MEMORY_PROTECTION, SC },
#endif
#ifdef _SC_MESSAGE_PASSING
	{ "_posix_message_passing", _SC_MESSAGE_PASSING, SC },
#endif
#ifdef _SC_MQ_OPEN_MAX
	{ "_posix_mq_open_max", _SC_MQ_OPEN_MAX, SC },
#endif
#ifdef _SC_MQ_PRIO_MAX
	{ "_posix_mq_prio_max", _SC_MQ_PRIO_MAX, SC },
#endif
	{ "_posix_name_max", _PC_NAME_MAX, PC },
	{ "_posix_ngroups_max", _SC_NGROUPS_MAX, SC },
	{ "_posix_no_trunc", _PC_NO_TRUNC, PC },
	{ "_posix_open_max", _SC_OPEN_MAX, SC },
	{ "_posix_path_max", _PC_PATH_MAX, PC },
	{ "_posix_pipe_buf", _PC_PIPE_BUF, PC },
#ifdef _PC_PRIO_IO
	{ "_posix_prio_io", _PC_PRIO_IO, SC },
#endif
#ifdef _SC_PRIORITIZED_IO
	{ "_posix_prioritized_io", _SC_PRIORITIZED_IO, SC },
#endif
#ifdef _SC_PRIORITY_SCHEDULING
	{ "_posix_priority_scheduling", _SC_PRIORITY_SCHEDULING, SC },
#endif
#ifdef _SC_REALTIME_SIGNALS
	{ "_posix_realtime_signals", _SC_REALTIME_SIGNALS, SC },
#endif
#ifdef _SC_RTSIG_MAX
	{ "_posix_rtsig_max", _SC_RTSIG_MAX, SC },
#endif
	{ "_posix_saved_ids", _SC_SAVED_IDS, SC },
#ifdef _SC_SEM_NSEMS_MAX
	{ "_posix_sem_nsems_max", _SC_SEM_NSEMS_MAX, SC },
#endif
#ifdef _SC_SEM_VALUE_MAX
	{ "_posix_sem_value_max", _SC_SEM_VALUE_MAX, SC },
#endif
#ifdef _SC_SEMAPHORES
	{ "_posix_semaphores", _SC_SEMAPHORES, SC },
#endif
#ifdef _SC_SHARED_MEMORY_OBJECTS
	{ "_posix_shared_memory_objects", _SC_SHARED_MEMORY_OBJECTS, SC },
#endif
#ifdef _SC_SIGQUEUE_MAX
	{ "_posix_sigqueue_max", _SC_SIGQUEUE_MAX, SC },
#endif
#ifdef _SC_SSIZE_MAX
	{ "_posix_ssize_max", _SC_SSIZE_MAX, SC },
#endif
	{ "_posix_stream_max", _SC_STREAM_MAX, SC },
#ifdef _PC_SYNC_IO
	{ "_posix_sync_io", _PC_SYNC_IO, SC },
#endif
#ifdef _SC_SYNCHRONIZED_IO
	{ "_posix_synchronized_io", _SC_SYNCHRONIZED_IO, SC },
#endif
#ifdef _SC_TIMER_MAX
	{ "_posix_timer_max", _SC_TIMER_MAX, SC },
#endif
#ifdef _SC_TIMERS
	{ "_posix_timers", _SC_TIMERS, SC },
#endif
	{ "_posix_tzname_max", _SC_TZNAME_MAX, SC },
	{ "_posix_vdisable", _PC_VDISABLE, PC },
	{ "_posix_version", _SC_VERSION, SC },
#ifdef _SC_AIO_LISTIO_MAX
	{ "aio_listio_max", _SC_AIO_LISTIO_MAX, SC },
#endif
#ifdef _SC_AIO_MAX
	{ "aio_max", _SC_AIO_MAX, SC },
#endif
#ifdef _SC_AIO_PRIO_DELTA_MAX
	{ "aio_prio_delta_max", _SC_AIO_PRIO_DELTA_MAX, SC },
#endif
	{ "arg_max", _SC_ARG_MAX, SC },
	{ "bc_base_max", _SC_BC_BASE_MAX, SC },
	{ "bc_dim_max", _SC_BC_DIM_MAX, SC },
	{ "bc_scale_max", _SC_BC_SCALE_MAX, SC },
	{ "bc_string_max", _SC_BC_STRING_MAX, SC },
	{ "child_max", _SC_CHILD_MAX, SC },
	{ "clk_tck", _SC_CLK_TCK, SC },
	{ "coll_weights_max", _SC_COLL_WEIGHTS_MAX, SC },
#ifdef _SC_DELAYTIMER_MAX
	{ "delaytimer_max", _SC_DELAYTIMER_MAX, SC },
#endif
	{ "expr_nest_max", _SC_EXPR_NEST_MAX, SC },
	{ "line_max", _SC_LINE_MAX, SC },
	{ "link_max", _PC_LINK_MAX, PC },
	{ "max_canon", _PC_MAX_CANON, PC },
	{ "max_input", _PC_MAX_INPUT, PC },
#ifdef _SC_MQ_OPEN_MAX
	{ "mq_open_max", _SC_MQ_OPEN_MAX, SC },
#endif
#ifdef _SC_MQ_PRIO_MAX
	{ "mq_prio_max", _SC_MQ_PRIO_MAX, SC },
#endif
	{ "name_max", _PC_NAME_MAX, PC },
	{ "ngroups_max", _SC_NGROUPS_MAX, SC },
	{ "open_max", _SC_OPEN_MAX, SC },
#ifdef _SC_PAGESIZE
	{ "pagesize", _SC_PAGESIZE, SC },
#endif
	{ "path", _CS_PATH, CS },
	{ "path_max", _PC_PATH_MAX, PC },
	{ "pipe_buf", _PC_PIPE_BUF, PC },
	{ "posix2_bc_base_max", _SC_BC_BASE_MAX, SC },
	{ "posix2_bc_dim_max", _SC_BC_DIM_MAX, SC },
	{ "posix2_bc_scale_max", _SC_BC_SCALE_MAX, SC },
	{ "posix2_bc_string_max", _SC_BC_STRING_MAX, SC },
	{ "posix2_c_bind", _SC_2_C_BIND, SC },
	{ "posix2_c_dev", _SC_2_C_DEV, SC },
	{ "posix2_char_term", _SC_2_CHAR_TERM, SC },
	{ "posix2_coll_weights_max", _SC_COLL_WEIGHTS_MAX, SC },
	{ "posix2_expr_nest_max", _SC_EXPR_NEST_MAX, SC },
	{ "posix2_fort_dev", _SC_2_FORT_DEV, SC },
	{ "posix2_fort_run", _SC_2_FORT_RUN, SC },
	{ "posix2_line_max", _SC_LINE_MAX, SC },
	{ "posix2_localedef", _SC_2_LOCALEDEF, SC },
	{ "posix2_re_dup_max", _SC_RE_DUP_MAX, SC },
	{ "posix2_sw_dev", _SC_2_SW_DEV, SC },
	{ "posix2_upe", _SC_2_UPE, SC },
	{ "posix2_version", _SC_2_VERSION, SC },
	{ "re_dup_max", _SC_RE_DUP_MAX, SC },
#ifdef _SC_RTSIG_MAX
	{ "rtsig_max", _SC_RTSIG_MAX, SC },
#endif
#ifdef _SC_SEM_NSEMS_MAX
	{ "sem_nsems_max", _SC_SEM_NSEMS_MAX, SC },
#endif
#ifdef _SC_SEM_VALUE_MAX
	{ "sem_value_max", _SC_SEM_VALUE_MAX, SC },
#endif
#ifdef _SC_SIGQUEUE_MAX
	{ "sigqueue_max", _SC_SIGQUEUE_MAX, SC },
#endif
	{ "stream_max", _SC_STREAM_MAX, SC },
#ifdef _SC_TIMER_MAX
	{ "timer_max", _SC_TIMER_MAX, SC },
#endif
	{ "tzname_max", _SC_TZNAME_MAX, SC },
	{ NULL }
};

int	aflag, stdinflag;

int	 display __P((char *, LIST *, char *, int));
LIST	*search __P((char *));
int	 lcompare __P((const void *, const void *));
__dead void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	LIST *lp;
	int ch, eval;

	while ((ch = getopt(argc, argv, "a")) != EOF) {
		switch (ch) {
		case 'a':
			aflag = 1;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	eval = 0;
	switch (argc) {
	case 0:				/* getconf -a */
		if (aflag) {
			for (lp = &list[0]; lp->name != NULL; ++lp)
				if (display(lp->name, lp, NULL, 1))
					eval = 1;
		} else
			usage();
		break;
	case 1:
		if (aflag) {		/* getconf -a path */
			stdinflag = strcmp(argv[0], "-") == 0;
			for (lp = &list[0]; lp->name != NULL; ++lp)
				if (display(lp->name, lp, argv[0], 1))
					eval = 1;
		} else			/* getconf system_variable */
			if (display(argv[0], NULL, NULL, 0))
				eval = 1;
		break;
	case 2:				/* getconf path_variable path */
		stdinflag = strcmp(argv[1], "-") == 0;
		if (display(argv[0], NULL, argv[1], 0))
			eval = 1;
		break;
	default:
		usage();
		/* NOTREACHED */
	}
	exit(eval);
}

/*
 * display --
 *	Optionally search for and display value.
 */
int
display(name, lp, path, all)
	char *name, *path;
	LIST *lp;
	int all;
{
	long value;
	char buf[2048];

	if (lp == NULL && (lp = search(name)) == NULL)
		return (1);

	switch (lp->type) {
	case CS:
		value = confstr(lp->indx, buf, sizeof(buf));
		switch (value) {
		case -1:
			break;
		case 0:
			/*
			 * XXX
			 * Confstr returns 0 if there's no configuration
			 * defined value.
			 */
			if (all)
				printf("%s = ", name);
			printf("<undefined>\n");
			return (0);
		default:
			if (all)
				printf("%s = ", name);
			printf("%s\n", buf);
			return (0);
		}
		break;
	case PC:
		errno = 0;
		value = stdinflag ?
		    fpathconf(STDIN_FILENO, lp->indx) :
		    pathconf(path == NULL ? "/" : path, lp->indx);
		switch (value) {
		case -1:
			/*
			 * XXX
			 * Pathconf doesn't modify errno if there's no limit,
			 * and getconf doesn't appear to have any way to deal
			 * with that.  Use the maximum integer for now.
			 */
			if (errno == 0) {
				if (all)
					printf("%s = ", name);
				printf("%d\n", INT_MAX);
				return (0);
			}
			/*
			 * XXX
			 * Pathconf sets EINVAL if the functionality isn't
			 * supported for the specified file.  Ignore those
			 * errors if listing all values.
			 */
			if (all && errno == EINVAL)
				return (0);
			break;
		default:
			if (all)
				printf("%s = ", name);
			printf("%ld\n", value);
			return (0);
		}
		break;
	case SC:
		errno = 0;
		value = sysconf(lp->indx);
		switch (value) {
		case -1:
			/*
			 * XXX
			 * Sysconf doesn't modify errno if functionality
			 * isn't supported.
			 */
			if (errno == 0) {
				if (all)
					printf("%s = ", name);
				printf("%d\n", 0);
				return (0);
			}
			break;
		default:
			if (all)
				printf("%s = ", name);
			printf("%ld\n", value);
			return (0);
		}
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	if (path == NULL)
		warn("%s", name);
	else
		warn("%s: %s", name, path);
	return (1);
}

/*
 * search --
 *	Search for a particular name.
 */
LIST *
search(name)
	char *name;
{
	LIST *lp, tmp;

	tmp.name = name;
	lp = bsearch(&tmp, list,
	    sizeof(list) / sizeof(list[0]), sizeof(list[0]), lcompare);
	if (lp == NULL) {
		warnx("unknown name %s", name);
		return (NULL);
	}
	return (lp);
}

/*
 * lcompare --
 *	List comparison routine.
 */
int
lcompare(a, b)
	const void *a, *b;
{
	return (strcmp(((LIST *)a)->name, ((LIST *)b)->name));
}

__dead void
usage()
{

	(void)fprintf(stderr, "usage:%s%s%s",
	    "\tgetconf -a [path]\n",
	    "\tgetconf system_variable\n",
	    "\tgetconf path_variable path\n");
	exit(1);
}
