/*
 * Copyright (c) 1994
 * Berkeley Software Design Inc.  All Rights Reserved.
 *
 *	BSDI template.h,v 2.2 2002/04/24 17:34:42 prb Exp
 */

/*
 * Some files (Makefile, ioconf.c) are built from source templates.
 * Lines beginning (after optional whitespace) with `%' are treated
 * as directives, and result in calls to functions that substitute the
 * appropriate text, whatever that may be.
 */
struct template {
	char	*t_name;
	int	t_minargs, t_maxargs;
	int	(*t_fn) __P((FILE *ofp));
};

/*
 * When a template function is running, it can get at `extra' info in
 * this global variable.  Yes, gross, but it keeps the clutter down.
 */
struct templateinfo {
	char	*ti_ifname;	/* input file name */
	int	ti_lineno;	/* line number in same */
	int	ti_argc;	/* argument count */
	const char **ti_argv;	/* arguments */
};
extern struct templateinfo templateinfo;

/*
 * Given a base file name (Makefile, ioconf.c), build that file
 * from the template.  The template functions are in a sorted list
 * (suitable for use in bsearch()).  The given function f is called
 * to start things off, before applying the template.  If `flags'
 * has T_SIMPLE, templates never have arguments and must appear with no
 * leading whitespace.  Otherwise, template calls must have t_nargs
 * arguments (e.g., %FOO(a, b)); these arguments are passed to the
 * template function as a vector.  Any output is left in-stream and
 * the rest of the line (following the close paren after args, if any)
 * is processed for further expansions, and so on.
 *
 * If T_OPTIONAL is set then it is not an error if the template does
 * not exist.
 *
 * (The template file's source line number is also passed to the
 * callee.)
 *
 * Each of the called functions should return 0 for success, nonzero
 * for output write error.
 */
int	template __P((const char *fname, int (*f)(FILE *), int simple,
		const struct template *templates, int ntemplates));

#define	T_SIMPLE	0x01
#define	T_OPTIONAL	0x02
