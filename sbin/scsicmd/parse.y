/*
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI parse.y,v 2.4 2001/12/19 18:21:19 chrisk Exp
 */

%{
#include <sys/types.h>
#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dev/scsi/scsi.h>
#include <dev/scsi/scsi_ioctl.h>

#include "scsicmd.h"

extern int yylineno;
int yylex __P((void));
int yyparse __P((void));

size_t scsicmdtab_size;
bf *current_bf;
size_t current_bf_size;
pm *current_pm;
size_t current_pm_size;

static int parse_error;
%}

%union {
	int	number;
	struct	range *range;
	char	*string;
	struct	name *name;
	enum	cmdtype cmdtype;
	enum	format format;
}

%token COMMAND
%token BUFFER
%token BYTE
%token BIT
%token STRING
%token NUMBER
%token RANGE
%token LENGTH
%token CMDTYPE
%token FORMAT

%type <number> NUMBER number_opt offset_opt
%type <range> RANGE range
%type <string> STRING string_opt
%type <name> name
%type <cmdtype> CMDTYPE cmdtype_opt
%type <format> FORMAT format_opt

%%

start:
	commands {
		if (parse_error)
			YYABORT;
		add_command(0, 0);
	};

commands:
		commands command
	|
		error
	|
		/* empty */;

/* command: name */
command:
	COMMAND name cmdtype_opt { add_command($2, $3); } buffers;

cmdtype_opt:
		CMDTYPE
	|
		{ $$ = CMD_READ; };

buffers:
		buffers buffer
	|
		error
	|
		/* empty */;

/* buffer: description (abbreviation): size */
buffer:
		BUFFER offset_opt name number_opt
		{ add_buffer($3, $4, $2); } bytes;

offset_opt:
		'+' NUMBER { $$ = $2; }
	|
		/* empty */ { $$ = 0; };

bytes:
		bytes byte
	|
		error
	|
		/* empty */;

/* bytes 0-3: ... */
byte:
		BYTE range { add_byte($2); } bits
	|
		BYTE range { add_byte($2); } name format_opt number_opt
			{ add_bit(0, $4, $5, $6); };

bits:
		bits bit
	|
		error
	|
		;

/* bits 3-5: description (name): initializer */
bit:
		BIT range name format_opt number_opt
			{ add_bit($2, $3, $4, $5); }
	|
		LENGTH NUMBER name format_opt number_opt
			{ add_bit(add_range(0, $2 - 1), $3, $4, $5); };

name:
	STRING string_opt { $$ = add_name($1, $2); };

string_opt:
		STRING
	|
		/* empty */ { $$ = 0; };

format_opt:
		FORMAT
	|
		/* empty */ { $$ = FORMAT_DEFAULT; };

number_opt:
		NUMBER
	|
		/* empty */ { $$ = 0; };

range:
		NUMBER { $$ = add_range($1, $1); }
	|
		RANGE;

%%

static struct {
	char *name;
	void (*code) __P((pm *));
} code_table[] = {
	{ "rs", request_sense_code },
	{ "in", inquiry_code },
	{ 0 }
};

static void
check_code()
{

	int i;

	for (i = 0; code_table[i].name; ++i)
		if (strcmp(current_cmd->cmd_name, code_table[i].name) == 0) {
			current_cmd->cmd_code = code_table[i].code;
			return;
		}
}

static void
check_cmdtype(cmdtype)
	int cmdtype;
{
	cmd *cmdp;

	switch (cmdtype) {
	case CMD_READ:
	default:
		current_cmd->cmd_ex = read_cmd;
		break;
	case CMD_WRITE:
		current_cmd->cmd_ex = write_cmd;
		break;
	case CMD_READ_VAR:
		current_cmd->cmd_ex = read_var_cmd;
		break;
	case CMD_READ_DATA:
		current_cmd->cmd_ex = read_data_cmd;
		break;
	case CMD_SELECT:
		current_cmd->cmd_ex = mode_select_cmd;
		break;
	case CMD_SENSE:
		current_cmd->cmd_ex = mode_sense_cmd;
		break;
	case CMD_RS:
		current_cmd->cmd_ex = request_sense_cmd;
		break;
	}
}

