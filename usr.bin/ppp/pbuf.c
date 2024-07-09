/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI pbuf.c,v 2.2 1997/11/06 20:43:38 chrisk Exp
 */
#include <sys/types.h>
#include <stdlib.h>
#include "pbuf.h"

pbuf_t *
pbuf_alloc(size_t size)
{
	pbuf_t *p;

	/*
	 * always make sure the size is a multiple of the alignment size.
	 * This ensures that an empty pbuf always has its data pointer aligned.
	 */
	size += sizeof(long);
	size &= ~sizeof(long);

	if ((p = malloc(sizeof(pbuf_t) + 2 * (size - PBUF_DEFSIZE))) != NULL) {
		p->data = p->buffer + size;
		p->len = 0;
		p->end = p->data + size;
		p->dynamic = 1;
	}
	return(p);
}
