/**************************************************************************
 * LPRng IFHP Filter
 * Copyright 1994-1999 Patrick Powell, San Diego, CA <papowell@astart.com>
 **************************************************************************/
/**** HEADER *****
$Id: linelist.h,v 1.9 2000/03/17 22:23:53 papowell Exp papowell $
 **** HEADER *****/

#if !defined(_LINELIST_H_)
#define _LINELIST_H_ 1

/*
 * arrays of pointers to lines
 */
struct line_list {
	char **list;	/* array of pointers to lines */
	int count;		/* number of entries */
	int max;		/* maximum number of entries */
};

/*
 * Types of options that we can initialize or set values of
 */
enum key_type { FLAG_K, INTEGER_K, STRING_K, LIST_K };

/*
 * datastructure for initialization
 */

struct keywords{
    char *keyword;		/* name of keyword */
    enum key_type type;	/* type of entry */
    void *variable;		/* address of variable */
	int  maxval;		/* value of token */
	int  flag;			/* flag for variable */
	char *default_value;		/* default value */
};

/* PROTOTYPES */
void lowercase( char *s );
void uppercase( char *s );
char *trunc_str( char *s);
void *malloc_or_die( size_t size, const char *file, int line );
void *realloc_or_die( void *p, size_t size, const char *file, int line );
char *safestrdup (const char *p, const char *file, int line);
char *safestrdup2( const char *s1, const char *s2, const char *file, int line );
char *safestrdup3( const char *s1, const char *s2, const char *s3,
	const char *file, int line );
char *safestrdup4( const char *s1, const char *s2,
	const char *s3, const char *s4,
	const char *file, int line );
void Init_line_list( struct line_list *l );
void Free_line_list( struct line_list *l );
void Check_max( struct line_list *l, int incr );
void Add_line_list( struct line_list *l, char *str,
		char *sep, int sort, int uniq );
void Merge_list( struct line_list *dest, struct line_list *src,
	char *sep, int sort, int uniq );
void Split( struct line_list *l, char *str, char *sep,
	int sort, char *keysep, int uniq, int trim, int nocomments );
void Split_count( int count, struct line_list *l, char *str, char *sep,
	int sort, char *keysep, int uniq, int trim, int nocomments );
void Split_cmd_line( struct line_list *l, char *line, int count );
void Dump_line_list( char *title, struct line_list *l );
int Find_last_key( struct line_list *l, char *key, char *sep, int *m );
int Find_first_key( struct line_list *l, char *key, char *sep, int *m );
char *Find_value( struct line_list *l, char *key, char *sep );
char *Find_exists_value( struct line_list *l, char *key, char *sep );
char *Find_str_value( struct line_list *l, char *key, char *sep );
int Find_flag_value( struct line_list *l, char *key, char *sep );
char *Fix_val( char *s );
int Read_file_list( struct line_list *model, char *str,
	char *sep, int sort, char *keysep, int uniq, int trim,
	int marker, int doinclude, int nocomment );
void Read_fd_and_split( struct line_list *list, int fd,
	char *sep, int sort, char *keysep, int uniq, int trim, int nocomment );
int Read_file_and_split( struct line_list *list, char *file,
	char *sep, int sort, char *keysep, int uniq, int trim, int nocomment );
char *Select_model_info( struct line_list *model, struct line_list *list,
	char *id, int init );
void Remove_line_list( struct line_list *l, int n );
void Set_str_value( struct line_list *l, char *key, const char *value );
char *Join_vector( char **list, int count, char *sep );
char *Join_line_list( struct line_list *l, char *sep );
char *Join_line_list_with_sep( struct line_list *l, char *sep );
char *Get_file_image( char *file );

#endif
