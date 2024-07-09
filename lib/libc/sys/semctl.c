/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 * BSDI semctl.c,v 2.1 1996/07/01 18:51:06 mdickson Exp 
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

int 
semctl(semid, semnum, cmd, semun)
	int semid, semnum;
	int cmd;
	union semun semun;
{
	return (__semctl(semid, semnum, cmd, &semun));
}
