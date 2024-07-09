/*-
 *  Copyright (c) 1996,1997 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI types.h,v 1.4 1999/07/28 01:31:56 prb Exp
 */
typedef struct cmd_t {
	char		*cmd;		/* Command to issue */
	int		line;		/* Line number from source file */
	char		*file;		/* File name of source file */
	int		*depedent;	/* Only issue if dependent defined */
	int		*tnedeped;	/* Do not issue if tnedeped defined */
	struct cmd_t	*next;
} cmd_t;

typedef struct jmp_t {
	int		where;
	struct jmp_t	*next;
} jmp_t;

typedef struct node_t {
	cmd_t		*cmds;
	jmp_t		*true;
	jmp_t		*false;
} node_t;

#define	False	false->where
#define	True	true->where

typedef struct range_t {
	char		type;
	char		size;
	u_long		min;
	u_long		max;
	u_long		mask;
	q128_t		qmin;
	q128_t		qmax;
	cmd_t		*iface;
	struct range_t	*next;
	cmd_t		*cmds;
} range_t;

typedef struct filter_t {
	char		*name;
	int		usecnt;
	cmd_t		*cmds;
	jmp_t		*label;
	struct filter_t	*next;
	int		def;
} filter_t;

typedef struct var_t {
	char		*name;
	int		location;
	int		state;
	int		used;
	struct var_t	*next;
} var_t;

typedef struct switch_t {
	int		state;
	struct switch_t	*next;
} switch_t;
