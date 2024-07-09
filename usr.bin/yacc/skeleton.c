/*	BSDI skeleton.c,v 2.7 2001/05/08 21:55:24 polk Exp	*/

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Paul Corbett.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char sccsid[] = "@(#)skeleton.c	5.8 (Berkeley) 4/29/95";
#endif /* not lint */

/*	BSDI skeleton.c,v 2.7 2001/05/08 21:55:24 polk Exp	*/

#include "defs.h"

/*  The definition of yysccsid in the banner should be replaced with	*/
/*  a #pragma ident directive if the target C compiler supports		*/
/*  #pragma ident directives.						*/
/*									*/
/*  If the skeleton is changed, the banner should be changed so that	*/
/*  the altered version can be easily distinguished from the original.	*/
/*									*/
/*  The #defines included with the banner are there because they are	*/
/*  useful in subsequent code.  The macros #defined in the header or	*/
/*  the body either are not useful outside of semantic actions or	*/
/*  are conditional.							*/

char *banner[] =
{
    "#ifndef lint",
    "static char yysccsid[] = \"@(#)yaccpar	1.9 (Berkeley) 02/21/93 (BSDI)\";",
    "#endif",
    "#include <stdlib.h>",
    "#define YYBYACC 1",
    "#define YYMAJOR 1",
    "#define YYMINOR 9",
    "#define YYEMPTY (-1)",
    "#define YYLEX yylex()",
    "#define yyclearin (yychar=YYEMPTY)",
    "#define yyerrok (yyerrflag=0)",
    "#define YYRECOVERING (yyerrflag!=0)",
    0
};


char *tables[] =
{
    "extern short yylhs[];",
    "extern short yylen[];",
    "extern short yydefred[];",
    "extern short yydgoto[];",
    "extern short yysindex[];",
    "extern short yyrindex[];",
    "extern short yygindex[];",
    "extern short yytable[];",
    "extern short yycheck[];",
    "#if YYDEBUG",
    "extern char *yyname[];",
    "extern char *yyrule[];",
    "#endif",
    0
};


char *header[] =
{
    "#ifdef YYSTACKSIZE",
    "#undef YYMAXDEPTH",
    "#define YYMAXDEPTH YYSTACKSIZE",
    "#else",
    "#ifdef YYMAXDEPTH",
    "#define YYSTACKSIZE YYMAXDEPTH",
    "#else",
    "#define YYSTACKSIZE 10000",
    "#define YYMAXDEPTH 10000",
    "#endif",
    "#endif",
    "#define YYINITSTACKSIZE 200",
    "int yydebug;",
    "int yynerrs;",
    "struct yystack {",
    "    short *ssp;",
    "    YYSTYPE *vsp;",
    "    short *ss;",
    "    YYSTYPE *vs;",
    "    int stacksize;",
    "    short *sslim;",
    "};",
    "int yychar; /* some people use this, so we copy it in & out */",
    "int yyerrflag; /* must be global for yyerrok & YYRECOVERING */",
    "YYSTYPE yylval;",
    0
};


