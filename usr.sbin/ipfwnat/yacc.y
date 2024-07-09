/*-
 *  Copyright (c) 1999 Berkeley Software Design, Inc.
 *  All rights reserved.
 *  The Berkeley Software Design Inc. software License Agreement specifies
 *  the terms and conditions for redistribution.
 *
 *	BSDI yacc.y,v 1.5 2000/01/20 18:14:02 prb Exp
 */
%{
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ipfw.h>
#include <netinet/ipfw_nat.h>
#include <netdb.h>
#include <stdio.h>
#include <stdarg.h>
#include "yacc.h"

int error;
int linecnt = 1;
char *proto;
int protocol;

e_t *new_e();

void *nat_definition;
int nat_space = sizeof(ip_natdef_t);
ip_natmap_t *maps = NULL;
ip_natservice_t *sers = NULL;
int nservices, nmaps;
struct {
	ipfw_filter_t	ipfw;
	ip_nat_hdr_t	nh;
} s;


%}

%union {
	u_int	n;
	char	*str;
	e_t	*e;
}

%token		INTERNAL EXTERNAL SERVICE MAP TCP UDP POINTER
%token		BUCKETS PREFILL MAXSESSIONS INTERFACE TIMEOUT
%token		ICMP TAG PRIORITY EXPIRE IN AT
%token	<n>	NUMBER
%token	<str>	STRING QSTRING

%type	<n>	port addr quad proto serial expire
%type	<e>	addrmask addrport entry entries protoaddrmask protoaddrport

%%

start	: program
	{
		ip_natdef_t *nd;
		ip_natent_t *ne, *e;
		ip_natservice_t *ns;
		ip_natmap_t *nm;

		nd = malloc(nat_space);
		if (nd == NULL)
			err(1, NULL);

		nd->nd_nservices = nservices;
		nd->nd_nmaps = nmaps;
		ns = (ip_natservice_t *)(nd + 1);
		while (sers) {
			memcpy(ns, sers, sizeof(*ns));
			ns++;
			sers = sers->ns_next;
		}
		nm = (ip_natmap_t *)ns;
		while (maps) {
			memcpy(nm, maps, sizeof(*nm));
			nm->nm_nentries = 0;
			ne = (ip_natent_t *)(nm + 1);
			for (e = maps->nm_entries; e; e = e->ne_next) {
				nm->nm_nentries++;
				memcpy(ne, e, sizeof(*ne));
				ne++;
			}
			maps = maps->nm_next;
			nm = (ip_natmap_t *)ne;
		}
		nat_definition = nd;
	}
	;

program	: line
	| program line
	;

line	: TAG QSTRING ';'
	{
		if (strlen($2) > IPFW_TAGLEN)
			yyerror("tag too long");
		strncpy(s.ipfw.tag, $2, IPFW_TAGLEN);
	}
	| PRIORITY NUMBER ';'
	{
		s.ipfw.priority = $2;
	}
	| PRIORITY '-' NUMBER ';'
	{
		s.ipfw.priority = -$3;
	}
	| SERVICE serial protoaddrport POINTER addrport expire ';'
	{
		ip_natservice_t *ser, *s;

		++nservices;
		nat_space += sizeof(*ser);
		ser = malloc(sizeof(*ser));
		if (ser == NULL)
			err(1, NULL);
		ser->ns_serial = $2;
		ser->ns_expire = $6;
		ser->ns_protocol = $3->protocol;
		ser->ns_external = $3->addr;
		ser->ns_eport = $3->port;
		ser->ns_internal = $5->addr;
		ser->ns_iport = $5->port;
		ser->ns_next = NULL;
		if (sers == NULL) {
			sers = ser;
		} else {
			for (s = sers; s->ns_next; s = s->ns_next)
				;
			s->ns_next = ser;
		}
	}
	| MAP serial protoaddrmask POINTER entries expire ';'
	{
		ip_natmap_t *map;
		ip_natmap_t *m;
		ip_natent_t *ent;

		++nmaps;
		ent = NULL;
		nat_space += sizeof(*map);
		map = malloc(sizeof(*map));
		if (map == NULL)
			err(1, NULL);
		map->nm_serial = $2;
		map->nm_expire = $6;
		map->nm_next = NULL;
		map->nm_internal = $3->addr;
		map->nm_mask = $3->mask;
		map->nm_protocol = $3->protocol;
		while ($5) {
			nat_space += sizeof(*ent);
			if (ent) {
				ent->ne_next = malloc(sizeof(*ent));
				ent = ent->ne_next;
			} else {
				map->nm_entries = malloc(sizeof(*ent));
				ent = map->nm_entries;
			}
			if (ent == NULL)
				err(1, NULL);

			ent->ne_firstaddr = $5->addr;
			ent->ne_lastaddr = $5->addr;
			ent->ne_firstaddr.s_addr &= $5->mask.s_addr;
			ent->ne_lastaddr.s_addr |= ~ $5->mask.s_addr;
			if ($5->mask.s_addr && $5->mask.s_addr != 0xffffffff) {
				ent->ne_firstaddr.s_addr =
				    htonl(ntohl(ent->ne_firstaddr.s_addr)+1);
				ent->ne_lastaddr.s_addr =
				    htonl(ntohl(ent->ne_lastaddr.s_addr)-1);
			}

			ent->ne_firstport = $5->port;
			ent->ne_lastport = $5->endport;

			ent->ne_luaddr = ent->ne_firstaddr;
			ent->ne_luport = ent->ne_firstport;
			$5 = $5->next;
		}
		ent->ne_next = NULL;
		if (maps == NULL) {
			maps = map;
		} else {
			for (m = maps; m->nm_next; m = m->nm_next)
				;
			m->nm_next = map;
		}
	}
	| BUCKETS NUMBER ';'
	{
		s.nh.size = $2;
	}
	| PREFILL NUMBER ';'
	{
		s.nh.prefill = $2;
	}
	| MAXSESSIONS NUMBER ';'
	{
		s.nh.maxsessions = $2;
	}
	| INTERFACE STRING ';'
	{
		if (s.nh.index)
			yyerror("only one interface may be declared");
		else {
			s.nh.index = if_nametoindex($2);
			if (s.nh.index < 1)
				yyerror("%s: no such interface", $2);
		}
	}
	| TIMEOUT proto NUMBER ';' 
	{
		if ($3 <= 0)
			yyerror("invalid timeout");
		s.nh.timeouts[$2] = $3;
	}
	;

