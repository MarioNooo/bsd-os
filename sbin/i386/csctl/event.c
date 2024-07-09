/*
 * WILDBOAR $Wildboar: event.c,v 1.2 1996/03/17 08:35:15 shigeya Exp $
 */
/*
 *  Portions or all of this file are Copyright(c) 1994,1995,1996
 *  Yoichi Shinoda, Yoshitaka Tokugawa, WIDE Project, Wildboar Project
 *  and Foretune.  All rights reserved.
 *
 *  This code has been contributed to Berkeley Software Design, Inc.
 *  by the Wildboar Project and its contributors.
 *
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE WILDBOAR PROJECT AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 *  WILDBOAR PROJECT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 *  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "csctl.h"

#define	CSE_CMD_INSERT	0
#define	CSE_CMD_REMOVE	1
#define	CSE_CMD_MAX	2
typedef struct csspec_event  {
	struct csspec_event	*cse_next;
	char			cse_dname[CSS_DNAMELEN];
	char			*cse_str;
	int			cse_len;
	char			*cse_cmd[CSE_CMD_MAX];
} csspec_event_t;

static csspec_event_t * get_event(char *dname, char *str, int len);
static int parse_event_one(char **linep, csspec_t *cp, char *dname);
static void do_event(int mode, int slot, cs_cfg_t *cfg);

static csspec_event_t *csse_head;

int
parse_event(char **linep, csspec_t *cp, char *dname)
{
	char *p;
	char *start;
	int len;
	char dbuf[CSS_DNAMELEN];

	for (start = *linep; dname; dname = p) {
		*linep = start;
		if (p = strchr(dname, ','))
			len = p++ - dname;
		else
			len = strlen(dname);
		if (len + 1 > sizeof(dbuf))
			return -1;
		strncpy(dbuf, dname, len);
		dbuf[len] = '\0';
		if (parse_event_one(linep, cp, dbuf) < 0)
			return -1;
	}
	return 0;
}

void
free_event(void)
{
	csspec_event_t *ce, *nce;
	char *p;
	int i;

	for (ce = csse_head; ce; ce = nce) {
		nce = ce->cse_next;
		for (i = 0; i < CSE_CMD_MAX; i++) {
			if (p = ce->cse_cmd[i])
				free(p);
		}
		free(ce);
	}
	csse_head = NULL;
}

void
show_event(char *dname, char *str, int len)
{
	char *p;
	csspec_event_t *ce;

	if ((ce = get_event(dname, str, len)) != NULL) {
		if (p = ce->cse_cmd[CSE_CMD_INSERT]) {
			printf("\t@insert ");
			show_quote(p, strlen(p));
			putchar('\n');
		}
		if (p = ce->cse_cmd[CSE_CMD_REMOVE]) {
			printf("\t@remove ");
			show_quote(p, strlen(p));
			putchar('\n');
		}
	}
}

int
have_event(void)
{
	return csse_head ? 1 : 0;
}

void
insert_event(int slot, cs_cfg_t *cfg)
{
	do_event(CSE_CMD_INSERT, slot, cfg);
}

void
remove_event(int slot, cs_cfg_t *cfg)
{
	do_event(CSE_CMD_REMOVE, slot, cfg);
}

static csspec_event_t *
get_event(char *dname, char *str, int len)
{
	csspec_event_t *ce;

	/* 1. check dname and str */
	for (ce = csse_head; ce; ce = ce->cse_next) {
		if (len >= ce->cse_len && ce->cse_len != 0 &&
		    bcmp(str, ce->cse_str, ce->cse_len) == 0 &&
		    strcmp(dname, ce->cse_dname) == 0)
			return ce;
	}
	/* 2. default entry within driver */
	for (ce = csse_head; ce; ce = ce->cse_next) {
		if (ce->cse_len == 0 && strcmp(dname, ce->cse_dname) == 0)
			return ce;
	}
	/* 3. global default */
	for (ce = csse_head; ce; ce = ce->cse_next) {
		if (strcmp("DEFAULT", ce->cse_dname) == 0)
			return ce;
	}
	return NULL;
}

static int
parse_event_one(char **linep, csspec_t *cp, char *dname)
{
	csspec_event_t *ce, **cep;
	char *p;
	int mode, len;
	int i;
	char buf[BUFSIZ];

	for (p = *linep; *p; p++) {
		if (!isspace(*p))
			break;
	}
	if (strncasecmp(p, "@insert", sizeof("@insert")-1) == 0) {
		mode = CSE_CMD_INSERT;
		p += sizeof("@insert")-1;
	} else if (strncasecmp(p, "@remove", sizeof("@remove")-1) == 0) {
		mode = CSE_CMD_REMOVE;
		p += sizeof("@remove")-1;
	} else {
		return -1;
	}
	*linep = p;
        len = parse_quote(linep, buf, sizeof(buf));
	if (len == 0)
	        return -1;
	for (i = 0, p = buf; i < len; i++, p++)
	        if (*p == '\0')
	                *p = '\n';
	*p = '\0';
	for (cep = &csse_head; ce = *cep; cep = &ce->cse_next) {
		/* share same strings */
		if (ce->cse_len == cp->css_len && ce->cse_str == cp->css_str &&
		    strcmp(ce->cse_dname, dname) == 0)
			break;
	}
	if (ce == NULL) {
		ce = calloc(sizeof(*ce), 1);
		if (ce == NULL) {
			fprintf(stderr, "no more memory\n");
			exit(1);
		}
		strcpy(ce->cse_dname, dname);
		ce->cse_str = cp->css_str;
		ce->cse_len = cp->css_len;
		*cep = ce;
	}
	if (ce->cse_cmd[mode])
		free(ce->cse_cmd[mode]);
	if ((ce->cse_cmd[mode] = strdup(buf)) == NULL) {
		fprintf(stderr, "no more memory\n");
		exit(1);
	}
	return 0;
}

static void
do_event(int mode, int slot, cs_cfg_t *cfg)
{
	int unit;
	char *p, *q;
	csspec_event_t *ce;
	char dname[CSS_DNAMELEN];
	char cmdbuf[BUFSIZ];

	strcpy(dname, cfg->cscfg_devname);
	for (p = dname + strlen(dname) - 1; p > dname && isdigit(*p); p--)
		;
	p++;
	unit = atoi(p);
	*p = '\0';

	ce = get_event(dname, (char *)cfg->cscfg_V1_str+2, cfg->cscfg_V1_len-2);
	if (ce == NULL || (p = ce->cse_cmd[mode]) == NULL) {
#if 0
printf("slot %d: %s%d %s: no event\n", slot, dname, unit, mode ? "remove" : "insert");
#endif
		return;
	}
	for (q = cmdbuf; *q = *p; p++, q++) {
		if (*p == '%') {
			switch (*++p) {
			case 'D':
				strcpy(q, dname);
				break;
			case 'U':
				sprintf(q, "%d", unit);
				break;
			case 'S':
				sprintf(q, "%d", slot);
				break;
			default:
				q[1] = '\0';
				break;
			}
			q += strlen(q) - 1;
		}
	}
	system(cmdbuf);
}
