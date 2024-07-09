/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI pbuf.h,v 2.1 1996/03/27 05:10:58 prb Exp
 */
#define	PBUF_DEFSIZE	1504		/* Big enough for any LCP packet */

typedef struct pbuf_t {
	int	dynamic:1;
	int	len:15;
	u_char	*data;
	u_char	*end;
	u_char	buffer[PBUF_DEFSIZE * 2];
} pbuf_t;

#define	pbuf_end(p)	((p)->data + (p)->len)
#define	pbuf_left(p)	((p)->end - pbuf_end(p))
#define	pbuf_prepend(p, s) \
	((p)->data - (s) >= (p)->buffer ? (p)->len += (s),(p)->data -= (s) : (u_char *)0)

#define	pbuf_append(p, s) \
	((p)->data + (s) <= (p)->end ? (p)->data + ((p)->len += (s)) - (s) \
				     : (u_char *)0)
#define	pbuf_clear(p) { \
	((p)->len = 0; \
	(p)->data = (p)->buffer + ((p)->end - (p)->buffer) / 2; \
}

#define	pbuf_trim(p, s) { \
	if ((s)  >= 0) { \
		(p)->data += (s); \
		(p)->len -= (s); \
	} else \
		(p)->len += (s); \
	if ((p)->len <= 0) { \
		(p)->data = (p)->buffer + ((p)->end - (p)->buffer) / 2; \
		(p)->len = 0; \
	} \
}

#define	pbuf_free(p)	{ if (p && p->dynamic) free(p); }

#define	pbuf_init(p) { \
	(p)->dynamic = 0; \
	(p)->len = 0; \
	(p)->data = (p)->buffer + PBUF_DEFSIZE; \
	(p)->end = (p)->buffer + 2 * PBUF_DEFSIZE; \
}

pbuf_t *pbuf_alloc(size_t);
