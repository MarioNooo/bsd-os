/*	BSDI test.c,v 2.6 2000/03/10 23:12:05 donn Exp	*/

/*-
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
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
"@(#) Copyright (c) 1992, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)test.c	8.3 (Berkeley) 4/2/94";
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>

#include "operators.h"

#define	STACKSIZE	12
#define	NESTINCR	16

/* data types */
#define	STRING	0
#define	INTEGER	1
#define	BOOLEAN	2

#define	IS_BANG(s) (s[0] == '!' && s[1] == '\0')

/*
 * This structure hold a value.  The type keyword specifies the type of
 * the value, and the union u holds the value.  The value of a boolean
 * is stored in u.num (1 = TRUE, 0 = FALSE).
 */
struct value {
	int type;
	union {
		char *string;
		long num;
	} u;
	char digits[sizeof(long) * 4];
};

struct operator {
	short op;		/* Which operator. */
	short pri;		/* Priority of operator. */
};

struct filestat {
	char *name;		/* Name of file. */
	int rcode;		/* Return code from stat. */
	struct stat stat;	/* Status info on file. */
};

static int	expr_is_false __P((struct value *));
static void	expr_operator __P((int, struct value *, struct filestat *));
static void	get_int __P((char *, long *));
static int	lookup_op __P((char *, const char *const *));
static void	overflow __P((void));
static int	posix_binary_op __P((char **));
static int	posix_unary_op __P((char **));
static void	syntax __P((void));
static jmp_buf	jmperr;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct operator opstack[STACKSIZE];
	struct operator *opsp;
	struct value valstack[STACKSIZE + 1];
	struct value *valsp;
	struct filestat fs;
	char  c, **ap, *opname, *p;
	int binary, nest, op, pri, ret_val, skipping;

	op = -1;

	if ((p = argv[0]) == NULL) {
		fprintf(stderr, "test: argc is zero\n");
		return (2);
	}

	if (*p != '\0' && p[strlen(p) - 1] == '[') {
		if (strcmp(argv[--argc], "]")) {
			fprintf(stderr, "test: missing ]\n");
			return (2);
		}
		argv[argc] = NULL;
	}
	ap = argv + 1;
	fs.name = NULL;

	/*
	 * Because test is a builtin to the shell we need a way
	 * to return the exit code rather than exit with it.
	 */
	if ((c = setjmp(jmperr)) != 0)
		return (c);

	/*
	 * Test(1) implements an inherently ambiguous grammer.  In order to
	 * assure some degree of consistency, we special case the POSIX 1003.2
	 * requirements to assure correct evaluation for POSIX scripts.  The
	 * following special cases comply with POSIX P1003.2/D11.2 Section
	 * 4.62.4.
	 */
	switch(argc - 1) {
	case 0:				/* % test */
		return (1);
		break;
	case 1:				/* % test arg */
		return (argv[1] == NULL || *argv[1] == '\0') ? 1 : 0;
		break;
	case 2:				/* % test op arg */
		opname = argv[1];
		if (IS_BANG(opname))
			return (*argv[2] == '\0') ? 0 : 1;
		else {
			ret_val = posix_unary_op(&argv[1]);
			if (ret_val >= 0)
				return (ret_val);
		}
		break;
	case 3:				/* % test arg1 op arg2 */
		ret_val = posix_binary_op(&argv[1]);
		if (ret_val >= 0)
			return (ret_val);
		if (IS_BANG(argv[1])) {
			ret_val = posix_unary_op(&argv[1]);
			if (ret_val >= 0)
				return (!ret_val);
		}
		break;
	case 4:				/* % test ! arg1 op arg2 */
		if (IS_BANG(argv[1])) {
			ret_val = posix_binary_op(&argv[2]);
			if (ret_val >= 0)
				return (!ret_val);
		}
		break;
	default:
		break;
	}

	/*
	 * We use operator precedence parsing, evaluating the expression as
	 * we parse it.  Parentheses are handled by bumping up the priority
	 * of operators using the variable "nest."  We use the variable
	 * "skipping" to turn off evaluation temporarily for the short
	 * circuit boolean operators.  (It is important do the short circuit
	 * evaluation because under NFS a stat operation can take infinitely
	 * long.)
	 */
	opsp = opstack + STACKSIZE;
	valsp = valstack;
	nest = skipping = 0;
	if (*ap == NULL) {
		valstack[0].type = BOOLEAN;
		valstack[0].u.num = 0;
		goto done;
	}
	for (;;) {
		opname = *ap++;
		if (opname == NULL)
			syntax();
		if (opname[0] == '(' && opname[1] == '\0') {
			nest += NESTINCR;
			continue;
		} else if (*ap && (op = lookup_op(opname, unary_op)) >= 0) {
			if (opsp == &opstack[0])
				overflow();
			--opsp;
			opsp->op = op;
			opsp->pri = op_priority[op] + nest;
			continue;
		} else {
			valsp->type = STRING;
			valsp->u.string = opname;
			valsp++;
		}
		for (;;) {
			opname = *ap++;
			if (opname == NULL) {
				if (nest != 0)
					syntax();
				pri = 0;
				break;
			}
			if (opname[0] != ')' || opname[1] != '\0') {
				if ((op = lookup_op(opname, binary_op)) < 0)
					syntax();
				op += FIRST_BINARY_OP;
				pri = op_priority[op] + nest;
				break;
			}
			if ((nest -= NESTINCR) < 0)
				syntax();
		}
		while (opsp < &opstack[STACKSIZE] && opsp->pri >= pri) {
			binary = opsp->op;
			for (;;) {
				valsp--;
				c = op_argflag[opsp->op];
				if (c == OP_INT) {
					if (valsp->type == STRING)
						get_int(valsp->u.string,
						    &valsp->u.num);
					valsp->type = INTEGER;
				} else if (c >= OP_STRING) {	
					            /* OP_STRING or OP_FILE */
					if (valsp->type == INTEGER) {
#ifdef SHELL
						fmtstr(valsp->digits, sizeof(valsp->digits), "%d", 
						    valsp->u.num);
#else
						snprintf(valsp->digits, sizeof(valsp->digits),
						    "%ld", valsp->u.num);
#endif
						valsp->u.string = valsp->digits;
					} else if (valsp->type == BOOLEAN) {
						if (valsp->u.num)
							valsp->u.string = 
						            "true";
						else
							valsp->u.string = "";
					}
					valsp->type = STRING;
					if (c == OP_FILE && (fs.name == NULL ||
					    strcmp(fs.name, valsp->u.string))) {
						fs.name = valsp->u.string;
						fs.rcode = (opsp->op == ISSYMLINK ?
							lstat : stat)(valsp->u.string, &fs.stat);
					}
				}
				if (binary < FIRST_BINARY_OP)
					break;
				binary = 0;
			}
			if (!skipping)
				expr_operator(opsp->op, valsp, &fs);
			else if (opsp->op == AND1 || opsp->op == OR1)
				skipping--;
			valsp++;		/* push value */
			opsp++;			/* pop operator */
		}
		if (opname == NULL)
			break;
		if (opsp == &opstack[0])
			overflow();
		if (op == AND1 || op == AND2) {
			op = AND1;
			if (skipping || expr_is_false(valsp - 1))
				skipping++;
		}
		if (op == OR1 || op == OR2) {
			op = OR1;
			if (skipping || !expr_is_false(valsp - 1))
				skipping++;
		}
		opsp--;
		opsp->op = op;
		opsp->pri = pri;
	}