char *body[] =
{
    "/* allocate initial stack */",
    "#if defined(__STDC__) || defined(__cplusplus)",
    "static int yyinitstack(struct yystack *sp)",
    "#else",
    "static int yyinitstack(sp)",
    "    struct yystack *sp;",
    "#endif",
    "{",
    "    int newsize;",
    "    short *newss;",
    "    YYSTYPE *newvs;",
    "",
    "    newsize = YYINITSTACKSIZE;",
    "    newss = (short *)malloc(newsize * sizeof *newss);",
    "    newvs = (YYSTYPE *)malloc(newsize * sizeof *newvs);",
    "    sp->ss = sp->ssp = newss;",
    "    sp->vs = sp->vsp = newvs;",
    "    if (newss == NULL || newvs == NULL) return -1;",
    "    sp->stacksize = newsize;",
    "    sp->sslim = newss + newsize - 1;",
    "    return 0;",
    "}",
    "",
    "/* double stack size, up to YYMAXDEPTH */",
    "#if defined(__STDC__) || defined(__cplusplus)",
    "static int yygrowstack(struct yystack *sp)",
    "#else",
    "static int yygrowstack(sp)",
    "    struct yystack *sp;",
    "#endif",
    "{",
    "    int newsize, i;",
    "    short *newss;",
    "    YYSTYPE *newvs;",
    "",
    "    if ((newsize = sp->stacksize) >= YYMAXDEPTH) return -1;",
    "    if ((newsize *= 2) > YYMAXDEPTH) newsize = YYMAXDEPTH;",
    "    i = sp->ssp - sp->ss;",
    "    if ((newss = (short *)realloc(sp->ss, newsize * sizeof *newss)) == NULL)",
    "        return -1;",
    "    sp->ss = newss;",
    "    sp->ssp = newss + i;",
    "    if ((newvs = (YYSTYPE *)realloc(sp->vs, newsize * sizeof *newvs)) == NULL)",
    "        return -1;",
    "    sp->vs = newvs;",
    "    sp->vsp = newvs + i;",
    "    sp->stacksize = newsize;",
    "    sp->sslim = newss + newsize - 1;",
    "    return 0;",
    "}",
    "",
    "#define YYFREESTACK(sp) { free((sp)->ss); free((sp)->vs); }",
    "",
    "#define YYABORT goto yyabort",
    "#define YYREJECT goto yyabort",
    "#define YYACCEPT goto yyaccept",
    "#define YYERROR goto yyerrlab",
    "",
    "/*",
    " * We do the work in this function so that 'return' statements",
    " * in yacc actions won't cause us to leak the memory for the stack.",
    " */",
    "static int",
    "yy_do_parse(struct yystack *yystkp)",
    "{",
    "    register int yym, yyn, yystate, yych;",
    "    register YYSTYPE *yyvsp;",
    "    YYSTYPE yyval;",
    "#if YYDEBUG",
    "    register char *yys;",
    "    extern char *getenv();",
    "",
    "    if (yys = getenv(\"YYDEBUG\"))",
    "    {",
    "        yyn = *yys;",
    "        if (yyn >= '0' && yyn <= '9')",
    "            yydebug = yyn - '0';",
    "    }",
    "#endif",
    "",
    "    yynerrs = 0;",
    "    yyerrflag = 0;",
    "    yychar = yych = YYEMPTY;",
    "",
    "    if (yyinitstack(yystkp)) goto yyoverflow;",
    "    *yystkp->ssp = yystate = 0;",
    "",
    "yyloop:",
    "    if (yyn = yydefred[yystate]) goto yyreduce;",
    "    if (yych < 0)",
    "    {",
    "        if ((yych = YYLEX) < 0) yych = 0;",
    "        yychar = yych;",
    "#if YYDEBUG",
    "        if (yydebug)",
    "        {",
    "            yys = 0;",
    "            if (yych <= YYMAXTOKEN) yys = yyname[yych];",
    "            if (!yys) yys = \"illegal-symbol\";",
    "            printf(\"%sdebug: state %d, reading %d (%s)\\n\",",
    "                    YYPREFIX, yystate, yych, yys);",
    "        }",
    "#endif",
    "    }",
    "    if ((yyn = yysindex[yystate]) && (yyn += yych) >= 0 &&",
    "            yyn <= YYTABLESIZE && yycheck[yyn] == yych)",
    "    {",
    "#if YYDEBUG",
    "        if (yydebug)",
    "            printf(\"%sdebug: state %d, shifting to state %d\\n\",",
    "                    YYPREFIX, yystate, yytable[yyn]);",
    "#endif",
    "        if (yystkp->ssp >= yystkp->sslim && yygrowstack(yystkp))",
    "            goto yyoverflow;",
    "        *++yystkp->ssp = yystate = yytable[yyn];",
    "        *++yystkp->vsp = yylval;",
    "        yychar = yych = YYEMPTY;",
    "        if (yyerrflag > 0)  --yyerrflag;",
    "        goto yyloop;",
    "    }",
    "    if ((yyn = yyrindex[yystate]) && (yyn += yych) >= 0 &&",
    "            yyn <= YYTABLESIZE && yycheck[yyn] == yych)",
    "    {",
    "        yyn = yytable[yyn];",
    "        goto yyreduce;",
    "    }",
    "    if (yyerrflag) goto yyinrecovery;",
    "#ifdef lint",
    "    goto yynewerror;",
    "#endif",
    "yynewerror:",
    "    yyerror(\"syntax error\");",
    "#ifdef lint",
    "    goto yyerrlab;",
    "#endif",
    "yyerrlab:",
    "    ++yynerrs;",
    "yyinrecovery:",
    "    if (yyerrflag < 3)",
    "    {",
    "        yyerrflag = 3;",
    "        for (;;)",
    "        {",
    "            if ((yyn = yysindex[*yystkp->ssp]) &&",
    "                    (yyn += YYERRCODE) >= 0 &&",
    "                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)",
    "            {",
    "#if YYDEBUG",
    "                if (yydebug)",
    "                    printf(\"%sdebug: state %d, error recovery shifting\\",
    " to state %d\\n\", YYPREFIX, *yystkp->ssp, yytable[yyn]);",
    "#endif",
    "                if (yystkp->ssp >= yystkp->sslim && yygrowstack(yystkp))",
    "                    goto yyoverflow;",
    "                *++yystkp->ssp = yystate = yytable[yyn];",
    "                *++yystkp->vsp = yylval;",
    "                goto yyloop;",
    "            }",
    "            else",
    "            {",
    "#if YYDEBUG",
    "                if (yydebug)",
    "                    printf(\"%sdebug: error recovery discarding state %d\
\\n\",",
    "                            YYPREFIX, *yystkp->ssp);",
    "#endif",
    "                if (yystkp->ssp <= yystkp->ss) goto yyabort;",
    "                --yystkp->ssp;",
    "                --yystkp->vsp;",
    "            }",
    "        }",
    "    }",
    "    else",
    "    {",
    "        if (yych == 0) goto yyabort;",
    "#if YYDEBUG",
    "        if (yydebug)",
    "        {",
    "            yys = 0;",
    "            if (yych <= YYMAXTOKEN) yys = yyname[yych];",
    "            if (!yys) yys = \"illegal-symbol\";",
    "            printf(\"%sdebug: state %d, error recovery discards token %d\
 (%s)\\n\",",
    "                    YYPREFIX, yystate, yych, yys);",
    "        }",
    "#endif",
    "        yychar = yych = YYEMPTY;",
    "        goto yyloop;",
    "    }",
    "yyreduce:",
    "#if YYDEBUG",
    "    if (yydebug)",
    "        printf(\"%sdebug: state %d, reducing by rule %d (%s)\\n\",",
    "                YYPREFIX, yystate, yyn, yyrule[yyn]);",
    "#endif",
    "    yym = yylen[yyn];",
    "    yyvsp = yystkp->vsp; /* for speed in code under switch() */",
    "    yyval = yyvsp[1-yym];",
    "    switch (yyn)",
    "    {",
    0
};