protoaddrport	: proto addrport
		{
			$$ = $2;
			$$->protocol = $1;
		}
		| addrport
		{
			$$ = $1;
		}
		;

protoaddrmask	: proto addrmask
		{
			$$ = $2;
			$$->protocol = $1;
		}
		| addrmask
		{
			$$ = $1;
		}
		;

serial	: /* EMPTY */
	{
		$$ = 0;
	}
	| '[' NUMBER ']'
	{
		if ($2 < 0 || $2 >= 1000000000)
			yyerror("%d: invalid serial number", $2);
		$$ = $2;
	}
	;

proto	: TCP
	{
		proto = "tcp";
		protocol = $$ = IPPROTO_TCP;
	}
	| UDP
	{
		proto = "udp";
		protocol = $$ = IPPROTO_UDP;
	}
	| ICMP
	{
		proto = "icmp";
		protocol = $$ = IPPROTO_ICMP;
	}
	| NUMBER
	{
		if ($1 < 0 || $1 > 255)
			yyerror("invalid protocol number");
		proto = "";
		protocol = $$ = $1;
	}
	;

entries	: entry
	{
		$$ = $1;
	}
	| entry ',' entries
	{
		$$ = $1;
		$$->next = $3;
	}
	;

entry	: addrmask
	{
		$$ = $1;
	}
	| addrmask port '-' port
	{
		if (protocol != IPPROTO_TCP && protocol != IPPROTO_UDP)
			yyerror("protocol does not use port numbers");
		$$ = $1;
		if ($2 < 0 || $4 < 0 || $2 > 0xffff || $4 > 0xffff)
			yyerror("invalid port");
		if ($2 > $4) {
			$$->port = $4;
			$$->endport = $2;
		} else {
			$$->port = $2;
			$$->endport = $4;
		}
	}
	;

addrmask : addr
	{
		$$ = new_e();
		$$->addr.s_addr = $1;
		$$->mask.s_addr = $1 ? 0xffffffff : 0;
	}
	| addr '/' NUMBER
	{
		if ($3 < 0 || $3 > 32 || $3 == 31)
			yyerror("invalid netmask");
		$$ = new_e();
		$$->addr.s_addr = $1;
		if ($3 == 0)
			$$->mask.s_addr = 0;
		else
			$$->mask.s_addr = 0xffffffff << (32 - $3);
		$$->mask.s_addr = htonl($$->mask.s_addr);
	}
	;

addrport : addr
	{
		$$ = new_e();
		$$->addr.s_addr = $1;
	}
	| addr ':' port
	{
		if (protocol != IPPROTO_TCP && protocol != IPPROTO_UDP)
			yyerror("protocol does not use port numbers");
		$$ = new_e();
		$$->addr.s_addr = $1;
		$$->port = $3;
	}
	;

addr	: STRING
	{
		struct hostent *he;

		if ((he = gethostbyname($1)) == NULL) {
			yyerror("invalid hostname");
			$$ = 0;
		} else
			$$ = *(u_int32_t *)he->h_addr;
	}
	| NUMBER
	{
		$$ = htonl($1);
	}
	| quad '.' quad
	{       
		$$ = ($1 << 24) | $3;
		$$ = htonl($$);
	}       
	| quad '.' quad '.' quad
	{       
		$$ = ($1 << 24) | ($3 << 16) | $5;
		$$ = htonl($$);
	}       
	| quad '.' quad '.' quad '.' quad
	{ 
		$$ = ($1 << 24) | ($3 << 16) | ($5 << 8) | $7;
		$$ = htonl($$);
	}       
	; 

quad	: NUMBER
	{
		if ($1 > 255)
			yyerror("dotted quad element > 255");
		else if ($1 < 0)
			yyerror("negative dotted quad element");
		$$ = $1;
	}       
	;       

port	: NUMBER
	{
		if ($1 < 0 || $1 > 0xffff)
			yyerror("invalid port number");
		$$ = htons($1);
	}
	| STRING
	{
		struct servent *se;

		if ((se = getservbyname($1, proto)) == NULL) {
			yyerror("invalid port number");
			$$ = 0;
		} else
			$$ = se->s_port;
	}
	;

expire	: /* EMPTY */
	{
		$$ = 0;
	}
	| EXPIRE IN NUMBER
	{
		$$ = time(0) + $3;
	}
	| EXPIRE AT NUMBER
	{
		$$ = $3;
	}
	;

%%

int
yyerror(const char *fmt, ...)
{
	va_list	ap;

	fprintf(stderr, "%d: ", linecnt);

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n");
	error = 1;
	exit (1);
}

int
yywrap()
{
	return (1);
}

e_t *
new_e()
{
	e_t *n = malloc(sizeof(*n));
	if (n == NULL)
		err(1, NULL);
	memset(n,  0, sizeof(*n));
	return (n);
}