done:	return (expr_is_false(&valstack[0]));
}

static int
expr_is_false(val)
	struct value *val;
{

	if (val->type == STRING) {
		if (val->u.string[0] == '\0')
			return (1);
	} else {		/* INTEGER or BOOLEAN */
		if (val->u.num == 0)
			return (1);
	}
	return (0);
}


static uid_t
euid()
{
	static int ready;
	static uid_t cache_euid;

	if (ready == 0)
		cache_euid = geteuid();
	ready = 1;
	return (cache_euid);
}


static int
groupmember(gid)
	gid_t gid;
{
	static gid_t groups[NGROUPS_MAX];
	static int ngroups = -1;
	int i;

	if (ngroups == -1 && (ngroups = getgroups(NGROUPS_MAX, groups)) == -1)
		return (0);

	for (i = 0; i < ngroups; ++i)
		if (groups[i] == gid)
			return (1);

	return (0);
}


/*
 * Execute an operator.  Op is the operator.  Sp is the stack pointer;
 * sp[0] refers to the first operand, sp[1] refers to the second operand
 * (if any), and the result is placed in sp[0].  The operands are converted
 * to the type expected by the operator before expr_operator is called.
 * Fs is a pointer to a structure which holds the value of the last call
 * to stat, to avoid repeated stat calls on the same file.
 */
