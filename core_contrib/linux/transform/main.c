/*	BSDI main.c,v 1.3 2000/12/08 03:16:53 donn Exp	*/

#include <sys/types.h>
#include <sys/stat.h>
#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "transform.h"

int kflag;
int status;
char *source_dir;

static char *outfile;

static void start_path(void);
static void add_path(char *);
static void end_path(const char *);
static void usage(void);

int
main(int argc, char **argv)
{
	char *infile;
	char *cp;
	size_t len;
	int c;

	start_path();

	while ((c = getopt(argc, argv, "d:i:k")) != EOF)
		switch (c) {
		case 'd':
			db_prefix = optarg;
			break;
		case 'i':
			add_path(optarg);
			break;
		case 'k':
			kflag = 1;
			break;
		default:
			fatal("unrecognized option `%c'", c);
			usage();
			break;
		}

	argc -= optind;
	argv += optind;
	if (argc != 1)
		usage();

	/* Generate an absolute pathname for the input file.  */
	infile = argv[0];

	/* Open the input file.  */
	push_file(infile);

	end_path(infile);

	/*
	 * Create the output file in the current directory
	 * with a '.c' suffix (replacing a '.x' suffix if one is present).
	 */
	if ((cp = strrchr(infile, '/')) != NULL) {
		++cp;
		len = cp - infile;
		if ((source_dir = malloc(len + 1)) == NULL)
			fatal(NULL);
		memcpy(source_dir, infile, len);
		source_dir[len] = '\0';
		infile = cp;
	}
	if ((cp = strrchr(infile, '.')) != NULL &&
	    cp[1] == 'x' && cp[2] == '\0')
		len = cp - infile;
	else
		len = strlen(infile);
	if ((outfile = malloc(len + 3)) == NULL)
		fatal(NULL);
	memcpy(outfile, infile, len);
	strcpy(outfile + len, ".c");

	if (freopen(outfile, "w", stdout) == NULL)
		err(1, "can't open `%s'", outfile);

	/* Emit some standard macros and inlines.  */
	os_prologue();

	/* Parse the input file and generate C code.  */
	if (yyparse() != 0)
		status = 1;

	emit_functions();

	if (ferror(stdout))
		fatal("%s: write error", outfile);

	if (status != 0)
		unlink(outfile);

	return (status);
}

static void
usage()
{

	fprintf(stderr, "usage: transform [-d database-prefix] "
	    "[-i include-dir] [-k] file.x\n");
	exit(1);
}

/*
 * A structure to hold elements of the include path.
 */
struct path {
	TAILQ_ENTRY(path) list;
	char *dir;
};

static TAILQ_HEAD(path_head, path) paths, *pathp;

/*
 * Set up the include directory mechanism, but don't turn it on.
 */
static void
start_path()
{

	TAILQ_INIT(&paths);
	pathp = NULL;
}

/*
 * Add a directory to the include path.
 */
static void
add_path(char *dir)
{
	struct stat sb;
	struct path *p;

	if (stat(dir, &sb) == -1) {
		fprintf(stderr, "%s: can't stat include directory", dir);
		status = 1;
		return;
	}
	if (!S_ISDIR(sb.st_mode)) {
		fprintf(stderr, "%s: not a directory", dir);
		status = 1;
		return;
	}
	if ((p = malloc(sizeof (*p))) == NULL)
		fatal(NULL);
	p->dir = dir;
	TAILQ_INSERT_TAIL(&paths, p, list);
}

/*
 * Finish up include path setup and turn it on.
 */
static void
end_path(const char *file)
{
	struct path *p;
	size_t len;
	char *cp;

	if ((cp = strrchr(file, '/')) != NULL) {
		if ((p = malloc(sizeof (*p))) == NULL)
			fatal(NULL);
		len = cp - file;
		if ((p->dir = malloc(len + 1)) == NULL)
			fatal(NULL);
		memcpy(p->dir, file, len);
		p->dir[len] = '\0';
		TAILQ_INSERT_HEAD(&paths, p, list);
	}

	if ((p = malloc(sizeof (*p))) == NULL)
		fatal(NULL);
	p->dir = ".";
	TAILQ_INSERT_HEAD(&paths, p, list);

	pathp = &paths;
}

/*
 * Open a file using the include path.
 * We malloc() the actual path and return it through *name.
 * We free() the original name.
 */
FILE *
fpathopen(char **name, const char *mode)
{
	struct path *p;
	FILE *f;
	char *s;

	if (**name == '/' || pathp == NULL)
		return (fopen(*name, mode));

	for (p = TAILQ_FIRST(pathp); p != NULL; p = TAILQ_NEXT(p, list)) {
		if (p->dir[0] == '.' && p->dir[1] == '\0')
			s = strdup(*name);
		else
			s = xsmprintf("%s/%s", p->dir, *name);
		if ((f = fopen(s, mode)) != NULL) {
			free(*name);
			*name = s;
			return (f);
		}
		free(s);
		if (errno != ENOENT)
			return (NULL);
	}
	errno = ENOENT;
	return (NULL);
}

/*
 * Implement the alias command.
 */
void
add_alias(struct ident *alias, struct ident *value)
{

	printf("asm(\".weak %s; .set %s,%s\\n\");\n\n", alias->name,
	    alias->name, value->name);
	free(alias->name);
	free(value->name);
}

/*
 * Concatenate two strings into a malloc()ed third string.
 */
char *
concat(const char *p, const char *s)
{
	size_t plen = strlen(p);
	size_t slen = strlen(s);
	char *r;

	if ((r = malloc(plen + slen + 1)) == NULL)
		fatal(NULL);
	memcpy(r, p, plen);
	memcpy(r + plen, s, slen + 1);
	return (r);
}

/*
 * Print a format and arguments into a malloc()ed output string.
 */
char *
xsmprintf(const char *format, ...)
{
	va_list ap;
	size_t len;
	char *s;

	va_start(ap, format);
	len = vsnprintf(NULL, 0, format, ap);
	if ((s = malloc(len + 1)) == NULL)
		fatal(NULL);
	va_end(ap);
	va_start(ap, format);
	vsprintf(s, format, ap);
	va_end(ap);
	return (s);
}

/*
 * Print a message about a terrible event and exit.
 */
__dead void
fatal(const char *format, ...)
{
	va_list ap;
	int save_errno = errno;

	if (outfile != NULL)
		unlink(outfile);

	if (format == NULL) {
		errno = save_errno;
		err(1, NULL);
	}

	va_start(ap, format);
	verrx(1, format, ap);
	va_end(ap);		/* never reached */
}
