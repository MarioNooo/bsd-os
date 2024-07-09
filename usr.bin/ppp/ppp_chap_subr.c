/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI ppp_chap_subr.c,v 2.6 1997/11/11 19:02:39 chrisk Exp
 */
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <net/if.h>
#include <net/if_dl.h>

#include <err.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include "pbuf.h"
#include "ppp.h"
#include "ppp_proto.h"
#include "ppp_var.h"


typedef struct chal_t {
	struct chal_t *next;
	int id;
	int length;
	u_char challange[1];
} chal_t;

static chal_t *challanges = 0;

static void
addchallange(int id, u_char *chal, int len)
{
	chal_t *ch = malloc(sizeof(chal_t) + len);
	if (ch == 0)
		err(1, "allocating challange structure");
	ch->id = id;
	ch->length = len;
	memcpy(ch->challange, chal, len);
	ch->next = challanges;
	challanges = ch;
}

static chal_t *
findchallange(int id)
{
	chal_t *ch = challanges;

	while (ch && ch->id != id)
		ch = ch->next;
	return(ch);
}

char *
chap_challange(ppp_t *ppp, int id, pbuf_t *m)
{
	int sv[2];
	int pid;
	int status;
	u_char *p;

	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, sv) < 0)
		return("could not create socketpair");

	switch(pid = fork()) {
	case -1:
		close(sv[0]);
		close(sv[1]);
		return("could not fork");
	case 0:
		close(sv[1]);
		if (sv[0] != 0) {
			close(0);
			dup2(sv[0], 0);
			close(sv[0]);
		}
		dup2(0, 1);
		dup2(0, 2);
		execl(ppp->ppp_chapscript, "ppp-chap", (char *)NULL);
		err(1, ppp->ppp_chapscript);
	default:
		close(sv[0]);
		p = pbuf_end(m);
		while ((status = read(sv[1], pbuf_end(m), pbuf_left(m))) > 0)
			pbuf_append(m, status);
		close(sv[1]);

		if (waitpid(pid, &status, 0) < 0)
			return("waitpid failed");
		if (!WIFEXITED(status))
			return("authentication denied (script failure)");
		if (WEXITSTATUS(status))
			return("authentication denied");
		addchallange(id, p, pbuf_end(m) - p);
		return(0);
	}
}

char *
chap_response(ppp_t *ppp, char *user, int id, u_char *chal, int len, pbuf_t *m)
{
	int wv[2];
	int rv[2];
	int pid;
	int status;

	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, wv) < 0)
		return("could not create socketpair");
	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, rv) < 0) {
		close(wv[0]);
		close(wv[1]);
		return("could not create socketpair");
	}

	if (ppp->ppp_chapsecret)
		user = ppp->ppp_chapsecret;
	else if (!user || *user == 0)
		user = ppp->ppp_chapname;

	switch(pid = fork()) {
	case -1:
		close(rv[0]);
		close(rv[1]);
		close(wv[0]);
		close(wv[1]);
		return("could not fork");
	case 0:
		close(wv[1]);
		close(rv[1]);
		if (rv[0] != 0) {
			close(0);
			dup2(rv[0], 0);
			close(rv[0]);
		}
		if (wv[0] != 1) {
			close(1);
			dup2(wv[0], 1);
			close(wv[0]);
		}
		dup2(1, 2);
		sprintf((char *)m->data, "%d", id);
		execl(ppp->ppp_chapscript, "ppp-chap", user, m->data,
		    (char *) NULL);
		err(1, ppp->ppp_chapscript);
	default:
		close(wv[0]);
		close(rv[0]);
		write(rv[1], chal, len);
		close(rv[1]);
		while ((status = read(wv[1], pbuf_end(m), pbuf_left(m))) > 0)
			pbuf_append(m, status);
		close(wv[1]);

		if (waitpid(pid, &status, 0) < 0)
			return("waitpid failed");
		if (!WIFEXITED(status))
			return("authentication denied (script failure)");
		if (WEXITSTATUS(status)) {
			while (m->len && pbuf_end(m)[-1] == '\n')
				pbuf_trim(m, -1);
			*pbuf_end(m) = 0;
			return((char *)m->data);
		}
		return(0);
	}
}

char *
chap_check(ppp_t *ppp, char *user, int id, u_char *response, int len)
{
	pbuf_t pb;
	int wv[2];
	int rv[2];
	int pid;
	int status;
	u_char c;
	chal_t *ch;

	if (ppp->ppp_chapsecret)
		user = ppp->ppp_chapsecret;
	else if (!user || *user == 0)
		user = ppp->ppp_chapname;

	if ((ch = findchallange(id)) == NULL)
		return("not my challange");

	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, wv) < 0)
		return("could not create socketpair");
	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, rv) < 0) {
		close(wv[0]);
		close(wv[1]);
		return("could not create socketpair");
	}

	pbuf_init(&pb);

	switch(pid = fork()) {
	case -1:
		close(wv[0]);
		close(wv[1]);
		close(rv[0]);
		close(rv[1]);
		return("could not fork");
	case 0:
		close(rv[1]);
		close(wv[1]);
		if (rv[0] != 0) {
			close(0);
			dup2(rv[0], 0);
			close(rv[0]);
		}
		if (wv[0] != 1) {
			close(1);
			dup2(wv[0], 1);
			close(wv[0]);
		}
		dup2(1, 2);
		sprintf((char *)pb.data, "%d", id);
		execl(ppp->ppp_chapscript, "ppp-chap", "-v", user, pb.data,
		    (char *)NULL);
		err(1, ppp->ppp_chapscript);
	default:
		close(wv[0]);
		close(rv[0]);
		c = len;
		write(rv[1], &c, 1);
		write(rv[1], response, len);
		write(rv[1], ch->challange, ch->length);
		close(rv[1]);

		while ((status = read(wv[1], pbuf_end(&pb), pbuf_left(&pb))) >0)
			pbuf_append(&pb, status);

		while (pb.len && pbuf_end(&pb)[-1] == '\n')
			pbuf_trim(&pb, -1);

		close(wv[1]);

		if (waitpid(pid, &status, 0) < 0)
			return("waitpid failed");
		if (!WIFEXITED(status))
			return("authentication denied (script failure)");
		if (WEXITSTATUS(status)) {
			*pbuf_end(&pb) = 0;
			return((char *)pb.data);
		}
		return(0);
	}
}
