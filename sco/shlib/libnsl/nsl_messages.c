/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI nsl_messages.c,v 2.1 1995/02/03 15:19:13 polk Exp
 */

#include "nsl_compat.h"

/*
 * Before release need to go through and change messages that
 * will never be returned
 */

static char *_t_errlist[] = {
    "No error",
    "Error in address format",
    "Error in option format",
    "Permissioned denied",
    "Bad file descriptor",
    "Could not allocate address",
    "Out of state",
    "Bad call sequence number",
    "System error",
    "Event requires attention",
    "Illegal amount of data",
    "Buffer too small",
    "Flow control",
    "No data available",
    "discon_ind not found",
    "unitdata error not found",
    "Bad flags",
    "no ord rel found on q",
    "Unsupported operation",
    "state is in process of changing",
};

tli_initerr()
{
	t_errlist = _t_errlist;
	t_nerr = sizeof (_t_errlist) / sizeof (_t_errlist[1]);
}

