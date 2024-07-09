/* Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved. */

/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.  All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI strings.c,v 2.6 2001/10/03 17:29:58 polk Exp
 */

#include <sys/types.h>
#include <a.out.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "strings_private.h"

static void usage(void);
static off_t whole_file(off_t *);
sec_t (*classify[])(FILE *, off_t *);

int
main(int argc, char **argv)
{
	static char *stdin_argv[] = { "/dev/stdin", NULL };
	static char ibuf[2];
	off_t (*get_section)(off_t *) = NULL;
	off_t offset;
	off_t new_offset;
	off_t size;
	size_t threshold = DEFAULT_THRESHOLD;
	size_t bufsize;
	FILE *f;
	char *buf;
	char *p;
	int print_headings;
	int set_threshold = 0;
	int print_filename = 0;
	int print_offset = 0;
	int rval = 0;
	int save_errno;
	int print;
	int t;
	int c;
	int i;
	int check_whole_file = 0;

	for (i = optind;
	    (c = getopt(argc, argv, "-0123456789afn:o")) != -1;
	    i = optind) {
		switch (c) {
		/* Compatibility with non-POSIX strings(1).  */
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			/*
			 * 'i' tracks the current argument number.
			 * If it changes, we reset and start over;
			 * this makes 'strings -3 -5' work like
			 * 'strings -5', not 'strings -35'.
			 */
			if (set_threshold != i) {
				threshold = 0;
				set_threshold = i;
			}
			ibuf[0] = optopt;
			threshold = threshold * 10 + atoi(ibuf);
			continue;
		/* Compatibility with non-POSIX strings(1).  */
		case '-':
		case 'a':
			check_whole_file = 1;
			break;
		case 'f':
			print_filename = 1;
			break;
		case 'n':
			if ((threshold = atoi(optarg)) < 1)
				errx(1, "illegal threshold (%lu)",
				    (unsigned long)threshold);
			break;
		case 'o':
			print_offset = 1;
			break;
		default:
			usage();
			break;
		}
		/* Make 'strings -3f5' work like 'strings -f -5'.  */
		set_threshold = 0;
	}

	if (threshold < 1)
		errx(1, "invalid threshold (%lu)", (unsigned long)threshold);

	bufsize = threshold + 1;
	if (bufsize < DEFAULT_BUFSIZE)
		bufsize = DEFAULT_BUFSIZE;
	if ((buf = malloc(bufsize)) == NULL)
		err(1, NULL);

	argv += optind;

	if (*argv == NULL)
		argv = stdin_argv;

	for (; *argv; ++argv) {
		if ((f = fopen(*argv, "r")) == NULL) {
			warn("%s", *argv);
			rval = 1;
			continue;
		}

		/*
		 * Figure out what kind of object file we have,
		 * and arrange to scan it with the correct scan routine.
		 */
		new_offset = 0;
		get_section = NULL;
		if (check_whole_file != NULL)
			get_section = whole_file;
		else
			for (i = 0; get_section == NULL; ++i)
				get_section = (*classify[i])(f, &new_offset);
		if (feof(f) || ferror(f)) {
			if (ferror(f)) {
				warn("%s", *argv);
				rval = 1;
			}
			fclose(f);
			continue;
		}
		offset = new_offset;

		/*
		 * Scan the file a section at a time.
		 */
		while ((size = (*get_section)(&new_offset)) || new_offset) {
			print_headings = 1;

			/* Skip bytes up to the new offset.  */
			if (fseek(f, (long)new_offset, SEEK_SET) == 0)
				offset = new_offset;
			else
				for (; offset != new_offset; ++offset)
					if (getc_unlocked(f) == EOF)
						goto out;

#define	HEADINGS() { \
	if (print_headings && print_filename) \
		printf("%s:", *argv); \
	if (print_headings && print_offset) \
		printf("%07llu ", offset - (p - buf)); \
}

			/* Scan bytes in this section.  */
			t = threshold;
			for (p = buf; size != 0; --size, ++offset) {
				if ((c = getc(f)) == EOF) {
					if (p >= buf + t) {
						save_errno = errno;
						HEADINGS();
						*p = '\0';
						puts(buf);
						errno = save_errno;
					}
					goto out;
				}

				t = print_headings ? threshold : 0;

				print = isprint(c) || isblank(c);

				/* Flush if end of string or end of buffer.  */
				if (!print && p >= buf + t ||
				    p >= buf + bufsize - 1) {
					HEADINGS();
					*p = '\0';
					p = buf;
					if (!print) {
						puts(buf);
						print_headings = 1;
					} else {
						fputs(buf, stdout);
						print_headings = 0;
					}
				}

				if (print)
					*p++ = c;
				else
					p = buf;
			}

			/* Flush any trailing string.  */
			if (p >= buf + t) {
				HEADINGS();
				*p = '\0';
				puts(buf);
			}

			new_offset = offset;
		}

	out:
		if (ferror(f)) {
			warn("%s", *argv);
			rval = 1;
		}
		fclose(f);
	}

	free(buf);

	return (rval);
}

void
unget(void *v, size_t n, FILE *f)
{
	char *cp = v;

	while (n-- > 0)
		ungetc(cp[n], f);
}

/*
 * The scan routine used for non-executable files, or with -a.
 */
off_t
whole_file(off_t *op)
{

	return (OFF_T_MAX);
}

sec_t
check_whole_file(FILE *f, off_t *offset)
{

	*offset = 0;
	return (whole_file);
}

/*
 * The classification table contains a list of routines that
 * read a little bit of a file and return a section-scanning routine
 * on success, and NULL on failure (rewinding the file).
 * Check_whole_file() always succeeds, and it terminates the list.
 */
sec_t (*classify[])(FILE *, off_t *) = {
#ifdef	OMAGIC
	check_aout,
#endif
	check_elf,
	check_whole_file
};

void
usage()
{

	fprintf(stderr, "usage: strings [-afo] [-n threshold] [files ...]\n");
	exit(1);
}
