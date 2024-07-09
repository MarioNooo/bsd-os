/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI nsl_compat.h,v 2.1 1995/02/03 15:18:47 polk Exp
 */


#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include "tiuser.h"

extern int t_errno;
extern int t_nerr;
extern char **t_errlist;
extern void (*__memcpy)(void *dst, void *src, size_t len);
extern void *memalloc(size_t size);
extern int (*__strlen)(const char *s);
extern int *sco__errno;
extern int *ibc__errno;


typedef struct ts {
	struct	ts *ts_next;	/* chain pointer */
	int	ts_fd;		/* file discriptor this struc is for */
	u_char	ts_event;	/* some even seen for which tlook was given */
	u_char	ts_domain;
	u_char	ts_protocol;
	u_char	ts_state;
	u_char	ts_eof;		/* eof seen on input */
	u_char	ts_unuseable;	/* This descriptor is not useable. For
				 * instance it has been shutdown and
				 * so goes to idle state, but we can't
				 * do a accept on it */
} ts_t;

typedef struct t_info t_info_t;
typedef struct netbuf netbuf_t;
typedef struct t_bind t_bind_t;
typedef struct t_optmgmt t_optmgmt_t;
typedef struct t_discon t_discon_t;
typedef struct t_call t_call_t;
typedef struct t_unitdata t_unitdata_t;
typedef struct t_uderr t_uderr_t;

typedef struct t_info	    info_t;
typedef struct t_bind	    bind_t;
typedef struct t_optmgmt    optmgmt_t;
typedef struct t_discon	    discon_t;
typedef struct t_call	    call_t;
typedef struct t_unitdata   unitdata_t;
typedef struct t_uderr	    uderr_t;






ts_t *ts_list;

extern ts_t *ts_get(int fd);
extern void nsl_maperrno();
extern int sock_in(netbuf_t*, struct sockaddr*, int, int);
extern int sock_out(netbuf_t*, struct sockaddr*);
