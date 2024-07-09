/*	BSDI func.c,v 1.10 2001/01/23 03:45:00 donn Exp	*/

/*
 * Handle function transformations for the template transformer.
 */

#include <sys/types.h>
#include <db.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "transform.h"
#include "../liblinux.common/trace.h"

static void set_body_type(enum body_type, const char *);
static void add_entry(struct decl *);
static void emit_struct_glue(struct parameter *, struct parameter *,
    struct overload *);
static void emit_overload_call(const char *, struct func *,
    struct overload *, int);
static void check_for_fast_syscall_func(struct func *);
static void check_for_fast_syscall(struct entry *);
static void check_for_switchable_overloads_func(struct func *);
static void check_for_switchable_overloads(struct entry *);
static size_t atof_prefix(const DBT *, const DBT *);
static void open_func_db(void);

static TAILQ_HEAD(entry_head, entry) entries;
static struct entry *en;
static struct func *fn;
static struct overload *ol;

char *db_prefix;

static char *ftoa_db_name;
static DB *ftoa_db;
static char *atof_db_name;
static DB *atof_db;
static BTREEINFO bt = { R_DUP, 0, 0, 0, 0, NULL, atof_prefix, 0 };

static void
set_body_type(enum body_type b, const char *s)
{

	if (fn->body_type != BODY_UNSET) {
		fn->body_type = BODY_FULL;
		if (fn->body_value != NULL)
			free(fn->body_value);
	} else {
		fn->body_type = b;
		if (s != NULL)
			fn->body_value = strdup(s);
	}
}

/*
 * Add an entry for a new library function / syscall implementation,
 * or locate an existing one.
 */
static void
add_entry(struct decl *d)
{
	struct entry *e;
	struct func **fp = NULL;
	char *name = d->id.name;
	static int first = 1;
	static int library_len;
	static int kernel_len;
	enum { DEFAULT, LIBRARY, KERNEL } ftype;

	if (first) {
		TAILQ_INIT(&entries);
		library_len = strlen(library);
		kernel_len = strlen(kernel);
		first = 0;
	}

	if (strncmp(name, library, library_len) == 0) {
		ftype = LIBRARY;
		name += library_len;
	} else if (strncmp(name, kernel, kernel_len) == 0) {
		ftype = KERNEL;
		name += kernel_len;
	} else
		ftype = DEFAULT;

	for (e = TAILQ_FIRST(&entries); e != NULL; e = TAILQ_NEXT(e, list))
		if (strcmp(e->name, name) == 0)
			break;

	if (e == NULL) {
		if ((e = malloc(sizeof (*e))) == NULL)
			fatal(NULL);
		e->name = strdup(name);
		e->sequence = 0;
		e->generic = NULL;
		e->library = NULL;
		e->kernel = NULL;
		TAILQ_INSERT_TAIL(&entries, e, list);
	}

	en = e;

	switch (ftype) {
	case DEFAULT: fp = &e->generic; break;
	case LIBRARY: fp = &e->library; break;
	case KERNEL:  fp = &e->kernel;  break;
	}

	if (*fp == NULL) {
		if ((*fp = malloc(sizeof (**fp))) == NULL)
			fatal(NULL);
		TAILQ_INIT(&(*fp)->o_head);
		(*fp)->body_type = BODY_UNSET;
		(*fp)->body_value = NULL;
		(*fp)->cookie_pos = -1;
		(*fp)->alternative_type = ALTERNATIVE_IFELSE;
	}

	fn = *fp;
}

/*
 * Emit code for an overloaded function.
 */
