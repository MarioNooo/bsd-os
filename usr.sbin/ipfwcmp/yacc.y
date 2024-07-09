/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*-
 *  Copyright (c) 1996,1997 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI yacc.y,v 1.22 2003/10/09 18:23:04 prb Exp
 */
%{
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/ipfw.h>
#include <net/bpf.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "types.h"

extern int linecnt;
void append(cmd_t *, cmd_t *);
cmd_t * emit(cmd_t *, char *, ...);
cmd_t * emitd(cmd_t *, var_t *, char *, ...);
cmd_t * emitt(cmd_t *, var_t *, char *, ...);
cmd_t * emitdt(cmd_t *, var_t *, var_t *, char *, ...);
cmd_t * emits(cmd_t *, var_t *, var_t *, char *);
void allocatevar(var_t **, char *);
void assignvar(var_t **, char *);
void emitcode(cmd_t *);
int checkret(cmd_t *);
jmp_t *nextlabelp(jmp_t *);
void appendlabel(jmp_t*, jmp_t *);
void appendlabeln(jmp_t*, int);
extern int error;
extern char *srcfile;
extern FILE *asmout;
static filter_t *filter_list = NULL;
static filter_t *cf = NULL;
static var_t *var_list = NULL;
static switch_t *sw = NULL;
static char *rpmin(range_t *);
static char *rpmax(range_t *);
static char *qprint(q128_t *);
int def = -1;
void setdef(int);
void start_filter(char *);
void start_switch();
extern int state;
int protocol = 4;
int rangesize = 0;
var_t	*proto_v = NULL, *version_v = NULL, *hlen_v = NULL, *src_v = NULL,
	*dst_v = NULL, *hlen1_v = NULL, *off_v = NULL, *foff_v = NULL,
	*time_v = NULL; 
int ipv6 = 0;
int debug = 0;
int nocpplines = 0;

#ifndef	M_CANFASTFWD
#define	M_CANFASTFWD	0
#endif
%}

%union {
	cmd_t	*cmds;
	node_t	*node;
	range_t	*range;
	filter_t*filt;
	u_long	k;
	q128_t	q;
	char	*s;
}

%type	<cmds>	cmd ecmdlist cmdlist stat swhat what ifaces
%type	<node>	cond cases lastcase protocol ipv4inipv4 ipv6inipv4
%type	<range>	range ranges scases srange
%type	<k>	number quad which proto user users icmptype icmpcode denyicmp
%type	<k>	unumber interface ipaddr netnumber

%token	<k>	SRCADDR DSTADDR PROTOCOL INTERFACE SRCPORT DSTPORT IMPLICIT
%token	<k>	ELSE ERR DENY PERMIT FILTER TCPFLAGS ESTABLISHED FORWARDING
%token	<k>	ICMPTYPE ICMPCODE DATA IPDATA IPV IPV4 IPV6 TOOBIG NEXT
%token	<k>	IPOFFSET IPFRAG IPDONTFRAG IPFIRSTFRAG IPMOREFRAG IPHLEN BIND
%token	<k>	INPUT OUTPUT RETURN BROADCAST SET CHANGE FROM TO BREAK
%token	<k>	IP ICMP IGMP GGP IPIP TCP EGP PUP UDP IDP TP EON ENCAP OSPF
%token  <k>     ROUTING FRAGMENT NONE DSTOPTS ESP AH ICMPV6
%token	<k>	USER CALL CHAIN ACCUMULATOR CASE SWITCH DEFAULT BLOCK BREAK
%token	<k>	PACKETLENGTH FASTFWD LOCAL IPV4inIPV4 IPV6inIPV4 IPV4inIPV6
%token	<k>	IPV6inIPV6 IPLEN IPDATALEN CHKSRC PREHEADER DECAPSULATED
%token	<k>	NEXTHOP REQUEST TIME DAY

%token		AND OR NOT
%token	<cmds>	CMD IFACE
%token	<k>	NUMBER
%token	<q>	QNUMBER
%token	<s>	STRING

%left	ELSE
%left	OR
%left	AND
%right	NOT

%%

start	: {
		allocatevar(&dst_v, "_DSTADDR");
		allocatevar(&hlen_v, "_HEADERLENGTH");
		allocatevar(&hlen1_v, "_HEADERLENGTH1");
		allocatevar(&proto_v, "_PROTOCOL");
		allocatevar(&src_v, "_SRCADDR");
		allocatevar(&version_v, "_VERSION");
		allocatevar(&off_v, "_OFFSET");
		allocatevar(&foff_v, "_FLAGOFFSET");
		allocatevar(&time_v, "_TIMEOFDAY");
	} cmdlist
	{
		if (error == 0) {
			cmd_t *c;

			c = emit(NULL, "// Filter preamble");
			if (version_v->location ||
			    proto_v->location ||
			    hlen_v->location ||
			    src_v->location ||
			    dst_v->location ||
			    foff_v->location ||
			    off_v->location) {
				emit(c, "ld	[0:1]");
				emit(c, "rsh	# 4");
				emitd(c, version_v, "st	M[%%d]");
				if (proto_v->location ||
				    hlen_v->location ||
				    src_v->location ||
				    dst_v->location ||
				    foff_v->location ||
				    off_v->location) {
					long L1 = nextlabel();
					long L2 = nextlabel();
					if (ipv6) {
						emit(c, "jeq	# 6 - L%d", L1);
						emitd(c, src_v, "ld	[8 : 16]");
						emitd(c, src_v, "st	M[%%d : 16]");
						emitd(c, dst_v, "ld	[24 : 16]");
						emitd(c, dst_v, "st	M[%%d : 16]");
						emit(c, "ldx	msh [0]");
						emitd(c, proto_v, "st	M[%%d]");
						emitd(c, hlen_v, "stx	M[%%d]");
						emit(c, "jmp	L%d", L2);
						emit(c, "L%d:", L1);
					}
					emitd(c, src_v, "ld	[12 : 4]");
					if (ipv6)
					emitd(c, src_v, "zmem	M[%%d]");
					emitd(c, src_v, "st	M[%%d]");

					emitd(c, dst_v, "ld	[16 : 4]");
					if (ipv6)
					emitd(c, dst_v, "zmem	M[%%d]");
					emitd(c, dst_v, "st	M[%%d]");

					emitd(c, proto_v, "ld	[9 : 1]");
					emitd(c, proto_v, "st	M[%%d]");

					emitd(c, hlen_v, "ldx	4 * ([0] & 0xf)");
					emitd(c, hlen_v, "stx	M[%%d]");

					emitd(c, foff_v,"ld	[6 : 2]");
					emitd(c, foff_v,"st	M[%%d]");
					emitd(c, off_v,	"and	# 0x1fff");
					emitd(c, off_v,	"lsh	# 3");
					emitd(c, off_v, "st	M[%%d]");
					if (ipv6)
						emit(c, "L%d:", L2);
				}
			}

			if (time_v->location) {
				emitd(c, time_v, "ld	time");
				emitd(c, time_v, "st	M[%%d]");
			}
			emit(c, "// End of preamble");

			append(c, $2);
			$2 = c;
				
			emitcode($2);
			while (filter_list != NULL) {
				if (filter_list->usecnt > 0) {
					fprintf(asmout, "L%d: // %s\n",
					    filter_list->label->where,
					    filter_list->name);
					def = filter_list->def;
					emitcode(filter_list->cmds);
				} else
					fprintf(stderr,
					    "warning: filter %s not used\n",
					    filter_list->name);
				filter_list = filter_list->next;
			}
		}
	}
	;

filter	: FILTER STRING { start_filter($2); } '{' cmdlist '}'
	{
		if (cf != NULL) {
			cf->next = filter_list;
			filter_list = cf;
			cf->cmds = $5;
			cf = NULL;
		}
	}
	;

cmdlist	: stat
	{
		$$ = $1;
	}
	| stat cmdlist
	{
		$$ = $1;
		append($$, $2);
	}
	;

ecmdlist: /* EMPTY */
	{
		$$ = emit(NULL, "// nop");
	}
	| cmdlist
	{
		$$ = $1;
	}
	;
	