static void
expr_operator(op, sp, fs)
	int op;
	struct value *sp;
	struct filestat *fs;
{
	int i;

	if ((i = op_argflag[op]) == OP_FILE && fs->rcode == -1) {
		sp->u.num = 0;
		sp->type = op == ISSIZE ? INTEGER : BOOLEAN;
		return;
	}

	switch (op) {
	case NOT:
		sp->u.num = expr_is_false(sp);
		sp->type = BOOLEAN;
		break;
	case ISEXIST:
		/* false case evaluated above */
		goto true;
	case ISREAD:
		if (euid() == 0)
			goto true;
		i = S_IROTH;
		goto permission;
	case ISWRITE:
		if (euid() == 0)
			goto true;
		i = S_IWOTH;
		goto permission;
	case ISEXEC:
		if (euid() == 0) {
			if ((fs->stat.st_mode & S_IFMT) == S_IFDIR)
				goto true;
			i = S_IXOTH | S_IXGRP | S_IXUSR;
			goto filebit;
		}
		i = S_IXOTH;
permission:	if (fs->stat.st_uid == euid())
			i <<= 6;
		else if (groupmember(fs->stat.st_gid))
			i <<= 3;
		goto filebit;	/* true if (stat.st_mode & i) != 0 */
	case ISFILE:
		i = S_IFREG;
		goto filetype;
	case ISDIR:
		i = S_IFDIR;
		goto filetype;
	case ISCHAR:
		i = S_IFCHR;
		goto filetype;
	case ISBLOCK:
		i = S_IFBLK;
		goto filetype;
	case ISSYMLINK:
		i = S_IFLNK;
filetype:	if ((fs->stat.st_mode & S_IFMT) == i)
true:			sp->u.num = 1;
		else
false:			sp->u.num = 0;
		sp->type = BOOLEAN;
		break;
	case ISFIFO:		/* ISFIFO true for S_IFIFO or S_IFSOCK */
		if (S_ISFIFO(fs->stat.st_mode))
			sp->u.num = 1;
		else
			sp->u.num = 0;
		sp->type = BOOLEAN;
		break;
	case ISSETUID:
		i = S_ISUID;
		goto filebit;
	case ISSETGID:
		i = S_ISGID;
		goto filebit;
	case ISSTICKY:
		i = S_ISVTX;
filebit:	if (fs->stat.st_mode & i)
			goto true;
		goto false;
	case ISSIZE:
		sp->u.num = fs->stat.st_size;
		sp->type = INTEGER;
		break;
	case ISTTY:
		sp->u.num = isatty(sp->u.num);
		sp->type = BOOLEAN;
		break;
	case NULSTR:
		if (sp->u.string[0] == '\0')
			goto true;
		goto false;
	case STRLEN:
		sp->u.num = strlen(sp->u.string);
		sp->type = INTEGER;
		break;
	case OR1:
	case AND1:
		/*
		 * These operators are mostly handled by the parser.  If we
		 * get here it means that both operands were evaluated, so
		 * the value is the value of the second operand.
		 */
		*sp = *(sp + 1);
		break;
	case STREQ:
	case STRNE:
		i = 0;
		if (!strcmp(sp->u.string, (sp + 1)->u.string))
			i++;
		if (op == STRNE)
			i = 1 - i;
		sp->u.num = i;
		sp->type = BOOLEAN;
		break;
	case EQ:
		if (sp->u.num == (sp + 1)->u.num)
			goto true;
		goto false;
	case NE:
		if (sp->u.num != (sp + 1)->u.num)
			goto true;
		goto false;
	case GT:
		if (sp->u.num > (sp + 1)->u.num)
			goto true;
		goto false;
	case LT:
		if (sp->u.num < (sp + 1)->u.num)
			goto true;
		goto false;
	case LE:
		if (sp->u.num <= (sp + 1)->u.num)
			goto true;
		goto false;
	case GE:
		if (sp->u.num >= (sp + 1)->u.num)
			goto true;
		goto false;

	}
}

