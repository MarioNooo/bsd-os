#ifdef NEED_FDISK
/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	BSDI partition.h,v 2.5 2000/05/02 20:22:15 prb Exp	*/

struct Partition {
    unsigned char       number;
    unsigned char       type;
    unsigned char       original:1;
    unsigned char       modified:1;
    unsigned char       bootany:1;
    quad_t length;
    quad_t offset; 

    void Zero()         { number = type = original = 0; length = offset = 0; 
			  modified = 0; bootany = 0; }
    void Clean()        { number = type = original = 0; length = offset = 0;
			  bootany = 0; }
    Partition()         { Zero(); }
    int operator =(Partition &p2) { number = p2.number;
                                type = p2.type;
                                original = p2.original;
                                length = p2.length;
                                offset = p2.offset;
				return (0); }
    
    quad_t End()           { return(offset + length); }
    quad_t Start()         { return(offset); }
    void AdjustType();
    int IsDosType();
    int IsBSDType();
    int BootType();
    char *TypeName();
    
    int operator <(Partition &);
    int operator == (Partition &p)      { return(number == p.number); }
        
    void Print(int, quad_t);
    int Changed(Partition &);
};  

inline int 
Partition::operator <(Partition &p)
{
    return(offset + length <= p.offset);
}

inline int
Partition::Changed(Partition &p)   
{   
    return (p.type != type ||
            p.length != length ||
            p.offset != offset);
}   

extern void showtypes();
extern int PFindType(char *);
#endif

struct PType {
    char *name;
    int type;
    quad_t (*start)(quad_t);
    int minsector;              /* Sector that this can first exist on */
    int bootable;
};              

extern quad_t force_cyl(quad_t);
extern quad_t force_track(quad_t);
extern quad_t force_sector(quad_t);
extern PType & PTypeFind(int);