void
link_mode_commands()
{
	cmd	*select_cmd = NULL;
	cmd	*sense_cmd = NULL;
	cmd	*cmdp;

	/* force sharing between MODE SELECT and MODE SENSE buffers */
	for (cmdp = httab[0].ht_cmd; cmdp->cmd_ex; cmdp++) {
		if (cmdp->cmd_ex == mode_select_cmd) {
			select_cmd = cmdp;
		} else if (cmdp->cmd_ex == mode_sense_cmd) {
			sense_cmd = cmdp;
		}
	}
	if (sense_cmd && select_cmd) {
		if (select_cmd->cmd_bf || select_cmd->cmd_pm) {
			if (sense_cmd->cmd_bf || sense_cmd->cmd_pm) {
				errx(1, "link_mode_commands: MODE SELECT and MODE SENSE both have definitions in SCSITAB\n");
			}
			sense_cmd->cmd_bf = select_cmd->cmd_bf;
			sense_cmd->cmd_pm = select_cmd->cmd_pm;
		} else if (sense_cmd->cmd_bf || sense_cmd->cmd_pm) {
			if (select_cmd->cmd_bf || select_cmd->cmd_pm) {
				errx(1, "link_mode_commands: MODE SELECT and MODE SENSE both have definitions in SCSITAB\n");
			}
			select_cmd->cmd_bf = sense_cmd->cmd_bf;
			select_cmd->cmd_pm = sense_cmd->cmd_pm;
		}
	}
}

void
add_command(name, cmdtype)
	struct name *name;
	enum cmdtype cmdtype;
{
	size_t osize;

	if (current_pm) {
		add_byte(0);
		current_pm = NULL;
	}
	if (current_bf) {
		add_buffer(0, 0, 0);
		current_bf = NULL;
	}

	if (httab[0].ht_cmd == 0) {
		if ((httab[0].ht_cmd = malloc(sizeof (cmd) * DEF_CMDS)) == 0)
			err(1, "add_command");
		current_cmd = httab[0].ht_cmd;
		scsicmdtab_size = DEF_CMDS;
		bzero(httab[0].ht_cmd, sizeof (cmd) * scsicmdtab_size);
	} else if (++current_cmd >= &httab[0].ht_cmd[scsicmdtab_size]) {
		osize = scsicmdtab_size;
		scsicmdtab_size *= 2;
		if ((httab[0].ht_cmd =
		    realloc(httab[0].ht_cmd,
		    sizeof (cmd) * scsicmdtab_size)) == 0)
			err(1, "add_command");
		current_cmd = &httab[0].ht_cmd[osize];
		bzero(current_cmd, sizeof (cmd) * osize);
	}
	if (name) {
		current_cmd->cmd_name = name->name;
		current_cmd->cmd_desc = name->desc;
		free(name);
		check_code();
		check_cmdtype(cmdtype);
	}
}

void
add_buffer(name, bf_size, offset)
	struct name *name;
	size_t bf_size;
	int offset;
{
	bf *obf;
	size_t osize;
	scsi_user_cdb_t suc;	/* used only in a sizeof expression */

	if (current_cmd->cmd_bf == 0) {
		if ((current_cmd->cmd_bf = malloc(sizeof (bf) * DEF_BFS)) == 0)
			err(1, "add_buffer");
		current_bf = current_cmd->cmd_bf;
		current_bf_size = DEF_BFS;
		bzero(current_cmd->cmd_bf, sizeof (bf) * current_bf_size);
	} else if (++current_bf >= &current_cmd->cmd_bf[current_bf_size]) {
		osize = current_bf_size;
		current_bf_size *= 2;
		if ((current_cmd->cmd_bf =
		    realloc(current_cmd->cmd_bf,
		    sizeof (bf) * current_bf_size)) == 0)
			err(1, "add_buffer");
		current_bf = &current_cmd->cmd_bf[osize];
		bzero(current_bf, sizeof (bf) * osize);
	}

	if (name == 0)
		return;

	/* If it's the command buffer then allocate space for the command */
	if (current_bf == &current_cmd->cmd_bf[0]) {
		if (bf_size == 0)
			bf_size = sizeof (suc.suc_cdb);
		if ((current_bf->bf_buf = malloc(bf_size)) == 0)
                        err(1, "add_buffer");
                bzero(current_bf->bf_buf, bf_size);
                current_bf->bf_len = bf_size;
	} else if (current_bf == &current_cmd->cmd_bf[1]) {
		/*
		 * If it's the returned data buffer allocate at least
		 * 256 bytes to accomodate devices that need extra space.
		 * But, keep bf_len at the expected returned capacity.
		 */
                if ((current_bf->bf_buf = 
		    malloc(bf_size < BUFFER_SIZE ? BUFFER_SIZE : bf_size)) == 0)
                        err(1, "add_buffer");
                bzero(current_bf->bf_buf, 
		    bf_size < BUFFER_SIZE ? BUFFER_SIZE : bf_size);
                current_bf->bf_len = bf_size == 0 ? BUFFER_SIZE : bf_size;
	} else {
                obf = &current_cmd->cmd_bf[1];
                current_bf->bf_buf = obf->bf_buf + offset;
                current_bf->bf_len = obf->bf_len - offset;
	}

	current_bf->bf_name = name->name;
	current_bf->bf_desc = name->desc;
	free(name);
}