stat	: cmd
	{
		$$ = $1;
	}
	| filter
	{
		$$ = emit(NULL, "// filter");
	}
	| cond '{' cmdlist '}'
	{
		$$ = $1->cmds;
		emitlabels($$, $1->true);
		append($$, $3);
		emitlabels($$, $1->false);
	}
	| cond '{' cmdlist '}' ELSE '{' cmdlist '}'
	{
		long tl = nextlabel();

		$$ = $1->cmds;
		emitlabels($$, $1->true);
		append($$, $3);
		if (checkret($$) == 0)
			emit($$, "jmp	L%d", tl);
		emitlabels($$, $1->false);
		append($$, $7);
		emit($$, "L%d:", tl);
	}
	| protocol '{' cmdlist '}' { protocol = 4; }
	{
		$$ = $1->cmds;
		emitlabels($$, $1->true);
		append($$, $3);
		emitlabels($$, $1->false);
	}
	| ipv4inipv4 '{' cmdlist '}'
	{
		if (protocol != 4) 
			yyerror("IPv4inIPv4 only valid for IPv4 packets");

		
		$$ = $1->cmds;
		emitlabels($$, $1->true);

		/*
		 * If we are IPV4 in IPV4 then execute the commands
		 * the user requested.  The commands are executed on
		 * the wrapper and not on the packet inside.
		 */
		append($$, $3);

		/*
		 * Now we conditionally reload our prestored variables with
		 * the nested packet.
		 * This will cause the rest of the filter to examine the
		 * packet on the inside rather than the wrapper packet
		 */
		assignvar(&version_v, NULL);
		assignvar(&hlen_v, NULL);
		assignvar(&proto_v, NULL);

		emit($$, "ld	M[%d]", version_v->location);
		emit($$, "jeq	# 4 - L%d", $1->False);
		emit($$, "ld	M[%d]", proto_v->location);
		emit($$, "jeq	# %d - L%d", IPPROTO_IPIP, $1->False);

		emit($$, "ldx	M[%d]", hlen_v->location);
		emit($$, "ld	[X + 0 : 1]");
		emit($$, "rsh	# 4");
		emit($$, "jeq	# 4 - L%d", $1->False);

		emitd($$, src_v, "ld	[X + 12 : 4]");
		emitd($$, src_v, "zmem   M[%%d]");
		emitd($$, src_v, "st     M[%%d]");

		emitd($$, dst_v, "ld	[X + 16 : 4]");
		emitd($$, dst_v, "zmem   M[%%d]");
		emitd($$, dst_v, "st     M[%%d]");

		emitd($$, proto_v, "ld	[X + 9 : 1]");
		emitd($$, proto_v, "zmem   M[%%d]");
		emitd($$, proto_v, "st     M[%%d]");

		emitd($$, hlen1_v, "ld     M[%d]", hlen_v->location);
		emitd($$, hlen1_v, "st     M[%%d]");
		emit($$, "txa");
		emit($$, "ldx	4 * ([X + 0] & 0xf)");
		emit($$, "add	X");
		emit($$, "zmem   M[%d]", hlen_v->location);
		emit($$, "st     M[%d]", hlen_v->location);

		emitlabels($$, $1->false);
	}
	| ipv6inipv4 '{' cmdlist '}' { protocol = 6; }
	{
		$$ = $1->cmds;
		emitlabels($$, $1->true);

		/*
		 * If we are IPV6 in IPV4 then execute the commands
		 * the user requested.  The commands are executed on
		 * the wrapper and not on the packet inside.
		 */
		append($$, $3);

		/*
		 * Now we conditionally reload our prestored variables with
		 * the nested packet.
		 * This will cause the rest of the filter to examine the
		 * packet on the inside rather than the wrapper packet
		 */
		assignvar(&version_v, NULL);
		assignvar(&hlen_v, NULL);
		assignvar(&proto_v, NULL);

		emit($$, "ld	M[%d]", version_v->location);
		emit($$, "jeq	# 4 - L%d", $1->False);
		emit($$, "ld	M[%d]", proto_v->location);
		emit($$, "jeq	# %d - L%d", IPPROTO_IPV6, $1->False);

		emit($$, "ldx	M[%d]", hlen_v->location);
		emit($$, "ld	[X + 0 : 1]");
		emit($$, "rsh	# 4");
		emit($$, "jeq	# 6 - L%d", $1->False);

		emitd($$, version_v, "st M[%%d]");
		emitd($$, src_v, "ld     [X + 8 : 16]");
		emitd($$, src_v, "st     M[%%d : 16]");
		emitd($$, dst_v, "ld     [X + 24 : 16]");
		emitd($$, dst_v, "st     M[%%d : 16]");

		emitd($$, hlen1_v, "ld     M[%d]", hlen_v->location);
		emitd($$, hlen1_v, "st     M[%%d]");

		emit($$, "ldx    mshx [0]");
		emitd($$, proto_v, "st   M[%%d]");
		emitd($$, hlen_v, "stx   M[%%d]");

		emitd($$, proto_v, "ld	[X + 9 : 1]");
		emitd($$, proto_v, "zmem   M[%%d]");
		emitd($$, proto_v, "st     M[%%d]");

		emitlabels($$, $1->false);
	}
	| STRING '=' what ';'
	{
		var_t *v = NULL;

		assignvar(&v, $1);

		if (v) {
			v->state = state;
			$$ = $3;
			emit($$, "st	M[%d]", v->location);
		}
		state = 0;
	}
	| BLOCK '{' cases lastcase '}'
	{
		$$ = $3->cmds;
		if ($4 != NULL) {
			if (checkret($$) == 0)
				emit($$, "jmp     L%d", $4->false->where);
			emitlabels($$, $3->false);
			append($$, $4->cmds);
			$3->false = $4->false;
		}
		emitlabels($$, $3->false);
	}
	| SWITCH swhat { start_switch(); }  '{' scases { sw = sw->next; } '}'
	{
		u_long lastmask;
		long nl, cl;
		jmp_t *ll;
		range_t *r;
		int nj;
		char cb[32];

		$$ = $2;

		/*
		 * If we have any masks, save off A in X
		 */
		lastmask = 0xffffffff;
		r = $5;
		while (r) {
			if ((r->mask & lastmask) != r->mask) {
				emit($$, "st	M[0]");
				break;
			}
			if (r->mask)
				lastmask = r->mask;
			else if (lastmask != 0xffffffff) {
				emit($$, "st	M[0]");
				break;
			}
			r = r->next;
		}
		ll = 0;
		nl = 0;
		cl = 0;
		nj = 1;

		lastmask = 0xffffffff;
		r = $5;
		while (r) {
			if (ll == 0)
				ll = nextlabelp(0);
			if (nj == 0)
				emit($$, "jmp	L%d", ll->where);
			if (nl)
				emit($$, "L%d:", nl);
			nl = r->next ? nextlabel() : ll->where;

			if (r->type == '&') {
				if ((lastmask & r->mask) != r->mask) {
					emit($$, "ld	M[0]");
					lastmask = 0xffffffff;
				}
			} else if (r->type != ' ' && lastmask != 0xffffffff) {
				emit($$, "ld	M[0]");
				lastmask = 0xffffffff;
			}

			if (r->cmds) {
				if (r->cmds->next == NULL &&
				    strncmp(r->cmds->cmd, "\t//", 3) == 0) {
					if (cl) {
						sprintf(cb, "L%ld", cl);
						appendlabeln(ll, cl);
						cl = 0;
					} else
						sprintf(cb, "L%d", ll->where);
					r->cmds = NULL;
					nj = 1;
				} else {
					cb[0] = '-';
					cb[1] = '\0';
					nj = checkret(r->cmds);
				}
			} else {
				if (cl == 0)
					cl = nextlabel();
				nj = 1;
				sprintf(cb, "L%ld", cl);
			}

			switch (r->type) {
			default:
				yyerror("unknown range type");
				break;
			case ' ':
				break;
			case '&':
				if (lastmask != r->mask)
					emit($$, "and	# 0x%08x",
						r->mask);
				lastmask = r->mask;
				emit($$, "jeq	%s %s L%d", rpmin(r), cb, nl);
				break;
			case 0:
				emit($$, "jeq	%s %s L%d", rpmin(r), cb, nl);
				break;
			case '>' + '=':
				emit($$, "jge	%s %s L%d", rpmin(r), cb, nl);
				break;
			case '>':
				emit($$, "jgt	%s %s L%d", rpmin(r), cb, nl);
				break;
			case '<' + '=':
				emit($$, "jle	%s %s L%d", rpmin(r), cb, nl);
				break;
			case '<':
				emit($$, "jlt	%s %s L%d", rpmin(r), cb, nl);
				break;
			case '/':
				emit($$, "jnet	%s/%d %s L%d", rpmin(r), r->mask, cb, nl);
				break;
			case '-':
				emit($$, "jlt	%s L%d -", rpmin(r), nl);
				emit($$, "jle	%s %s L%d", rpmax(r), cb, nl);
				break;
			case 'i':
                                emit($$, "jeq	# interface(%s) %s L%d",
				    r->iface->cmd, cb, nl);
				break;
			}
			if (r->cmds) {
				if (cl) {
					emit($$, "L%d:", cl);
					cl = 0;
				}
				append($$, r->cmds);
			}
				
			r = r->next;
		}
		if (cl)
			emit($$, "L%d:", cl);
		if (nl && (ll == NULL || nl != ll->where))
			emit($$, "L%d:", nl);
		emitlabels($$, ll);
		state = 0;
	}
	;

lastcase:	/* EMPTY */
	{
		$$ = NULL;	
	}
	| DEFAULT ':' cmdlist BREAK ';'
	{
		$$ = malloc(sizeof(node_t));

		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);
		$$->cmds = $3;
	}
	| DEFAULT ':' cmdlist
	{
		if (checkret($3) == 0) {
			int lc = linecnt;
			linecnt = $1;
			yyerror("missing break at end of case");
			linecnt = lc;
		}

		$$ = malloc(sizeof(node_t));

		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);
		$$->cmds = $3;
	}
	;

