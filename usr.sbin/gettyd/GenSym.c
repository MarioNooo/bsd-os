/*-
 * Copyright (c) 1996 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI GenSym.c,v 1.2 1999/01/04 22:45:00 chrisk Exp
 */
#include <string.h>
#include <stdlib.h>
#include "GenSym.h"

GenSym_t *
GenSym(GenSym_t **root, char *n, int nlen, void *arg, int alen, int flags)
{
	register GenSym_t *c, *nc;
	int cmp = 0;

	if (nlen == 0)
		nlen = strlen(n);

	if (*root == NULL) {
		if ((flags & GS_INSERT) == 0)
			return(NULL);
		if ((c = MalGenSym()) != NULL) {
			if ((c->gs_name = malloc(nlen + 1)) == NULL) {
				free(c);
				return(NULL);
			}
			memcpy(c->gs_name, n, nlen);
			c->gs_name[nlen] = 0;
			c->gs_namelen = nlen;
			c->gs_next = c->gs_prev = NULL;
			c->gs_flag = flags;
			goto setvalue;
		}
		return(*root);
	}

	c = *root;

	while (c) { 
		if ((cmp = strncmp(n, c->gs_name, nlen)) <= 0 || c->gs_next == NULL)
			break;
		c = c->gs_next;
	}
	if (cmp == 0 && c->gs_namelen != nlen)
		cmp = -1;

	if (cmp == 0) {
		if ((flags & GS_REPLACE) != GS_REPLACE)
			return(c);

		if (c->gs_value && (c->gs_flag & GS_DYNAMIC))
			free(c->gs_value);

		goto setvalue;
	}
	if ((flags & GS_INSERT) != GS_INSERT)
		return(NULL);

	if ((nc = MalGenSym()) == NULL)
		return(NULL);

	if((nc->gs_name = malloc(nlen + 1)) == NULL) {
		free(nc);
		return(NULL);
	}
	memcpy(nc->gs_name, n, nlen);
	nc->gs_name[nlen] = 0;
	nc->gs_namelen = nlen;

	if (cmp < 0) {
		if (c->gs_prev)
			c->gs_prev->gs_next = nc;
		nc->gs_prev = c->gs_prev;
		nc->gs_next = c;
		c->gs_prev = nc;
	} else {
		c->gs_next = nc;
		nc->gs_prev = c;
		nc->gs_next = NULL;
	}
	if (!nc->gs_prev)
		*root = nc;
	c = nc;

setvalue:
	if (alen && arg && (flags & GS_DYNAMIC)) {
		if ((c->gs_value = malloc(alen + 1)) != NULL) {
			memcpy(c->gs_value, arg, alen);
			((char *)(c->gs_value))[alen] = 0;
		} else {
			free(c->gs_name);
			free(c);
			return(NULL);
		}
	} else
		c->gs_value = arg;
	c->gs_size = alen;
	if (*root == NULL)
		*root = c;
	return(c);
}

void
GenSymDelete(GenSym_t **root, char *name)
{
	register GenSym_t *c = *root;

	if (root == NULL)
		return;

	while (c) {
		if (!strcmp(c->gs_name, name)) {
			if (!c->gs_prev && !c->gs_next)
				(*root) = NULL;
			else if (!c->gs_prev) {
				(*root) = c->gs_next;
				(*root)->gs_prev = NULL;
			} else if (!c->gs_next) {
				c->gs_prev->gs_next = NULL;
			} else {
				c->gs_prev->gs_next = c->gs_next;
				c->gs_next->gs_prev = c->gs_prev;
			}
			if (c->gs_value && (c->gs_flag & GS_DYNAMIC))
				free(c->gs_value);
			free(c->gs_name);
			free(c);
			return;
		}
		c = c->gs_next;
	}
	return;
}
