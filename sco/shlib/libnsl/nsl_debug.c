/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI nsl_debug.c,v 2.2 1997/10/31 03:14:05 donn Exp
 */

#include "nsl_compat.h"
#include "nsl_debug.h"
#include <stdarg.h>
#include <stdlib.h>
#include <sys/exec.h>
#include <sys/param.h>
#include <machine/vmlayout.h>

int nsl_debug;
char **environ;

dbg_enter(char *routine, ...)
{
	char buffer [200];
	va_list ap;
	void (*printer)();
	void *data;

	if (environ == NULL) {
		/* XXX !!! */
		struct ps_strings *ps = (struct ps_strings *)
		    (0x80000000 - sizeof (struct ps_strings));

		environ = ps->ps_envp;
		data = getenv("NSL_DEBUG");
		if (data)
			nsl_debug = 1;
		else
			nsl_debug = -1;
	}
	if (nsl_debug < 0)
		return;
	sprintf(buffer, "tli:%d: function %s \n", getpid(), routine);
	write(2, buffer, strlen(buffer));
	va_start(ap, routine);

	sprintf(buffer, "tli:%d: ",getpid());
	while ((printer = (void(*))va_arg(ap, int)) != NULL) {
		data = va_arg(ap, void *);
		(*printer)(buffer, data);
	}
	va_end(ap);
}

dbg_exit(char *routine, int retval, ...)
{
	char buffer [200];
	va_list ap;
	void (*printer)();
	void *data;

	if (nsl_debug < 0)
		return;


	sprintf(buffer, "tli:%d: exit %s %d, %d %d:%d \n",
		getpid(), routine, retval, t_errno, errno,
		sco__errno ? *sco__errno : *ibc__errno);
	write(2, buffer, strlen(buffer));

	va_start(ap, retval);
	sprintf(buffer, "tli:%d: ",getpid());
	while ((printer = (void(*))va_arg(ap, int)) != NULL) {
		data = va_arg(ap, void *);
		(*printer)(buffer, data);
	}
	va_end(ap);
	return;
}

void
phex(char *pres, u_int *value)
{
	char buffer [200];

	sprintf(buffer, "%s %x\n", pres, *value);
	write (2, buffer, strlen(buffer));
}
 
void
pstring(char *pres, char *string) {
	char buffer [200];

	sprintf(buffer, "%s %s\n", pres, string);
	write (2, buffer, strlen(buffer));
}

#define BS(b, one, two) strcpy((b), (one)); strcat((b), (two))

void
pnetbuf(char *pres, netbuf_t *n)
{
	char buffer[200];
	char hexit[100];
	u_long i,j;
	struct sockaddr_in *s;

	if (n == NULL) {
		pstring(pres, "netbuf = NULL");
		return;
	}
	BS(buffer, pres, "nb.maxlen"); 
	phex(buffer, &n->maxlen);
	BS(buffer, pres, "nb.len"); 
	phex(buffer, &n->len);
	if (n->len == 0)
		return;
	if (n->buf == NULL) {
		pstring(pres, "nb.addr = NULL");
		return;
	}

	for (i = 0; i < 24 && i < n->len; i++) {
		j = (u_char)(n->buf[i]);
		sprintf(&hexit[i * 2], "%02x", j);
	}

	s = (struct sockaddr_in*)n->buf;
	BS(buffer, pres, "nb.buf");
	phex(buffer, (u_int*)&n->buf);
	pstring(buffer, hexit);
}

void
pt_call(char *pres, call_t *c)
{
	char buffer[200];

	if (c == NULL) {
		pstring(pres, "t_call = NULL");
		return;
	}
	BS(buffer, pres, "c.addr."); 
	pnetbuf(buffer, &c->addr);
	BS(buffer, pres, "call.opt."); 
	pnetbuf(buffer, &c->opt);
	BS(buffer, pres, "call.udata."); 
	pnetbuf(buffer, &c->udata);
	BS(buffer, pres, "call.sequence"); 
	phex(buffer, (u_int*)&c->sequence);
}