cases	: CASE cond ':' ecmdlist BREAK ';'
	{
		$$ = malloc(sizeof(node_t));

		$$->true = $2->true;
		$$->false = $2->false;
		$$->cmds = $2->cmds;
		emitlabels($$->cmds, $2->true);
		append($$->cmds, $4);
	}
	| CASE cond ':' cmdlist
	{
		if (checkret($4) == 0) {
			int lc = linecnt;
			linecnt = $1;
			yyerror("missing break at end of case");
			linecnt = lc;
		}

		$$ = malloc(sizeof(node_t));

		$$->true = $2->true;
		$$->false = $2->false;
		$$->cmds = $2->cmds;
		emitlabels($$->cmds, $2->true);
		append($$->cmds, $4);
	}
	| CASE cond ':' cmdlist cases
	{
		if (checkret($4) == 0) {
			int lc = linecnt;
			linecnt = $1;
			yyerror("missing break at end of case");
			linecnt = lc;
		}

		$$ = malloc(sizeof(node_t));

		$$->true = $2->true;
		$$->false = $5->false;
		$$->cmds = $2->cmds;

		emitlabels($$->cmds, $2->true);
		append($$->cmds, $4);
		if (checkret($$->cmds) == 0)
			emit($$->cmds, "jmp	L%d", $5->false->where);

		emitlabels($$->cmds, $2->false);
		append($$->cmds, $5->cmds);
	}
	| CASE cond ':' ecmdlist BREAK ';' cases
	{
		$$ = malloc(sizeof(node_t));

		$$->true = $2->true;
		$$->false = $7->false;
		$$->cmds = $2->cmds;

		emitlabels($$->cmds, $2->true);
		append($$->cmds, $4);
		if (checkret($$->cmds) == 0)
			emit($$->cmds, "jmp	L%d", $7->false->where);

		emitlabels($$->cmds, $2->false);
		append($$->cmds, $7->cmds);
	}
	;

srange	: { state = sw->state; } range
	{
		if (rangesize && rangesize != $2->size)
			yyerror("inappropriate size of element");
		$$ = $2;
		state = 0;
	}
	;

scases	: CASE srange ':' ecmdlist BREAK ';'
	{
		$$ = $2;
		$$->cmds = $4;
	}
	| CASE srange ':' cmdlist
	{
		if (checkret($4) == 0) {
			int lc = linecnt;
			linecnt = $1;
			yyerror("missing break at end of case");
			linecnt = lc;
		}

		$$ = $2;
		$$->cmds = $4;
	}
	| DEFAULT ':' ecmdlist BREAK ';'
	{
		$$ = malloc(sizeof(range_t));
		memset($$, 0, sizeof(range_t));
		$$->size = 4;
		$$->type = ' ';
		$$->cmds = $3;
	}
	| DEFAULT ':' cmdlist
	{
		if (checkret($3) == 0) {
			int lc = linecnt;
			linecnt = $1;
			yyerror("missing break at end of case");
			linecnt = lc;
		}

		$$ = malloc(sizeof(range_t));
		memset($$, 0, sizeof(range_t));
		$$->size = 4;
		$$->type = ' ';
		$$->cmds = $3;
	}
	| CASE srange ':' scases
	{
		$$ = $2;
		$$->cmds = NULL;
		$$->next = $4;
	}
	| CASE srange ':' ecmdlist BREAK ';' scases
	{
		$$ = $2;
		$$->cmds = $4;
		$$->next = $7;
	}
	| CASE srange ':' cmdlist scases
	{
		if (checkret($4) == 0) {
			int lc = linecnt;
			linecnt = $1;
			yyerror("missing break at end of case");
			linecnt = lc;
		}

		$$ = $2;
		$$->cmds = $4;
		$$->next = $5;
	}
	;

