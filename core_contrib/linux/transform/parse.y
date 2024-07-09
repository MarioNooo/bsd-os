/*	BSDI parse.y,v 1.2 2000/12/08 03:17:24 donn Exp	*/

/*
 * Scanner for the syscall transformation template translator
 * (what a mouthful).
 */

%{
#include <sys/types.h>
#include <err.h>
#include <stdio.h>

#include "transform.h"

int yyerror(const char *);
%}

%union {
	char *string;
	struct ident ident;
	struct decl *decl;
}

/* keywords */
%token ALIAS
%token FLAG
%token COOKIE
%token INCLUDE
%token STRUCT
%token TYPEDEF
%token IN
%token OUT

/* not quite keywords */
%token <string> TYPE_SPECIFIER
%token <string> TYPE_QUALIFIER

/* other terminals */
%token <string> NUMBER
%token <ident> NAME
%token <string> STRING
%token <string> C_TEXT
%token <ident> DIRECTION

%type <decl> declaration
%type <string> function_body

%%

start:
	statements;

statements:
	statement |
	statements statement;

statement:
	';' |
	C_TEXT
		{ puts($1); } |
	alias |
	cookie |
	flag |
	function |
	include |
	struct |
	typedef |
	error ';';

alias:
	ALIAS NAME '=' NAME ';'
		{ add_alias(&$2, &$4); };

include:
	INCLUDE STRING
		{ push_file($2); };

typedef:
	TYPEDEF declaration
		{ start_typedef($2); }
	transforms_opt ';'
		{ end_typedef(); };

transforms_opt:
	/* EMPTY */ |
	'{' transforms '}';

transforms:
	transform |
	transforms transform;

transform:
	DIRECTION '(' NAME ')' function_body
		{ add_transform(&$1, &$3, NULL, NULL, $5); } |
	DIRECTION '(' NAME ',' NAME ')' function_body
		{ add_transform(&$1, &$3, &$5, NULL, $7); } |
	DIRECTION '(' NAME ',' NAME ',' NAME ')' function_body
		{ add_transform(&$1, &$3, &$5, &$7, $9); } |
	DIRECTION '(' error '}';	/* problems */

cookie:
	COOKIE declaration
		{ start_cookie($2); }
	cookie_transforms_opt ';'
		{ end_cookie(); }

cookie_transforms_opt:
	/* EMPTY */ |
	'{' cookie_transforms '}';

cookie_transforms:
	cookie_transform |
	cookie_transforms cookie_transform;

cookie_transform:
	NAME NUMBER ';'
		{ add_cookie(&$1, $2); } |
	C_TEXT
		{ puts($1); } |
	transform;

flag:
	FLAG declaration
		{ start_flag($2); }
	flag_transforms_opt ';'
		{ end_flag(); };

flag_transforms_opt:
	/* EMPTY */ |
	'{' flag_transforms '}';

flag_transforms:
	flag_transform |
	flag_transforms flag_transform;

flag_transform:
	NAME NUMBER ';'
		{ add_flag(&$1, $2); } |
	NAME ';'
		{ add_flag(&$1, NULL); } |
	C_TEXT
		{ puts($1); } |
	transform;

struct:
	STRUCT NAME
		{ start_struct(&$2); }
	members_opt ';'
		{ end_struct(); };

members_opt:
	/* EMPTY */ |
	'{' member_transforms '}';

member_transforms:
	member_transform |
	member_transforms member_transform;

member_transform:
	declaration ';'
		{ add_struct_member($1); } |
	C_TEXT
		{ puts($1); } |
	transform;

function:
	declaration
		{ start_overload($1); }
	'(' parameters_opt ')'
		{ end_parameters(); }
	function_tail
		{ end_overload(); };

function_tail:
	'=' NAME ';'
		{ add_function_default(&$2); } |
	'=' NUMBER ';'
		{ add_function_value($2); } |
	';'
		{ add_function_default(NULL); } |
	function_body
		{ add_function_body($1); };

parameters_opt:
	/* EMPTY */ |
	parameters;

parameters:
	parameter |
	parameters ',' parameter;

parameter:
	NAME
		{ add_parameter_member(&$1); } |
	declaration
		{ add_parameter_decl($1); };

	/*
	 * The scanner contains a hack to deal with two different
	 * syntaxes for C text that goes directly into the output.
	 * As in 'lex', we always recognize '%{ ... }%' as delimiting
	 * C text, and we recognize '{ ... }' as C text in context.
	 * The context is a close parenthesis followed by an open brace.
	 */
function_body:
	C_TEXT
		{ $$ = $1; };

/*
 * A simplified declaration parser for C.
 */

declaration:
	struct_specifiers declarator
		{ $$ = end_decl(); }

struct_specifiers:
	STRUCT NAME
		{ add_struct_decl(&$2); } |
	TYPE_QUALIFIER STRUCT NAME
		{ add_type_qual($1); add_struct_decl(&$3); } |
	specifiers;

specifiers:
	specifier |
	specifiers specifier;

specifier:
	TYPE_SPECIFIER
		{ add_decl_text($1); } |
	TYPE_QUALIFIER
		{ add_type_qual($1); };

declarator:
	pointers_opt direct_declarator;

pointers_opt:
	/* EMPTY */ |
	pointers;

pointers:
	pointer |
	pointers pointer;

pointer:
	'*'
		{ add_decl_text("*"); };

/* This deliberately omits function declarations.  */
direct_declarator:
	NAME
		{ add_decl(&$1); } |
	direct_declarator '[' NUMBER ']'
		{ add_array_decl($3); } |
	direct_declarator '[' NAME ']'
		{ add_array_decl($3.name); };
