/*
 * Copyright (c) 1997 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	misc.c,v 1.1 1997/10/20 20:57:40 ewv Exp
 */
#include "cpu.h"

void
phex(void *data, int len)
{
	u_char *p = data;
	char *sep = "";

	while (len--) {
		printf("%s%2.2x", sep, *p++);
		sep = " ";
	}
}

/*
 * Dump hex data with offsets
 */
void
hexdump(void *b, int len)
{
	u_char *buf = b;
	int offset;

	offset = 0;
	while (len--) {
		if (offset && (offset & 0x0f) == 0)
			printf("\n%4.4x:", offset);
		printf(" %2.2x", buf[offset]);
		offset++;
	}
	printf("\n");
}