cond	: cond OR cond
	{
		$$ = malloc(sizeof(node_t));
		$$->cmds = $1->cmds;
		emitlabels($$->cmds, $1->false);
		append($$->cmds, $3->cmds);
		$$->true = $1->true;
		$$->false = $3->false;
		appendlabel($$->true, $3->true);
	}
	| cond AND cond
	{
		$$ = malloc(sizeof(node_t));
		$$->cmds = $1->cmds;
		emitlabels($$->cmds, $1->true);
		append($$->cmds, $3->cmds);
		$$->true = $3->true;
		$$->false = $1->false;
		appendlabel($$->false, $3->false);
	}
	| '(' cond ')'
	{
		$$ = $2;
	}
	| NOT cond
	{
		$$ = malloc(sizeof(node_t));
		$$->cmds = $2->cmds;
		$$->true = $2->false;
		$$->false = $2->true;
	}
	| CALL '(' NUMBER ')'
	{
		$$ = malloc(sizeof(node_t));

		$$->cmds = emit(NULL, "ld	R[%d]", IPFWM_POINT);
		emit($$->cmds, "call	#%d", $3);
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);
		emit($$->cmds, "jset	# 0x%x L%d L%d", IPFW_ACCEPT,
		    $$->True, $$->False);
	}
	| CALL '(' NUMBER ':' NUMBER')'
	{
		$$ = malloc(sizeof(node_t));

		if ($5 < 0 || $5 > 15)
			yyerror("filter specific data out of range");

		$$->cmds = emit(NULL, "ld	R[%d]", IPFWM_POINT);
		emit($$->cmds, "or	# 0x%x", $5 << 12);
		emit($$->cmds, "call	#%d", $3);
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);
		emit($$->cmds, "jset	# 0x%x L%d L%d", IPFW_ACCEPT,
		    $$->True, $$->False);
	}
	| CALL '(' STRING ')'
	{
		$$ = malloc(sizeof(node_t));

		$$->cmds = emit(NULL, "ld	R[%d]", IPFWM_POINT);
		emit($$->cmds, "call	# filter(%s)", $3);
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);
		emit($$->cmds, "jset	# 0x%x L%d L%d", IPFW_ACCEPT,
		    $$->True, $$->False);
	}
	| CALL '(' STRING ':' NUMBER ')'
	{
		$$ = malloc(sizeof(node_t));

		if ($5 < 0 || $5 > 15)
			yyerror("filter specific data out of range");

		$$->cmds = emit(NULL, "ld	R[%d]", IPFWM_POINT);
		emit($$->cmds, "or	# 0x%x", $5 << 12);
		emit($$->cmds, "call	# filter(%s)", $3);
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);
		emit($$->cmds, "jset	# 0x%x L%d L%d", IPFW_ACCEPT,
		    $$->True, $$->False);
	}
	| what '(' ranges ')'
	{
		u_long lastmask = 0xffffffff;
		long tl;
		range_t *r = $3;
		$$ = malloc(sizeof(node_t));

		$$->cmds = $1;
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);

		/*
		 * If we have any masks, save off A in X
		 */
		while (r) {
			if ((r->mask & lastmask) != r->mask) {
				emit($$->cmds, "st	M[0]");
				break;
			}
			if (r->mask)
				lastmask = r->mask;
			else if (lastmask != 0xffffffff) {
				emit($$->cmds, "st	M[0]");
				break;
			}
			r = r->next;
		}

		lastmask = 0xffffffff;
		r = $3;
		while (r) {
			if (r->type == '&') {
				if ((lastmask & r->mask) != r->mask) {
					emit($$->cmds, "ld	M[0]");
					lastmask = 0xffffffff;
				}
			} else if (lastmask != 0xffffffff) {
				emit($$->cmds, "ld	M[0]");
				lastmask = 0xffffffff;
			}
			switch (r->type) {
			default:
				yyerror("unknown range type");
				break;
			case '&':
				if (lastmask != r->mask)
					emit($$->cmds, "and	# 0x%08x",
						r->mask);
				lastmask = r->mask;
				if (r->next)
					emit($$->cmds, "jeq	%s L%d -",
					    rpmin(r), $$->True);
				else
					emit($$->cmds, "jeq	%s L%d L%d",
					    rpmin(r), $$->True, $$->False);
				break;
			case 0:
				if (r->next)
					emit($$->cmds, "jeq	%s L%d -",
					    rpmin(r), $$->True);
				else
					emit($$->cmds, "jeq	%s L%d L%d",
					    rpmin(r), $$->True, $$->False);
				break;
			case '/':
				if (r->next)
					emit($$->cmds, "jnet	%s/%d L%d -",
					    rpmin(r), r->mask, $$->True, $$->False);
				else
					emit($$->cmds, "jnet	%s/%d L%d L%d",
					    rpmin(r), r->mask, $$->True, $$->False);
				break;
			case '>' + '=':
				if (r->next)
					emit($$->cmds, "jge	%s L%d -",
					    rpmin(r), $$->True);
				else
					emit($$->cmds, "jge	%s L%d L%d",
					    rpmin(r), $$->True, $$->False);
				break;
			case '>':
				if (r->next)
					emit($$->cmds, "jgt	%s L%d -",
					    rpmin(r), $$->True);
				else
					emit($$->cmds, "jgt	%s L%d L%d",
					    rpmin(r), $$->True, $$->False);
				break;
			case '<' + '=':
				if (r->next)
					emit($$->cmds, "jle	%s L%d -",
					    rpmax(r), $$->True);
				else
					emit($$->cmds, "jle	%s L%d L%d",
					    rpmax(r), $$->True, $$->False);
				break;
			case '<':
				if (r->next)
					emit($$->cmds, "jlt	%s L%d -",
					    rpmax(r), $$->True);
				else
					emit($$->cmds, "jlt	%s L%d L%d",
					    rpmax(r), $$->True, $$->False);
				break;
			case '-':
				tl = r->next ? nextlabel() : $$->False;
				if (lastmask != 0xffffffff) {
					emit($$->cmds, "ld	M[0]");
					lastmask = 0xffffffff;
				}
				emit($$->cmds, "jlt	%s L%d -",
				    rpmin(r), tl);
				emit($$->cmds, "jle	%s L%d L%d",
				    rpmax(r), $$->True, tl);
				if (tl != $$->False)
					emit($$->cmds, "L%d:", tl);
				break;
			}
			r = r->next;
		}
		state = 0;
	}
	| tcpflags '(' ranges ')'
	{
		u_long lastmask = 0xffffffff;
		range_t *r = $3;
		$$ = malloc(sizeof(node_t));

		assignvar(&hlen_v, NULL);
		$$->cmds = emit(NULL, "ldx	M[%d]", hlen_v->location);
		emit($$->cmds, "ld	[x + 13 : 1]");
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);

		/*
		 * If we have any masks, save off A in X
		 */
		while (r) {
			if (r->mask > lastmask) {
				emit($$->cmds, "st	M[0]");
				break;
			}
			if (r->mask)
				lastmask = r->mask;
			else if (lastmask != 0xffffffff) {
				emit($$->cmds, "st	M[0]");
				break;
			}
			r = r->next;
		}

		lastmask = 0xffffffff;
		r = $3;
		while (r) {
			if (r->type == '&') {
				if ((lastmask & r->mask) != r->mask) {
					emit($$->cmds, "ld	M[0]");
					lastmask = 0xffffffff;
				}
			} else if (lastmask != 0xffffffff) {
				emit($$->cmds, "ld	M[0]");
				lastmask = 0xffffffff;
			}

			switch (r->type) {
			case '&':
				if (lastmask != r->mask)
					emit($$->cmds, "and	# 0x%08x",
						r->mask);
				lastmask = r->mask;
				if (r->next)
					emit($$->cmds, "jeq	%s L%d -",
					    rpmin(r), $$->True);
				else
					emit($$->cmds, "jeq	%s L%d L%d",
					    rpmin(r), $$->True, $$->False);
				break;
			case 0:
				if (r->next)
					emit($$->cmds, "jset	%s L%d -",
					    rpmin(r), $$->True);
				else
					emit($$->cmds, "jset	%s L%d L%d",
					    rpmin(r), $$->True, $$->False);
				break;
			default:
				yyerror("tcpflags() can only take values and masks");
			}
			r = r->next;
		}
	}
	| ESTABLISHED
	{
		$$ = malloc(sizeof(node_t));

		assignvar(&hlen_v, NULL);
		$$->cmds = emit(NULL, "ldx	M[%d]", hlen_v->location);
		emit($$->cmds, "ld	[x + 13 : 1]");
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);
		emit($$->cmds, "and	# 0x%02x", TH_FIN|TH_SYN|TH_RST|TH_ACK);
		emit($$->cmds, "jeq	# 0x%02x L%d -", TH_ACK, $$->True);
		emit($$->cmds, "jeq	# 0x%02x L%d -", TH_ACK|TH_SYN, $$->True);
		emit($$->cmds, "jeq	# 0x%02x L%d -", TH_ACK|TH_FIN, $$->True);
		emit($$->cmds, "jeq	# 0x%02x L%d -", TH_RST, $$->True);
		emit($$->cmds, "jeq	# 0x%02x L%d L%d", TH_ACK|TH_RST,
		    $$->True, $$->False);
	}
	| TCP REQUEST
	{
		$$ = malloc(sizeof(node_t));

		assignvar(&hlen_v, NULL);
		$$->cmds = emit(NULL, "ldx	M[%d]", hlen_v->location);
		emit($$->cmds, "ld	[x + 13 : 1]");
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);
		emit($$->cmds, "and	# 0x%02x", TH_FIN|TH_SYN|TH_RST|TH_ACK);
		emit($$->cmds, "jeq	# 0x%02x L%d L%d", TH_SYN,
		    $$->True, $$->False);
	}
	| DECAPSULATED
	{
		$$ = malloc(sizeof(node_t));

		assignvar(&hlen_v, NULL);
		$$->cmds = emit(NULL, "ld	R[%d]", IPFWM_EXTRA);
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);
		emit($$->cmds, "JNE	# 0x0 L%d L%d", $$->True, $$->False);
	}
	| FORWARDING
	{
		$$ = malloc(sizeof(node_t));

		$$->cmds = emit(NULL, "ld	R[%d]", IPFWM_MFLAGS);
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);
		emit($$->cmds, "jset	# 0x%04x L%d L%d", M_FORW,
		    $$->True, $$->False);
	}
	| BROADCAST
	{
		$$ = malloc(sizeof(node_t));

		$$->cmds = emit(NULL, "ld	R[%d]", IPFWM_MFLAGS);
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);
		emit($$->cmds, "jset	# 0x%04x L%d L%d", M_BCAST,
		    $$->True, $$->False);
	}
	| TOOBIG
	{
		$$ = malloc(sizeof(node_t));
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);

		assignvar(&foff_v, NULL);
		assignvar(&off_v, NULL);
		$$->cmds = emit(NULL, "ld	[2 : 2]");
		emit($$->cmds, "tax");
		emit($$->cmds, "ld       M[%d]", off_v->location);
		emit($$->cmds, "add	X");
		emit($$->cmds, "jgt	# 0xffff L%d L%d", $$->True, $$->False);
		$$->cmds->line = $1;
	}
	| which interface '(' ifaces ')'
	{
		cmd_t *r = $4;
		$$ = malloc(sizeof(node_t));

		$$->cmds = emit(NULL, "ld	R[%d]", $1);
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);

		while (r) {
			if (r->next)
				emit($$->cmds, "jeq	# interface(%s) L%d -",
				    r->cmd, $$->True);
			else
				emit($$->cmds, "jeq	# interface(%s) L%d L%d",
				    r->cmd, $$->True, $$->False);
			r = r->next;
		}
		state = 0;
	}
	| CHKSRC
	{
		$$ = malloc(sizeof(node_t));
 
		$$->cmds = emit(NULL, "ld       R[%d]", IPFWM_SRCIF);
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);
 
		emit($$->cmds, "ldx     R[%d]", IPFWM_SRCRT);
		emit($$->cmds, "jeq     X L%d L%d", $$->True, $$->False);
		state = 0;
	}
	| proto
	{
		$$ = malloc(sizeof(node_t));
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);
		assignvar(&proto_v, NULL);
		$$->cmds = emit(NULL, "ld	M[%d]", proto_v->location);
		emit($$->cmds, "jeq	# %d L%d L%d", $1, $$->True, $$->False);
	}
	| IPFRAG
	{
		$$ = malloc(sizeof(node_t));
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);
		assignvar(&foff_v, NULL);
		$$->cmds = emit(NULL, "ld	M[%d]", foff_v->location);
		emit($$->cmds, "and	# 0x%x", IP_OFFMASK | 0x2000);
		emit($$->cmds, "jeq	# 0 L%d L%d", $$->False, $$->True);
	}
	| IPDONTFRAG
	{
		$$ = malloc(sizeof(node_t));
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);
		assignvar(&foff_v, NULL);
		$$->cmds = emit(NULL, "ld	M[%d]", foff_v->location);
		emit($$->cmds, "jset	# 0x%x L%d L%d", IP_DF,
		    $$->True, $$->False);
	}
	| IPMOREFRAG
	{
		$$ = malloc(sizeof(node_t));
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);
		assignvar(&foff_v, NULL);
		$$->cmds = emit(NULL, "ld	M[%d]", foff_v->location);
		emit($$->cmds, "jset	# 0x%x L%d L%d", IP_MF,
		    $$->True, $$->False);
	}
	| IPFIRSTFRAG
	{
		$$ = malloc(sizeof(node_t));
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);
		assignvar(&foff_v, NULL);
		$$->cmds = emit(NULL, "ld	M[%d]", foff_v->location);
		emit($$->cmds, "and	# 0x%x", IP_OFFMASK | IP_MF);
		emit($$->cmds, "jeq	# 0x%x L%d L%d", IP_MF,
		    $$->True, $$->False);
	}
	;

protocol: IPV4
	{
		$$ = malloc(sizeof(node_t));
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);
		assignvar(&version_v, NULL);
		$$->cmds = emit(NULL, "ld	M[%d]", version_v->location);
		$$->cmds->line = $1;
		emit($$->cmds, "jeq	# 4 L%d L%d", $$->True, $$->False);
		protocol = 4;
	}
	| IPV6
	{
		$$ = malloc(sizeof(node_t));
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);
		assignvar(&version_v, NULL);
		$$->cmds = emit(NULL, "ld	M[%d]", version_v->location);
		$$->cmds->line = $1;
		emit($$->cmds, "jeq	# 6 L%d L%d", $$->True, $$->False);
		protocol = 6;
		ipv6 = 1;
	}
	;

ipv4inipv4 : IPV4inIPV4
	{
		$$ = malloc(sizeof(node_t));
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);

		assignvar(&version_v, NULL);
		assignvar(&hlen_v, NULL);
		assignvar(&proto_v, NULL);

		$$->cmds = emit(NULL, "ld	M[%d]", version_v->location);
		$$->cmds->line = $1;
		emit($$->cmds, "jeq	# 4 - L%d", $$->False);
		emit($$->cmds, "ld	M[%d]", proto_v->location);
		emit($$->cmds, "jeq	# %d - L%d", IPPROTO_IPIP, $$->False);

		emit($$->cmds, "ldx	M[%d]", hlen_v->location);
		emit($$->cmds, "ld	[X + 0 : 1]");
		emit($$->cmds, "rsh	# 4");
		emit($$->cmds, "jeq	# 4 L%d L%d", $$->True, $$->False);
		protocol = 4;
	}

