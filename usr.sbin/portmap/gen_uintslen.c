/*	BSDI gen_uintslen.c,v 2.1 1998/02/05 22:46:48 don Exp */

#include <stdio.h>
#include <limits.h>

/*
 * just how many decimal digits do we really need for the largest u_int 
 * this platform can generate ?
 */
int
main(argc, argv)
	int argc;
	char *argv[];
{
        u_int u;
        int digits;

	/* poor man's log10() */
        for (digits = 0, u = UINT_MAX; u > 0; digits++)
                u /= 10;

        puts("/* this file is generated automatically, do not edit */");
        printf("#define UINT_MAX_DEC_SLEN	%u\n", digits);
	exit (0);
}
