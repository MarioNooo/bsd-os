/* BSDI summary.c,v 2.2 1996/02/29 18:14:58 bostic Exp */

#include <sys/types.h>
#include <sys/stat.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "sa.h"

#define NENTRY 521
struct entry {
	long    uid;
	char    command[10];
	void   *data;
	struct entry *next;
}      *table[NENTRY];

double  total_time = 0.0;
struct acctcmd *other_summary;
struct acctcmd *junk_summary;

static short
merge_memory(comp_t setime, comp_t retime, short sm, short rm)
{
	COMP_T  se = comp_conv(setime);
	COMP_T  re = comp_conv(retime);

	if (se + re != 0) {
		COMP_T  answer = (sm * se + rm * re) / (se + re);

		if (answer >= SHRT_MAX)
			return SHRT_MAX;
		return answer;
	}
	return (sm + rm) / 2;
}


#define MOVE(F)	s->F = record->F
#define ADD(F)	s->F = comp_add (s->F, record->F)

static void
merge_user(void *summary, const struct acct * record)
{
	struct acctusr *s = summary;

	ADD(ac_utime);
	ADD(ac_stime);
	ADD(ac_etime);
	ADD(ac_io);
	s->ac_mem = merge_memory(s->ac_etime, record->ac_etime,
	    s->ac_mem, record->ac_mem);
	++s->ac_count;
}

static void *
init_user(const struct acct * record)
{
	struct acctusr *s = malloc(sizeof(struct acctusr));

	if (s == NULL)
		fatal_error(NULL, "Out of memory.", EX_SOFTWARE);
	MOVE(ac_uid);
	MOVE(ac_utime);
	MOVE(ac_stime);
	MOVE(ac_etime);
	MOVE(ac_io);
	MOVE(ac_mem);
	s->ac_count = 1;
	return s;
}

static void
merge_command(void *summary, const struct acct * record)
{
	struct acctcmd *s = summary;

	ADD(ac_utime);
	ADD(ac_stime);
	ADD(ac_etime);
	ADD(ac_io);
	s->ac_mem = merge_memory(s->ac_etime, record->ac_etime,
	    s->ac_mem, record->ac_mem);
	++s->ac_count;
}

void   *
init_command(const struct acct * record)
{
	struct acctcmd *s = malloc(sizeof(struct acctusr));

	if (s == NULL)
		fatal_error(NULL, "Out of memory.", EX_SOFTWARE);
	memcpy(s->ac_comm, record->ac_comm, 10);
	MOVE(ac_utime);
	MOVE(ac_stime);
	MOVE(ac_etime);
	MOVE(ac_io);
	MOVE(ac_mem);
	s->ac_count = 1;
	return s;
}

static struct entry **
find_entry(const long uid, const char *command)
{
	int     i;
	struct entry *entry, **previous;
	unsigned hash = uid & 0xffff;

	for (i = 0; i < 10; ++i) {
		if (command[i] == 0)
			break;
		hash ^= command[i];
		hash &= 0xffff;
	}
	previous = &table[hash % NENTRY];
	for (entry = *previous; entry != NULL;
	    previous = &entry->next, entry = entry->next)
		if (entry->uid == uid &&
		    strncmp(entry->command, command, 10) == 0)
			break;
	return previous;
}

static void
add_to_summary(const struct acct * record,
    const long uid, const char *command,
    void (*merge) (void *, const struct acct *),
    void *(*init) (const struct acct *))
{
	struct entry **previous = find_entry(uid, command);

	if (*previous != NULL)
		merge((*previous)->data, record);
	else {
		struct entry *entry = *previous = malloc(sizeof(struct entry));

		if (entry == NULL)
			fatal_error(NULL, "Out of memory.", EX_SOFTWARE);
		entry->uid = uid;
		strncpy(entry->command, command, 10);
		entry->data = init(record);
	}
}

int
add_to_summaries(const struct acct * record)
{
	if (show_user_info || store_summaries)
		add_to_summary(record,
		    record->ac_uid, "", merge_user, init_user);
	if (!show_user_info || store_summaries)
		add_to_summary(record,
		    -1, record->ac_comm, merge_command, init_command);
	return TRUE;
}

static int
add_user_summary(const struct acctusr * record)
{
	/* silently weed out zero count * records */
	if (record->ac_count > 0) {
		struct entry **previous = find_entry(record->ac_uid, "");

		if (*previous == NULL) {
			struct acctusr *s = malloc(sizeof(struct acctusr));
			struct entry *entry =
			    *previous = malloc(sizeof(struct entry));

			if (s == NULL || entry == NULL)
				fatal_error(NULL,
				    "Out of memory.", EX_SOFTWARE);
			entry->uid = record->ac_uid;
			entry->command[0] = 0;
			entry->data = s;
			MOVE(ac_uid);
			MOVE(ac_utime);
			MOVE(ac_stime);
			MOVE(ac_etime);
			MOVE(ac_io);
			MOVE(ac_mem);
			MOVE(ac_count);
			return TRUE;
		}
		fatal_error(user_summary_path,
		    "Duplicate user id found.", EX_DATAERR);
	}
	return TRUE;
}

