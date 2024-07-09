/* BSDI options.c,v 2.2 1996/02/29 18:14:54 bostic Exp */

#include "sa.h"
#include "pathnames.h"

int sort_mode = TOTAL_TIME;

bool_t use_other = TRUE;
bool_t show_percent_time = FALSE;
bool_t show_total_disk = FALSE;
bool_t junk_interactive = TRUE;
bool_t read_summary = TRUE;
bool_t use_average_times = FALSE;
bool_t show_kcore_secs = FALSE;
bool_t separate_sys_and_user = FALSE;
bool_t show_user_info = FALSE;
bool_t reverse_sort = FALSE;
bool_t store_summaries = FALSE;
bool_t show_real_over_cpu = FALSE;
bool_t show_user_dump = FALSE;

int junk_threshold = 0;
#ifdef DEBUG
char *command_summary_path = "savacct";
char *user_summary_path = "usracct";
char *accounting_data_path = "acct";
#else
char *command_summary_path = _PATH_COMMAND_SUMMARY;
char *user_summary_path = _PATH_USER_SUMMARY;
char *accounting_data_path = _PATH_ACCT;
#endif
