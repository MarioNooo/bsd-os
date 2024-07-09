/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI init.c,v 1.2 1996/09/05 23:17:06 prb Exp
 */
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "token.h"
#include "tokendb.h"

static struct token_types types[] = {
	{ "activ", "ActivCard", "/etc/activ.db", "012345",
	    TOKEN_HEXINIT,
	    TOKEN_DECMODE | TOKEN_HEXMODE,			/* avail */
	    TOKEN_HEXMODE },					/* default */
	{ "crypto", "CRYPTOCard", "/etc/crypto.db", "012345",
	    TOKEN_HEXINIT | TOKEN_PHONE,
	    TOKEN_DECMODE | TOKEN_HEXMODE | TOKEN_PHONEMODE | TOKEN_RIM,
	    TOKEN_HEXMODE },					/* default */
	{ "snk", "SNK 004", "/etc/snk.db", "222333",
	    0,
	    TOKEN_DECMODE | TOKEN_HEXMODE,			/* avail */
	    TOKEN_DECMODE },					/* default */
	{ "token", "X9.9 Token", "/etc/x99token.db", "012345",
	    TOKEN_HEXINIT,
	    TOKEN_DECMODE | TOKEN_HEXMODE | TOKEN_RIM,		/* avail */
	    TOKEN_HEXMODE },					/* default */
};

static struct {
	char	*name;
	u_int	value;
} modes[] = {
	"hexadecimal",	TOKEN_HEXMODE,
	"hex",		TOKEN_HEXMODE,
	"decimal",	TOKEN_DECMODE,
	"dec",		TOKEN_DECMODE,
	"phonebook",	TOKEN_PHONEMODE,
	"phone",	TOKEN_PHONEMODE,
	"reduced-input",TOKEN_RIM,
	"rim",		TOKEN_RIM,
};

int
token_init(char *path)
{
	char *p;
	int i;

	if ((p = strrchr(path, '/')) && p[1] != '\0')
		path = p + 1;

	for (i = 0; i < sizeof(types)/sizeof(types[0]); ++i)
		if (strstr(path, types[i].name) != NULL) {
			tt = &types[i];
			return (0);
		}
	if ((p = strstr(path, "token")) != NULL) {
		fprintf(stderr, "Please invoke as one of:");
		for (i = 0; i < sizeof(types)/sizeof(types[0]); ++i)
			fprintf(stderr, " %.*s%s%s",
			    p - path, path, types[i].name, p + 5);
		fprintf(stderr, "\n");
		exit(1);

	}
	return (-1);
}

u_int
token_mode(char *mode)
{
	int i;

	for (i = 0; i < sizeof(modes)/sizeof(modes[0]); ++i)
		if (strstr(mode, modes[i].name) != NULL)
			return (tt->modes & modes[i].value);
				return (0);
	return (0);
}

char *
token_getmode(u_int mode)
{
	int i;
	static char buf[32];

	for (i = 0; i < sizeof(modes)/sizeof(modes[0]); ++i)
		if (mode == modes[i].value)
			return(modes[i].name);
	snprintf(buf, sizeof(buf), "0x%x", mode);
	return(buf);
}
