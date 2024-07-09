/***************************************************************************
 * LPRng - An Extended Print Spooler System
 *
 * Copyright 1988-2000, Patrick Powell, San Diego, CA
 *     papowell@astart.com
 * See LICENSE for conditions of use.
 * $Id: lpd.h,v 5.5 2000/04/14 20:40:19 papowell Exp papowell $
 ***************************************************************************/



#ifndef _LPD_H_
#define _LPD_H_ 1


/*
 * file_info - information in log file
 */

struct file_info {
	int fd;
	int max_size;	/* maximum file size */ 
	char *outbuffer;	/* for writing */
	int outmax;	/* max buffer size */
	char *inbuffer;		/* buffer for IO */
	int  inmax;	/* buffer size */
	int start;			/* starting offset */
	int count;			/* total size of info */
};

union val{
	int v;
	char s[sizeof(int)];
};


struct call_list{
	const char * *id;
	void (*p)( struct line_list *args );
};

extern struct call_list Calls[];

EXTERN int Server_pid;		/* PID of server */
EXTERN int Lpd_request;		/* pipe to server */
EXTERN int Foreground_LPD;
EXTERN int Worker_LPD;
EXTERN char *Logfile_LPD;
EXTERN int Reread_config;
EXTERN int Started_server;
EXTERN int Logger_fd;
EXTERN int Mail_fd;

/* PROTOTYPES */
int main(int argc, char *argv[], char *envp[]);
void Setup_log(char *logfile );
void Service_connection( struct line_list *args );
void Dispatch_input(int *talk, char *input );
void Reinit(void);
int Get_lpd_pid(void);
void Set_lpd_pid(void);
int Read_server_status( int fd );
void usage(void);
void Get_parms(int argc, char *argv[] );
void Setup_lpd_call( struct line_list *passfd, struct line_list *args );
int Make_lpd_call( struct line_list *passfd, struct line_list *args );
void Do_work( struct line_list *args );
void Lpd_worker( char **argv, int argc, int optindv  );
int Start_logger( int log_fd );
int Start_worker( struct line_list *parms, int fd  );
int Start_all( void );
void Service_all( struct line_list *args );
void Service_queue( struct line_list *args );
void Service_log( struct line_list *args );
plp_signal_t sigchld_handler (int signo);
void Setup_waitpid (void);
void Setup_waitpid_break (void);
void Fork_error( int fork_failed );

#endif
