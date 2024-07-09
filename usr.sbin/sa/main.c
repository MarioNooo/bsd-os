/* BSDI main.c,v 2.2 1996/02/29 18:14:53 bostic Exp */

#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include "sa.h"

static void
dump_acct(void)
{
	static int signals[] = {
		SIGHUP, SIGINT, SIGQUIT, SIGXCPU, SIGXFSZ, SIGVTALRM, 0,
	};

	trap_signals(signals, terminate);
	if (show_user_dump)
		iterate_accounting_file(accounting_data_path,
		    dump_user_command);
	else {
		initialize_summaries();
		iterate_accounting_file(accounting_data_path, add_to_summaries);
		show_summary();
		if (store_summaries)
			save_summaries();
	}
}

int
main(int argc, char *const * argv)
{
	int ch;
	while ((ch = getopt(argc, argv, "abcdDfijkKlmnrstuv:S:U:")) != EOF)
		switch (ch) {
		case 'a':
			use_other = FALSE;
			break;
		case 'b':
			sort_mode = AVERAGE_TIME;
			break;
		case 'c':
			show_percent_time = TRUE;
			break;
		case 'd':
			sort_mode = AVERAGE_DISK;
			break;
		case 'D':
			show_total_disk = TRUE;
			sort_mode = TOTAL_DISK;
			break;
		case 'f':
			junk_interactive = FALSE;
			break;
		case 'i':
			read_summary = FALSE;
			break;
		case 'j':
			use_average_times = TRUE;
			break;
		case 'k':
			sort_mode = AVERAGE_MEMORY;
			break;
		case 'K':
			show_kcore_secs = TRUE;
			sort_mode = KCORE_SECS;
			break;
		case 'l':
			separate_sys_and_user = TRUE;
			break;
		case 'm':
			show_user_info = TRUE;
			break;
		case 'n':
			sort_mode = CALL_COUNT;
			break;
		case 'r':
			reverse_sort = TRUE;
			break;
		case 's':
			store_summaries = TRUE;
			break;
		case 't':
			show_real_over_cpu = TRUE;
			break;
		case 'u':
			show_user_dump = TRUE;
			break;
		case 'v':
			junk_threshold = atoi(optarg);
			break;
		case 'S':
			command_summary_path = optarg;
			break;
		case 'U':
			user_summary_path = optarg;
			break;
		default:
			fprintf(stderr,
"Usage: sa [-abcdDfijkKlnrstuv] [-S savacctfile] [-U usracctfile] [file]\n");
			exit (1);
		}
	argc -= optind;
	argv += optind;

	if (*argv != NULL)
		accounting_data_path = *argv;
	dump_acct();
	exit (0);
}