ipv6inipv4 : IPV6inIPV4
	{
		$$ = malloc(sizeof(node_t));
		$$->true = nextlabelp(0);
		$$->false = nextlabelp(0);

		assignvar(&version_v, NULL);
		assignvar(&hlen_v, NULL);
		assignvar(&proto_v, NULL);

		$$->cmds = emit(NULL, "ld	M[%d]", version_v->location);
		$$->cmds->line = $1;
		emit($$->cmds, "jeq	# 4 - L%d", $$->False);
		emit($$->cmds, "ld	M[%d]", proto_v->location);
		emit($$->cmds, "jeq	# %d - L%d", IPPROTO_IPV6, $$->False);

		emit($$->cmds, "ldx	M[%d]", hlen_v->location);
		emit($$->cmds, "ld	[X + 0 : 1]");
		emit($$->cmds, "jeq	# 6 L%d L%d", $$->True, $$->False);
		protocol = 4;
	}
	;

proto	: IP
	| ICMP
	| IGMP
	| GGP
	| IPIP
	| TCP
	| EGP
	| PUP
	| UDP
	| IDP
	| TP
	| EON
	| ENCAP
	| OSPF
	| ROUTING
	| FRAGMENT
	| NONE
	| DSTOPTS
	| ESP   
	| AH    
	| ICMPV6
	;

cmd	: CMD
	{
		$$ = $1;
	}
	| BIND STRING ';'
	{
		filter_t *f = filter_list;

		while (f != NULL) {
			if (strcmp($2, f->name) == 0) {
				f->usecnt++;
				break;
			}
			f = f->next;
		}
		if (f == NULL)
			yyerror("bind to undeclared filter");
		$$ = emit(NULL, "jmp	L%d // %s", f ? f->label->where : 0,
		    f ? f->name : "undeclared filter");
		$$->line = $1;
	}
	| IPV6 ';'
	{
		$$ = emit(NULL, "// Allow IPv6");
		$$->line = $1;
		ipv6 = 1;
	}
	| DENY ';'
	{
		$$ = emit(NULL, "ret	# 0x%x", IPFW_REJECT);
		$$->line = $1;
	}
	| PERMIT ';'
	{
		$$ = emit(NULL, "ret	# 0x%x", IPFW_ACCEPT);
		$$->line = $1;
	}
	| FASTFWD ';'
	{
#ifdef	M_CANFASTFWD
		$$ = emit(NULL, "ld	R[%d]", IPFWM_MFLAGS);
		emit($$, "or	# 0x%x", M_CANFASTFWD);
		emit($$, "st	R[%d]", IPFWM_MFLAGS);
		$$->line = $1;
#else
		$$ = emit(NULL, "// M_CANFASTFWD not supported");
		fprintf(stderr, "warning: fastforward not supported.  Ignored.\n");
#endif
	}
	| LOCAL ';'
	{
		$$ = emit(NULL, "ld	R[%d]", IPFWM_MFLAGS);
		emit($$, "or	# 0x%x", M_LOCAL);
		emit($$, "st	R[%d]", IPFWM_MFLAGS);
		$$->line = $1;
	}
	| NEXT ';'
	{
		$$ = emit(NULL, "ret	# 0x%x", IPFW_NEXT);
		$$->line = $1;
	}
	| DENY '[' NUMBER ']' ';'
	{
		$$ = emit(NULL, "ret	# 0x%x",
		    (IPFW_REJECT|IPFW_REPORT) + $3);
		$$->line = $1;
	}
	| PERMIT '[' NUMBER ']' ';'
	{
		$$ = emit(NULL, "ret	# 0x%x",
		    (IPFW_ACCEPT|IPFW_REPORT) + $3);
		$$->line = $1;
	}
	| DENY '[' unumber ':' user ']' ';'
	{
		$$ = emit(NULL, "ret	# 0x%x",
		    (IPFW_REJECT|IPFW_REPORT|$5) + $3);
		$$->line = $1;
	}
	| PERMIT '[' unumber ':' user ']' ';'
	{
		$$ = emit(NULL, "ret	# 0x%x",
		    (IPFW_ACCEPT|IPFW_REPORT|$5) + $3);
		$$->line = $1;
	}
	| RETURN ';'
	{
		$$ = emit(NULL, "ret	A");
		$$->line = $1;
	}
	| RETURN '[' number ']' ';'
	{
		$$ = emit(NULL, "ret	# 0x%x", $3);
		$$->line = $1;
	}
	| CALL '(' NUMBER ')' ';'
	{
		$$ = emit(NULL, "ld	R[%d]", IPFWM_POINT);
		emit($$, "call	#%d", $3);
	}
	| CALL '(' NUMBER ':' NUMBER')' ';'
	{
		if ($5 < 0 || $5 > 15)
			yyerror("filter specific data out of range");

		$$ = emit(NULL, "ld	R[%d]", IPFWM_POINT);
		emit($$, "or	# 0x%x", $5 << 12);
		emit($$, "call	#%d", $3);
	}
	| CALL '(' STRING ')' ';'
	{
		$$ = emit(NULL, "ld	R[%d]", IPFWM_POINT);
		emit($$, "call	# filter(%s)", $3);
	}
	| CALL '(' STRING ':' NUMBER ')' ';'
	{
		if ($5 < 0 || $5 > 15)
			yyerror("filter specific data out of range");

		$$ = emit(NULL, "ld	R[%d]", IPFWM_POINT);
		emit($$, "or	# 0x%x", $5 << 12);
		emit($$, "call	# filter(%s)", $3);
	}
	| CHAIN '(' NUMBER ')' ';'
	{
		$$ = emit(NULL, "ld	R[%d]", IPFWM_POINT);
		emit($$, "call	#%d", $3);
		emit($$, "and	# 0x%0x", ~(IPFW_REPORT|IPFW_SIZE));
		emit($$, "ret	A");
	}
	| CHAIN '(' STRING ')' ';'
	{
		$$ = emit(NULL, "ld	R[%d]", IPFWM_POINT);
		emit($$, "call	# filter(%s)", $3);
		emit($$, "and	# 0x%0x", ~(IPFW_REPORT|IPFW_SIZE));
		emit($$, "ret	A");
	}
	| IMPLICIT DENY ';'
	{
		$$ = emit(NULL, "// implicit deny");
		$$->line = $1;
		setdef(IPFW_REJECT);
	}
	| IMPLICIT DENY '[' NUMBER ']' ';'
	{
		$$ = emit(NULL, "// implicit deny [%d]", $4);
		$$->line = $1;
		setdef((IPFW_REJECT|IPFW_REPORT) + $4);
	}
	| IMPLICIT DENY '[' unumber ':' user ']' ';'
	{
		$$ = emit(NULL, "// implicit deny [%d : %x]", $4, $6);
		$$->line = $1;
		setdef((IPFW_REJECT|IPFW_REPORT|$6) + $4);
	}
	| IMPLICIT PERMIT ';'
	{
		$$ = emit(NULL, "// implicit permit");
		$$->line = $1;
		setdef(IPFW_ACCEPT);
	}
	| IMPLICIT NEXT ';'
	{
		$$ = emit(NULL, "// implicit NEXT");
		$$->line = $1;
		setdef(IPFW_NEXT);
	}
	| IMPLICIT PERMIT '[' NUMBER ']' ';'
	{
		$$ = emit(NULL, "// implicit permit [%d]", $4);
		$$->line = $1;
		setdef((IPFW_ACCEPT|IPFW_REPORT) + $4);
	}
	| IMPLICIT PERMIT '[' unumber ':' user ']' ';'
	{
		$$ = emit(NULL, "// implicit permit [%d : %x]", $4, $6);
		$$->line = $1;
		setdef((IPFW_ACCEPT|IPFW_REPORT|$6) + $4);
	}
	| CHANGE SRCADDR FROM number TO number ';'
	{
		int sum;
		int label = nextlabel();

		if (protocol != 4)
			yyerror("change srcaddr only available for IPv4\n");

		if ($4 != $6) {
			sum = 0;
			sum += ($4 >> 16) & 0xffff;
			sum += $4 & 0xffff;
			sum += (~$6 >> 16) & 0xffff;
			sum += ~$6 & 0xffff;
			sum = (sum & 0xffff) + ((sum >> 16) & 0xffff);
			if (sum > 0xffff)
				sum -= 0xffff;
			assignvar(&hlen_v, NULL);
			$$ = emit(NULL, "ldx	M[%d]", hlen_v->location);
			emit($$, "ld	[x + 16 : 2]");
			emit($$, "jlt	# 0x%x L%d -", (~sum) & 0xffff, label);
			emit($$, "add	# 1");
			emit($$, "L%d:", label);
			emit($$, "add	# 0x%x", sum);
			emit($$, "st	[x + 16 : 2]");
			emit($$, "ld	# 0x%x", $6);
			emit($$, "st	[16 : 4]");
			$$->line = $1;
		} else
			$$ = emit(NULL, "// skipped change srcaddr");
	}
	| CHANGE DSTADDR FROM number TO number ';'
	{
		int sum;
		int label = nextlabel();

		if (protocol != 4)
			yyerror("change dstaddr only available for IPv4\n");

		if ($4 != $6) {
			sum = 0;
			sum += ($4 >> 16) & 0xffff;
			sum += $4 & 0xffff;
			sum += (~$6 >> 16) & 0xffff;
			sum += ~$6 & 0xffff;
			sum = (sum & 0xffff) + ((sum >> 16) & 0xffff);
			if (sum > 0xffff)
				sum -= 0xffff;
			assignvar(&hlen_v, NULL);
			$$ = emit(NULL, "ldx	M[%d]", hlen_v->location);
			emit($$, "ld	[x + 16 : 2]");
			emit($$, "jlt	# 0x%x L%d -", (~sum) & 0xffff, label);
			emit($$, "add	# 1");
			emit($$, "L%d:", label);
			emit($$, "add	# 0x%x", sum);
			emit($$, "st	[x + 16 : 2]");
			emit($$, "ld	# 0x%x", $6);
			emit($$, "st	[12 : 4]");
			$$->line = $1;
		} else
			$$ = emit(NULL, "// skipped change dstaddr");
	}

	| CHANGE SRCPORT FROM number TO number ';'
	{
		int sum;
		int label = nextlabel();

		if (protocol != 4)
			yyerror("change srcaddr only available for IPv4\n");
		if (($4 & ~0xffff) || ($6 & ~0xffff))
			yyerror("ports must be between 0 and 65536\n");

		if ($4 != $6) {
			sum = 0;
			sum += $4 & 0xffff;
			sum += ~$6 & 0xffff;
			sum = (sum & 0xffff) + ((sum >> 16) & 0xffff);
			if (sum > 0xffff)
				sum -= 0xffff;
			assignvar(&hlen_v, NULL);
			$$ = emit(NULL, "ldx	M[%d]", hlen_v->location);
			emit($$, "ld	[x + 16 : 2]");
			emit($$, "jlt	# 0x%x L%d -", (~sum) & 0xffff, label);
			emit($$, "add	# 1");
			emit($$, "L%d:", label);
			emit($$, "add	# 0x%x", sum);
			emit($$, "st	[x + 16 : 2]");
			emit($$, "ld	# 0x%x", $6);
			emit($$, "st	[x + 0 : 2]");
			$$->line = $1;
		} else
			$$ = emit(NULL, "// skipped change srcport");
	}
	| CHANGE DSTPORT FROM number TO number ';'
	{
		int sum;
		int label = nextlabel();

		if (protocol != 4)
			yyerror("change dstaddr only available for IPv4\n");
		if (($4 & ~0xffff) || ($6 & ~0xffff))
			yyerror("ports must be between 0 and 65536\n");

		if ($4 != $6) {
			sum = 0;
			sum += $4 & 0xffff;
			sum += ~$6 & 0xffff;
			sum = (sum & 0xffff) + ((sum >> 16) & 0xffff);
			if (sum > 0xffff)
				sum -= 0xffff;
			assignvar(&hlen_v, NULL);
			$$ = emit(NULL, "ldx	M[%d]", hlen_v->location);
			emit($$, "ld	[x + 16 : 2]");
			emit($$, "jlt	# 0x%x L%d -", (~sum) & 0xffff, label);
			emit($$, "add	# 1");
			emit($$, "L%d:", label);
			emit($$, "add	# 0x%x", sum);
			emit($$, "st	[x + 16 : 2]");
			emit($$, "ld	# 0x%x", $6);
			emit($$, "st	[x + 2: 2]");
			$$->line = $1;
		} else
			$$ = emit(NULL, "// skipped change dstport");
	}

	| SET SRCADDR '(' number ')' ';'
	{
		if (protocol != 4)
			yyerror("set srcaddr only available for IPv4\n");
		$$ = emit(NULL, "ld	# 0x%x", $4);
		emit($$, "st	[12 : 4]");
		$$->line = $1;
	}
	| SET DSTADDR '(' number ')' ';'
	{
		if (protocol != 4)
			yyerror("set dstaddr only available for IPv4\n");
		$$ = emit(NULL, "ld	# 0x%x", $4);
		emit($$, "st	[16 : 4]");
		$$->line = $1;
	}
	| SET NEXTHOP '(' number ')' ';'
	{
		if (protocol != 4)
			yyerror("set nexthop only available for IPv4\n");
		$$ = emit(NULL, "ld	# 0x%x", ntohl($4));
		emit($$, "st	R[%d]", IPFWM_DSTADDR);
		$$->line = $1;
	}
	| denyicmp '(' icmptype comma icmpcode ')' ';'
	{
		$$ = emit(NULL, "ld	# 0x%x", ($3 << 8) | $5);
		emit($$, "st	R[%d]", IPFWM_AUX);
		emit($$, "ret    # 0x%x", IPFW_REJECT|IPFW_ICMP);
		$$->line = $1;
	}
	| denyicmp '(' icmptype comma icmpcode ')' '[' number ']' ';'
	{
		$$ = emit(NULL, "ld	# 0x%x", ($3 << 8) | $5);
		emit($$, "st	R[%d]", IPFWM_AUX);
		emit($$, "ret    # 0x%x",
		    (IPFW_REJECT|IPFW_REPORT|IPFW_ICMP) + $8);
		$$->line = $1;
	}
	| denyicmp '(' icmptype comma icmpcode ')' '[' unumber ':' user ']' ';'
	{
		$$ = emit(NULL, "ld	# 0x%x", ($3 << 8) | $5);
		emit($$, "st	R[%d]", IPFWM_AUX);
		emit($$, "ret    # 0x%x",
		    (IPFW_REJECT|IPFW_REPORT|IPFW_ICMP|$10) + $8);
		$$->line = $1;
	}
	;

