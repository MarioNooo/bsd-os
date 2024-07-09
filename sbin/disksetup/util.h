/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI util.h,v 2.6 1999/09/10 02:51:13 prb Exp	*/

#include <errno.h>

class O {
public:
    enum CMD {
        UNSPEC,
	READ,
	RESTORE,
	INTERACTIVE,
	EDIT,
	W_ENABLE,
	W_DISABLE,
	BOOTBLKS,
	INSTALL,
	EXPRESS,
	DISKTYPE,
    };
    enum FLAG {
        EMPTY = 0,
	ForceBoot	= 0x000001,
	NoWrite		= 0x000002,
    	Ignore		= 0x000004 | 0x000008,
    	IgnoreBSD	= 0x000004,
    	IgnoreFDISK	= 0x000008,
    	NonBlock	= 0x000010,
    	Expert		= 0x000020,
    	InCoreOnly	= 0x000040,
    	UpdateFDISK	= 0x000080,
    	UpdateBSD	= 0x000100,
    	ReadKernel	= 0x000200,
    	ReadDisk	= 0x000400,
    	IgnoreGeometry	= 0x000800,
    	RequireBootany	= 0x001000,
    	DontUpdateLabel	= 0x002000,
    	NoBootBlocks	= 0x004000,
	WriteDiskOnly	= 0x008000,
	Express		= 0x010000,
	NoDiskLabel	= 0x020000,
    };
private:
    static CMD op;
    static int flag;
    static char *prog;
public:
    static void Usage(int = 1);
    static void Set(O::CMD, int = 0);
    static void Set(O::FLAG);
    static void Clear(O::FLAG);
    static int Cmd()		{ return(op); }
    static int Flags()		{ return(flag); }
    static int Flag(FLAG f)	{ return((flag & f) ? 1 : 0); }
    static char *Program(char * = 0);
};

class Errors {
    static int		serial;
    static Errors	*root;
    Errors		*next;
    int			value;
    char		*string;
public:
    Errors(char *s)		{ string = s; value = --serial;
				  next = root; root = this; }
    Errors(char *s, int v)	{ string = s; value = v;
				  next = root; root = this; }

    operator int()		{ return(value); }
    static char *String(int i);
};

int PhysMem(int = 0);

#ifdef	i386
extern char boot0[];
extern char boot1[];
extern char boot2[];
extern char *primary;
extern char *secondary;
extern char *master;
#endif
#if	defined(__powerpc__)
extern char *bootstrap;
extern char *master;
extern char boot0[];
extern char boot2[];
#endif
#ifdef	sparc
extern char *bootstrap;
extern char boot0[];
#endif

int edit();
int interactive();
void express();
void getbootblocks();
char *ChooseDisk();
char **FindDisks();
void SetGeometry(char **, Geometry &, Geometry &);
#ifdef	__powerpc__
void powerpc_writeboot(int);
#endif