void
start_overload(struct decl *d)
{
	char *s;

	add_entry(d);

	if ((ol = malloc(sizeof (*ol))) == NULL)
		fatal(NULL);

	ol->sequence = ++en->sequence;
	ol->type = extract_type(d);
	ol->declare = strdup(d->declare);
	ol->id = d->id;
	ol->flags = d->qualifiers & QUAL_NOERRNO ? OVER_NOERRNO : 0;
	TAILQ_INIT(&ol->p_head);

	if (strchr(ol->declare, '*') != NULL && (ol->flags & OVER_NOERRNO) == 0)
		id_warn(&d->id, "pointer return value without noerrno qualifier");

	printf("static inline\n");
	s = xsmprintf("%s_%d_n", en->name, ol->sequence);
	printf(ol->declare, s);
	printf("(");

	reset_decl();
}

/*
 * Add a normal function parameter to the current function's parameter list.
 */
void
add_parameter_decl(struct decl *d)
{
	struct parameter *p;

	if ((p = malloc(sizeof (*p))) == NULL)
		fatal(NULL);

	p->name = d->id.name;
	p->type = extract_type(d);
	if (strpbrk(d->declare, "*[]") != NULL) {
		if (d->qualifiers & QUAL_CONST)
			p->class = PARM_RDONLY;
		else if (d->qualifiers & QUAL_VOLATILE)
			p->class = PARM_RDWR;
		else
			p->class = PARM_WRONLY;
	} else
		p->class = PARM_OTHER;
	p->declare = strdup(d->declare);
	p->member = NULL;
	TAILQ_INSERT_TAIL(&ol->p_head, p, list);

	if (p != TAILQ_FIRST(&ol->p_head))
		printf(", ");
	if (d->qualifiers & QUAL_CONST)
		printf("const ");
	printf(d->declare, d->id.name);

	reset_decl();
}

/*
 * Add a cookie or flag member to the current function's parameter list.
 * The presence of such a member implies that this overloaded function
 * entry will only be called if the given parameter matches the member.
 */
void
add_parameter_member(struct ident *id)
{
	struct parameter *p;

	if ((p = malloc(sizeof (*p))) == NULL)
		fatal(NULL);

	p->name = id->name;
	p->type = NULL;
	p->class = PARM_OTHER;
	p->declare = "unsigned int %s";
	if ((p->member = lookup_member(id->name)) == NULL)
		id_err(id, "unrecognized symbol");
	TAILQ_INSERT_TAIL(&ol->p_head, p, list);

	ol->flags |= OVER_SPECIFIC;

	if (p != TAILQ_FIRST(&ol->p_head))
		printf(", ");
	printf("unsigned int _%s", id->name);
}

/*
 * Wrap up a parameter list.
 */
void
end_parameters()
{

	printf(")\n{\n");
}

/*
 * Emit code for a defaulted function body.
 */
void
add_function_default(struct ident *id)
{
	struct parameter *p;
	char *name;
	char *cast;

	cast = xsmprintf(ol->declare, "");

	if (id != NULL && id->name[0] == 'E') {
		printf("{\n\treturn ((%s)%serror_retval(%s));\n}\n\n",
		    cast, native, id->name);
		set_body_type(BODY_ERROR, id->name);
		free(cast);
		return;
	}

	name = id == NULL ? en->name : id->name;

	printf("{\n\treturn ((%s)%ssyscall(SYS_%s", cast, native, name);
	for (p = TAILQ_FIRST(&ol->p_head); p != NULL; p = TAILQ_NEXT(p, list))
		printf(", %s", p->name);
	printf("));\n}\n\n");

	set_body_type(BODY_DEFAULT, name);

	free(cast);
}

/*
 * Emit code for a defaulted function body that returns a constant.
 */
void
add_function_value(char *s)
{

	printf("{\n\treturn (%s);\n}\n\n", s);
	set_body_type(BODY_VALUE, s);
}

/*
 * Emit code for a user-provided function body.
 */
void
add_function_body(char *body)
{

	printf("{\n%s}\n\n", body);
	set_body_type(BODY_FULL, NULL);
}

/*
 * Make sure that the generic overload is at the end of the overload list.
 */