static int
lookup_op(name, table)
	char *name;
	const char *const * table;
{
	const char *const * tp;
	const char *p;
	char c;

	c = name[1];
	for (tp = table; (p = *tp) != NULL; tp++)
		if (p[1] == c && !strcmp(p, name))
			return (tp - table);
	return (-1);
}

/*
 * If argv[0] is a POSIX-or-extended unary operator, evaluate it specially;
 * otherwise return -1.
 */
static int
posix_unary_op(argv)
	char **argv;
{
	struct filestat fs;
	struct value valp;
	int op, c;
	char *opname;

	opname = *argv;
	if ((op = lookup_op(opname, unary_op)) < 0)
		return (-1);
	c = op_argflag[op];
	opname = argv[1];
	valp.u.string = opname;
	if (c == OP_FILE) {
		fs.name = opname;
		fs.rcode = (op == ISSYMLINK ? lstat: stat)(opname, &fs.stat);
	} else if (c != OP_STRING)
		return (-1);

	expr_operator(op, &valp, &fs);
	return (valp.u.num == 0);
}

/*
 * If argv[1] is a POSIX binary operator (anything but a conjunction),
 * evaluate it specially; otherwise return -1.
 */
static int
posix_binary_op(argv)
	char  **argv;
{
	struct value v[2];
	int op, c;
	char *opname;
#define is_conjunction(o) \
	((o) == AND1 || (o) == OR1 || (o) == AND2 || (o) == OR2)

	opname = argv[1];
	if ((op = lookup_op(opname, binary_op)) < 0)
		return (-1);
	op += FIRST_BINARY_OP;
	if (is_conjunction(op))
		return (-1);
	c = op_argflag[op];

	if (c == OP_INT) {
		get_int(argv[0], &v[0].u.num);
		get_int(argv[2], &v[1].u.num);
	} else {
		v[0].u.string = argv[0];
		v[1].u.string = argv[2];
	}
	expr_operator(op, v, NULL);
	return (v[0].u.num == 0);
}

/*
 * Integer type checking.
 */
static void
get_int(v, lp)
	char *v;
	long *lp;
{
	long val;
	char *ep;

	if (*v == '\0') {
		fprintf(stderr,
		    "test: null string where integer was expected\n");
		longjmp(jmperr, 2);
	}

	errno = 0;
	val = strtol(v, &ep, 10);
	if (ep == v) {
		fprintf(stderr, "test: unrecognized integer `%s'\n", v);
		longjmp(jmperr, 2);
	}
	while (isspace(*ep))
		++ep;
	if (*ep != '\0') {
		fprintf(stderr,
		    "test: trailing characters in integer `%s'\n", v);
		longjmp(jmperr, 2);
	}
	if (errno == ERANGE) {
		if (val == LONG_MIN) {
			fprintf(stderr, "test: %s: underflow\n", v);
			longjmp(jmperr, 2);
		}
		if (val == LONG_MAX) {
			fprintf(stderr, "test: %s: overflow\n", v);
			longjmp(jmperr, 2);
		}
	}
	*lp = val;
	return;
}

static void
syntax()
{
	fprintf(stderr, "test: syntax error\n");
	longjmp(jmperr, 2);
}

static void
overflow()
{
	fprintf(stderr, "test: expression is too complex\n");
	longjmp(jmperr, 2);
}