static int
add_command_summary(const struct acctcmd * record)
{
	/* silently weed out zero count records */
	if (record->ac_count >= 0) {
		struct acctcmd *s;
		struct entry **previous = find_entry(-1, record->ac_comm);
		struct entry *entry;

		if (*previous != NULL)	/* must not have been seen before */
			goto error;	/* might handle this more softly...
					 * XXX */
		s = malloc(sizeof(struct acctcmd));
		entry = *previous = malloc(sizeof(struct entry));
		if (s == NULL || entry == NULL)
			fatal_error(NULL, "Out of memory.", EX_SOFTWARE);
		entry->uid = -1;
		memcpy(entry->command, record->ac_comm, 10);
		entry->data = s;
		memcpy(s->ac_comm, record->ac_comm, 10);
		MOVE(ac_utime);
		MOVE(ac_stime);
		MOVE(ac_etime);
		MOVE(ac_io);
		MOVE(ac_mem);
		MOVE(ac_count);
		return TRUE;
error:
		fatal_error(command_summary_path,
		    "Duplicate command name found.", EX_DATAERR);
	}
	return TRUE;
}

void
initialize_summaries(void)
{
	other_summary = calloc(1, sizeof(struct acctcmd));
	if (other_summary == NULL)
		fatal_error(NULL, "Out of memory.", EX_SOFTWARE);
	strcpy(other_summary->ac_comm, "***other");
	junk_summary = calloc(1, sizeof(struct acctcmd));
	if (junk_summary == NULL)
		fatal_error(NULL, "Out of memory.", EX_SOFTWARE);
	strcpy(junk_summary->ac_comm, "**junk**");
	if (read_summary) {
		iterate_user_summary(add_user_summary);
		iterate_command_summary(add_command_summary);
	}
}

void
show_summary(void)
{
	int     i;
	struct entry *entry;

	for (i = 0; i < NENTRY; ++i)
	    for (entry = table[i]; entry != NULL; entry = entry->next)
		if (entry->uid == -1) {
			bool_t  add_to_sort = TRUE;
			if (!show_user_info || store_summaries) {
				struct acctcmd *s = reclassify(entry->data);

				if (s) {
					struct acctcmd *record = entry->data;

					ADD(ac_utime);
					ADD(ac_stime);
					ADD(ac_io);
					s->ac_mem = merge_memory(s->ac_etime,
					    record->ac_etime, s->ac_mem,
					    record->ac_mem);
					++s->ac_count;
					add_to_sort = FALSE;
				}
			}
			if (!show_user_info) {
				if (show_percent_time) {
					struct acctcmd *c = entry->data;

					total_time += comp_conv(c->ac_utime);
					total_time += comp_conv(c->ac_stime);
				}
				if (add_to_sort)
					add_to_sort_table(entry->data);
			}
		} else
			if (show_user_info) {
				if (show_percent_time) {
					struct acctusr *u = entry->data;

					total_time += comp_conv(u->ac_utime);
					total_time += comp_conv(u->ac_stime);
				}
				add_to_sort_table(entry->data);
			}

	if (show_user_info) {
		show_user_header();
		iterate_sort_table(user_compare_funcs[sort_mode],
		    show_user_entry);
	} else {
		if (other_summary->ac_count > 0)
			add_to_sort_table(other_summary);
		if (junk_summary->ac_count > 0)
			add_to_sort_table(junk_summary);
		show_command_header();
		iterate_sort_table(command_compare_funcs[sort_mode],
		    show_command_entry);
	}
}

void
save_summaries(void)
{
	struct entry *entry;
	sigset_t signals;
	FILE   *cmd = make_temporary('c', command_temp_name);
	FILE   *usr = make_temporary('u', user_temp_name);
	int     i;

	for (i = 0; i < NENTRY; ++i)
		for (entry = table[i]; entry != NULL; entry = entry->next) {
			if (entry->uid == -1) {
				if (fwrite(entry->data,
				    sizeof(struct acctcmd), 1, cmd) != 1)
					fatal_error(command_temp_name,
					    "Write error.", EX_IOERR);
			} else {
				if (fwrite(entry->data,
				    sizeof(struct acctusr), 1, usr) != 1)
					fatal_error(user_temp_name,
					    "Write error.", EX_IOERR);
			}
		}
	if (fclose(cmd) == EOF)
		fatal_error(command_temp_name, NULL, EX_IOERR);
	if (fclose(usr) == EOF)
		fatal_error(user_temp_name, NULL, EX_IOERR);

	block_signals(&signals);
	move(command_temp_name, command_summary_path);
	(void)chmod(command_summary_path,
	    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	move(user_temp_name, user_summary_path);
	(void)chmod(user_summary_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	unblock_signals(&signals);
}