void
end_overload()
{
	struct overload *last = TAILQ_LAST(&fn->o_head, overload_head);

	printf("}\n");

	if (last == NULL || last->flags & OVER_SPECIFIC)
		TAILQ_INSERT_TAIL(&fn->o_head, ol, list);
	else if ((ol->flags & OVER_SPECIFIC) == 0) {
		id_err(&ol->id, "duplicate generic function");
		id_err(&last->id, "previous generic function definition");
	} else
		TAILQ_INSERT_BEFORE(last, ol, list);
}

/*
 * Initialize the local variables associated with a transformable structure.
 */
static void
emit_struct_glue(struct parameter *p, struct parameter *gp, struct overload *o)
{
	struct parameter *l;
	char *s = concat("length_", p->name);

	/* Look for the length parameter.  */
	for (l = TAILQ_FIRST(&o->p_head); l != NULL; l = TAILQ_NEXT(l, list))
		if (strcmp(l->name, s) == 0)
			break;
	free(s);

	if (l == NULL) {
		printf("\t\t_%s_l_f = sizeof (%.*s);\n", p->name,
		    strrchr(p->type->declare, ' ') - p->type->declare,
		    p->type->declare);
		printf("\t\t_%s_l_n = sizeof (struct %s);\n",
		    p->name, p->type->name);
	} else {
		/*
		 * We assume that native and foreign sizes of a
		 * variable length structure must be the same.
		 * Assuming otherwise would require us to know how
		 * to transform the length parameter, which could
		 * be tough.
		 */
		printf("\t\t_%s_l_f = (void *)%s == NULL ? NULL : ",
		    p->name, gp->name);
		if (l->declare != NULL && (s = strchr(l->declare, '*')) != NULL)
			printf(s, l->name);
		else
			printf("%s", l->name);
		printf(";\n");
		printf("\t\t_%s_l_n = _%s_l_f;\n", p->name, p->name);
	}
	printf("\t\t%s_n = (void *)%s == NULL ? NULL : alloca(_%s_l_n);\n",
	    p->name, gp->name, p->name);
}

/*
 * Emit a call to an overloaded function from the wrapper function.
 * We emit any required tests for members too.
 */