denyicmp: DENY ICMP
	{
		state = ICMPTYPE;
		$$ = $1;
	}
	;

icmptype: number
	{
		if ($1 > 0xff)
			yyerror("invalid icmp type");
		state = $1;
		$$ = $1;
	}
	;

icmpcode: number
	{
		if ($1 > 0xff)
			yyerror("invalid icmp code");
		state = 0;
		$$ = $1;
	}
	;

interface: INTERFACE
	{
		state = INTERFACE;
		$$ = $1;
	}
	;

tcpflags: TCPFLAGS
	{
		rangesize = 4;
		state = TCPFLAGS;
	}
	;

swhat	: what
	{
		$$ = $1;
	}
	| which interface
	{
		rangesize = 1;
		$$ = emit(NULL, "ld	R[%d]", $1);
	}
	;

what	: ACCUMULATOR
	{
		rangesize = 4;

		$$ = emit(NULL, "// degenerate test of accumulator");
		$$->line = $1;
	}
	| STRING 
	{
		var_t *v = findvar($1);

		rangesize = 4;

		if (v) {
			$$ = emit(NULL, "ld	M[%d]", v->location);
			state = v->state;
		} else {
			yyerror("unknown variable");
			state = 0;
		}
	}
	| SRCADDR
	{
		rangesize = 0;
		state = SRCADDR;

		assignvar(&src_v, NULL);
		if (ipv6)
			$$ = emit(NULL, "ld	M[%d:16]", src_v->location);
		else
			$$ = emit(NULL, "ld	M[%d]", src_v->location);
		$$->line = $1;
	}
	| DSTADDR
	{
		rangesize = 0;
		state = DSTADDR;

		assignvar(&dst_v, NULL);
		if (ipv6)
			$$ = emit(NULL, "ld	M[%d:16]", dst_v->location);
		else
			$$ = emit(NULL, "ld	M[%d]", dst_v->location);
		$$->line = $1;
	}
	| PROTOCOL
	{
		rangesize = 4;
		assignvar(&proto_v, NULL);
		$$ = emit(NULL, "ld	M[%d]", proto_v->location);
		$$->line = $1;
	}
	| SRCPORT
	{
		rangesize = 4;
		state = SRCPORT;

		assignvar(&hlen_v, NULL);
		$$ = emit(NULL, "ldx	M[%d]", hlen_v->location);
		$$->line = $1;
		emit($$, "ld	[x + 0 : 2]");
	}
	| DSTPORT
	{
		rangesize = 4;
		state = DSTPORT;

		assignvar(&hlen_v, NULL);
		$$ = emit(NULL, "ldx	M[%d]", hlen_v->location);
		$$->line = $1;
		emit($$, "ld	[x + 2 : 2]");
	}
	| ICMPTYPE
	{
		rangesize = 4;
		state = ICMPTYPE;

		assignvar(&hlen_v, NULL);
		$$ = emit(NULL, "ldx	M[%d]", hlen_v->location);
		$$->line = $1;
		emit($$, "ld	[x + 0 : 1]");
	}
	| ICMPCODE
	{
		rangesize = 4;
		state = ICMPCODE;

		assignvar(&hlen_v, NULL);
		$$ = emit(NULL, "ldx	M[%d]", hlen_v->location);
		$$->line = $1;
		emit($$, "ld	[x + 1 : 1]");
	}
	| ICMP
	{
		rangesize = 4;
		state = ICMP;

		assignvar(&hlen_v, NULL);
		$$ = emit(NULL, "ldx	M[%d]", hlen_v->location);
		$$->line = $1;
		emit($$, "ld	[x + 0 : 2]");
	}
	| PREHEADER
	{
		rangesize = 4;
		$$ = emit(NULL, "ld	R[%d]", IPFWM_EXTRA);
		$$->line = $1;
	}
	| IPDATA '[' NUMBER ']'
	{
		rangesize = 4;
		assignvar(&hlen_v, NULL);
		$$ = emit(NULL, "ldx	M[%d]", hlen_v->location);
		$$->line = $1;
		emit($$, "ld	[x + 0x%0x : 1]", $3);
	}
	| IPDATA '[' NUMBER ':' NUMBER ']'
	{
		rangesize = 4;
		if ($5 != 1 && $5 != 2 && $5 != 4)
			yyerror("invalid data length, must be 1, 2 or 4");
		assignvar(&hlen_v, NULL);
		$$ = emit(NULL, "ldx	M[%d]", hlen_v->location);
		$$->line = $1;
		emit($$, "ld	[x + 0x%0x : %d]", $3, $5);
	}
	| DATA '[' NUMBER ']'
	{
		rangesize = 4;
		$$ = emit(NULL, "ld	[0x%0x : 1]", $3);
		$$->line = $1;
	}
	| DATA '[' NUMBER ':' NUMBER ']'
	{
		rangesize = 4;
		if ($5 != 1 && $5 != 2 && $5 != 4)
			yyerror("invalid data length, must be 1, 2 or 4");
		$$ = emit(NULL, "ld	[0x%0x : %d]", $3, $5);
		$$->line = $1;
	}
	| IPV
	{
		rangesize = 4;
		assignvar(&version_v, NULL);
		$$ = emit(NULL, "ld       M[%d]", version_v->location);
	}
	| IPOFFSET
	{
		rangesize = 4;
		if (protocol != 4)
			yyerror("offset only availble in IPv4");
		assignvar(&off_v, NULL);
		assignvar(&foff_v, NULL);
		$$ = emit(NULL, "ld       M[%d]", off_v->location);
		$$->line = $1;

	}
	| IPHLEN
	{
		rangesize = 4;
		if (protocol != 4)
			yyerror("iphlen only availble in IPv4");
		assignvar(&hlen_v, NULL);
		$$ = emitt(NULL, hlen1_v, "ldx	M[%d]", hlen_v->location);
		emitd($$, hlen1_v, "ldx	M[%%d]");
		$$->line = $1;
		emit($$, "txa");
	}
	| IPLEN
	{
		rangesize = 4;
		if (protocol != 4)
			yyerror("iplen only availble in IPv4");
		$$ = emit(NULL, "ld	[2 : 2]");
		$$->line = $1;
	}
	| IPDATALEN
	{
		rangesize = 4;
		if (protocol != 4)
			yyerror("ipdatalen only availble in IPv4");
		assignvar(&hlen_v, NULL);
		$$ = emit(NULL, "ld	[2 : 2]");
		emitt($$, hlen1_v, "ldx	M[%d]", hlen_v->location);
		emitd($$, hlen1_v, "ldx	M[%%d]");
		emit($$, "sub	X");
		$$->line = $1;
	}
	| PACKETLENGTH
	{
		rangesize = 4;
		$$ = emit(NULL, "ld	#LEN");
		$$->line = $1;
	}
	| TIME
	{
		assignvar(&time_v, NULL);
		$$ = emit(NULL, "ld	M[%d]", time_v->location);
		emit($$, "mod	# %d", 60 * 60 * 24);

		state = TIME;
	}
	| DAY
	{
		assignvar(&time_v, NULL);
		$$ = emit(NULL, "ld	M[%d]", time_v->location);
		emit($$, "div	# %d", 60 * 60 * 24);
		emit($$, "add	# %d", 3);
		emit($$, "mod	# %d", 7);
	}
	;

