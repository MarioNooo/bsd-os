/*	BSDI cg_arcs.h,v 1.2 2003/01/14 16:36:50 torek Exp	*/

#ifndef cg_arcs_h
#define cg_arcs_h

/*
 * Arc structure for call-graph.
 *
 * With pointers to the symbols of the parent and the child, a count
 * of how many times this arc was traversed, and pointers to the next
 * parent of this child and the next child of this parent.
 */
typedef struct arc
  {
    Sym *parent;		/* source vertice of arc */
    Sym *child;			/* dest vertice of arc */
    unsigned long count;	/* # of calls from parent to child */
    double time;		/* time inherited along arc */
    double child_time;		/* child-time inherited along arc */
    struct arc *next_parent;	/* next parent of CHILD */
    struct arc *next_child;	/* next child of PARENT */
    int has_been_placed;	/* have this arc's functions been placed? */
    struct arc *arc_next;	/* list of arcs on cycle */
    unsigned short arc_cyclecnt; /* num cycles involved in */
    unsigned short arc_flags;	/* see below */
  }
Arc;

/*
 * arc flags
 */
#define DEADARC		0x01	/* time should not propagate across the arc */
#define ONLIST		0x02	/* arc is on list of arcs in cycles */

/*
 * The cycle list.
 * For eaach subcycle within an identified cycle, we gather
 * its size and the list of included arcs.
 */
typedef struct cl
  {
    int size;			/* length of cycle */
    struct cl *next;		/* next member of list */
    Arc *list[1];		/* list of arcs in cycle */
    /* actually longer */
  }
Cl;

#define	CYCLEMAX	100	/* maximum cycles before cutting one of them */
extern Arc *archead;		/* the head of arcs in current cycle list */
extern Cl *cyclehead;		/* the head of the list */
extern int cyclecnt;		/* the number of cycles found */


extern unsigned int num_cycles;	/* number of cycles discovered */
extern Sym *cycle_header;	/* cycle headers */
extern Arc **arcs;
extern unsigned int numarcs;

extern void arc_add PARAMS ((Sym * parent, Sym * child, unsigned long count));
extern Arc *arc_lookup PARAMS ((Sym * parent, Sym * child));
extern Sym **cg_assemble PARAMS ((void));

extern boolean cycleanalyze PARAMS ((void));
extern boolean descend PARAMS ((Sym * node, Arc * * stkstart, Arc * * stkp));
extern boolean addcycle PARAMS ((Arc * * stkstart, Arc * * stkend));
extern void compresslist PARAMS ((void));
extern void printsubcycle PARAMS ((Cl * clp));

#endif /* cg_arcs_h */
