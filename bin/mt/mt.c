/*	BSDI	mt.c,v 2.7 1997/08/26 17:03:12 cp Exp	*/
/*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1980, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)mt.c	8.2 (Berkeley) 5/4/95";
#endif /* not lint */

/*
 * mt --
 *   magnetic tape manipulation program
 */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MT_BLKSIZE	-1

struct commands {
	char *c_name;			/* Name. */
	int c_code;			/* Ioctl code. */
	int c_ronly;			/* 1 if open tape drive O_RDONLY */
	int c_arg;			/* 1 if trailing argument. */
} const com[] = {
	{ "bsf",	MTBSF,	    1, 1 },
	{ "bsr",	MTBSR,	    1, 1 },
	{ "buffered",	MTCACHE,    1, 0 },
	{ "eof",	MTWEOF,	    0, 1 },
	{ "fsf",	MTFSF,	    1, 1 },
	{ "fsr",	MTFSR,	    1, 1 },
	{ "offline",	MTOFFL,	    1, 0 },
	{ "rewind",	MTREW,	    1, 0 },
	{ "rewoffl",	MTOFFL,	    1, 0 },
	{ "unload",	MTOFFL,	    1, 0 },
	{ "status",	MTNOP,	    1, 0 },
	{ "unbuffered",	MTNOCACHE,  1, 0 },
	{ "weof",	MTWEOF,	    0, 1 },
	{ "blksize",	MT_BLKSIZE, 1, 1 },	/* not regular MTIOCTOP */
	{ "blocksize",	MT_BLKSIZE, 1, 1 },	/* not regular MTIOCTOP */
	{ "load",	MTLOAD,	    1, 0 },
	{ "retension",	MTRETENSION,1, 0 },
	{ NULL }
};

struct pstatus {
	char 	*what;
} pstatus [] = {
	{ " Rewinding:\t\t"		},
	{ " At beginning of tape:\t"	},
	{ " At end of data:\t"		},
	{ " At end of medium:\t"	},
	{ " Rewind required:\t"		},
	{ " Drive failed:\t\t"		},
	{ " Read only:\t\t"		},
	{ " No media\t\t"		}
};

void printreg __P((char *, u_int, char *));
void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct commands const *comp;
	struct mtget mt_status;
	struct mtop mt_com;
	size_t len;
	long value;
	int ch, mtfd;
	char *p, *tape;

	if ((tape = getenv("TAPE")) == NULL)
		tape = DEFTAPE;

	while ((ch = getopt(argc, argv, "f:t:")) != EOF)
		switch(ch) {
		case 'f':
		case 't':
			tape = optarg;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc < 1 || argc > 2)
		usage();

	len = strlen(p = *argv++);
	for (comp = com;; comp++) {
		if (comp->c_name == NULL)
			errx(1, "%s: unknown command", p);
		if (strncmp(p, comp->c_name, len) == 0)
			break;
	}
	if ((mtfd = open(tape, comp->c_ronly ? O_RDONLY : O_RDWR)) < 0)
		err(1, "%s", tape);

	/* Get any associated argument. */
	if (comp->c_arg && *argv != NULL) {
		errno = 0;
		value = strtol(*argv, &p, 10);
		if (errno)
			err(1, "%s", *argv);
		if (value <= 0 || *argv == '\0' || *p != '\0')
			errx(1, "%s: illegal numeric argument", *argv);
	} else
		value = 0;

	switch (comp->c_code) {
	case MT_BLKSIZE:
		if (value) {
			if (ioctl(mtfd, MTIOCSFBS, &value) < 0)
				err(1, "%s: %s", tape, comp->c_name);
		} else {
			if (ioctl(mtfd, MTIOCGFBS, &len) < 0)
				err(1, "%s: %s", tape, comp->c_name);
			(void)printf("%s fixed block length = %d\n", tape, len);
		}
		break;
	case MTNOP:
	    {
		struct mt_name mn;
		struct mt_status ms;
		struct mt_position mp;
		int i;

		if (ioctl(mtfd, MTIOGNAME, &mn) < 0)
			err(1, "%s: MTIOGNAME", tape);
		printf("%s %s %s\n", mn.mn_vendor, mn.mn_product,
		    mn.mn_revision);
		if (ioctl(mtfd, MTIOGSTATUS, &ms) < 0)
			err(1, "%s: MTIOGSTATUS", tape);

		for (i = 0; i < 8; i++) {
			char *msg;
			
			if ((ms.ms_applicable & (1 << i)) == 0)
				continue;

			printf("%s\t", pstatus[i].what);
			if ((ms.ms_valid & (1 << i)) == 0) {
				printf("unknown\n");
				continue;
			}

			msg = (ms.ms_status & (1 << i)) ? "yes" : "no";
			printf("%s\n", msg);
		}
		if (ioctl(mtfd, MTIOCGPOSITION, &mp) < 0)
			err(1, "%s: MTIOCGPOSITION", tape);
		/*
		 * Move following 3 if blocks to a funcition when
		 * code is added to get position from drive
		 */
		if (mp.mp_flags & MT_PARTITION_APPLICABLE) {
			printf(" Partition number:\t\t");
			if (mp.mp_flags & MT_PARTITION_VALID)
				printf("%d\n", mp.mp_partition);
			else
				printf("unknown\n");
		}
		if (mp.mp_flags & MT_FILE_APPLICABLE) {
			printf(" File Number:\t\t\t");
			if (mp.mp_flags & MT_FILE_VALID)
				printf("%d\n", mp.mp_file);
			else
				printf("unknown\n");
		}
		if (mp.mp_flags & MT_BLOCK_APPLICABLE) {
			printf(" Block Number:\t\t\t");
			if (mp.mp_flags & MT_BLOCK_VALID)
				printf("%d\n", mp.mp_block);
			else
				printf("unknown\n");
		}

		break;
	    }
	default:
		mt_com.mt_op = comp->c_code;
		mt_com.mt_count = value ? value : 1;
		if (ioctl(mtfd, MTIOCTOP, &mt_com) < 0)
			err(1, "%s: %s", tape, comp->c_name);
	}
	exit (0);
}

void
usage()
{
	(void)fprintf(stderr,
	    "usage: mt [-f device] command [count | length]\n");
	exit(1);
}
