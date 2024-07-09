/*
 * WILDBOAR $Wildboar: parse.c,v 1.2 1996/03/17 08:35:16 shigeya Exp $
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
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>

#include "csctl.h"

static void free_config(csspec_t **headp);
static csspec_t * parse_line(char **linep);
static void include_file(char **linep, csspec_t **cpp);
static int parse_name(char **linep, char *buf, int len);

void
load_config(char *conffile, csspec_t **headp)
{
	FILE *fp;
	char buf[BUFSIZ];
	char *line;
	int off;
	char c, *p;
	csspec_t *cp;
	int l, lno;

	if (strcmp(conffile, "-") == 0)
		fp = stdin;
	else
		fp = fopen(conffile, "r");
	if (fp == NULL) {
		perror(conffile);
		return;
	}
	l = 0;
	off = 0;
	free_event();
	free_config(headp);
	while (fgets(buf + off, sizeof(buf) - off, fp)) {
		l++;
		if (off == 0)
			lno = l;
		if ((p = strchr(buf, '\n')) != NULL)
			*p = '\0';
		if (buf[0] == '\0' || buf[0] == '#')
			continue;
		c = getc(fp);
		ungetc(c, fp);
		if (c == ' ' || c == '\t') {
			off += strlen(buf + off);
			continue;
		}
		off = 0;
		line = buf;
		cp = parse_line(&line);
		if (cp == NULL) {
			if (line == NULL) {
				/* valid entry without data */
				continue;
			}
			fprintf(stderr, "%s: Syntax error at line %d",
				cmd, lno);
			if (l != lno)
				fprintf(stderr, "--%d", l);
			fprintf(stderr, "\n%s\n", buf);
			for (p = buf; p < line; p++) {
				if (*p == '\t')
					putc('\t', stderr);
				else if (*p == ' ')
					putc(' ', stderr);
				else
					putc('-', stderr);
			}
			putc('^', stderr);
			putc('\n', stderr);
			continue;
		}
		*headp = cp;
		/* for .include */
		for (headp = &cp->css_next; cp = *headp; headp = &cp->css_next)
			;
	}
	fclose(fp);
	*headp = NULL;
}

static void
free_config(csspec_t **headp)
{
	csspec_t *cp, *ncp;
	char *p;

	for (cp = *headp; cp; cp = ncp) {
		ncp = cp->css_next;
		if (p = cp->css_str)
			free(p);
		free(cp);
	}
	*headp = NULL;
}

static csspec_t *
parse_line(char **linep)
{
	char *p;
	csspec_t *cp = NULL;
	char buf[BUFSIZ];
	int i;

	p = *linep;
	if (*p == '.') {
		if (strncasecmp(p+1, "define", sizeof("define")-1) == 0) {
			p += sizeof("define");
			*linep = p;
			define_parm(linep);
			if (p == *linep || **linep)
				goto bad;
			*linep = NULL;
			return NULL;
		}
		if (strncasecmp(p+1, "include", sizeof("include")-1) == 0) {
			p += sizeof("include");
			*linep = p;
			include_file(linep, &cp);
			if (p == *linep || **linep)
				goto bad;
			return cp;
		}
		goto bad;
	}
	cp = calloc(sizeof(*cp), 1);

	if (parse_name(linep, buf, sizeof(buf)) == 0) {
  bad:
		if (cp) {
			if (cp->css_str)
				free(cp->css_str);
			free(cp);
		}
		return NULL;
	}
	strncpy(cp->css_dname, buf, sizeof(cp->css_dname)-1);
	cp->css_dname[sizeof(cp->css_dname)-1] = '\0';
	if (strcmp("DEFAULT", buf) == 0 || strchr(buf, ',') != NULL ||
	    (cp->css_len = parse_quote(linep, buf, sizeof(buf))) == 0) {
		if (parse_event(linep, cp, buf) == 0)
			goto event;
		goto bad;
	}
	cp->css_str = malloc(cp->css_len);
	if (cp->css_str == NULL)
		goto bad;
	memcpy(cp->css_str, buf, cp->css_len);
	for (i = 0; i < CSS_MAXPARM; i++) {
		p = *linep;
		if (*p == '\0' || *p == '@')
			break;
		cp->css_parm[i] = parse_parm(linep, cp->css_dname, i);
		if (*linep == p)
			goto bad;
	}
	strcpy(buf, cp->css_dname);
  event:
	while (*(p = *linep)) {
		if (parse_event(linep, cp, buf) < 0 || *linep == p)
			break;
	}
	for (p = *linep; *p; p++)
		if (!isspace(*p))
			goto bad;
	if (cp->css_str == NULL) {
		/* event only */
		*linep = NULL;
		free(cp);
		cp = NULL;
	}
	return cp;
}