which	: INPUT
	{
		$$ = IPFWM_SRCIF;
	}
	| OUTPUT
	{
		$$ = IPFWM_DSTIF;
	}
	| RETURN
	{
		$$ = IPFWM_SRCRT;
	}
	;

ifaces	: IFACE
	| IFACE comma ifaces
	{
		$$ = $1;
		append($$, $3);
	}
	;

quad    : /* empty */
	{
		$$ = 0x100;	/* special mark */
	}
	| NUMBER
	{
		if ($1 > 255)
			yyerror("dotted quad element > 255");
		$$ = $1;
	}
	;

number	: NUMBER
	| proto
	| ipaddr
	| '-' NUMBER
	{
		$$ = - $2;
	}
	| icmpopen icmptype comma icmpcode ']'
	{
		$$ = ($2 << 8) | $4;
		state = ICMP;
	}
	;

ipaddr	: quad '.' quad
	{
		if ($1 == 0x100) $1 = 0;
		if ($3 == 0x100) $3 = 0;
		$$ = ($1 << 24) | $3;
	}
	| quad '.' quad '.' quad
	{
		if ($3 == 0x100) $3 = 0;
		if ($5 == 0x100) $5 = 0;

		if ($1 == 0x100) {
			$$ = ($3 << 8) | $5;
		} else {
			$$ = ($1 << 24) | ($3 << 16) | $5;
		}
	}
	| quad '.' quad '.' quad '.' quad
	{
		if ($1 == 0x100) $1 = 0;
		if ($3 == 0x100) $3 = 0;
		if ($5 == 0x100) $3 = 0;
		if ($7 == 0x100) $7 = 0;
		$$ = ($1 << 24) | ($3 << 16) | ($5 << 8) | $7;
	}
	;

netnumber	: NUMBER
	| proto
	| '-' NUMBER
	{
		$$ = - $2;
	}
	| quad '.' quad
	{
		if ($1 == 0x100) $1 = 0;
		if ($3 == 0x100) $3 = 0;
		$$ = ($1 << 24) | ($3 << 16);
	}
	| quad '.' quad '.' quad
	{
		if ($1 == 0x100) $1 = 0;
		if ($3 == 0x100) $3 = 0;
		if ($5 == 0x100) $5 = 0;
		$$ = ($1 << 24) | ($3 << 16) | ($5 << 8);
	}
	| quad '.' quad '.' quad '.' quad
	{
		if ($1 == 0x100) $1 = 0;
		if ($3 == 0x100) $3 = 0;
		if ($5 == 0x100) $3 = 0;
		if ($7 == 0x100) $7 = 0;
		$$ = ($1 << 24) | ($3 << 16) | ($5 << 8) | $7;
	}
	;

icmpopen : '['
	{
		if (state != ICMP)
			yyerror("invalid use of '['");
	}
	;


range 	: number
	{
		$$ = malloc(sizeof(range_t));
		memset($$, 0, sizeof(range_t));
		$$->size = 4;
		$$->min = $1;
		$$->max = $1;
	}
	| '<' NUMBER
	{
		$$ = malloc(sizeof(range_t));
		memset($$, 0, sizeof(range_t));
		$$->size = 4;
		$$->max = $2;
		$$->type = '<';
	}
	| '<' '=' NUMBER
	{
		$$ = malloc(sizeof(range_t));
		memset($$, 0, sizeof(range_t));
		$$->size = 4;
		$$->max = $3;
		$$->type = '<' + '=';
	}
	| '>' NUMBER
	{
		$$ = malloc(sizeof(range_t));
		memset($$, 0, sizeof(range_t));
		$$->size = 4;
		$$->min = $2;
		$$->type = '>';
	}
	| '>' '=' NUMBER
	{
		$$ = malloc(sizeof(range_t));
		memset($$, 0, sizeof(range_t));
		$$->size = 4;
		$$->min = $3;
		$$->type = '>' + '=';
	}
	| number '-' number
	{
		$$ = malloc(sizeof(range_t));
		memset($$, 0, sizeof(range_t));
		$$->size = 4;
		if (($1 & 0xff000000) && ($3 & 0xff000000) == 0) {
			if ($3 & 0x00ff0000) {
				$$->min = $1;
				$$->max = ($1 & ~0xffffff) | $3;
			} else if ($3 & 0x0000ff00) {
				$$->min = $1;
				$$->max = ($1 & ~0xffff) | $3;
			} else {
				$$->min = $1;
				$$->max = ($1 & ~0xff) | $3;
			}
		} else {
			$$->min = $1;
			$$->max = $3;
		}
		if ($$->min > $$->max) {
			u_long t = $$->min;
			$$->min = $$->max;
			$$->max = t;
		}
		$$->type = '-';
	}
	| netnumber '/' number
	{
		$$ = malloc(sizeof(range_t));
		memset($$, 0, sizeof(range_t));
		$$->size = 4;
		$$->min = $1;
		$$->max = $1;
		if ($3 < 1 || $3 > 32)
			yyerror("invalid mask");
		else if ($3 == 32) {
			$$->mask = 0;
			$$->type = 0;
		} else {
			$$->mask = $3;
			$$->type = '/';
		}
	}
	| number '&' number
	{
		$$ = malloc(sizeof(range_t));
		memset($$, 0, sizeof(range_t));
		$$->size = 4;
		$$->min = $1;
		$$->max = $1;
		$$->mask = $3;
		$$->min &= $$->mask;
		$$->type = '&';
	}
	| QNUMBER
	{
		$$ = malloc(sizeof(range_t));
		memset($$, 0, sizeof(range_t));
		$$->size = 16;
		$$->qmin = $1;
		$$->qmax = $1;
	}
	| '<' QNUMBER
	{
		$$ = malloc(sizeof(range_t));
		memset($$, 0, sizeof(range_t));
		$$->size = 16;
		$$->qmax = $2;
		$$->type = '<';
	}
	| '<' '=' QNUMBER
	{
		$$ = malloc(sizeof(range_t));
		memset($$, 0, sizeof(range_t));
		$$->size = 16;
		$$->qmax = $3;
		$$->type = '<' + '=';
	}
	| '>' QNUMBER
	{
		$$ = malloc(sizeof(range_t));
		memset($$, 0, sizeof(range_t));
		$$->size = 16;
		$$->qmin = $2;
		$$->type = '>';
	}
	| '>' '=' QNUMBER
	{
		$$ = malloc(sizeof(range_t));
		memset($$, 0, sizeof(range_t));
		$$->size = 16;
		$$->qmin = $3;
		$$->type = '>' + '=';
	}
	| QNUMBER '-' QNUMBER
	{
		$$->size = 16;
		$$ = malloc(sizeof(range_t));
		memset($$, 0, sizeof(range_t));
		$$->qmin = $1;
		$$->qmax = $3;
		$$->type = '-';
	}
	| QNUMBER '/' number
	{
		$$ = malloc(sizeof(range_t));
		memset($$, 0, sizeof(range_t));
		$$->size = 16;
		$$->qmin = $1;
		$$->qmax = $1;
		if ($3 < 1 || $3 > 128)
			yyerror("invalid mask");
		else if ($3 == 128) {
			$$->mask = 0;
			$$->type = 0;
		} else {
			$$->mask = $3;
			$$->type = '/';
		}
	}
	| IFACE
	{
		$$ = malloc(sizeof(range_t));
		memset($$, 0, sizeof(range_t));
		$$->size = 1;
		$$->type = 'i';
		$$->iface = $1;
	}
	;

