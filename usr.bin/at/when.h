/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI when.h,v 2.1 1995/02/03 08:41:28 polk Exp
 */

time_t	when(int *argc, char **argv[]);
void	print_time(time_t t);
void	print_tm(struct tm *tp);

/* lex data */
extern	int	yy_ac;
extern	char	**yy_av;
extern	char	*parsed_buf;
extern	int	parsed_eof;

/* parser data */
extern	struct	tm time_v;
extern	struct	tm gm_time;
extern	struct	tm lc_time;
extern	time_t	now;
extern	int	zulu_offset;
