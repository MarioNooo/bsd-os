/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI include.h,v 1.1 1997/08/27 17:11:16 prb Exp
 */
void *start_auth(int);
void *init_auth(char *, char *);
int fd_auth(void *);
u_long random_auth();
int read_auth(void *, char *, int);
int send_auth(void *, char *);
int cleanstring(char *);
void encodestring(char *, char *, int);
int decodestring(char *);
int GetPty(char *);
void GetPtySlave(char *, int); 

extern FILE *trace;
extern int traceall;
