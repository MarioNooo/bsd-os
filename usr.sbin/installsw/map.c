/*-
 * Copyright (c) 1993, 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI map.c,v 2.3 1999/09/24 19:07:45 polk Exp
 */

#include <sys/param.h>
#include <sys/queue.h>

#include <err.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "installsw.h"

#define N_DOT	0x01
#define N_VER	0x02
#define N_UPPER	0x04
#define N_RR	0x08

static FILE *mapf;
static int name_mapping_bits;

int
determine_name_mapping()
{
	static int flags[] = {			/* In preference order. */
		N_RR,
		N_DOT,
		N_VER,
		N_DOT | N_VER,
		N_UPPER,
		N_UPPER | N_DOT,
		N_UPPER | N_VER,
		N_UPPER | N_DOT | N_VER,
	};
	int i;
	char name[128];

	(void)printf("\tTrying to copy the ISO-9660 file map...\n");

	if (mapf != NULL)
		(void)fclose(mapf);

	(void)unlink(TMPMAP);
	for (i = 0; i < sizeof(flags) / sizeof(flags[0]); ++i) {
		name_mapping_bits = flags[i];
		(void)snprintf(name, sizeof(name), "%s/TRANS.TBL;1", 
			PACKAGEDIR);
		if (get_remote_file(name, TMPMAP) == 0 &&
		    (mapf = fopen(TMPMAP, "r")) != NULL) {
#ifdef	USE_YOUNG_MINDS_FORMAT
			goto found;
		}
		(void)snprintf(name, sizeof(name), "%s/FILEMAP.;1", PACKAGEDIR);
		if (get_remote_file(name, TMPMAP) == 0 &&
		    (mapf = fopen(TMPMAP, "r")) != NULL) {
found:		
#endif
			(void)printf("\tsucceeded.\n");
			return (0);
		}
	}
	name_mapping_bits = N_RR;
	(void)printf("\tfailed!\nAssuming no file map exists.  Continuing.\n");
	return (0);
}

char *
map_name(name)
	char *name;
{
	size_t len;
	int n;
	char *p, flags[1024], iname[1024], pname[1024];

	if (media != M_CDROM ||
	    location != L_REMOTE || (name_mapping_bits & N_RR) != 0) {
		if ((p = strchr(name, ';')) != NULL)
			*p = 0;
		len = strlen(name);
		if (len > 0 && name[len - 1] == '.')
			name[len - 1] = 0;
		return (name);
	}

	if (mapf != NULL) {
		/* Find the file name. */
		if ((p = strrchr(name, '/')) == NULL)
			p = name;
		else
			++p;

		/* Search for a file name mapping. */
		rewind(mapf);
		while ((n =
		    fscanf(mapf, "%s %s %s", flags, iname, pname)) != EOF)
			if (n == 3 && !strcmp(p, pname))
				break;

		/* If no mapping found, keep going, it might still be there. */
		if (n != EOF)
			(void)strcpy(p, iname);
	}
		
	if (!(name_mapping_bits & N_UPPER))
		for (p = name; *p != '\0'; ++p)
			if (isupper(*p))
				*p = tolower(*p);

	if (!(name_mapping_bits & N_VER))
		if ((p = strchr(name, ';')) != NULL)
			*p = '\0';

	if (!(name_mapping_bits & N_DOT)) {
		len = strlen(name);
		if (len > 0 && name[len - 1] == '.')
			name[len - 1] = '\0';
	}
	return (name);
}
