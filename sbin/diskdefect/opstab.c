/*
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#include <sys/types.h>
#include <sys/disklabel.h>

#include "diskdefect.h"
#include "bbops.h"

/*
 * Bad-sector operations table.
 * Should probably be built automatically, but for now...
 */

extern struct bbops null_ops;
extern struct bbops bad144_ops;
extern struct bbops scsi_ops;

struct bbops *bbops[] = {
	&null_ops,
	&bad144_ops,
	&scsi_ops,
};
int	nbbops = sizeof bbops / sizeof *bbops;
