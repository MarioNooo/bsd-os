/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI phone.h,v 1.1 1996/05/29 19:30:58 prb Exp
 */
typedef struct rule_t rule_t;

rule_t	*phone_ruleset(char *);
char	*phone_variable(char *);
char	*phone_variable_set(char *, char *);
char	*process_number(char *, char *, int, rule_t *);
int	 parse_rule_file(char *);