static void
include_file(char **linep, csspec_t **cpp)
{
	char *p, *fname;
	int len, plen;
	static int level;

	if (level > 10) {
		fprintf(stderr, "too many levels of includes\n");
		return;
	}
	for (p = *linep; *p && isspace(*p); p++)
		;
	*linep = p;
	for (p = *linep; *p && !isspace(*p); p++)
		;
	len = p - *linep;
	if (len == 0)
		return;
	plen = (**linep == '/') ? 0 : sizeof("/etc/")-1;
	if ((fname = malloc(len + plen + 1)) == NULL) {
		fprintf(stderr, "no more memory\n");
		exit(1);
	}
	if (plen)
		strcpy(fname, "/etc/");
	strncpy(fname + plen, *linep, len);
	fname[plen + len] = '\0';
	*linep = p;
	level++;
	load_config(fname, cpp);
	level--;
	free(fname);
}

static int
parse_name(char **linep, char *buf, int len)
{
	char *p;

	for (p = *linep; *p; p++) {
		if (isspace(*p))
			break;
	}
	if (p - *linep >= len)
		return 0;
	len = p - *linep;
	strncpy(buf, *linep, len);
	buf[len] = '\0';
	*linep = p;
	return len;
}

int
parse_quote(char **linep, char *buf, int len)
{
	char *p, *q;
	int i;

	p = *linep;
	while (*p && isspace(*p))
		p++;
	if (*p != '"') {
  bad:
		*linep = p;
		return 0;
	}
	for (p++, q = buf;  ; p++, q++, len--) {
		if (len <= 0 || *p == '\0')
			goto bad;
		*q = *p;
		if (*q == '\\') {
			switch (*++p) {
			case '\0':
				goto bad;
			case 'b':	*q = '\b';	break;
			case 't':	*q = '\t';	break;
			case 'n':	*q = '\n';	break;
			case 'x':
#define	HEXC(c)		(isdigit(c) ? (c) - '0' : tolower(c) - 'a' + 10)
				*q = 0;
				for (i = 0; i < 2 && isxdigit(*p); i++, p++)
					*q = *q * 16 + HEXC(*p);
				p--;
				break;
			default:
#define	isodigit(c)	((c) >= '0' && (c) <= '7')
				if (!isodigit(*p)) {
					*q = *p;
					break;
				}
				*q = 0;
				for (i = 0; i < 3 && isodigit(*p); i++, p++)
					*q = *q * 8 + (*p - '0');
				p--;
				break;
			}
			continue;
		}
		if (*p == '"') {
			while (*++p && isspace(*p))
				;
			if (*p != '"')
				break;
			*q = '\0';
		}
	}
	*linep = p;
	return q - buf;
}

void
show_quote(char *buf, int len)
{
	u_char *p;
	u_char c;

	putchar('"');
	for (p = (u_char *)buf; len > 0 && *p != 0xff; p++, len--) {
		c = *p;
		if (c < ' ' || c >= 0x7f) {
			putchar('\\');
			switch (c) {
			case '\t':	putchar('t');	break;
			case '\n':	putchar('n');	break;
			case '\b':	putchar('b');	break;
			case '\r':	putchar('r');	break;
			case '\0':
				if (p[1] < '0' || p[1] > '7') {
					putchar('0');
					break;
				}
				/*FALLTHRU*/
			default:
				printf("%03o", c);
				break;
			}
		} else
			putchar(c);
	}
	putchar('"');
}
