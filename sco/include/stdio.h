/*	BSDI stdio.h,v 1.1 1995/07/10 18:22:12 donn Exp	*/

#ifndef	_SCO_STDIO_H_
#define	_SCO_STDIO_H_

/* from iBCS2 p6-54 - 6-55 */

#define	NULL		0
#define	EOF		(-1)

#define	BUFSIZ		1024
#define	_NFILE		60
#define	FOPEN_MAX	_NFILE

typedef long fpos_t;

#include <machine/ansi.h>

#ifdef _SCO_SIZE_T_
typedef _SCO_SIZE_T_ size_t;
#undef _SCO_SIZE_T_
#endif

typedef struct {
	int		_cnt;
	unsigned char	*_ptr;
	unsigned char	*_base;
	char		_flag;
	char		_file;
} FILE;

extern FILE _iob[_NFILE];
#define	stdin		(&_iob[0])
#define	stdout		(&_iob[1])
#define	stderr		(&_iob[2])

#define	_IOFBF		0x00
#define	_IONBF		0x04
#define	_IOEOF		0x10
#define	_IOERR		0x20
#define	_IOLBF		0x40

#define	clearerr(f)	((f)->_flag &= ~(_IOEOF|_IOERR))
#define	feof(f)		((f)->_flag & _IOEOF)
#define	ferror(f)	((f)->_flag & _IOERR)
#define	fileno(f)	(f)->_file

#define	L_ctermid	9
#define	L_cuserid	9
#define	P_tmpdir	"/usr/tmp/"
#define	L_tmpnam	(sizeof (P_tmpdir) + 15)

/* prototypes required by POSIX.1 */

#include <sys/cdefs.h>

int fclose __P((FILE *));
FILE *fdopen __P((int, const char *));
int fflush __P((FILE *));
int fgetc __P((FILE *));
int fgetpos __P((FILE *, fpos_t *));
char *fgets __P((char *, size_t, FILE *));
FILE *fopen __P((const char *, const char *));
int fprintf __P((FILE *, const char *, ...));
int fputc __P((int, FILE *));
int fputs __P((const char *, FILE *));
size_t fread __P((void *, size_t, size_t, FILE *));
FILE *freopen __P((const char *, const char *, FILE *));
int fscanf __P((FILE *, const char *, ...));
int fseek __P((FILE *, long, int));
int fsetpos __P((FILE *, const fpos_t *));
long ftell __P((const FILE *));
size_t fwrite __P((const void *, size_t, size_t, FILE *));
int getc __P((FILE *));
int getchar __P((void));
char *gets __P((char *));
void perror __P((const char *));
int printf __P((const char *, ...));
int putc __P((int, FILE *));
int putchar __P((int));
int puts __P((const char *));
int remove __P((const char *));
int rename  __P((const char *, const char *));
void rewind __P((FILE *));
int scanf __P((const char *, ...));
void setbuf __P((FILE *, char *));
int setvbuf __P((FILE *, char *, int, size_t));
int sprintf __P((char *, const char *, ...));
int sscanf __P((const char *, const char *, ...));
FILE *tmpfile __P((void));
char *tmpnam __P((char *));
int ungetc __P((int, FILE *));
int vfprintf __P((FILE *, const char *, char *));
int vprintf __P((const char *, char *));
int vsprintf __P((char *, const char *, char *));

#endif	/* _SCO_STDIO_H_ */
