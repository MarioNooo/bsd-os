/* BSDI acct.c,v 2.2 1996/02/29 18:14:48 bostic Exp */

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

#include "sa.h"


static void
iterator(const char *file, size_t size,
    bool_t must_exist, int (*func) (const void *))
{
	FILE   *f = fopen(file, "r");

	if (f != NULL) {
		void   *record = alloca(size);	/* assume success? XXX */

		for (;;) {
			if (fread(record, size, 1, f) == 1) {
				if (func(record))
					continue;
				fclose(f);
				return;
			}
			if (feof(f)) {
				fclose(f);
				return;
			}
			fatal_error(file, "Read error.", EX_IOERR);
		}
	}
	if (errno == ENOENT && !must_exist)
		return;
	fatal_error(file, NULL, EX_NOINPUT);
}

void
iterate_accounting_file(const char *file, int (*func) (const struct acct *))
{
	iterator(file,
	    sizeof(struct acct), TRUE, (int (*) (const void *)) func);
}

void
iterate_user_summary(int (*func) (const struct acctusr *))
{
	iterator(user_summary_path,
	    sizeof(struct acctusr), FALSE, (int (*) (const void *)) func);
}

void
iterate_command_summary(int (*func) (const struct acctcmd *))
{
	iterator(command_summary_path,
	    sizeof(struct acctcmd), FALSE, (int (*) (const void *)) func);
}
comp_t
comp_add(comp_t left, comp_t right)
{
	int     ml = left & 0x1fff;
	int     mr = right & 0x1fff;
	int     el = left >> 13;
	int     er = right >> 13;

	if (el < er) {
		do {
			ml >>= 3;
			++el;
		}
		while (el < er);
	} else
		if (er < el) {
			do {
				mr >>= 3;
				++er;
			}
			while (er < el);
		}
	ml += mr;
	if (ml & ~0x1fff) {
		if (el == 7)
			return 0xffff;
		ml >>= 3;
		++el;
	}
	return (ml & 0x1fff) | (el << 13);
}
#ifndef __GNUC__
double
comp_conv(comp_t c)
{
	return ldexp(c & 0x1fff, c >> 13);
}
#endif

#ifdef notyet
void
remerge_accounting_files(void)
{
}
#endif