char *trailer[] =
{
    "    }",
    "    yystkp->ssp -= yym;",
    "    yystate = *yystkp->ssp;",
    "    yystkp->vsp -= yym;",
    "    yym = yylhs[yyn];",
    "    yych = yychar;",
    "    if (yystate == 0 && yym == 0)",
    "    {",
    "#if YYDEBUG",
    "        if (yydebug)",
    "            printf(\"%sdebug: after reduction, shifting from state 0 to\\",
    " state %d\\n\", YYPREFIX, YYFINAL);",
    "#endif",
    "        yystate = YYFINAL;",
    "        *++yystkp->ssp = YYFINAL;",
    "        *++yystkp->vsp = yyval;",
    "        if (yych < 0)",
    "        {",
    "            if ((yych = YYLEX) < 0) yych = 0;",
    "            yychar = yych;",
    "#if YYDEBUG",
    "            if (yydebug)",
    "            {",
    "                yys = 0;",
    "                if (yych <= YYMAXTOKEN) yys = yyname[yych];",
    "                if (!yys) yys = \"illegal-symbol\";",
    "                printf(\"%sdebug: state %d, reading %d (%s)\\n\",",
    "                        YYPREFIX, YYFINAL, yych, yys);",
    "            }",
    "#endif",
    "        }",
    "        if (yych == 0) goto yyaccept;",
    "        goto yyloop;",
    "    }",
    "    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&",
    "            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)",
    "        yystate = yytable[yyn];",
    "    else",
    "        yystate = yydgoto[yym];",
    "#if YYDEBUG",
    "    if (yydebug)",
    "        printf(\"%sdebug: after reduction, shifting from state %d \\",
    "to state %d\\n\", YYPREFIX, *yystkp->ssp, yystate);",
    "#endif",
    "    if (yystkp->ssp >= yystkp->sslim && yygrowstack(yystkp))",
    "        goto yyoverflow;",
    "    *++yystkp->ssp = yystate;",
    "    *++yystkp->vsp = yyval;",
    "    goto yyloop;",
    "yyoverflow:",
    "    yyerror(\"yacc stack overflow\");",
    "yyabort:",
    "    return (1);",
    "yyaccept:",
    "    return (0);",
    "}",
    "",
    "int",
    "yyparse()",
    "{",
    "    struct yystack yystk;",
    "    int result;",
    "",
    "    result = yy_do_parse(&yystk);",
    "    YYFREESTACK(&yystk);",
    "    return (result);",
    "}",
    0
};


write_section(section)
char *section[];
{
    register int c;
    register int i;
    register char *s;
    register FILE *f;

    f = code_file;
    for (i = 0; s = section[i]; ++i)
    {
	++outline;
	while (c = *s)
	{
	    putc(c, f);
	    ++s;
	}
	putc('\n', f);
    }
}
