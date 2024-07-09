/*
 * Copyright (c) 1992, 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	scsicmd.h,v 2.5 2000/10/23 22:42:09 richards Exp
 */

#include "pathnames.h"

typedef struct parameter {
	u_char	pm_bufno;	/* one of several buffers (0 => SCSI CDB) */
	u_char	pm_bit;		/* bit offset within byte */
	u_short	pm_len;		/* length of field in bits */
	u_int	pm_byte;	/* byte offset of high bit */
	char	*pm_name;	/* short name for parameter arguments */
	char	*pm_desc;	/* long name for printed description */
	u_long	pm_value;	/* value set by user or returned by SCSI */
	char	*pm_string;	/* like pm_value but for string parameters */
	const	char *pm_code;	/* translated status code for pm_value */
} pm;

typedef struct buffer {
	u_char	*bf_buf;	/* pointer to buffer space */
	u_long	bf_len;		/* length of buffer space */
	char	*bf_name;	/* short name for parameter arguments */
	char	*bf_desc;	/* long name for printed description */
	u_long	bf_return_len;	/* returned length */
} bf;

typedef struct command {
	bf	*cmd_bf;	/* address of buffer table */
	pm	*cmd_pm;	/* address of parameter table */
	void	(*cmd_ex) __P((void));		/* execute function */
	char	*cmd_name;	/* name of command */
	char	*cmd_desc;	/* description of command */
	void	(*cmd_code) __P((pm *));	/* code translation function */
	int	cmd_flag;	/* initialization state */
} cmd;

#define	CMD_INIT	0x1	/* committed initializations to buffers */

extern cmd *current_cmd;

typedef struct hosttype {
	cmd	*ht_cmd;	/* address of command table */
	char	*ht_name;	/* name (e.g. SCSI, aha, etc.) */
	char	*ht_desc;	/* description of host type */
} ht;

extern ht *current_ht;
extern ht httab[];

typedef u_char	ucdb12[12];
typedef u_char	ucdb10[10];
typedef u_char	ucdb6[6];

extern int scsi;
extern char *scsiname;
extern char *cmdtabfile;

void execute_command __P((void));
pm *extract_bf __P((char *, int));
void inquiry_code __P((pm *));
void mode_select_cmd __P((void));
void mode_sense_cmd __P((void));
void link_mode_commands __P((void));
void print_parameters __P((char *));
void read_cmd __P((void));
void read_data_cmd __P((void));
void read_var_cmd __P((void));
void request_sense_cmd __P((void));
void request_sense_code __P((pm *));
void scsi_sense __P((struct scsi_user_sense *));
cmd *search_command __P((char *));
void set_parameters __P((char *));
char *sprintasc __P((unsigned, unsigned));
void start_command __P((char *));
void store_pm __P((pm *));
void write_cmd __P((void));

#define	BUFFER_SIZE	256

struct name {
	char *name;
	char *desc;
};

struct range {
	int low;
	int high;
};

enum cmdtype {
	CMD_READ,
	CMD_WRITE,
	CMD_READ_VAR,
	CMD_READ_DATA,
	CMD_SELECT,
	CMD_SENSE,
	CMD_RS
};

enum format {
	FORMAT_DEFAULT,
	FORMAT_CODE,
	FORMAT_STRING
};

#define	DEF_CMDS	2
#define	DEF_BFS		2
#define	DEF_PMS		2

void add_command __P((struct name *, enum cmdtype));
void add_buffer __P((struct name *, size_t, int));
void add_byte __P((struct range *));
void add_bit __P((struct range *, struct name *, enum format, u_long));
struct name *add_name __P((char *, char *));
struct range *add_range __P((int, int));
struct range *scan_range __P((void));
