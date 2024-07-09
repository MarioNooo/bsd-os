/*	BSDI type.c,v 1.2 2000/12/08 03:29:24 donn Exp	*/

/*
 * Generic type-handling for the template transformer.
 */

#include <sys/types.h>
#include <db.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "transform.h"

static DB *type_db;
static DB *member_db;
static int memb_flags;

static void *lookup_db(DB *, char *);

/*
 * Initialize the type and member databases.
 */
void
init_type()
{

	if ((type_db = dbopen(NULL, O_RDWR, 0, DB_HASH, NULL)) == NULL ||
	    (member_db = dbopen(NULL, O_RDWR, 0, DB_HASH, NULL)) == NULL)
		fatal(NULL);
}

/*
 * Add a type to the type database.
 */
void
add_type(struct type *t, struct ident *id)
{
	DBT key;
	DBT data;

	key.data = t->name;
	key.size = strlen(t->name);
	data.data = &t;
	data.size = sizeof (t);

	switch (type_db->put(type_db, &key, &data, R_NOOVERWRITE)) {
	case 1:
		id_err(id, "redefined");
		break;
	case 0:
		break;
	default:
		fatal(NULL);
	}

	/* Also add the type name to the keyword database.  */
	if (t->class != CLASS_STRUCTPTR)
		add_key(t->name);
}

/*
 * Generic name-to-pointer DB lookup routine.
 */
static void *
lookup_db(DB *db, char *name)
{
	void *v;
	DBT key;
	DBT data;

	key.data = name;
	key.size = strlen(name);

	switch (db->get(db, &key, &data, 0)) {
	case 0:
		memcpy(&v, data.data, data.size);
		break;
	case 1:
		v = NULL;
		break;
	default:
		fatal(NULL);
		break;
	}

	return (v);
}

/*
 * Locate the record for the given transformable type.
 */
struct type *
lookup_type(char *name)
{

	return (lookup_db(type_db, name));
}

/*
 * Given a type declaration string, look up the corresponding type record.
 */
struct type *
extract_type(struct decl *d)
{
	struct type *t;
	char *cp = NULL;
	char *name;
	size_t len = 0;

	/* Look for "NAME %s" or "struct NAME *%s".  */
	if (strncmp(d->declare, "struct ", 7) == 0) {
		if (strcmp(&d->declare[d->len - 4], " *%s") == 0) {
			cp = &d->declare[7];
			len = d->len - (4 + 7);
		}
	} else if (d->len > 3 && strcmp(&d->declare[d->len - 3], " %s") == 0) {
		cp = d->declare;
		len = d->len - 3;
	}
	if (cp != NULL && memchr(cp, ' ', len) != NULL)
		cp = NULL;

	/* If we matched a pattern, is NAME in the database?  */
	if (cp != NULL) {
		if ((name = malloc(len + 1)) == NULL)
			fatal(NULL);
		memcpy(name, cp, len);
		name[len] = '\0';
		t = lookup_type(name);
		free(name);
		return (t);
	}

	return (NULL);
}

/*
 * Look for member-specific flags.
 * Currently this means that we check the name for suspicious prefixes.
 */
int
extract_memb_flags(const char *s)
{

	if (strncmp(s, foreign, strlen(foreign)) == 0 ||
	    strncmp(s, FOREIGN, strlen(FOREIGN)) == 0)
		return (MEMB_FOREIGN);
	if (strncmp(s, native, strlen(native)) == 0 ||
	    strncmp(s, NATIVE, strlen(NATIVE)) == 0)
		return (MEMB_NATIVE);
	return (0);
}

/*
 * Start a cookie or flag type definition, which basically consists
 * of a type for the object and a list of members, with possible transforms.
 */
struct type *
start_member(enum type_class class, struct decl *d)
{
	struct type *t;

	if ((t = malloc(sizeof (*t))) == NULL)
		fatal(NULL);

	t->name = d->id.name;
	TAILQ_INIT(&t->m_head);

	t->class = CLASS_OPAQUE;
	if (strpbrk(d->declare, "*[]") != NULL)
		id_err(&d->id, "type not integral");
	else if (strncmp(d->id.name, "struct ", sizeof ("struct ") - 1) == 0)
		id_err(&d->id, "type not integral");
	else
		t->class = class;
	t->declare = strdup(d->declare);

	memb_flags = extract_memb_flags(d->id.name);

	printf("typedef ");
	printf(d->declare, d->id.name);
	printf(";\n\n");

	add_type(t, &d->id);

	start_transform(t);

	reset_decl();

	return (t);
}

/*
 * Add a member to a cookie or flag definition.
 */
void
add_member(struct type *t, struct ident *id, char *number)
{
	struct member *m;
	DBT key;
	DBT data;

	if ((m = malloc(sizeof (*m))) == NULL)
		fatal(NULL);

	m->name = id->name;
	m->type = t;
	TAILQ_INSERT_TAIL(&t->m_head, m, list);

	m->flags = memb_flags | extract_memb_flags(id->name);
	if (number == NULL) {
		if (m->flags & MEMB_FOREIGN)
			id_err(id, "missing value");
		m->flags |= MEMB_NATIVE;
		number = strdup("0");
	}
	m->value = number;

	if ((m->flags & MEMB_NATIVE) == 0)
		printf("#define %s%s %s\n",
		    m->flags & MEMB_FOREIGN ? "" : FOREIGN, id->name, number);

	key.data = m->name;
	key.size = strlen(m->name);
	data.data = &m;
	data.size = sizeof (m);
	switch (member_db->put(member_db, &key, &data, R_NOOVERWRITE)) {
	case 1:
		id_err(id, "redefined");
		break;
	case 0:
		break;
	default:
		fatal(NULL);
		break;
	}
}

/*
 * Look up the name of a cookie or flag member.
 */
struct member *
lookup_member(char *name)
{

	return (lookup_db(member_db, name));
}
