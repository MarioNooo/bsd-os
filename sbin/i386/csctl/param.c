/*
 * WILDBOAR $Wildboar: param.c,v 1.2 1996/03/17 08:35:15 shigeya Exp $
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

#include "csctl.h"

struct parm_list {
	struct parm_list	*pl_next;
	char			*pl_name;
	int			pl_nlen;
	char			*pl_val;
};

enum op {
	OP_NIL, OP_EQU, OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
	OP_LSH, OP_RSH, OP_AND, OP_XOR, OP_OR
};

#define	SOP_MINUS	0x01
#define	SOP_INV		0x02

struct optbl {
	char *name;
	int len;
	enum op op;
	int pri;
};

static char *replace_parm(char **lp);
static int eval(char *dname, int idx, char **lp, int retval, enum op op, int plevel);
static enum op getop(char **lp);
static int getpri(enum op op);
static int doop(enum op, int base, int val);

static struct parm_list *parm_list;

static struct optbl optbl[] = {
	{ "*",  1, OP_MUL, 13 },
	{ "/",  1, OP_DIV, 13 },
	{ "%",  1, OP_MOD, 13 },
	{ "+",  1, OP_ADD, 12 },
	{ "-",  1, OP_SUB, 12 },
	{ "<<", 2, OP_LSH, 11 },
	{ ">>", 2, OP_RSH, 11 },
	{ "&",  1, OP_AND,  8 },
	{ "^",  1, OP_XOR,  7 },
	{ "|",  1, OP_OR,   6 },
	{ "=",  1, OP_EQU,  2 },
	{ NULL, 0, OP_NIL,  0 }
};

void
define_parm(char **linep)
{
	char *p, *name, *val;
	int nlen, vlen;
	struct parm_list *pl, **plp;

	for (p = *linep; *p && isspace(*p); p++)
		;
	name = p;
	while (*p && !isspace(*p))
		p++;
	nlen = p - name;
	if (nlen == 0)
		return;
	while (*p && isspace(*p))
		p++;
	val = p;
	while (*p && !isspace(*p))
		p++;
	vlen = p - val;
	if (vlen == 0) {
		*linep = val;
		return;
	}
	while (*p && isspace(*p))
		p++;
	*linep = p;
	if ((pl = malloc(sizeof(*pl))) == NULL ||
	    (pl->pl_name = malloc(nlen+1)) == NULL ||
	    (pl->pl_val = malloc(vlen+1)) == NULL) {
		fprintf(stderr, "no more memory\n");
		exit(1);
	}
	strncpy(pl->pl_name, name, nlen);
	pl->pl_name[nlen] = '\0';
	pl->pl_nlen = nlen;
	strncpy(pl->pl_val, val, vlen);
	pl->pl_val[vlen] = '\0';

	/* longest first */
	for (plp = &parm_list; *plp; plp = &(*plp)->pl_next) {
		if ((*plp)->pl_nlen == nlen &&
		    strcmp((*plp)->pl_name, pl->pl_name) == 0) {
			fprintf(stderr, "Warning: %s redefined\n", pl->pl_name);
			/* XXX: *plp entry will be ignored */
			break;
		}
		if ((*plp)->pl_nlen < nlen)
			break;
	}
	pl->pl_next = *plp;
	*plp = pl;
}

static char *
replace_parm(char **lp)
{
	struct parm_list *pl;
	char *p;

	p = *lp;
	if (*p == '\0')
		return 0;
	for (pl = parm_list; pl; pl = pl->pl_next) {
		if (strncasecmp(p, pl->pl_name, pl->pl_nlen) == 0) {
			*lp = p + pl->pl_nlen;
			return pl->pl_val;
		}
	}
	return NULL;
}


int
parse_parm(char **linep, char *dname, int idx)
{
	char *p;

	p = *linep;
	if (*p == '\0')
		return 0;
	while (*p && isspace(*p))
		p++;
	*linep = p;
	return eval(dname, idx, linep, 0, OP_EQU, 0);
}

static int
eval(char *dname, int idx, char **lp, int retval, enum op op, int plevel)
{
	int val;
	int sop = 0;
	enum op nop;
	int pri, npri;
	char *parmp[MAXPARMLEVEL];
	char **olinep;
	char *p;
	int valmode = 1;
	int slevel = plevel;

	olinep = lp;
	pri = getpri(op);
	for (;;) {
		p = *lp;
		if (*p == '\0' && plevel > slevel) {
			if (--plevel == slevel)
				lp = olinep;
			else
				lp = &parmp[plevel-1];
			continue;
		}
		if (parmp[plevel] = replace_parm(lp)) {
			lp = &parmp[plevel++];
			if (plevel == MAXPARMLEVEL) {
				fprintf(stderr, "expression recursion too deep\n");
				break;
			}
			continue;
		}
		if (valmode) {
			*lp = p + 1;
			switch (*p) {
			case '+':
				/* do nothing */
				continue;
			case '-':
				sop ^= SOP_MINUS;
				continue;
			case '~':
				sop ^= SOP_INV;
				continue;
			case '(':	/*)*/
				val = eval(dname, idx, lp, 0, OP_EQU, plevel);
				p = *lp;
				if (*p != /*(*/ ')') {
					/* cause error */
					if (*p == '\0' || isspace(*p))
						*lp = p - 1;
					return 0;
				}
				*lp = p + 1;
				break;
			default:
				val = strtoul(p, lp, 0);
				if (p == *lp)
					return 0;
				break;
			}
			if (sop & SOP_MINUS)
				val = -val;
			if (sop & SOP_INV)
				val = ~val;
			sop = 0;
		} else {
			nop = getop(lp);
			npri = (nop != OP_NIL) ? getpri(nop) : pri;
			if (npri > pri) {
				val = eval(dname, idx, lp, val, nop, plevel);
				nop = getop(lp);
			}
			retval = doop(op, retval, val);
			if (npri < pri)
				break;
			op = nop;
			if (op == OP_NIL && plevel == slevel)
				break;
		}
		valmode = !valmode;
	}
	p = *lp;
	return (retval);
}

static enum op
getop(char **lp)
{
	struct optbl *ot;
	char *p;

	p = *lp;
	for (ot = optbl; ot->name; ot++) {
		if (strncmp(p, ot->name, ot->len) == 0)
			break;
	}
	*lp = p + ot->len;
	return (ot->op);
}

static int
getpri(enum op op)
{
	struct optbl *ot;

	for (ot = optbl; ot->name; ot++) {
		if (ot->op == op)
			break;
	}
	return (ot->pri);
}

static int
doop(enum op op, int base, int val)
{
	switch (op) {
	case OP_MUL:	base *= val;	break;
	case OP_DIV:	base /= val;	break;
	case OP_ADD:	base += val;	break;
	case OP_SUB:	base -= val;	break;
	case OP_MOD:	base %= val;	break;
	case OP_LSH:	base <<= val;	break;
	case OP_RSH:	base >>= val;	break;
	case OP_EQU:	base = val;	break;
	case OP_AND:	base &= val;	break;
	case OP_XOR:	base ^= val;	break;
	case OP_OR:	base |= val;	break;
	case OP_NIL:			break;
	}
	return base;
}
