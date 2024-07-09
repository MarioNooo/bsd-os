/***************************************************************************
 * LPRng - An Extended Print Spooler System
 *
 * Copyright 1988-2000, Patrick Powell, San Diego, CA
 *     papowell@astart.com
 * See LICENSE for conditions of use.
 * $Id: printjob.h,v 5.4 2000/04/14 20:40:32 papowell Exp papowell $
 ***************************************************************************/



#ifndef _PRINTJOB_H_
#define _PRINTJOB_H_ 1

/* PROTOTYPES */
int Wait_for_pid( int of_pid, char *name, int suspend, int timeout,
	plp_status_t *ps_status);
int Print_job( int output, struct job *job, int timeout );
void Print_banner( char *name, char *pgm, struct job *job );
int Write_outbuf_to_OF( struct job *job, char *title,
	int of_pid, int of_fd, int of_error,
	char *buffer, int outlen,
	char *msg, int msgmax,
	int timeout, int suspend, int max_wait, int *exit_status,
	plp_status_t *ps_status);

#endif
