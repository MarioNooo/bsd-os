/*	BSDI transform.c,v 1.2 2000/02/12 19:10:02 donn Exp	*/

/*
 * Default transformation functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "transform.h"

int in_transform;
int out_transform;

static struct type *tf_type;
static struct ident in_ident;
static char *in_from;
static char *in_to;
static char *in_len;
static char *in_body;
static struct ident out_ident;
static char *out_from;
static char *out_to;
static char *out_len;
static char *out_body;

static void emit_in_transform(struct ident *, char *, char *, char *, char *);
static void emit_out_transform(struct ident *, char *, char *, char *, char *);

/*
 * Start looking for default transformation functions.
 */
void
start_transform(struct type *t)
{

	tf_type = t;
	in_transform = 0;
	out_transform = 0;
}

/*
 * Queue up a default transformation function.
 * We can't emit these yet because we may be emitting code for members.
 */
void
add_transform(struct ident *dir, struct ident *p1, struct ident *p2,
    struct ident *p3, char *body)
{

	if (tf_type->class == CLASS_OPAQUE) {
		id_err(dir, "transformation function for untransformable type");
		return;
	}

	if (strcmp(dir->name, "in") == 0) {
		if (in_transform != 0) {
			id_err(dir, "redefined, previous definition at line %d",
			    in_transform);
			return;
		}
		in_transform = dir->line;
		in_ident = *dir;
		in_from = p1->name;
		in_to = p2 == NULL ? NULL : p2->name;
		in_len = p3 == NULL ? NULL : p3->name;
		in_body = body;
	} else if (strcmp(dir->name, "out") == 0) {
		if (out_transform != 0) {
			id_err(dir, "redefined, previous definition at line %d",
			    out_transform);
			return;
		}
		out_transform = dir->line;
		out_ident = *dir;
		out_from = p1->name;
		out_to = p2 == NULL ? NULL : p2->name;
		out_len = p3 == NULL ? NULL : p3->name;
		out_body = body;
	} else
		fatal("internal error in add_transform");
}

static void
emit_in_transform(struct ident *id, char *from, char *to, char *len, char *body)
{

	printf("static inline %s\n",
	    tf_type->class == CLASS_STRUCTPTR ? "void" : "unsigned int");
	printf("%s%s_in(", tf_type->name,
	    tf_type->class == CLASS_TYPEDEF ? "" : "_default");
	switch (tf_type->class) {
	case CLASS_TYPEDEF:
	case CLASS_COOKIE:
		if (to != NULL)
			id_err(id, "extra parameter `%s'", to);
		if (len != NULL)
			id_err(id, "extra parameter `%s'", len);
		printf(tf_type->declare, from);
		break;
	case CLASS_FLAG:
		if (to == NULL) {
			id_err(id, "missing second parameter");
			break;
		}
		if (len != NULL)
			id_err(id, "extra parameter `%s'", len);
		printf(tf_type->declare, from);
		printf(", unsigned int %s", to);
		break;
	case CLASS_STRUCTPTR:
		if (to == NULL) {
			id_err(id, "missing second parameter");
			break;
		}
		printf("const ");
		printf(tf_type->declare, from);
		printf(", struct %s *%s", tf_type->name, to);
		if (len != NULL)
			printf(", size_t %s", len);
		else
			printf(", size_t _l");
		break;
	case CLASS_OPAQUE:
		break;
	}
	printf(")\n{\n%s\n}\n\n", body);

	free(id->name);
	free(from);
	if (to != NULL)
		free(to);
	if (len != NULL)
		free(len);
	free(body);
}

static void
emit_out_transform(struct ident *id, char *from, char *to, char *len,
    char *body)
{
	char *s;

	printf("static inline\n");
	s = xsmprintf("%s%s_out", tf_type->name,
	    tf_type->class == CLASS_TYPEDEF ? "" : "_default");
	if (tf_type->class == CLASS_STRUCTPTR)
		printf("void %s", s);
	else
		printf(tf_type->declare, s);
	free(s);
	printf("(");
	switch (tf_type->class) {
	case CLASS_TYPEDEF:
	case CLASS_COOKIE:
		if (to != NULL)
			id_err(id, "extra parameter `%s'", to);
		if (len != NULL)
			id_err(id, "extra parameter `%s'", len);
		printf("unsigned int %s", from);
		break;
	case CLASS_FLAG:
		if (to == NULL) {
			id_err(id, "missing second parameter");
			break;
		}
		if (len != NULL)
			id_err(id, "extra parameter `%s'", len);
		printf("unsigned int %s, ", from);
		printf(tf_type->declare, to);
		break;
	case CLASS_STRUCTPTR:
		if (to == NULL) {
			id_err(id, "missing second parameter");
			break;
		}
		printf("const struct %s *%s, ", tf_type->name, from);
		printf(tf_type->declare, to);
		if (len != NULL)
			printf(", size_t %s", len);
		else
			printf(", size_t _l");
		break;
	case CLASS_OPAQUE:
		break;
	}
	printf(")\n{\n%s\n}\n\n", body);

	free(id->name);
	free(from);
	if (to != NULL)
		free(to);
	if (len != NULL)
		free(len);
	free(body);
}

/*
 * Generate the code for the default transformation functions.
 * Our caller is responsible for checking in_transform and out_transform
 * to find out whether this type has default transformations.
 */
void
end_transform()
{

	if (in_transform)
		emit_in_transform(&in_ident, in_from, in_to, in_len, in_body);
	if (out_transform)
		emit_out_transform(&out_ident, out_from, out_to, out_len,
		    out_body);
}

/*
 * Generate code for a transformation.
 */
void
transform(struct type *t, char *dir, char *from, char *to, char *member)
{
	char *ptr;

	if (member != NULL)
		ptr = "->";
	else {
		ptr = "";
		member = "";
	}

	printf("\t");
	switch (t->class) {
	case CLASS_TYPEDEF:
	case CLASS_COOKIE:
	case CLASS_FLAG:
		printf("%s%s%s = %s_%s(%s%s%s)", to, ptr, member, t->name, dir,
		    from, ptr, member);
		break;
	case CLASS_STRUCTPTR:
		printf("%s_%s(%s%s%s, %s%s%s, sizeof (*%s%s%s))", t->name, dir,
		    from, ptr, member, to, ptr, member, to, ptr, member);
		break;
	case CLASS_OPAQUE:
		break;
	}
	printf(";\n");
}
