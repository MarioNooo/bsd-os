/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI argv.c,v 1.3 1997/10/16 17:04:32 prb Exp
 */
#include <sys/types.h>
#include <libdialer.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

argv_t *
start_argv(char *arg, ...)
{
	va_list ap;

	argv_t *avt = (argv_t *)malloc(sizeof(argv_t));

	if (avt == 0)
		return(0);

	avt->argc = 0;
	avt->max_argc = 32;
	avt->argv = (char **)malloc(sizeof(char *) * avt->max_argc);
	if (avt->argv == 0) {
		free(avt);
		return(0);
	}
	avt->argv[0] = 0;

	va_start(ap, arg);

	while (arg) {
		if (add_argv(avt, arg) == 0) {
			free_argv(avt);
			avt = 0;
			break;
		}
		arg = va_arg(ap, char *);
	}
	va_end(ap);
	return(avt);
}

char *
add_argv(argv_t *avt, char *arg)
{
	char **av;

	if (avt->argc + 1 >= avt->max_argc) {
		avt->max_argc += 16;
		av = (char **)realloc(avt->argv, sizeof(char *)*avt->max_argc);
		if (av == 0) {
			avt->max_argc -= 16;
			return(0);
		}
	}

	if ((avt->argv[avt->argc] = strdup(arg)) != NULL) {
		avt->argv[avt->argc + 1] = 0;
		return(avt->argv[avt->argc++]);
	}
	return(0);
}

char *
add_argvn(argv_t *avt, char *arg, int n)
{
	char **av;

	if (avt->argc + 1 >= avt->max_argc) {
		avt->max_argc += 16;
		av = (char **)realloc(avt->argv, sizeof(char *)*avt->max_argc);
		if (av == 0) {
			avt->max_argc -= 16;
			return(0);
		}
	}

	if ((avt->argv[avt->argc] = malloc(n + 1)) != NULL) {
		memcpy(avt->argv[avt->argc], arg, n);
		avt->argv[avt->argc][n] = 0;
		avt->argv[avt->argc + 1] = 0;
		return(avt->argv[avt->argc++]);
	}
	return(0);
}

void
free_argv(argv_t *avt)
{
	while (avt->argc-- > 0)
		free(avt->argv[avt->argc]);
	free(avt->argv);
	free(avt);
}
