/*	BSDI openprom.c,v 1.2 2000/09/20 10:21:48 torek Exp	*/

/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef lint
static char copyright[] =
    "Copyright (c) 1992 Lawrence Berkeley Laboratory\nAll rights reserved.\n";
#endif

/*
 * openprom dump utility
 *
 * usage:
 *
 *	openprom
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>

#include <machine/openpromio.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pathnames.h"

static	char openprom[] = _PATH_OPENPROM;
int	opfd;

void	dumpnode(int, int);
void	dumplist(int, int);
int	classify(struct opiocdesc *, int *);
int	isstr(struct opiocdesc *);
void	printbytes(char *, int);
void	printstrs(char *, int);

#define	T_MISC		0	/* generic, print as bytes */
#define	T_STRINGS	1	/* string sequence (zero or more strs) */
#define	T_INTSEQ	2	/* integer sequence */

/* subtypes, for int only */
#define	ST_DEFAULT	0	/* hex and decimal */
#define	ST_HEXONLY	1	/* 8-character hex only */

/* ARGSUSED */
int
main(int argc, char **argv)
{
	int node;

	if ((opfd = open(openprom, O_RDONLY, 0)) < 0)
		err(1, "open(%s)", openprom);

	/* next(0) gets the first one */
	node = 0;
	if (ioctl(opfd, OPIOCGETNEXT, &node) < 0)
		err(1, "ioctl(OPIOCGETNEXT)");
	dumplist(node, 0);
	exit(0);
}

/*
 * Dump all the nodes in the list, nesting their children.
 */
void
dumplist(int first, int depth)
{
	int node, child;

	for (node = first; node != 0;) {
		/* dump this one */
		dumpnode(node, depth);

		/* depth-first: dump list of children of this node */
		child = node;
		if (ioctl(opfd, OPIOCGETCHILD, &child) < 0)
			err(1, "ioctl(OPIOCGETCHILD)");
		dumplist(child, depth + 1);

		/* move on to the next entry */
		if (ioctl(opfd, OPIOCGETNEXT, &node) < 0)
			err(1, "ioctl(OPIOCGETNEXT)");
	}
}

/*
 * Dump all the properties of a node.
 */
void
dumpnode(int node, int depth)
{
	int i;
	struct opiocdesc *dp;
	struct opiocdesc desc;
	int type, subtype, v;
	static int beenhere;
	char *name, *cp, *sep;
	char buf[1024], buf2[sizeof(buf)];

	if (beenhere)
		(void)putchar('\n');
	beenhere = 1;
	for (i = 1; i < depth; i += 2)
		(void)putchar('\t');
	if (i == depth)
		(void)printf("    ");
	printf("node %x:\n", node);
	depth++;

	dp = &desc;
	dp->op_nodeid = node;

	/* Prime the pump with a zero length name */
	dp->op_name = buf;
	dp->op_name[0] = '\0';
	dp->op_namelen = 0;
	dp->op_buf = buf2;
	for (;;) {
		/* Get the next property name */
		dp->op_buflen = sizeof(buf);
		if (ioctl(opfd, OPIOCNEXTPROP, dp) < 0)
			err(1, "ioctl(OPIOCNEXTPROP)");

		/* Zero length name means we're done */
		if (dp->op_buflen <= 0)
			return;

		/* Clever hack, swap buffers */
		cp = dp->op_buf;
		dp->op_buf = dp->op_name;
		dp->op_name = cp;
		dp->op_namelen = dp->op_buflen;

		/* Get the value */
		dp->op_buflen = sizeof(buf);
		if (ioctl(opfd, OPIOCGET, dp) < 0)
			err(1, "ioctl(OPIOCGET) (for \"%.*s\")",
			    dp->op_namelen, dp->op_name);
		for (i = 1; i < depth; i += 2)
			(void)putchar('\t');
		if (i == depth)
			(void)printf("    ");
		(void)printf("%.*s=", dp->op_namelen, dp->op_name);
		type = classify(dp, &subtype);
		switch (type) {

		case T_STRINGS:
			printstrs(dp->op_buf, dp->op_buflen);
			break;

		case T_INTSEQ:
			cp = dp->op_buf;
			sep = "";
			for (i = 0; i < dp->op_buflen; i += sizeof(int)) {
				bcopy(cp, &v, sizeof v);
				if (subtype == ST_HEXONLY)
					(void)printf("%s0x%8.8x", sep, v);
				else
					(void)printf("%s0x%x (%d)", sep, v, v);
				cp += sizeof v;
				sep = " ";
			}
			break;

		default:
			printbytes(dp->op_buf, dp->op_buflen);
			break;
		}
		(void)putchar('\n');
	}
}

int
classify(struct opiocdesc *dp, int *subtype)
{
	static struct cls {
		char	*name;
		u_short	type;
		u_char	subtype;
		u_char	namelen;
	} table[] = {
		{ "compatible", T_STRINGS, 0, 0 },
		/* { "interrupt-map", T_INTSEQ, ST_HEXONLY, 0 }, -- nope */
		{ "ranges", T_INTSEQ, ST_HEXONLY, 0 },
		{ "reg", T_INTSEQ, ST_HEXONLY, 0 },
		{ "translations", T_INTSEQ, ST_HEXONLY, 0 },
		{ 0, 0, 0, 0 }
	};
	struct cls *cp;

	for (cp = table; cp->name != NULL; cp++) {
		if (cp->namelen == 0)
			cp->namelen = strlen(cp->name);
		if (cp->namelen == dp->op_namelen &&
		    memcmp(cp->name, dp->op_name, cp->namelen) == 0) {
			*subtype = cp->subtype;
			return (cp->type);
		}
	}
	if (isstr(dp))
		return (T_STRINGS);
	if (dp->op_buflen == sizeof(int)) {
		*subtype = ST_DEFAULT;
		return (T_INTSEQ);
	}
	return (T_MISC);
}

/*
 * Decide whether something "is a string".  It is not a string if
 * it contains unprintable characters, excluding a possible final NUL.
 */
int
isstr(struct opiocdesc *dp)
{
	char *p;
	int n;

	p = dp->op_buf;
	for (n = dp->op_buflen; --n >= 0; p++)
		if (!isascii(*p) || !isprint(*p))
			if (n > 0 || *p != 0)
				return (0);

	return (1);
}

void
printbytes(char *cp, int n)
{
	char *s = "";

	while (--n >= 0) {
		printf("%s%2.2x", s, *(unsigned char *)cp++);
		s = " ";
	}
}

void
printstrs(char *cp, int n)
{
	char *s = "", *zp, c;
	int len, plus1;

	while (n > 0) {
		if ((zp = memchr(cp, '\0', n)) != NULL) {
			len = zp - cp;
			plus1 = 1;
		} else {
			len = n;
			plus1 = 0;
		}
		printf("%s", s);
		putchar('"');
		n -= len + plus1;
		while (len) {
			c = *cp++;
			len--;
			if (c == '"' || c == '\\')
				putchar('\\');
			putchar(c);
		}
		putchar('"');
		s = " ";
		cp += plus1;
	}
}
