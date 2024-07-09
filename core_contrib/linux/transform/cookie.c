/*	BSDI cookie.c,v 1.1.1.1 1998/12/23 19:57:21 prb Exp	*/

/*
 * Process cookies for the template transformer.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "transform.h"

static struct type *ck;

/*
 * Prepare to process a cookie definition.
 * Called after parsing the declaration but before the transforms.
 */
void
start_cookie(struct decl *d)
{

	ck = start_member(CLASS_COOKIE, d);
}

/*
 * Add a cookie transformation.
 */
void
add_cookie(struct ident *id, char *number)
{

	add_member(ck, id, number);
}

/*
 * Generate the generic cookie transform functions.
 * Called after we've seen all of the cookies in this cookie type.
 */
void
end_cookie()
{
	char *name = ck->name;
	struct member *m;

	end_transform();

	/* Emit the input cookie transformation.  */
	printf("\nstatic inline unsigned int\n");
	printf("%s_in(%s _f)\n{\n\tswitch (_f) {\n", name, name);
	for (m = TAILQ_FIRST(&ck->m_head); m != NULL; m = TAILQ_NEXT(m, list)) {
		if (m->flags & (MEMB_FOREIGN|MEMB_NATIVE))
			continue;
		printf("\tcase %s%s:\n\t\treturn (%s);\n", FOREIGN, m->name,
		    m->name);
	}
	if (in_transform)
		printf("\tdefault:\n\t\treturn (%s_default_in(_f));\n", name);
	else
		printf("\tdefault:\n\t\tbreak;\n");
	printf("\t}\n\treturn (_f);\n}\n\n");

	/* Emit the output cookie transformation.  */
	printf("static inline %s\n", name);
	printf("%s_out(unsigned int _n)\n{\n\tswitch (_n) {\n", name);
	for (m = TAILQ_FIRST(&ck->m_head); m != NULL; m = TAILQ_NEXT(m, list)) {
		if (m->flags & (MEMB_FOREIGN|MEMB_NATIVE))
			continue;
		printf("\tcase %s:\n\t\treturn (%s%s);\n", m->name,
		    FOREIGN, m->name);
	}
	if (out_transform)
		printf("\tdefault:\n\t\treturn (%s_default_out(_n));\n", name);
	else
		printf("\tdefault:\n\t\tbreak;\n");
	printf("\t}\n\treturn (_n);\n}\n\n");
}