void
pt_bind(char *pres, bind_t *b)
{
	char buffer[200];

	if (b == NULL) {
		pstring(pres, "t_bind = NULL");
		return;
	}
	BS(buffer, pres, "bind.addr."); 
	pnetbuf(buffer, &b->addr);
	BS(buffer, pres, "bind.qlen"); 
	phex(buffer, (u_int*)&b->qlen);
}


void
pt_optmgmt(char *pres, optmgmt_t *o)
{
	char buffer[200];

	if (o == NULL) {
		pstring(pres, "t_optmgmt = NULL");
		return;
	}
	BS(buffer, pres, "optmgmt.opt."); 
	pnetbuf(buffer, &o->opt);
	BS(buffer, pres, "optmgmt.flags"); 
	phex(buffer, (u_int*)&o->flags);
}

void
pt_info(char *pres, info_t *i)
{
	char buffer[200];

	if (i == NULL) {
		pstring(pres, "t_info = NULL");
		return;
	}
	BS(buffer, pres, "info.addr"); 
	phex(buffer, (u_int*)&i->addr);
	BS(buffer, pres, "info.options"); 
	phex(buffer, (u_int*)&i->options);
	BS(buffer, pres, "info.tsdu"); 
	phex(buffer, (u_int*)&i->tsdu);
	BS(buffer, pres, "info.etsdu"); 
	phex(buffer, (u_int*)&i->etsdu);
	BS(buffer, pres, "info.connect"); 
	phex(buffer, (u_int*)&i->connect);
	BS(buffer, pres, "info.discon"); 
	phex(buffer, (u_int*)&i->discon);
	BS(buffer, pres, "info.servtype"); 
	phex(buffer, (u_int*)&i->servtype);
}

void
pt_discon(char *pres, discon_t *d)
{
	char buffer[200];

	if (d == NULL) {
		pstring(pres, "t_discon = NULL");
		return;
	}
	BS(buffer, pres, "discon.udata."); 
	pnetbuf(buffer, &d->udata);
	BS(buffer, pres, "discon.reason"); 
	phex(buffer, (u_int*)&d->reason);
	BS(buffer, pres, "discon.sequence"); 
	phex(buffer, (u_int*)&d->sequence);
}

void
pt_unitdata(char *pres, unitdata_t *u)
{
	char buffer[200];

	if (u == NULL) {
		pstring(pres, "t_unitdata = NULL");
		return;
	}
	BS(buffer, pres, "unitdata.addr."); 
	pnetbuf(buffer, &u->addr);
	BS(buffer, pres, "unitdata.opt."); 
	pnetbuf(buffer, &u->opt);
	BS(buffer, pres, "unitdata.udata."); 
	pnetbuf(buffer, &u->udata);

}

void
pt_uderr(char *pres, uderr_t *u)
{
	char buffer[200];

	if (u == NULL) {
		pstring(pres, "t_uderr = NULL");
		return;
	}
	BS(buffer, pres, "uderr.addr."); 
	pnetbuf(buffer, &u->addr);
	BS(buffer, pres, "uderr.opt."); 
	pnetbuf(buffer, &u->opt);
	BS(buffer, pres, "unitdata.error."); 
	phex(buffer, (u_int*)&u->error);
}

void
psockaddr_in(char *pres, struct sockaddr_in *in)
{
	char buffer[200];
	u_int i;
	BS(buffer, pres, "sin.len"); 
	i = in->sin_len;
	phex(buffer, &i);
	BS(buffer, pres, "sin.family"); 
	i = in->sin_family;
	phex(buffer, &i);
	BS(buffer, pres, "sin.port"); 
	i = in->sin_port;
	phex(buffer, &i);
	BS(buffer, pres, "sin.addr"); 
	i = in->sin_addr.s_addr;
	phex(buffer, &i);
}