void
add_byte(range)
	struct range *range;
{
	size_t osize;

	if (current_cmd->cmd_pm == 0) {
		if ((current_cmd->cmd_pm = malloc(sizeof (pm) * DEF_PMS)) == 0)
			err(1, "add_byte");
		current_pm = current_cmd->cmd_pm;
		current_pm_size = DEF_PMS;
		bzero(current_cmd->cmd_pm, sizeof (pm) * current_pm_size);
	} else if (++current_pm >= &current_cmd->cmd_pm[current_pm_size]) {
		osize = current_pm_size;
		current_pm_size *= 2;
		if ((current_cmd->cmd_pm =
		    realloc(current_cmd->cmd_pm,
		    sizeof (pm) * current_pm_size)) == 0)
			err(1, "add_byte");
		current_pm = &current_cmd->cmd_pm[osize];
		bzero(current_pm, sizeof (pm) * osize);
	}
	if (range) {
		current_pm->pm_bufno = current_bf - current_cmd->cmd_bf;
		current_pm->pm_byte = range->low;
		current_pm->pm_bit = 0;
		current_pm->pm_len = (range->high + 1 - range->low) * 8;
		free(range);
	}
}

void
add_bit(range, name, format, init)
	struct range *range;
	struct name *name;
	enum format format;
	u_long init;
{

	if (current_pm->pm_name) {
		add_byte(0);
		current_pm->pm_bufno = (current_pm - 1)->pm_bufno;
		current_pm->pm_byte = (current_pm - 1)->pm_byte;
	}
	if (range) {
		current_pm->pm_bit = range->low;
		current_pm->pm_len = range->high + 1 - range->low;
		free(range);
	}
	current_pm->pm_name = name->name;
	current_pm->pm_desc = name->desc;
	free(name);
	current_pm->pm_value = init;
	if (format == FORMAT_STRING)
		current_pm->pm_string = "";
	else if (format == FORMAT_CODE)
		current_pm->pm_code = "";
}

struct name *
add_name(full_name, abbreviation)
	char *full_name, *abbreviation;
{
	struct name *name;

	if ((name = malloc(sizeof *name)) == 0)
		err(1, "add_name(%s, %s)", full_name, abbreviation);
	name->name = abbreviation;
	name->desc = full_name;
	return (name);
}

struct range *
add_range(low, high)
	int low, high;
{
	struct range *range;

	if ((range = malloc(sizeof *range)) == 0)
		err(1, "add_range(%d, %d)", low, high);
	range->low = low;
	range->high = high;
	return (range);
}

yyerror(s)
	char *s;
{

	fprintf(stderr, "syntax error");
	if (current_cmd && current_cmd->cmd_name) {
		fprintf(stderr, " after command %s", current_cmd->cmd_name);
		if (current_bf && current_bf->bf_name)
			fprintf(stderr, ", buffer %s", current_bf->bf_name);
		if (current_pm && current_pm->pm_name)
			fprintf(stderr, ", param %s", current_pm->pm_name);
	}
	fprintf(stderr, " near line %d\n", yylineno);
	parse_error = 1;
}