static void
emit_overload_call(const char *name, struct func *f, struct overload *o,
    int iskernel)
{
	struct overload *go = TAILQ_LAST(&f->o_head, overload_head);
	struct parameter *p;
	struct parameter *gp;
	struct member *m;
	char *s;
	int first = 1;
	int pos;
	int void_function = strcmp(o->declare, "void %s") == 0;

	/* Emit any required tests.  */
	for (pos = 0, p = TAILQ_FIRST(&o->p_head),
	    gp = TAILQ_FIRST(&go->p_head);
	    p != NULL;
	    ++pos, p = TAILQ_NEXT(p, list), gp = TAILQ_NEXT(gp, list)) {
		if (gp == NULL) {
			id_err(&o->id, "too many parameters");
			break;
		}
		if (f->alternative_type == ALTERNATIVE_SWITCH) {
			if (pos < f->cookie_pos)
				continue;
			if ((m = p->member) == NULL)
				printf("\tdefault:\n");
			else
				printf("\tcase %s%s:\n",
				    m->flags & MEMB_FOREIGN ? "" : FOREIGN,
				    p->name);
			break;
		}
		if ((m = p->member) == NULL)
			continue;
		if (first) {
			printf("\tif (");
			first = 0;
		} else
			printf(" && ");
		if (m->type->class == CLASS_COOKIE)
			printf("%s == %s%s", gp->name,
			    m->flags & MEMB_FOREIGN ? "" : FOREIGN, p->name);
		else if (m->type->class == CLASS_FLAG)
			printf("%s & %s%s", gp->name,
			    m->flags & MEMB_FOREIGN ? "" : FOREIGN, p->name);
		else
			fatal("bad member type");
	}
	if (first == 0)
		printf(") {\n");
	else
		printf("\t{\n");

	/* Emit local variable declarations.  */
	if (!void_function) {
		printf("\t\t");
		printf(o->declare, "_r_n");
		printf(";\n");
	}
	for (p = TAILQ_FIRST(&o->p_head); p != NULL; p = TAILQ_NEXT(p, list)) {
		printf("\t\t");
		s = concat(p->name, "_n");
		if (p->type == NULL || p->type->class == CLASS_OPAQUE)
			printf(p->declare, s);
		else if (p->member != NULL)
			printf("unsigned int %s", s);
		else if (p->type->class == CLASS_STRUCTPTR) {
			printf("size_t _%s_l_f;\n", p->name);
			printf("\t\tsize_t _%s_l_n;\n", p->name);
			printf("\t\t");
			printf(p->declare, s);
		} else
			printf(p->declare, s);
		free(s);
		printf(";\n");
	}
	printf("\n");

	/* Emit input transformations.  */
	for (p = TAILQ_FIRST(&o->p_head), gp = TAILQ_FIRST(&go->p_head);
	    p != NULL && gp != NULL;
	    p = TAILQ_NEXT(p, list), gp = TAILQ_NEXT(gp, list)) {
		if (p->type == NULL || p->type->class == CLASS_OPAQUE) {
			printf("\t\t%s_n = (", p->name);
			printf(p->declare, "");
			printf(")%s;\n", gp->name);
		} else if (p->member != NULL) {
			if (p->member->flags & MEMB_FOREIGN)
				printf("\t\t%s_n = %s;\n", p->name, gp->name);
			else
				printf("\t\t%s_n = %s;\n", p->name, p->name);
		} else if (p->type->class == CLASS_STRUCTPTR) {
			emit_struct_glue(p, gp, o);
			if (p->class == PARM_WRONLY)
				continue;
			printf("\t\tif (%s_n != NULL)\n", p->name);
			printf("\t\t\t%s_in((", p->type->name);
			printf(p->type->declare, "");
			printf(")%s, %s_n, _%s_l_n);\n", gp->name, p->name,
			    p->name);
		} else {
			printf("\t\t%s_n = %s_in((", p->name, p->type->name);
			printf(p->type->declare, "");
			printf(")%s);\n", gp->name);
		}
	}

	/* Call the native function.  */
	printf("\t\t%s%s_%d_n(", void_function ? "" : "_r_n = ",
	    name, o->sequence);
	for (p = TAILQ_FIRST(&o->p_head); p != NULL; p = TAILQ_NEXT(p, list)) {
		if (p != TAILQ_FIRST(&o->p_head))
			printf(", ");
		printf("%s_n", p->name);
	}
	printf(");\n");

	/* Check for errors.  */
	s = xsmprintf(go->declare, "");
	if (!void_function) {
		printf("\t\tif (%serror(_r_n)) {\n", native);
		if ((o->flags & OVER_NOERRNO) == 0) {
			if (iskernel)
				printf("\t\t\treturn ((%s)"
				    "%serror_retval("
				    "__errno_out("
				    "%serror_val(_r_n))));\n",
				    s, native, native);
			else {
				printf("\t\t\terrno = __errno_out("
				    "%serror_val(_r_n));\n", native);
				printf("\t\t\treturn ((%s)-1);\n", s);
			}
		} else
			printf("\t\t\treturn ((%s)_r_n);\n", s);
		printf("\t\t}\n");
	}

	/* Emit output transformations.  */
	for (p = TAILQ_FIRST(&o->p_head), gp = TAILQ_FIRST(&go->p_head);
	    p != NULL && gp != NULL;
	    p = TAILQ_NEXT(p, list), gp = TAILQ_NEXT(gp, list)) {
		if (p->type == NULL || p->type->class != CLASS_STRUCTPTR ||
		    !(p->class == PARM_RDWR || p->class == PARM_WRONLY))
			continue;
		printf("\t\tif (%s_n != NULL)\n", p->name);
		printf("\t\t\t%s_out(%s_n, (", p->type->name, p->name);
		printf(p->type->declare, "");
		printf(")%s, _%s_l_f);\n", gp->name, p->name);
	}

	/* Emit the return value transformation (if any).  */
	if (void_function)
		printf("\t\treturn;\n");
	else {
		if (o->type != NULL && o->type->class != CLASS_OPAQUE) {
			if (o->type->class == CLASS_STRUCTPTR)
				fatal("returning a transformable struct pointer?!?");
			printf("\t\t_r_n = %s_out(_r_n);\n", o->type->name);
		}
		printf("\t\treturn ((%s)_r_n);\n", s);
	}

	printf("\t}\n\n");
	free(s);
}

