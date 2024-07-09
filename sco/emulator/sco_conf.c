/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI sco_conf.c,v 2.1 1995/02/03 15:14:04 polk Exp
 */

#include <sys/param.h>
#include <unistd.h>
#include "emulate.h"
#include "sco.h"

/*
 * Support for pathconf/sysconf emulation.
 */

/* from iBCS2 p 6-83 */
const int pathconf_in_map[] = {
	_PC_LINK_MAX,
	_PC_MAX_CANON,
	_PC_MAX_INPUT,
	_PC_NAME_MAX,
	_PC_PATH_MAX,
	_PC_PIPE_BUF,
	_PC_CHOWN_RESTRICTED,
	_PC_NO_TRUNC,
	_PC_VDISABLE,
};
const int max_pathconf_in_map = sizeof (pathconf_in_map) / sizeof (int);

/* from iBCS2 p 6-82 */
const int sysconf_in_map[] = {
	_SC_ARG_MAX,
	_SC_CHILD_MAX,
	_SC_CLK_TCK,
	_SC_NGROUPS_MAX,
	_SC_OPEN_MAX,
	_SC_JOB_CONTROL,
	_SC_SAVED_IDS,
	_SC_VERSION,
};
const int max_sysconf_in_map = sizeof (sysconf_in_map) / sizeof (int);
