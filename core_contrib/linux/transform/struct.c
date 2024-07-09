/*	BSDI struct.c,v 1.1.1.1 1998/12/23 19:57:21 prb Exp	*/

/*
 * Structure transformations for the template transformer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "transform.h"

static struct type *st;

static int is_foreign_member(struct decl *);
static int struct_memb_flags;

/*
 * Prepare to process a structure transformation.
 */
void
start_struct(struct ident *id)
{

	if ((st = malloc(sizeof (*st))) == NULL)
		fatal(NULL);

	st->name = id->name;
	st->class = CLASS_STRUCTPTR;
	TAILQ_INIT(&st->m_head);

	struct_memb_flags = extract_memb_flags(id->name);
	if (struct_memb_flags & MEMB_FOREIGN)
		st->declare = xsmprintf("struct %s *%%s", id->name);
	else
		st->declare = xsmprintf("struct %s%s *%%s", foreign, id->name);

	add_type(st, id);

	start_transform(st);

	if (struct_memb_flags & MEMB_FOREIGN)
		printf("struct %s {\n", id->name);
	else
		printf("struct %s%s {\n", foreign, id->name);
}

/*
 * If a structure member is 'foreign', we don't transform it automatically.
 */
static int
is_foreign_member(struct decl *d)
{

	if (struct_memb_flags & MEMB_FOREIGN)
		return (1);

	if (extract_memb_flags(d->id.name) & MEMB_FOREIGN)
		return (1);

	return (0);
}

/*
 * Declare a struct member and queue it for later transformation.
 */
void
add_struct_member(struct decl *d)
{
	struct member *m = NULL;
	struct type *t = extract_type(d);

	if (!is_foreign_member(d)) {
		if ((m = malloc(sizeof (*m))) == NULL)
			fatal(NULL);

		m->name = d->id.name;
		m->value = NULL;
		m->flags = strchr(d->declare, '[') != NULL ? MEMB_ARRAY : 0;
		m->type = t;
		TAILQ_INSERT_TAIL(&st->m_head, m, list);
	}

	printf("\t");
	if (d->qualifiers & QUAL_CONST)
		printf("const ");
	else if (d->qualifiers & QUAL_VOLATILE)
		printf("volatile ");
	if (t != NULL)
		printf(t->declare, d->id.name);
	else
		printf(d->declare, d->id.name);
	printf(";\n");

	reset_decl();
}

/*
 * Emit the structure transformation functions.
 */
void
end_struct()
{
	struct member *m;

	printf("};\n\n");

	end_transform();

	/*
	 * Emit the in-transform function.
	 */
	printf("static inline void\n");
	printf("%s_in(const ", st->name);
	printf(st->declare, "_f");
	printf(", struct %s *_n, size_t _l)\n{\n", st->name);
	for (m = TAILQ_FIRST(&st->m_head); m != NULL; m = TAILQ_NEXT(m, list)) {
		if (m->type && m->type->class != CLASS_OPAQUE)
			transform(m->type, "in", "_f", "_n", m->name);
		else if (m->flags & MEMB_ARRAY) {
			/* Assumes native and foreign arrays are same size... */
			printf("\tmemcpy(_n->%s, _f->%s, ", m->name, m->name);
			if (m == TAILQ_LAST(&st->m_head, member_head))
				printf("_l - ((char *)_n->%s - (char *)_n));\n",
				    m->name);
			else
				printf("sizeof (_n->%s));\n", m->name);
		} else
			printf("\t_n->%s = _f->%s;\n", m->name, m->name);
	}
	if (in_transform)
		printf("\t%s_default_in(_f, _n, _l);\n", st->name);
	printf("}\n\n");

	/*
	 * Emit the out-transform function.
	 */
	printf("static inline void\n");
	printf("%s_out(const struct %s *_n, ", st->name, st->name);
	printf(st->declare, "_f");
	printf(", size_t _l)\n{\n");
	while ((m = TAILQ_FIRST(&st->m_head)) != NULL) {
		if (m->type && m->type->class != CLASS_OPAQUE)
			transform(m->type, "out", "_n", "_f", m->name);
		else if (m->flags & MEMB_ARRAY) {
			/* Assumes native and foreign arrays are same size... */
			printf("\tmemcpy(_f->%s, _n->%s, ", m->name, m->name);
			if (m == TAILQ_LAST(&st->m_head, member_head))
				printf("_l - ((char *)_f->%s - (char *)_f));\n",
				    m->name);
			else
				printf("sizeof (_f->%s));\n", m->name);
		} else
			printf("\t_f->%s = _n->%s;\n", m->name, m->name);
		TAILQ_REMOVE(&st->m_head, m, list);
		free(m->name);
		free(m);
	}
	if (out_transform)
		printf("\t%s_default_out(_n, _f, _l);\n", st->name);
	printf("}\n\n");
}
