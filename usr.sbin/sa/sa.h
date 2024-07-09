/* BSDI sa.h,v 2.5 1998/03/18 20:14:27 jch Exp */

#include <sys/types.h>
#include <sys/acct.h>

#include <limits.h>
#include <signal.h>
#include <stdio.h>

struct acctcmd {
  char		ac_comm[10];	/* command name */
  comp_t	ac_utime;	/* user time */
  comp_t	ac_stime;	/* system time */
  comp_t	ac_etime;	/* elapsed time */
  short		ac_mem;		/* average memory usage */
  comp_t	ac_io;		/* count of IO blocks */
  u_long	ac_count;	/* count of calls */
};
struct acctusr {
  uid_t		ac_uid;		/* user id */
  comp_t	ac_utime;	/* user time */
  comp_t	ac_stime;	/* system time */
  comp_t	ac_etime;	/* elapsed time */
  short		ac_mem;		/* average memory usage */
  comp_t	ac_io;		/* count of IO blocks */
  u_long	ac_count;	/* count of calls */
};

typedef char bool_t;
#define TRUE 1
#define FALSE 0


extern int sort_mode;
#define TOTAL_TIME 0
#define AVERAGE_TIME 1
#define AVERAGE_DISK 2
#define TOTAL_DISK 3
#define AVERAGE_MEMORY 4
#define KCORE_SECS 5
#define CALL_COUNT 6
#define NSMODE 7

#ifdef __GNUC__
#define COMP_T long long
#else
#define COMP_T double
#endif


extern bool_t use_other;
extern bool_t show_percent_time;
extern bool_t show_total_disk;
extern bool_t junk_interactive;
extern bool_t read_summary;
extern bool_t use_average_times;
extern bool_t show_kcore_secs;
extern bool_t separate_sys_and_user;
extern bool_t show_user_info;
extern bool_t reverse_sort;
extern bool_t store_summaries;
extern bool_t show_real_over_cpu;
extern bool_t show_user_dump;

extern int junk_threshold;

extern char *command_summary_path;
extern char *user_summary_path;
extern char *accounting_data_path;

extern char command_temp_name[PATH_MAX];
extern char user_temp_name[PATH_MAX];


extern int (*user_compare_funcs[NSMODE]) (const void *, const void *);
extern int (*command_compare_funcs[NSMODE]) (const void *, const void *);

extern double total_time;
extern struct acctcmd *other_summary;
extern struct acctcmd *junk_summary;


void iterate_accounting_file (const char *, int (*) (const struct acct *));
void iterate_user_summary (int (*) (const struct acctusr *));
void iterate_command_summary (int (*) (const struct acctcmd *));
comp_t comp_add (comp_t, comp_t);

int dump_user_command (const struct acct *);

void done (int);
void terminate (void);
void error (const char *, const char *);
void fatal_error (const char *, const char *, const int);

void initialize_summaries (void);
int add_to_summaries (const struct acct *);
void show_summary (void);
void save_summaries (void);

void add_to_sort_table (const void *);
void iterate_sort_table (int (*) (const void *, const void *),
			 int (*) (const void *));

struct acctcmd *reclassify (const struct acctcmd *record);
void show_user_header (void);
void show_command_header (void);
int show_user_entry (const void *);
int show_command_entry (const void *);
FILE *make_temporary (const char, char *);
void move (const char *, const char *);
int copy_and_truncate (const char *, const char *);

void trap_signals (int *, void (*) (void));

#ifdef __GNUC__
static inline COMP_T comp_conv (comp_t c)
{
  COMP_T r = (COMP_T)(c & 0x1fff) << ((c >> 13) * 3);

  return r;
}
#else
COMP_T comp_conv (comp_t);
#endif

#ifdef __GNUC__
static inline void block_signals (sigset_t *m)
{
  sigset_t nm;
  (void) sigfillset (&nm);
#ifdef DEBUG
  (void) sigdelset (&nm, SIGTRAP);
#endif
  (void) sigprocmask (SIG_SETMASK, &nm, m);
}

static inline void unblock_signals (sigset_t *m)
{
  (void) sigprocmask (SIG_SETMASK, m, NULL);
}
#else				/* sigh */
#ifdef DEBUG
#define block_signals(M) do { \
			   sigset_t _nm; \
			   (void) sigfillset (&_nm); \
			   (void) sigdelset (&_nm, SIGTRAP); \
			   (void) sigprocmask (SIG_SETMASK, &nm, (M)); \
			 while (0)
#else
#define block_signals(M) do { \
			   sigset_t _nm; \
			   (void) sigfillset (&_nm); \
			   (void) sigprocmask (SIG_SETMASK, &nm, (M)); \
			 while (0)
#endif

#define unblock_signals(M) (void) sigprocmask (SIG_SETMASK, (M), NULL)
#endif
