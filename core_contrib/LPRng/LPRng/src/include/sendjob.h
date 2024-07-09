/***************************************************************************
 * LPRng - An Extended Print Spooler System
 *
 * Copyright 1988-2000, Patrick Powell, San Diego, CA
 *     papowell@astart.com
 * See LICENSE for conditions of use.
 * $Id: sendjob.h,v 5.4 2000/04/14 20:40:33 papowell Exp papowell $
 ***************************************************************************/



#ifndef _SENDJOB_1_
#define _SENDJOB_1_ 1

/* PROTOTYPES */
int Send_job( struct job *job, struct job *logjob,
	int connect_timeout_len, int connect_interval, int max_connect_interval,
	int transfer_timeout );
int Send_normal( int *sock, struct job *job, struct job *logjob, int transfer_timeout, int block_fd );
int Send_control( int *sock, struct job *job, struct job *logjob, int transfer_timeout,
	int block_fd );
int Send_data_files( int *sock, struct job *job, struct job *logjob, int transfer_timeout,
	int block_fd );
int Send_block( int *sock, struct job *job, struct job *logjob, int transfer_timeout );

#endif
