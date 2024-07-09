/*	BSDI flag.c,v 1.1.1.1 1998/12/23 19:57:21 prb Exp	*/

/*
 * Process flags for the template transformer.
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "transform.h"

static void common_bits(void);

static struct type *fl;

/*
 * Prepare to process a flag definition.
 * Called after parsing the declaration but before the transforms.
 * XXX This could be common code with start_cookie() if we wanted...
 */
void
start_flag(struct decl *d)
{

	fl = start_member(CLASS_FLAG, d);
}

/*
 * Add a flag transformation.
 */
void
add_flag(struct ident *id, char *number)
{

	add_member(fl, id, number);
}

/*
 * Write out an expression for the bits that are common
 * between native and foreign flags.
 * It's just the complement of the set of specified flags
 * (both native and foreign versions).
 */
static void
common_bits()
{
	const char *sep = "";
	struct member *m;

	printf("~(\n");
	for (m = TAILQ_FIRST(&fl->m_head); m != NULL; m = TAILQ_NEXT(m, list)) {
		printf("\t\t");
		if ((m->flags & MEMB_FOREIGN) == 0) {
			printf("%s%s", sep, m->name);
			sep = "|";
		}
		if ((m->flags & MEMB_NATIVE) == 0) {
			printf("%s%s%s", sep,
			    m->flags & MEMB_FOREIGN ? "" : FOREIGN, m->name);
			sep = "|";
		}
		printf("\n");
	}
	printf("\t)");
}

/*
 * Generate the generic flag transform functions.
 * Called after we've seen all of the flags in this flag type.
 */
void
end_flag()
{
	struct member *m;

	end_transform();

	/*
	 * Emit the input flag transformation.
	 */
	printf("\nstatic inline unsigned int\n");
	printf("%s_in(%s _f)\n{\n", fl->name, fl->name);
	printf("\tunsigned int _n = _f & ");
	common_bits();
	printf(";\n\n");
	for (m = TAILQ_FIRST(&fl->m_head); m != NULL; m = TAILQ_NEXT(m, list))
		if ((m->flags & (MEMB_FOREIGN|MEMB_NATIVE)) == 0) {
			printf("\tif (_f & %s%s)\n", FOREIGN, m->name);
			printf("\t\t_n |= %s;\n", m->name);
		}
	if (in_transform)
		printf("\t_n = %s_default_in(_f, _n);\n", fl->name);
	printf("\treturn (_n);\n}\n\n");

	/*
	 * Emit the output flag transformation.
	 */
	printf("static inline %s\n", fl->name);
	printf("%s_out(unsigned int _n)\n{\n", fl->name);
	printf("\t%s _f = _n &", fl->name);
	common_bits();
	printf(";\n\n");
	for (m = TAILQ_FIRST(&fl->m_head); m != NULL; m = TAILQ_NEXT(m, list))
		if ((m->flags & (MEMB_FOREIGN|MEMB_NATIVE)) == 0) {
			printf("\tif (_n & %s)\n", m->name);
			printf("\t\t_f |= %s%s;\n", FOREIGN, m->name);
		}
	if (out_transform)
		printf("\t_f = %s_default_out(_n, _f);\n", fl->name);
	printf("\treturn (_f);\n}\n\n");
}
