/* BSDI compare.c,v 2.2 1996/02/29 18:14:49 bostic Exp */

#include "sa.h"

#define SETUP if (! reverse_sort) { const void *t = left; left = right; right = t; }


#define DECLARE(N) static int user_##N (const void *left, const void *right)
#define L(F) ((* (struct acctusr **) left)->F)
#define R(F) ((* (struct acctusr **) right)->F)
#include "compare.i"
#undef DECLARE
#undef L
#undef R


#define DECLARE(N) static int command_##N (const void *left, const void *right)
#define L(F) ((* (struct acctcmd **) left)->F)
#define R(F) ((* (struct acctcmd **) right)->F)
#include "compare.i"
#undef DECLARE
#undef L
#undef R


int (*user_compare_funcs[NSMODE]) (const void *, const void *) = {
  user_total_time, user_average_time, user_average_disk,
  user_total_disk, user_average_memory, user_kcore_secs,
  user_call_count,
};

int (*command_compare_funcs[NSMODE]) (const void *, const void *) = {
  command_total_time, command_average_time, command_average_disk,
  command_total_disk, command_average_memory, command_kcore_secs,
  command_call_count,
};