comma	:	','
	|	';'
	;

ranges	: range
	{
		if (rangesize && rangesize != $1->size)
			yyerror("inappropriate size of element");
		$$ = $1;
	}
	| range comma ranges
	{
		if (rangesize && rangesize != $1->size)
			yyerror("inappropriate size of element");
		$$ = $1;
		$1->next = $3;
	}
	;

unumber	: NUMBER
	{
		$$ = $1;
		state = USER;
	}
	;

user	: users
	{
		$$ = $1;
		state = 0;
	}
	;

users	: USER
	{
		$$ = $1;
	}
	| USER users
	{
		$$ = $1 | $2;
	}
	;

%%

void
append(cmd_t *a, cmd_t *b)
{
	while (a->next)
		a = a->next;
	a->next = b;
}

cmd_t *
emitd(cmd_t *a, var_t *d, char *fmt, ...)
{
	char buf[1024];
	va_list ap;

	buf[0] = '\t';
	va_start(ap, fmt);
	vsnprintf(buf+1, sizeof(buf)-1, fmt, ap);
	va_end(ap);
	return (emits(a, d, NULL, buf));
}

cmd_t *
emitt(cmd_t *a, var_t *t, char *fmt, ...)
{
	char buf[1024];
	va_list ap;

	buf[0] = '\t';
	va_start(ap, fmt);
	vsnprintf(buf+1, sizeof(buf)-1, fmt, ap);
	va_end(ap);
	return (emits(a, NULL, t, buf));
}

cmd_t *
emitdt(cmd_t *a, var_t *d, var_t *t, char *fmt, ...)
{
	char buf[1024];
	va_list ap;

	buf[0] = '\t';
	va_start(ap, fmt);
	vsnprintf(buf+1, sizeof(buf)-1, fmt, ap);
	va_end(ap);
	return (emits(a, d, t, buf));
}

cmd_t *
emit(cmd_t *a, char *fmt, ...)
{
	char buf[1024];
	va_list ap;

	buf[0] = '\t';
	va_start(ap, fmt);
	vsnprintf(buf+1, sizeof(buf)-1, fmt, ap);
	va_end(ap);
	return (emits(a, NULL, NULL, buf));
}

cmd_t *
emits(cmd_t *a, var_t *d, var_t *t, char *buf)
{
	if (a == NULL)
	    a = malloc(sizeof(cmd_t));
	else {
		while (a->next)
			a = a->next;
		a->next = malloc(sizeof(cmd_t));
		a = a->next;
	}

	a->cmd = strdup(buf);
	a->line = linecnt;
	a->file = srcfile;
	a->depedent = d ? &(d->location) : NULL;
	a->tnedeped = t ? &(t->location) : NULL;
	if ((a->cmd[1] == 'l' || a->cmd[1] == 'L') && isdigit(a->cmd[2]))
		++a->cmd;
	if (a->cmd[1] == '/' && a->cmd[2] == '/')
		++a->cmd;
	a->next = NULL;
	return(a);
}

int
nextlabel()
{
	static int nl = 100000000;
	return(nl++);
}

jmp_t *
nextlabelp(jmp_t *a)
{
	jmp_t *b = malloc(sizeof(jmp_t));
	b->where = nextlabel();
	b->next = a;
	return(b);
}

void
appendlabel(jmp_t *a, jmp_t *b)
{
	while (a->next)
		a = a->next;
	a->next = b;
}

void
appendlabeln(jmp_t *a, int n)
{
	while (a->next)
		a = a->next;
	a->next = malloc(sizeof(jmp_t));
	a->next->where = n;
	a->next->next = NULL;
}

int
yyerror(const char *s)
{
	fprintf(stderr, "%s: %d: %s\n", srcfile, linecnt, s);
	error = 1;
	return (0);
}

int
yywrap()
{
	return (1);
}

void
emitlabels(cmd_t *a, jmp_t *b)
{
	while (a->next)
		a = a->next;
	while (b) {
		emit(a, "L%d:", b->where);
		b = b->next;
	}
}

int
checkret(cmd_t *a)
{
	char *b;
	int isret = 0;

	while (a) {
		if (a->cmd) {
			for (b = a->cmd; *b == ' ' || *b == '\t'; ++b)
				;
			if (strncasecmp(b, "ret", 3) == 0 &&
			    (b[3] == ' ' || b[3] == '\t' || b[3] == '\0'))
				isret = 1;
			else if (b[0] && (b[0] != '/' || b[1] != '/'))
				isret = 0;
		}
		a = a->next;
	}
	return(isret);
}

char *
varname(int *x)
{
	var_t *v;

        for (v = var_list; v; v = v->next) {
		if (x == &v->location)
			return(v->name);
	}
	return ("Unknown");
}

void
emitcode(cmd_t *a)
{
	int lc = 1;
	char *sf = "";
	if (def == -1)
		def = IPFW_NEXT;
	emit(a, "ret	# 0x%0x", def);
	while (a) {
		if ((a->depedent && *(a->depedent) == 0) ||
		    (a->tnedeped && *(a->tnedeped) != 0)) {
			if (debug) {
				fprintf(asmout, "// %s:%d", a->file, a->line);
				if (a->depedent)
					fprintf(asmout, " d:%s",
					     varname(a->depedent));
				if (a->tnedeped)
					fprintf(asmout, " t:%s",
					     varname(a->tnedeped));
				fprintf(asmout, " %s\n", a->cmd);
			}
			a = a->next;
			continue;
		}
		if ((a->line && lc != a->line) ||
		    (a->file && strcmp(a->file, sf) != 0)) {
			lc = a->line;
			if (a->file)
				sf = a->file;
			if (!nocpplines)
				fprintf(asmout, "# %d \"%s\"\n", lc, sf);
		}
		if (a->cmd) {
			if (a->depedent) {
				fprintf(asmout, a->cmd, *(a->depedent));
				fprintf(asmout, "\n");
			} else
				fprintf(asmout, "%s\n", a->cmd);
			++lc;
		}
		a = a->next;
	}
}

void
start_filter(char *name)
{
	if (cf != NULL)
		yyerror("filters cannot nest");
	cf = malloc(sizeof(filter_t));
	memset(cf, 0, sizeof(filter_t));
	cf->def = -1;
	cf->label = nextlabelp(0);
	cf->name = name;
}

void
start_switch()
{
	switch_t *s;
	s = malloc(sizeof(switch_t));
	s->state = state;
	state = 0;
	s->next = sw;
	sw = s;
}

void
setdef(int b)
{
	int *a = cf ? &(cf->def) : &def;
	if (*a != -1) {
		if (cf != NULL)
			yyerror("implicit return already set for filter");
		else
			yyerror("implicit return already set for program");
	}
	*a = b;
}

var_t *
findvar(char *name)
{
	var_t *v;

	for (v = var_list; v; v = v->next)
		if (strcmp(name, v->name) == 0)
			return (v);
	return (NULL);
}

void
assignvar(var_t **vp, char *name)
{
	static int maxvar = 1;	/* 0 is reserved for ranges */

	allocatevar(vp, name);

	if (*vp == NULL)
		return;
	if ((*vp)->location == 0) {
		if (maxvar >= BPF_MEMWORDS) {
			yyerror("too many local variables");
			return;
		}
		(*vp)->location = maxvar++;
	}
}

void
allocatevar(var_t **vp, char *name)
{
	var_t *v;

	if (*vp)
		return;

	for (v = var_list; v; v = v->next)
		if (strcmp(name, v->name) == 0) {
			*vp = v;
			return;
		}
	if ((v = malloc(sizeof(var_t))) == NULL) {
		yyerror("out of memory");
		return;
	}
	memset(v, 0, sizeof(var_t));
	v->name = name;
	v->next = var_list;
	var_list = v;
	*vp = v;
	return;
}

static char *
rpmin(range_t *r)
{
	static char buf[16];
	
	if (r->size == 16)
		return(qprint(&r->qmin));
	sprintf(buf, "# 0x%lx", r->min);
	return(buf);
}

static char *
rpmax(range_t *r)
{
	static char buf[16];

	if (r->size == 16)
		return(qprint(&r->qmax));
	sprintf(buf, "# 0x%lx", r->max);
	return(buf);
}

static char *
qprint(q128_t *q)
{
	static char buf[8*5 + 4] = { 'Q', '#', ' ', };
	buf[3] = '\0';
	if (inet_ntop(AF_INET6, q, buf+3, sizeof(buf)-3) == NULL)
		yyerror("invalid IPv6 address");
	return (buf);
}
