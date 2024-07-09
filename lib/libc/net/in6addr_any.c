/*-
 * Copyright (c) 1997 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI in6addr_any.c,v 2.1 1997/07/18 12:55:04 dab Exp
 */

/*
 * These two structures are needed as
 * per RFC 2133, sections 3.8 and 3.9.
 */
#include <sys/types.h>
#include <netinet/in.h>

const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;