/*
 * Set the body type to BODY_SYSCALL if the only transformations are
 * for the syscall number and mechanism, and the errno value.
 * We can implement such function bodies with fast assembly code.
 */
static void
check_for_fast_syscall_func(struct func *f)
{
	struct overload *o;
	struct parameter *p;

	if (f->body_type != BODY_DEFAULT)
		return;

	f->body_type = BODY_FULL;
	o = TAILQ_FIRST(&f->o_head);
	if (o->flags & OVER_SPECIFIC)
		return;
	if (o->type != NULL && o->type->class != CLASS_OPAQUE)
		return;
	for (p = TAILQ_FIRST(&o->p_head); p != NULL; p = TAILQ_NEXT(p, list))
		if (p->type != NULL && p->type->class != CLASS_OPAQUE)
			return;
	f->body_type = BODY_SYSCALL;
}

static void
check_for_fast_syscall(struct entry *e)
{

	if (e->generic)
		check_for_fast_syscall_func(e->generic);
	if (e->library)
		check_for_fast_syscall_func(e->library);
	if (e->kernel)
		check_for_fast_syscall_func(e->kernel);
}

/*
 * If we can handle the tests with a switch, then use one.
 * A heap test or a computed goto is much faster than linear if-else tests.
 */
static void
check_for_switchable_overloads_func(struct func *f)
{
	struct overload *o;
	struct parameter *p;
	int cookie_pos = -1;
	int pos;

	f->alternative_type = ALTERNATIVE_IFELSE;

	if (f->body_type != BODY_FULL)
		return;

	for (o = TAILQ_FIRST(&f->o_head); o != NULL; o = TAILQ_NEXT(o, list)) {
		if (o->flags & OVER_SPECIFIC)
			for (pos = 0, p = TAILQ_FIRST(&o->p_head);
			    p != NULL;
			    ++pos, p = TAILQ_NEXT(p, list)) {
				if (p->member == NULL)
					continue;
				if (p->member->type->class != CLASS_COOKIE)
					return;
				if (cookie_pos >= 0 && pos != cookie_pos)
					return;
				cookie_pos = pos;
			}

		if (cookie_pos == -1)
			return;
	}

	f->alternative_type = ALTERNATIVE_SWITCH;
	f->cookie_pos = cookie_pos;
}

static void
check_for_switchable_overloads(struct entry *e)
{

	if (e->generic)
		check_for_switchable_overloads_func(e->generic);
	if (e->library)
		check_for_switchable_overloads_func(e->library);
	if (e->kernel)
		check_for_switchable_overloads_func(e->kernel);
}

