/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI filesys.h,v 2.6 2000/02/01 20:44:32 prb Exp	*/

#include <memory.h>
#include <sys/types.h>
#include <sys/disklabel.h>

struct FileSystem {
    unsigned char       number;
    unsigned char       type;
    unsigned char       original:1;
    unsigned char       modified:1;
    unsigned char       fixed:1;
#ifdef	NEED_FDISK
    signed char		part:3;
#endif
    char		mount[65];
    quad_t length;
    quad_t offset; 
    int fsize;
    int frag;
    int cpg;

    void Zero()         { number = type = original = 0; length = offset = 0;
			  mount[0] = 0; fixed = 0; modified = 0; fsize = 0;
			  frag = 0; cpg = 0; }
    void Clean()        { number = type = original = 0; length = offset = 0;
			  mount[0] = 0; fixed = 0; fsize = 0; frag = 0;
			  cpg = 0; }
    FileSystem()         { Zero(); }
    int operator =(FileSystem &p2) { number = p2.number;
                                type = p2.type;
                                original = p2.original;
                                fixed = p2.fixed;
				memcpy(mount, p2.mount, sizeof(mount));
                                length = p2.length;
                                offset = p2.offset;
				fsize = p2.fsize;
				frag = p2.frag;
				cpg = p2.cpg;
				return(0);
				}
    
    quad_t End()           { return(offset + length); }
    quad_t Start()         { return(offset); }
    void AdjustType();
    
    int operator <(FileSystem &);
    int operator == (FileSystem &p)      { return(number == p.number); }
        
    void Print(int, quad_t);
    void Load(disklabel::partition *);
    int Changed(FileSystem &);
    void ForceCyl(quad_t = 0);
};  

inline int 
FileSystem::operator <(FileSystem &p)
{
    return(offset + length <= p.offset);
}

inline int 
FileSystem::Changed(FileSystem &p)
{
    return (p.type != type ||
	    p.length != length ||
	    p.offset != offset);
}

extern int FFindType(char *);
extern PType & FTypeFind(int);
#ifdef	NEED_FDISK
extern int PartToBSDType(int);
#endif
