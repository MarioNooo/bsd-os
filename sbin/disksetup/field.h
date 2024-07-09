/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI field.h,v 2.3 1998/08/31 20:43:56 prb Exp	*/

class Field {
public:
    int y;
    int x;
    int length;
    char **full_help;
    char **full_qhelp;
    void (*redisplay)(quad_t, quad_t);
    int (*check)(char c);

    int NumericEdit(quad_t &, quad_t = -1, quad_t (*)(quad_t) = force_sector,
	quad_t = 0, quad_t = 0);
    int NumberEdit(quad_t &);
    int TextEdit(char *, int (*validate)(char *) = 0, int space_okay = 0);
    int LetterEdit(char &);
};

#define ESTART  17