void
emit_full_function(const char *name, struct func *f, int iskernel)
{
	struct overload *go = TAILQ_LAST(&f->o_head, overload_head);
	struct overload *o;
	struct parameter *p;
	int pos;
	char *s;

	/* Make sure that the last overload is the generic case.  */
	if (go->flags & OVER_SPECIFIC) {
		id_err(&go->id, "no generic function has been defined");
		return;
	}

	/* Emit the function head.  */
	printf("static ");
	if (iskernel)
		s = xsmprintf("%s%s%s", native, kernel, name);
	else
		s = concat(foreign, name);
	printf(go->declare, s);
	free(s);

	printf("(");
	for (p = TAILQ_FIRST(&go->p_head); p != NULL; p = TAILQ_NEXT(p, list)) {
		if (p != TAILQ_FIRST(&go->p_head))
			printf(", ");
		if (p->class == PARM_RDONLY)
			printf("const ");
		if (p->type != NULL)
			printf(p->type->declare, p->name);
		else
			printf(p->declare, p->name);
	}
	printf(")\n{\n");

	if (f->alternative_type == ALTERNATIVE_SWITCH) {
		for (pos = 0, p = TAILQ_FIRST(&go->p_head);
		    p != NULL && pos < f->cookie_pos; 
		    ++pos, p = TAILQ_NEXT(p, list))
			;
		printf("\n\tswitch ((unsigned int)%s) {\n", p->name);
	}

	/* Emit the tests and calls to the overload functions.  */
	for (o = TAILQ_FIRST(&f->o_head); o != NULL; o = TAILQ_NEXT(o, list))
		emit_overload_call(name, f, o, iskernel);

	if (f->alternative_type == ALTERNATIVE_SWITCH)
		printf("\t}\n");

	printf("}\n\n");
}

void
emit_functions()
{
	struct entry *e;

	for (e = TAILQ_FIRST(&entries); e != NULL; e = TAILQ_NEXT(e, list)) {
		check_for_fast_syscall(e);
		check_for_switchable_overloads(e);
		os_emit_library_function(e);
		if (kflag)
			os_emit_kernel_function(e);
	}
}

static size_t
atof_prefix(const DBT *key1 UNUSED, const DBT *key2)
{

	return (key2->size);
}

static void
open_func_db()
{

	if (db_prefix == NULL)
		fatal("function name/address database required");

	ftoa_db_name = concat(db_prefix, ".ftoa.db");
	atof_db_name = concat(db_prefix, ".atof.db");

	if ((ftoa_db = dbopen(ftoa_db_name, O_RDONLY|O_SHLOCK, 0, DB_HASH,
	    NULL)) == NULL)
		fatal("%s: %s", ftoa_db_name, strerror(errno));
	if ((atof_db = dbopen(atof_db_name, O_RDONLY|O_SHLOCK, 0, DB_BTREE,
	    &bt)) == NULL)
		fatal("%s: %s", atof_db_name, strerror(errno));
}

void
emit_aliases(const char *name, struct overload *o)
{
	DBT key;
	DBT addr;
	DBT alias;
	char *s;
	char *t;
	int r;

	if (ftoa_db == NULL)
		open_func_db();

	key.data = (char *)name;
	key.size = strlen(name);

	/* Do we have a mapping for this name? */
	if ((r = ftoa_db->get(ftoa_db, &key, &addr, 0)) != 0) {
		if (r == -1)
			id_err(&o->id, "can't locate aliases: %s",
			    strerror(errno));
		return;
	}

	/*
	 * We use the btree method's R_DUP feature to find
	 * all of the aliases under the function's address.
	 * We step through the aliases and emit code for them.
	 */
	for (key = addr, r = atof_db->seq(atof_db, &key, &alias, R_CURSOR);
	    r == 0 && key.size == addr.size &&
	    memcmp(key.data, addr.data, key.size) == 0;
	    r = atof_db->seq(atof_db, &key, &alias, R_NEXT)) {
		s = alias.data;
		if ((t = malloc(alias.size)) == NULL)
			fatal(NULL);
		memcpy(t, s + 1, alias.size - 1);
		t[alias.size - 1] = '\0';
		printf(o->declare, t);
		free(t);
		printf("() __attribute__((");
		if (*s != 'G')
			printf("__weak__, ");
		printf("__alias__(\"%s%s\")));\n", foreign, name);
	}
}
