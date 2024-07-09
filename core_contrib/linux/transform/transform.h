/*	BSDI transform.h,v 1.4 2001/01/23 03:40:35 donn Exp	*/

/*
 * Common declarations for the syscall transformation template translator.
 */

#include <sys/types.h>
#include <sys/queue.h>
#include <stdio.h>

/*
 * Structure definitions.
 */

/*
 * Identifiers, that is, words found by the scanner that were not keywords.
 */
struct ident {
	char *name;		/* the string that we scanned */
	char *file;		/* the file in which we found it */
	int line;		/* the line number where we found it */
};

/*
 * The classes of the transformable types.
 */
enum type_class {
	CLASS_OPAQUE,
	CLASS_COOKIE,
	CLASS_FLAG,
	CLASS_TYPEDEF,
	CLASS_STRUCTPTR,
};

/*
 * A 'struct type' describes a type.
 * Transformable types may use all of the fields;
 * non-transformable types set 'class' to CLASS_OTHER and provide 'declare'.
 */
struct type {
	char *name;		/* type name */
	char *declare;		/* printf-style format for foreign objects */
	enum type_class class;
	TAILQ_HEAD(member_head, member) m_head;
};

/*
 * A 'struct member' describes a member of a transformable type
 * (cookie, flag or struct).
 */
struct member {
	TAILQ_ENTRY(member) list;
	char *name;
	char *value;
	int flags;
	struct type *type;
};

#define	MEMB_FOREIGN	0x01
#define	MEMB_NATIVE	0x02
#define	MEMB_ARRAY	0x04

/*
 * This structure keeps track of library and syscall functions
 * with (potentially) common implementations.
 */
struct entry {
	TAILQ_ENTRY(entry) list;
	char *name;
	int sequence;
	struct func *generic;
	struct func *library;
	struct func *kernel;
};

/*
 * The various supported function body types.
 * Certain very simple classes of function transformations can be
 * supported using specific, hand-optimized code.
 */
enum body_type {
	BODY_UNSET = 0,
	BODY_VALUE,
	BODY_ERROR,
	BODY_DEFAULT,
	BODY_SYSCALL,
	BODY_FULL
};

/*
 * Alternative types.  Certain function overload alternatives can be
 * handled using a C switch rather than a linear if-else chain.
 */
enum alternative_type {
	ALTERNATIVE_IFELSE,
	ALTERNATIVE_SWITCH
};

/*
 * A 'struct func' records information about functions.
 * Principally, it keeps track of 'overloads' (function transformations),
 * but it also maintains state for various optimizations.
 */
struct func {
	TAILQ_HEAD(overload_head, overload) o_head;
	enum body_type body_type;
	char *body_value;
	int cookie_pos;
	enum alternative_type alternative_type;
};

/*
 * A 'struct overload' describes one particular function transformation.
 * There may be several such transformations for a given function.
 * There may be zero or more 'specific overloads' that have some
 * constant parameter that controls transformations of
 * other parameters or the return value, and there is always
 * exactly one 'generic overload' that handles the remaining cases;
 * the generic overload is always last in the function's overload list.
 */
struct overload {
	TAILQ_ENTRY(overload) list;
	struct ident id;
	struct type *type;	/* type of return value */
	char *declare;		/* return type if not transformable */
	int sequence;
	int flags;
	TAILQ_HEAD(parameter_head, parameter) p_head;
};

#define	OVER_SPECIFIC	0x01
#define	OVER_NOERRNO	0x02

/*
 * The 'parameter_class' marks a transformable pointer parameter
 * with information about when it must be transformed.
 */
enum parameter_class {
	PARM_OTHER,
	PARM_RDONLY,
	PARM_RDWR,
	PARM_WRONLY
};

/*
 * A 'struct parameter' records the type of a function parameter.
 */
struct parameter {
	TAILQ_ENTRY(parameter) list;
	char *name;
	struct type *type;
	enum parameter_class class;
	char *declare;		/* type if not transformable */
	struct member *member;	/* controlling member, if any */
};

/*
 * A 'struct decl' is used by the declaration processor
 * to represent a declaration.
 */
struct decl {
	struct ident id;
	char *declare;		/* printf-style format to declare an object */
	size_t len;		/* length of declaration */
	size_t size;		/* size of declaration memory */
	int qualifiers;
};

#define	QUAL_CONST	0x01	/* input transformations only */
#define	QUAL_VOLATILE	0x02	/* both input and output transformations */
#define	QUAL_NOERRNO	0x04	/* on functions, don't transform errno */

/*
 * External declarations.
 */

/* config.c: */
extern const char foreign[];
extern const char FOREIGN[];
extern const char native[];
extern const char NATIVE[];
extern const char kernel[];
extern const char library[];

/* main.c: */
extern int kflag;
extern int status;
extern char *source_dir;
FILE *fpathopen(char **, const char *);
void add_alias(struct ident *, struct ident *);
char *concat(const char *, const char *);
char *xsmprintf(const char *, ...);
__dead void fatal(const char *, ...);

/* parse.y: */
int yyerror(const char *);
int yyparse(void);

/* scan.l: */
void add_key(char *);
void push_file(char *);
void id_err(struct ident *, const char *, ...);
void id_warn(struct ident *, const char *, ...);
int yylex(void);

/* decl.c: */
void reset_decl(void);
void add_decl_text(const char *);
void add_type_qual(const char *);
void add_struct_decl(struct ident *);
void add_array_decl(char *);
void add_decl(struct ident *);
struct decl *end_decl(void);

/* type.c: */
void init_type(void);
void add_type(struct type *, struct ident *);
struct type *lookup_type(char *);
struct type *extract_type(struct decl *);
int extract_memb_flags(const char *);
struct type *start_member(enum type_class, struct decl *);
void add_member(struct type *, struct ident *, char *);
struct member *lookup_member(char *);

/* transform.c: */
extern int in_transform;
extern int out_transform;
void start_transform(struct type *);
void add_transform(struct ident *, struct ident *, struct ident *,
    struct ident *, char *);
void end_transform(void);
void transform(struct type *, char *, char *, char *, char *);

/* typedef.c: */
void start_typedef(struct decl *);
void end_typedef(void);

/* cookie.c: */
void start_cookie(struct decl *);
void add_cookie(struct ident *, char *);
void end_cookie(void);

/* flag.c: */
void start_flag(struct decl *);
void add_flag(struct ident *, char *);
void end_flag(void);

/* struct.c: */
void start_struct(struct ident *);
void add_struct_member(struct decl *);
void end_struct(void);

/* func.c: */
extern char *db_prefix;
void start_overload(struct decl *);
void add_parameter_decl(struct decl *);
void add_parameter_member(struct ident *);
void end_parameters(void);
void add_function_default(struct ident *);
void add_function_value(char *);
void add_function_body(char *);
void end_overload(void);
void emit_full_function(const char *, struct func *, int);
void emit_functions(void);
void emit_aliases(const char *, struct overload *);

/* OS-specific functions: */
void os_prologue(void);
void os_emit_kernel_function(struct entry *);
void os_emit_library_function(struct entry *);

/*
 * Conveniences.
 */
#define	UNUSED	__attribute__((__unused__))
