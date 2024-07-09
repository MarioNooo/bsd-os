/* libmain - flex run-time support library "main" function */

/* /master/usr.bin/lex/libmain.c,v 1.3 1997/09/25 00:10:19 jch Exp */

extern int yylex();

int main( argc, argv )
int argc;
char *argv[];
	{
	while ( yylex() != 0 )
		;

	return 0;
	}
