/*	BSDI typedef.c,v 1.1.1.1 1998/12/23 19:57:21 prb Exp	*/

/*
 * Process typedefs for the template transformer.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "transform.h"

static struct type *td;

/*
 * Prepare to process a typedef definition.
 * Called after parsing the declaration but before the transforms.
 */
void
start_typedef(struct decl *d)
{
	char *s;
	int flags;

	if ((td = malloc(sizeof (*td))) == NULL)
		fatal(NULL);

	td->name = d->id.name;
	TAILQ_INIT(&td->m_head);	/* no members */

	/* We only allow transformations for integral types.  */
	if (strpbrk(d->declare, "*[]") == NULL &&
	    strncmp(d->id.name, "struct ", sizeof ("struct ") - 1) != 0)
		td->class = CLASS_TYPEDEF;
	else
		td->class = CLASS_OPAQUE;
	flags = extract_memb_flags(d->id.name);
	if (flags & MEMB_FOREIGN)
		td->declare = xsmprintf("%s %%s", d->id.name);
	else
		td->declare = xsmprintf("%s%s %%s", foreign, d->id.name);

	printf("typedef ");
	if (flags & MEMB_FOREIGN)
		printf(d->declare, d->id.name);
	else {
		s = concat(foreign, d->id.name);
		printf(d->declare, s);
		free(s);
	}
	printf(";\n\n");

	add_type(td, &d->id);

	start_transform(td);

	reset_decl();
}

/*
 * Emit any default typedef transformations.
 */
void
end_typedef()
{
	char *s;

	end_transform();

	if (!in_transform && !out_transform) {
		/* Mark this type as untransformable.  */
		td->class = CLASS_OPAQUE;
		return;
	}

	/* Avoid using the native type, in case it doesn't exist.  */
	if (!in_transform) {
		printf("static inline int\n");
		printf("%s_in(", td->name);
		printf(td->declare, "_f");
		printf(")\n{\n\treturn (_f);\n}\n\n");
	}

	if (!out_transform) {
		printf("static inline\n");
		s = xsmprintf("%s_out", td->name);
		printf(td->declare, s);
		free(s);
		printf("(int _n)\n");
		printf("{\n\treturn (_n);\n}\n\n");
	}
}
