/*	BSDI scan.l,v 1.2 2001/01/23 03:39:39 donn Exp	*/

/*
 * Lexical analyzer for the syscall transformation template translator.
 * The lexical rules are not that different from lex itself.
 */

%{
#include <sys/types.h>
#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "transform.h"
#include "y.tab.h"

static DB *key_db;

static void init_key(void);
static int lookup_key(void);
static int pop_file(void);
static int string(void);
static int number(void);
static void need_ctext(size_t);
static void start_ctext(void);
static void add_ctext(char *, size_t);
static int end_ctext(void);
static void line_err(int, const char *, ...);

struct file_stack {
	char *file;
	YY_BUFFER_STATE state;
	int line;
};

static struct file_stack *fs;
static int fs_top = -1;
static int fs_size;

static char *ctext;
static size_t ctext_size;
static size_t ctext_len;

static int line_number;
static int comment_start;
static int brace_start;
static int brace_count;
static int quote_start;
%}

%x COMMENT
%x CTEXT
%x CTEXT2
%x MAYBECTEXT2
%x C2COMMENT
%x MC2COMMENT
%x CPP
%x DQUOTE
%x SQUOTE

%%

	/* Handle C comments.  */
"/*"			BEGIN(COMMENT); comment_start = line_number;
<COMMENT,C2COMMENT,MC2COMMENT>{
	[^/*\n]+
	"*"+/[^/]
	"/"
	\n		++line_number;
	<<EOF>>		{
		line_err(comment_start, "unterminated comment");
		if (pop_file() == EOF)
			yyterminate();
	}
}
<COMMENT>"*/"		BEGIN(INITIAL);
<MC2COMMENT>"*/"	BEGIN(MAYBECTEXT2);
<C2COMMENT>"*/"		{
	for (; comment_start < line_number; ++comment_start)
		add_ctext("\n", 1);
	BEGIN(CTEXT2);
}

	/* Handle double quotes in transform commands.  */
"\""[^"\n]*"\""		return (string());
"\""[^"\n]*$		{
	line_err(line_number, "unterminated string");
	return (string());
}

	/* Handle untranslated escapes to C.  */
"%{"			BEGIN(CTEXT); brace_start = line_number; start_ctext();
<CTEXT>{
	[^%}\n]+	add_ctext(yytext, yyleng);
	"%"+/[^}]	add_ctext(yytext, yyleng);
	\n		++line_number; add_ctext(yytext, yyleng);
	<<EOF>>		{
		line_err(brace_start, "unterminated C escape");
		if (pop_file() == EOF)
			yyterminate();
	}
	"%}"		BEGIN(INITIAL); return (end_ctext());
	"}"		add_ctext(yytext, yyleng);
}

	/* Handle C-style numbers in transform commands.  */
-?0x[0-9a-fA-F]+	|
-?[0-9]+		return (number());

	/* Handle keywords and identifiers.  */
alias			return (ALIAS);
flag			return (FLAG);
cookie			return (COOKIE);
include			return (INCLUDE);
struct			return (STRUCT);
typedef			return (TYPEDEF);
in			|
out			{
	if ((yylval.ident.name = strdup(yytext)) == NULL)
		fatal(NULL);
	yylval.ident.file = fs[fs_top].file;
	yylval.ident.line = line_number;
	return (DIRECTION);
}
[a-zA-Z_][a-zA-Z_0-9]*	return (lookup_key());

	/* Handle operators and syntactic sugar.  ')' is handled below.  */
[;=(\{\}*\[\],]		return (yytext[0]);

	/* Handle whitespace.  */
\n			++line_number;
[ \t\v\f]

	/*
	 * Handle C text bounded only by braces.
	 * We look at close parentheses; if we see one followed by a brace,
	 * we assume that the brace starts C text.
	 */
")"			BEGIN(MAYBECTEXT2); return (')');
<MAYBECTEXT2>{
	[ \t\v\f]
	"/*"		BEGIN(MC2COMMENT); comment_start = line_number;
	\n		++line_number;
	<<EOF>>		if (pop_file() == EOF) yyterminate();
	"{"		{
		BEGIN(CTEXT2);
		brace_start = line_number;
		brace_count = 1;
		start_ctext();
	}
	")"		return (')');
	.		BEGIN(INITIAL); yyless(0);
}

	/* We're committed to C text now...  */
<CTEXT2>{
	^[ \t\v\l]*#	BEGIN(CPP); add_ctext(yytext, yyleng);
	"/*"		BEGIN(C2COMMENT); comment_start = line_number;
	['"]		{
		add_ctext(yytext, yyleng);
		quote_start = line_number;
		if (yytext[0] == '\'')
			BEGIN(SQUOTE);
		else
			BEGIN(DQUOTE);
	}
	\n		++line_number; add_ctext(yytext, yyleng);
	<<EOF>>		{
		line_err(brace_start, "unterminated compound statement");
		if (pop_file() == EOF)
			yyterminate();
	}
	"{"		++brace_count; add_ctext(yytext, yyleng);
	"}"		{
		if (--brace_count <= 0) {
			BEGIN(INITIAL);
			return (end_ctext());
		} else
			add_ctext(yytext, yyleng);
	}
	.		add_ctext(yytext, yyleng);
}

<CPP>{
	[^\\\n]+	add_ctext(yytext, yyleng);
	\\\n		add_ctext(yytext, yyleng);
	\\		add_ctext(yytext, yyleng);
	\n		add_ctext(yytext, yyleng); ++line_number; BEGIN(CTEXT2);
}

<DQUOTE,SQUOTE>{
	\\\n		add_ctext(yytext, yyleng); ++line_number;
	.$		{
		line_err(quote_start, "unterminated %s",
		    YY_START == SQUOTE ? "character constant" : "string");
		add_ctext(yytext, yyleng);
	}
}
<DQUOTE>{
	[^"]		add_ctext(yytext, yyleng);
	"\""		add_ctext(yytext, yyleng); BEGIN(CTEXT2);
}
<SQUOTE>{
	[^']		add_ctext(yytext, yyleng);
	"'"		add_ctext(yytext, yyleng); BEGIN(CTEXT2);
}

<<EOF>>			if (pop_file() == EOF) yyterminate();

.			{
	line_err(line_number, "unrecognized character `%c'", yytext[0]);
}

%%

static struct keyword {
	char *name;
	int tag;
} keys[] = {
	{ "void", TYPE_SPECIFIER },
	{ "char", TYPE_SPECIFIER },
	{ "short", TYPE_SPECIFIER },
	{ "int", TYPE_SPECIFIER },
	{ "long", TYPE_SPECIFIER },
	{ "signed", TYPE_SPECIFIER },
	{ "unsigned", TYPE_SPECIFIER },
	{ "const", TYPE_QUALIFIER },
	{ "volatile", TYPE_QUALIFIER },
	{ "noerrno", TYPE_QUALIFIER },
};

/*
 * Initialize the type specifier / qualifier keyword database.
 */
static void
init_key()
{
	DBT key;
	DBT data;
	size_t i;

	if ((key_db = dbopen(NULL, O_RDWR, 0, DB_HASH, NULL)) == NULL)
		fatal(NULL);

	for (i = 0; i < (sizeof keys) / sizeof (keys[0]); ++i) {
		key.data = keys[i].name;
		key.size = strlen(keys[i].name);
		data.data = &keys[i].tag;
		data.size = sizeof (keys[i].tag);
		if (key_db->put(key_db, &key, &data, 0) != 0)
			fatal(NULL);
	}
}

/*
 * Install a new type specifier in the keyword database.
 */
void
add_key(char *name)
{
	DBT key;
	DBT data;
	int i = TYPE_SPECIFIER;

	key.data = name;
	key.size = strlen(name);
	data.data = &i;
	data.size = sizeof (int);
	if (key_db->put(key_db, &key, &data, 0) != 0)
		fatal(NULL);
}

/*
 * Classify an identifier as a specifier, a qualifier or a name.
 */
static int
lookup_key()
{
	DBT key;
	DBT data;
	int tag;
	int r;

	key.data = yytext;
	key.size = yyleng;
	if ((r = key_db->get(key_db, &key, &data, 0)) == -1)
		fatal(NULL);

	if (r == 1) {
		if ((yylval.ident.name = strdup(yytext)) == NULL)
			fatal(NULL);
		yylval.ident.file = fs[fs_top].file;
		yylval.ident.line = line_number;
		return (NAME);
	}

	yylval.string = yytext;
	memcpy(&tag, data.data, sizeof tag);
	return (tag);
}

/*
 * Go to the next level of file inclusion.
 */
void
push_file(char *file)
{
	int save_fs_size;

	if (key_db == NULL) {
		init_key();
		init_type();
	}

	if ((yyin = fpathopen(&file, "r")) == NULL) {
		line_err(line_number, "can't include `%s': %s",
		    file, strerror(errno));
		exit(1);
	}

	if (fs_top >= 0) {
		fs[fs_top].state = YY_CURRENT_BUFFER;
		fs[fs_top].line = line_number;
	}

	++fs_top;
	save_fs_size = fs_size;
	if (fs_size == 0)
		fs_size = 4;
	else if (fs_top >= fs_size)
		fs_size <<= 1;
	if (fs_size != save_fs_size)
		if ((fs = realloc(fs, fs_size * sizeof fs[0])) == NULL)
			fatal(NULL);

	fs[fs_top].file = strdup(file);
	fs[fs_top].line = 1;
	fs[fs_top].state = yy_create_buffer(yyin, YY_BUF_SIZE);

	line_number = fs[fs_top].line;
	yy_switch_to_buffer(fs[fs_top].state);
}

/*
 * Resume processing of the previous file.
 */
static int
pop_file()
{

	yy_delete_buffer(fs[fs_top].state);

	if (--fs_top < 0)
		return (EOF);

	line_number = fs[fs_top].line;
	printf("#line %d \"%s\"\n", line_number, fs[fs_top].file);
	yy_switch_to_buffer(fs[fs_top].state);
	return (0);
}

/*
 * Eat a string.  We require strings to be on a single line (no \\\n).
 * XXX Should we interpret C escape sequences?
 */
static int
string()
{
	size_t len = yyleng - 2;

	if (yyleng == 1 || yytext[yyleng - 1] != '"')
		len = yyleng - 1;

	if ((yylval.string = malloc(len + 1)) == NULL)
		fatal(NULL);
	memcpy(yylval.string, yytext + 1, len);
	yylval.string[len] = '\0';

	return (STRING);
}

/*
 * Eat a number.
 * We don't care what the value of a number is, so we pass it uninterpreted.
 */
static int
number()
{

	if ((yylval.string = strdup(yytext)) == NULL)
		fatal(NULL);
	return (NUMBER);
}

/*
 * Make sure that we can handle 'len' more bytes of C text.
 * We must guarantee that the C text will be nul-terminated.
 */
static void
need_ctext(size_t len)
{

	if (ctext_len + len + 1 > ctext_size) {
		if (ctext_size == 0 && len + 1 < 256)
			ctext_size = 256;
		else
			ctext_size += len + 256;
		if ((ctext = realloc(ctext, ctext_size)) == NULL)
			fatal(NULL);
	}
}

/*
 * Get ready to process a C text escape,
 * from a generic transform function or from %{ ... %}.
 */
static void
start_ctext()
{
	char *file = fs[fs_top].file;
	size_t len = snprintf(NULL, 0, "#line %d \"%s\"\n", line_number, file);

	ctext_len = 0;
	need_ctext(len);
	sprintf(ctext, "#line %d \"%s\"\n", line_number, file);
	ctext_len = len;
}

/*
 * Append the given text to the current C text escape.
 */
static void
add_ctext(char *s, size_t len)
{

	need_ctext(len);
	memcpy(&ctext[ctext_len], s, len);
	ctext_len += len;
	ctext[ctext_len] = '\0';
}

/*
 * Deliver a C text escape to the parser.
 * We assume that the parser will process it immediately,
 * so that we don't need to copy the memory.
 */
static int
end_ctext()
{

	if (ctext == NULL)
		yylval.string = strdup("");
	else {
		if ((yylval.string = malloc(ctext_len + 1)) == NULL)
			err(1, NULL);
		memcpy(yylval.string, ctext, ctext_len + 1);
		ctext_len = 0;
		ctext[0] = '\0';
	}
	return (C_TEXT);
}

/*
 * Complain about an ID.  This will cause the compilation to fail.
 */
void
id_err(struct ident *id, const char *format, ...)
{
	va_list ap;
	char *s;

	status = 1;

	va_start(ap, format);

	s = xsmprintf("%s: %d: %s: %s\n", id->file, id->line, id->name, format);
	vfprintf(stderr, s, ap);
	free(s);
	va_end(ap);
}

/*
 * Warn about an ID.
 */
void
id_warn(struct ident *id, const char *format, ...)
{
	va_list ap;
	char *s;

	va_start(ap, format);

	s = xsmprintf("%s: %d: %s: warning: %s\n", id->file, id->line,
	    id->name, format);
	vfprintf(stderr, s, ap);
	free(s);
	va_end(ap);
}

/*
 * A scanner-internal way to complain about a particular line number.
 */
static void
line_err(int line, const char *format, ...)
{
	va_list ap;
	int i = fs_top > 0 ? fs_top : 0;
	char *s;

	status = 1;

	va_start(ap, format);

	s = xsmprintf("%s: %d: %s\n", fs[i].file, line, format);
	vfprintf(stderr, s, ap);
	free(s);
	va_end(ap);
}

/*
 * The filename and/or line number may be vague here...
 */
int
yyerror(const char *s)
{
	int i = fs_top > 0 ? fs_top : 0;

	status = 1;

	fprintf(stderr, "%s: %d: %s\n", fs[i].file, line_number, s);

	return (0);	/* XXX ??? */
}
