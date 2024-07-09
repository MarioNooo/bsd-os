/*	BSDI decl.c,v 1.1.1.1 1998/12/23 19:57:21 prb Exp	*/

/*
 * Handle declarations for the template transformer.
 */

#include <stdlib.h>
#include <string.h>

#include "transform.h"

static struct decl d;

/*
 * Make sure that we have enough space to store a declaration format.
 */
static void
need_declare(size_t len)
{

	if (d.len + len + 1 > d.size) {
		if (d.size == 0 && len + 1 < 32)
			d.size = 32;
		else
			d.size += len + 32;
		if ((d.declare = realloc(d.declare, d.size)) == NULL)
			fatal(NULL);
	}
}

/*
 * Reset the parse state for declarations.
 */
void
reset_decl()
{

	d.id.name = NULL;
	d.id.file = NULL;
	d.id.line = 0;
	d.len = 0;
	d.qualifiers = 0;
}

/*
 * Append text to a pending declaration.
 */
void
add_decl_text(const char *s)
{
	size_t len = strlen(s);
	int c;

	/*
	 * Add a space between tokens, except around punctuation.
	 * Exception: always put a space after text and before '*'.
	 */
	if (d.len > 0 && *s != '[' && *s != ']' &&
	    (c = d.declare[d.len - 1]) != '[' && c != ']' && c != '*') {
		need_declare(1);
		d.declare[d.len] = ' ';
		++d.len;
	}

	need_declare(len);
	memcpy(d.declare + d.len, s, len + 1);
	d.len += len;
}

/*
 * Add a type qualifier.  We're only interested in the outermost one;
 * otherwise, we treat it like any other type token.
 */
void
add_type_qual(const char *s)
{

	if (d.len > 0) {
		add_decl_text(s);
		return;
	}

	switch (*s) {
	/*
	 * For functions, don't check for errors and transform errno.
	 * This is useful if the error return value can't be guessed
	 * using the usual rules, or if the function calls a Linux
	 * function that sets the Linux errno value.
	 */
	case 'n':
		d.qualifiers |= QUAL_NOERRNO;
		break;
	/*
	 * If a transformable struct pointer parameter is marked 'volatile',
	 * we treat the structure as read/write rather than write-only.
	 */
	case 'v':
		d.qualifiers |= QUAL_VOLATILE;
		break;
	/*
	 * If a transformable struct pointer parameter is marked 'const',
	 * we treat the structure as read-only.
	 */
	case 'c':
		d.qualifiers |= QUAL_CONST;
		break;
	default:
		fatal("unrecognized type qualifier `%s'", s);
		break;
	}
}

/*
 * Add a structure declaration (not a definition).
 */
void
add_struct_decl(struct ident *id)
{

	add_decl_text("struct");
	add_decl_text(id->name);
	free(id->name);
}

/*
 * Add an array bound.
 */
void
add_array_decl(char *s)
{

	add_decl_text("[");
	add_decl_text(s);
	add_decl_text("]");
	free(s);
}

/*
 * Record the name of the object or type that we're declaring.
 * Insert a printf format code so that we can use the declaration string
 * to emit declarations of objects with this type.
 */
void
add_decl(struct ident *id)
{

	d.id = *id;
	add_decl_text("%s");
}

/*
 * Provide the current declaration to code that needs it.
 * Note that the contents are overwritten by a subsequent start_decl().
 */
struct decl *
end_decl()
{

	return (&d);
}
