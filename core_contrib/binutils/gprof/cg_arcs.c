/*	BSDI cg_arcs.c,v 1.2 2003/01/14 16:36:50 torek Exp	*/

/*
 * Copyright (c) 1983, 1993, 2001
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
#include "libiberty.h"
#include "gprof.h"
#include "search_list.h"
#include "source.h"
#include "symtab.h"
#include "call_graph.h"
#include "cg_arcs.h"
#include "cg_dfn.h"
#include "cg_print.h"
#include "utils.h"
#include "sym_ids.h"

static int cmp_topo PARAMS ((const PTR, const PTR));
static void propagate_time PARAMS ((Sym *));
static void cycle_time PARAMS ((void));
static void cycle_link PARAMS ((void));
static void inherit_flags PARAMS ((Sym *));
static void propagate_flags PARAMS ((Sym **));
static int cmp_total PARAMS ((const PTR, const PTR));

Sym *cycle_header;
unsigned int num_cycles;
Arc **arcs;
unsigned int numarcs;
Arc *archead;
Cl *cyclehead;
int cyclecnt;

#ifdef DEBUG
static int viable;
static int visited;
static int oldcycle;
static int newcycle;
#endif

/*
 * Return true iff PARENT has an arc to covers the address
 * range covered by CHILD.
 */
Arc *
arc_lookup (parent, child)
     Sym *parent;
     Sym *child;
{
  Arc *arc;

  if (!parent || !child)
    {
      printf ("[arc_lookup] parent == 0 || child == 0\n");
      return 0;
    }
  DBG (LOOKUPDEBUG, printf ("[arc_lookup] parent %s child %s\n",
			    parent->name, child->name));
  for (arc = parent->cg.children; arc; arc = arc->next_child)
    {
      DBG (LOOKUPDEBUG, printf ("[arc_lookup]\t parent %s child %s\n",
				arc->parent->name, arc->child->name));
      if (child->addr >= arc->child->addr
	  && child->end_addr <= arc->child->end_addr)
	{
	  return arc;
	}
    }
  return 0;
}


/*
 * Add (or just increment) an arc:
 */
void
arc_add (parent, child, count)
     Sym *parent;
     Sym *child;
     unsigned long count;
{
  static unsigned int maxarcs = 0;
  Arc *arc, **newarcs;

  DBG (TALLYDEBUG, printf ("[arc_add] %lu arcs from %s to %s\n",
			   count, parent->name, child->name));
  arc = arc_lookup (parent, child);
  if (arc)
    {
      /*
       * A hit: just increment the count.
       */
      DBG (TALLYDEBUG, printf ("[tally] hit %lu += %lu\n",
			       arc->count, count));
      arc->count += count;
      return;
    }
  arc = (Arc *) xmalloc (sizeof (*arc));
  memset (arc, 0, sizeof (*arc));
  arc->parent = parent;
  arc->child = child;
  arc->count = count;

  /* If this isn't an arc for a recursive call to parent, then add it
     to the array of arcs.  */
  if (parent != child)
    {
      /* If we've exhausted space in our current array, get a new one
	 and copy the contents.   We might want to throttle the doubling
	 factor one day.  */
      if (numarcs == maxarcs)
	{
	  /* Determine how much space we want to allocate.  */
	  if (maxarcs == 0)
	    maxarcs = 1;
	  maxarcs *= 2;

	  /* Allocate the new array.  */
	  newarcs = (Arc **)xmalloc(sizeof (Arc *) * maxarcs);

	  /* Copy the old array's contents into the new array.  */
	  memcpy (newarcs, arcs, numarcs * sizeof (Arc *));

	  /* Free up the old array.  */
	  free (arcs);

	  /* And make the new array be the current array.  */
	  arcs = newarcs;
	}

      /* Place this arc in the arc array.  */
      arcs[numarcs++] = arc;
    }

  /* prepend this child to the children of this parent: */
  arc->next_child = parent->cg.children;
  parent->cg.children = arc;

  /* prepend this parent to the parents of this child: */
  arc->next_parent = child->cg.parents;
  child->cg.parents = arc;
}


static int
cmp_topo (lp, rp)
     const PTR lp;
     const PTR rp;
{
  const Sym *left = *(const Sym **) lp;
  const Sym *right = *(const Sym **) rp;

  return left->cg.top_order - right->cg.top_order;
}


static void
propagate_time (parent)
     Sym *parent;
{
  Arc *arc;
  Sym *child;
  double share, prop_share;

  if (parent->cg.prop.fract == 0.0)
    {
      return;
    }

  /* gather time from children of this parent: */

  for (arc = parent->cg.children; arc; arc = arc->next_child)
    {
      if (arc->arc_flags & DEADARC)
	{
	  continue;
	}
      child = arc->child;
      if (arc->count == 0 || child == parent || child->cg.prop.fract == 0)
	{
	  continue;
	}
      if (child->cg.cyc.head != child)
	{
	  if (parent->cg.cyc.num == child->cg.cyc.num)
	    {
	      continue;
	    }
	  if (parent->cg.top_order <= child->cg.top_order)
	    {
	      fprintf (stderr, "[propagate] toporder botches\n");
	    }
	  child = child->cg.cyc.head;
	}
      else
	{
	  if (parent->cg.top_order <= child->cg.top_order)
	    {
	      fprintf (stderr, "[propagate] toporder botches\n");
	      continue;
	    }
	}
      if (child->cg.npropcall == 0)
	{
	  continue;
	}

      /* distribute time for this arc: */
      arc->time = child->hist.time * (((double) arc->count)
				      / ((double) child->cg.npropcall));
      arc->child_time = child->cg.child_time
	* (((double) arc->count) / ((double) child->cg.npropcall));
      share = arc->time + arc->child_time;
      parent->cg.child_time += share;

      /* (1 - cg.prop.fract) gets lost along the way: */
      prop_share = parent->cg.prop.fract * share;

      /* fix things for printing: */
      parent->cg.prop.child += prop_share;
      arc->time *= parent->cg.prop.fract;
      arc->child_time *= parent->cg.prop.fract;

      /* add this share to the parent's cycle header, if any: */
      if (parent->cg.cyc.head != parent)
	{
	  parent->cg.cyc.head->cg.child_time += share;
	  parent->cg.cyc.head->cg.prop.child += prop_share;
	}
      DBG (PROPDEBUG,
	   printf ("[prop_time] child \t");
	   print_name (child);
	   printf (" with %f %f %lu/%lu\n", child->hist.time,
		   child->cg.child_time, arc->count, child->cg.npropcall);
	   printf ("[prop_time] parent\t");
	   print_name (parent);
	   printf ("\n[prop_time] share %f\n", share));
    }
}


/*
 * Compute the time of a cycle as the sum of the times of all
 * its members.
 */
static void
cycle_time ()
{
  Sym *member, *cyc;

  for (cyc = &cycle_header[1]; cyc <= &cycle_header[num_cycles]; ++cyc)
    {
      for (member = cyc->cg.cyc.next; member; member = member->cg.cyc.next)
	{
	  if (member->cg.prop.fract == 0.0)
	    {
	      /*
	       * All members have the same propfraction except those
	       * that were excluded with -E.
	       */
	      continue;
	    }
	  cyc->hist.time += member->hist.time;
	}
      cyc->cg.prop.self = cyc->cg.prop.fract * cyc->hist.time;
    }
}


static void
cycle_link ()
{
  Sym *sym, *cyc, *member;
  Arc *arc;
  int num;

  /* count the number of cycles, and initialize the cycle lists: */

  num_cycles = 0;
  for (sym = symtab.base; sym < symtab.limit; ++sym)
    {
      /* this is how you find unattached cycles: */
      if (sym->cg.cyc.head == sym && sym->cg.cyc.next)
	{
	  ++num_cycles;
	}
    }

  /*
   * cycle_header is indexed by cycle number: i.e. it is origin 1,
   * not origin 0.
   */
  cycle_header = (Sym *) xmalloc ((num_cycles + 1) * sizeof (Sym));

  /*
   * Now link cycles to true cycle-heads, number them, accumulate
   * the data for the cycle.
   */
  num = 0;
  cyc = cycle_header;
  for (sym = symtab.base; sym < symtab.limit; ++sym)
    {
      if (!(sym->cg.cyc.head == sym && sym->cg.cyc.next != 0))
	{
	  continue;
	}
      ++num;
      ++cyc;
      sym_init (cyc);
      cyc->cg.print_flag = true;	/* should this be printed? */
      cyc->cg.top_order = DFN_NAN;	/* graph call chain top-sort order */
      cyc->cg.cyc.num = num;	/* internal number of cycle on */
      cyc->cg.cyc.head = cyc;	/* pointer to head of cycle */
      cyc->cg.cyc.next = sym;	/* pointer to next member of cycle */
      DBG (CYCLEDEBUG, printf ("[cycle_link] ");
	   print_name (sym);
	   printf (" is the head of cycle %d\n", num));

      /* link members to cycle header: */
      for (member = sym; member; member = member->cg.cyc.next)
	{
	  member->cg.cyc.num = num;
	  member->cg.cyc.head = cyc;
	}

      /*
       * Count calls from outside the cycle and those among cycle
       * members:
       */
      for (member = sym; member; member = member->cg.cyc.next)
	{
	  for (arc = member->cg.parents; arc; arc = arc->next_parent)
	    {
	      if (arc->parent == member)
		{
		  continue;
		}
	      if (arc->parent->cg.cyc.num == num)
		{
		  cyc->cg.self_calls += arc->count;
		}
	      else
		{
		  cyc->cg.npropcall += arc->count;
		}
	    }
	}
    }
}

/*
 * analyze cycles to determine breakup
 */
boolean
cycleanalyze ()
{
  Arc **cyclestack;
  Arc **stkp;
  Arc **arcpp;
  Arc **endlist;
  Arc *arcp;
  Sym *nlp;
  Cl *clp;
  boolean ret;
  boolean done;
  int size;
  unsigned int cycleno;

  /*
   * calculate the size of the cycle, and find nodes that
   * exit the cycle as they are desirable targets to cut
   * some of their parents
   */
  for (done = true, cycleno = 1; cycleno <= num_cycles; cycleno++)
    {
      size = 0;
      for (nlp = cycle_header[cycleno].cg.cyc.next; nlp; nlp = nlp->cg.cyc.next)
        {
          size += 1;
          nlp->cg.cyc.parentcnt = 0;
          nlp->cg.flags &= ~HASCYCLEXIT;
          for (arcp = nlp->cg.parents; arcp; arcp = arcp->next_parent)
            {
              nlp->cg.cyc.parentcnt += 1;
              if ((unsigned)arcp->parent->cg.cyc.num != cycleno)
                  nlp->cg.flags |= HASCYCLEXIT;
            }
        }
      if (size <= cyclethreshold)
          continue;
      done = false;
      cyclestack = (Arc **) calloc (size + 1, sizeof (Arc *));
      if (cyclestack == 0)
        {
          fprintf (stderr, "%s: No room for %d bytes of cycle stack\n",
                   whoami, (size + 1) * sizeof (Arc *));
          return true; /* not really, but don't want to get called again */
        }
      DBG (BREAKCYCLE,
           printf ("[cycleanalyze] starting cycle %d of %d, size %d\n",
                   cycleno, num_cycles, size));
      for (nlp = cycle_header[cycleno].cg.cyc.next; nlp; nlp = nlp->cg.cyc.next)
	{
	  stkp = &cyclestack[0];
	  nlp->cg.flags |= CYCLEHEAD;
	  ret = descend (nlp, cyclestack, stkp);
	  nlp->cg.flags &= ~CYCLEHEAD;
	  if (ret == false)
	    {
	      break;
	    }
	}
	free (cyclestack);
	if (cyclecnt > 0)
	  {
	    compresslist ();
	    for (clp = cyclehead; clp; )
	      {
		endlist = &clp->list[clp->size];
		for (arcpp = clp->list; arcpp < endlist; arcpp++)
		  {
		    (*arcpp)->arc_cyclecnt--;
		  }
		cyclecnt--;
		clp = clp->next;
		free (clp);
	      }
	    cyclehead = 0;
	  }
    }
  DBG (BREAKCYCLE,
       printf("%s visited %d, viable %d, newcycle %d, oldcycle %d\n",
	      "[doarcs]", visited, viable, newcycle, oldcycle));
  return done;
}

boolean
descend (node, stkstart, stkp)
     Sym *node;
     Arc **stkstart;
     Arc **stkp;
{
  Arc *arcp;
  boolean ret;

  for (arcp = node->cg.children; arcp; arcp = arcp->next_child)
    {
      DBG (BREAKCYCLE, visited++);
      if (arcp->child->cg.cyc.num != node->cg.cyc.num ||
	  (arcp->child->cg.flags & VISITED) ||
	  (arcp->arc_flags & DEADARC))
	{
	  continue;
	}
      DBG (BREAKCYCLE, viable++);
      *stkp = arcp;
      if (arcp->child->cg.flags & CYCLEHEAD)
	{
	  if (addcycle (stkstart, stkp) == false)
	    {
	      return false;
	    }
	  continue;
	}
      arcp->child->cg.flags |= VISITED;
      ret = descend (arcp->child, stkstart, stkp + 1);
      arcp->child->cg.flags &= ~VISITED;
      if (ret == false)
	{
	  return false;
	}
    }
  return true;  /* ??? this was missing; "true" seems the only obvious value */
}

boolean
addcycle (stkstart, stkend)
     Arc **stkstart;
     Arc **stkend;
{
  Arc **arcpp;
  Arc **stkloc;
  Arc **stkp;
  Arc **endlist;
  Arc *minarc;
  Arc *arcp;
  Cl *clp;
  int size;

  size = stkend - stkstart + 1;
  if (size <= 1)
    {
      return true;
    }
  minarc = *stkstart;
  stkloc = stkstart;
  for (arcpp = stkstart; arcpp <= stkend; arcpp++)
    {
      if (*arcpp > minarc)
	{
	  continue;
	}
      minarc = *arcpp;
      stkloc = arcpp;
    }
  for (clp = cyclehead; clp; clp = clp->next)
    {
      if (clp->size != size)
	{
	  continue;
	}
      stkp = stkloc;
      endlist = &clp->list[size];
      for (arcpp = clp->list; arcpp < endlist; arcpp++)
	{
	  if (*stkp++ != *arcpp)
	    {
	      break;
	    }
	  if (stkp > stkend)
	    {
	      stkp = stkstart;
	    }
	}
      if (arcpp == endlist)
	{
	  DBG (BREAKCYCLE, oldcycle++);
	  return true;
	}
    }
  clp = (Cl *) calloc (1, sizeof (Cl) + (size - 1) * sizeof(Arc *));
  if (clp == 0)
    {
      fprintf (stderr, "%s: No room for %d bytes of subcycle storage\n",
	       whoami, sizeof (Cl) + (size - 1) * sizeof (Arc *));
      return false;
    }
  stkp = stkloc;
  endlist = &clp->list[size];
  for (arcpp = clp->list; arcpp < endlist; arcpp++)
    {
      arcp = *arcpp = *stkp++;
      if (stkp > stkend)
	{
	  stkp = stkstart;
	}
      arcp->arc_cyclecnt++;
      if ((arcp->arc_flags & ONLIST) == 0)
	{
	  arcp->arc_flags |= ONLIST;
	  arcp->arc_next = archead;
	  archead = arcp;
	}
    }
  clp->size = size;
  clp->next = cyclehead;
  cyclehead = clp;
  DBG (BREAKCYCLE, newcycle++);
  DBG (SUBCYCLELIST,
       printsubcycle (clp));
  cyclecnt++;
  if (cyclecnt >= CYCLEMAX)
    {
      return false;
    }
  return true;
}

void
compresslist ()
{
  Cl *clp;
  Cl **prev;
  Arc **arcpp;
  Arc **endlist;
  Arc *arcp;
  Arc *maxarcp;
  Arc *maxexitarcp;
  Arc *maxwithparentarcp;
  Arc *maxnoparentarcp;
  int maxexitcnt;
  int maxwithparentcnt;
  int maxnoparentcnt;
  char *type;

  maxexitcnt = 0;
  maxwithparentcnt = 0;
  maxnoparentcnt = 0;
  /* the NULLs below are just to eliminiate gcc warnings */
  maxexitarcp = NULL;
  maxnoparentarcp = NULL;
  maxwithparentarcp = NULL;
  type = NULL;
  for (endlist = &archead, arcp = archead; arcp; )
    {
      if (arcp->arc_cyclecnt == 0)
	{
	  arcp->arc_flags &= ~ONLIST;
	  *endlist = arcp->arc_next;
	  arcp->arc_next = 0;
	  arcp = *endlist;
	  continue;
	}
      if (arcp->child->cg.flags & HASCYCLEXIT)
	{
	  if (arcp->arc_cyclecnt > maxexitcnt ||
	      (arcp->arc_cyclecnt == maxexitcnt &&
	       arcp->arc_cyclecnt < maxexitarcp->count))
	    {
	      maxexitcnt = arcp->arc_cyclecnt;
	      maxexitarcp = arcp;
	    }
	}
      else if (arcp->child->cg.cyc.parentcnt > 1)
	{
	  if (arcp->arc_cyclecnt > maxwithparentcnt ||
	      (arcp->arc_cyclecnt == maxwithparentcnt &&
	       arcp->arc_cyclecnt < maxwithparentarcp->count))
	    {
	      maxwithparentcnt = arcp->arc_cyclecnt;
	      maxwithparentarcp = arcp;
	    }
	}
      else
	{
	  if (arcp->arc_cyclecnt > maxnoparentcnt ||
	      (arcp->arc_cyclecnt == maxnoparentcnt &&
	       arcp->arc_cyclecnt < maxnoparentarcp->count))
	    {
	      maxnoparentcnt = arcp->arc_cyclecnt;
	      maxnoparentarcp = arcp;
	    }
	}
      endlist = &arcp->arc_next;
      arcp = arcp->arc_next;
    }
  if (maxexitcnt > 0)
    {
      /*
       *	first choice is edge leading to node with out-of-cycle parent
       */
      maxarcp = maxexitarcp;
      DBG (BREAKCYCLE, type = "exit");
    }
  else if (maxwithparentcnt > 0)
    {
      /*
       *	second choice is edge leading to node with at least one
       *	other in-cycle parent
       */
      maxarcp = maxwithparentarcp;
      DBG (BREAKCYCLE, type = "internal");
    }
  else
    {
      /*
       *	last choice is edge leading to node with only this arc as
       *	a parent (as it will now be orphaned)
       */
      maxarcp = maxnoparentarcp;
      DBG (BREAKCYCLE, type = "orphan");
    }
  maxarcp->arc_flags |= DEADARC;
  maxarcp->child->cg.cyc.parentcnt -= 1;
  maxarcp->child->cg.npropcall -= maxarcp->count;
  DBG (BREAKCYCLE,
       printf ("%s delete %s arc: %s (%lu) -> %s from %u cycle(s)\n",
	       "[compresslist]", type, maxarcp->parent->name,
	       maxarcp->count, maxarcp->child->name,
	       (unsigned)maxarcp->arc_cyclecnt));
  printf ("\t%s to %s with %lu calls\n", maxarcp->parent->name,
	  maxarcp->child->name, maxarcp->count);
  prev = &cyclehead;
  for (clp = cyclehead; clp; )
    {
      endlist = &clp->list[clp->size];
      for (arcpp = clp->list; arcpp < endlist; arcpp++)
	{
	  if ((*arcpp)->arc_flags & DEADARC)
	    {
	      break;
	    }
	}
      if (arcpp == endlist)
	{
	  prev = &clp->next;
	  clp = clp->next;
	  continue;
	}
      for (arcpp = clp->list; arcpp < endlist; arcpp++)
	{
	  (*arcpp)->arc_cyclecnt--;
	}
      cyclecnt--;
      *prev = clp->next;
      clp = clp->next;
      free (clp);
    }
}

void
printsubcycle (clp)
     Cl *clp;
{
  Arc **arcpp;
  Arc **endlist;

  arcpp = clp->list;
  printf ("%s <cycle %d>\n", (*arcpp)->parent->name,
	  (*arcpp)->parent->cg.cyc.num);
  for (endlist = &clp->list[clp->size]; arcpp < endlist; arcpp++)
    {
      printf ("\t(%lu) -> %s\n", (*arcpp)->count,
	      (*arcpp)->child->name);
    }
}

/*
 * Check if any parent of this child (or outside parents of this
 * cycle) have their print flags on and set the print flag of the
 * child (cycle) appropriately.  Similarly, deal with propagation
 * fractions from parents.
 */
static void
inherit_flags (child)
     Sym *child;
{
  Sym *head, *parent, *member;
  Arc *arc;

  head = child->cg.cyc.head;
  if (child == head)
    {
      /* just a regular child, check its parents: */
      child->cg.print_flag = false;
      child->cg.prop.fract = 0.0;
      for (arc = child->cg.parents; arc; arc = arc->next_parent)
	{
	  parent = arc->parent;
	  if (child == parent)
	    {
	      continue;
	    }
	  child->cg.print_flag |= parent->cg.print_flag;
	  /*
	   * If the child was never actually called (e.g., this arc
	   * is static (and all others are, too)) no time propagates
	   * along this arc.
	   */
	  if ((arc->arc_flags & DEADARC) == 0 && child->cg.npropcall != 0)
	    {
	      child->cg.prop.fract += parent->cg.prop.fract
		* (((double) arc->count) / ((double) child->cg.npropcall));
	    }
	}
    }
  else
    {
      /*
       * Its a member of a cycle, look at all parents from outside
       * the cycle.
       */
      head->cg.print_flag = false;
      head->cg.prop.fract = 0.0;
      for (member = head->cg.cyc.next; member; member = member->cg.cyc.next)
	{
	  for (arc = member->cg.parents; arc; arc = arc->next_parent)
	    {
	      if (arc->parent->cg.cyc.head == head)
		{
		  continue;
		}
	      parent = arc->parent;
	      head->cg.print_flag |= parent->cg.print_flag;
	      /*
	       * If the cycle was never actually called (e.g. this
	       * arc is static (and all others are, too)) no time
	       * propagates along this arc.
	       */
	      if ((arc->arc_flags & DEADARC) == 0 && head->cg.npropcall != 0)
		{
		  head->cg.prop.fract += parent->cg.prop.fract
		    * (((double) arc->count) / ((double) head->cg.npropcall));
		}
	    }
	}
      for (member = head; member; member = member->cg.cyc.next)
	{
	  member->cg.print_flag = head->cg.print_flag;
	  member->cg.prop.fract = head->cg.prop.fract;
	}
    }
}


/*
 * In one top-to-bottom pass over the topologically sorted symbols
 * propagate:
 *      cg.print_flag as the union of parents' print_flags
 *      propfraction as the sum of fractional parents' propfractions
 * and while we're here, sum time for functions.
 */
static void
propagate_flags (symbols)
     Sym **symbols;
{
  int index;
  Sym *old_head, *child;

  old_head = 0;
  for (index = symtab.len - 1; index >= 0; --index)
    {
      child = symbols[index];
      /*
       * If we haven't done this function or cycle, inherit things
       * from parent.  This way, we are linear in the number of arcs
       * since we do all members of a cycle (and the cycle itself)
       * as we hit the first member of the cycle.
       */
      if (child->cg.cyc.head != old_head)
	{
	  old_head = child->cg.cyc.head;
	  inherit_flags (child);
	}
      DBG (PROPDEBUG,
	   printf ("[prop_flags] ");
	   print_name (child);
	   printf ("inherits print-flag %d and prop-fract %f\n",
		   child->cg.print_flag, child->cg.prop.fract));
      if (!child->cg.print_flag)
	{
	  /*
	   * Printflag is off. It gets turned on by being in the
	   * INCL_GRAPH table, or there being an empty INCL_GRAPH
	   * table and not being in the EXCL_GRAPH table.
	   */
	  if (sym_lookup (&syms[INCL_GRAPH], child->addr)
	      || (syms[INCL_GRAPH].len == 0
		  && !sym_lookup (&syms[EXCL_GRAPH], child->addr)))
	    {
	      child->cg.print_flag = true;
	    }
	}
      else
	{
	  /*
	   * This function has printing parents: maybe someone wants
	   * to shut it up by putting it in the EXCL_GRAPH table.
	   * (But favor INCL_GRAPH over EXCL_GRAPH.)
	   */
	  if (!sym_lookup (&syms[INCL_GRAPH], child->addr)
	      && sym_lookup (&syms[EXCL_GRAPH], child->addr))
	    {
	      child->cg.print_flag = false;
	    }
	}
      if (child->cg.prop.fract == 0.0)
	{
	  /*
	   * No parents to pass time to.  Collect time from children
	   * if its in the INCL_TIME table, or there is an empty
	   * INCL_TIME table and its not in the EXCL_TIME table.
	   */
	  if (sym_lookup (&syms[INCL_TIME], child->addr)
	      || (syms[INCL_TIME].len == 0
		  && !sym_lookup (&syms[EXCL_TIME], child->addr)))
	    {
	      child->cg.prop.fract = 1.0;
	    }
	}
      else
	{
	  /*
	   * It has parents to pass time to, but maybe someone wants
	   * to shut it up by puttting it in the EXCL_TIME table.
	   * (But favor being in INCL_TIME tabe over being in
	   * EXCL_TIME table.)
	   */
	  if (!sym_lookup (&syms[INCL_TIME], child->addr)
	      && sym_lookup (&syms[EXCL_TIME], child->addr))
	    {
	      child->cg.prop.fract = 0.0;
	    }
	}
      child->cg.prop.self = child->hist.time * child->cg.prop.fract;
      print_time += child->cg.prop.self;
      DBG (PROPDEBUG,
	   printf ("[prop_flags] ");
	   print_name (child);
	   printf (" ends up with printflag %d and prop-fract %f\n",
		   child->cg.print_flag, child->cg.prop.fract);
	   printf ("[prop_flags] time %f propself %f print_time %f\n",
		   child->hist.time, child->cg.prop.self, print_time));
    }
}


/*
 * Compare by decreasing propagated time.  If times are equal, but one
 * is a cycle header, say that's first (e.g. less, i.e. -1).  If one's
 * name doesn't have an underscore and the other does, say that one is
 * first.  All else being equal, compare by names.
 */
static int
cmp_total (lp, rp)
     const PTR lp;
     const PTR rp;
{
  const Sym *left = *(const Sym **) lp;
  const Sym *right = *(const Sym **) rp;
  double diff;

  diff = (left->cg.prop.self + left->cg.prop.child)
    - (right->cg.prop.self + right->cg.prop.child);
  if (diff < 0.0)
    {
      return 1;
    }
  if (diff > 0.0)
    {
      return -1;
    }
  if (!left->name && left->cg.cyc.num != 0)
    {
      return -1;
    }
  if (!right->name && right->cg.cyc.num != 0)
    {
      return 1;
    }
  if (!left->name)
    {
      return -1;
    }
  if (!right->name)
    {
      return 1;
    }
  if (left->name[0] != '_' && right->name[0] == '_')
    {
      return -1;
    }
  if (left->name[0] == '_' && right->name[0] != '_')
    {
      return 1;
    }
  if (left->ncalls > right->ncalls)
    {
      return -1;
    }
  if (left->ncalls < right->ncalls)
    {
      return 1;
    }
  return strcmp (left->name, right->name);
}


/*
 * Topologically sort the graph (collapsing cycles), and propagates
 * time bottom up and flags top down.
 */
Sym **
cg_assemble ()
{
  Sym *parent, **time_sorted_syms, **top_sorted_syms;
  unsigned int index;
  Arc *arc;
  long pass;

  /*
   * initialize various things:
   *      zero out child times.
   *      count self-recursive calls.
   *      indicate that nothing is on cycles.
   */
  for (parent = symtab.base; parent < symtab.limit; parent++)
    {
      parent->cg.child_time = 0.0;
      arc = arc_lookup (parent, parent);
      if (arc && parent == arc->child)
	{
	  parent->ncalls -= arc->count;
	  parent->cg.self_calls = arc->count;
	}
      else
	{
	  parent->cg.self_calls = 0;
	}
      parent->cg.prop.fract = 0.0;
      parent->cg.prop.self = 0.0;
      parent->cg.prop.child = 0.0;
      parent->cg.print_flag = false;
      parent->cg.top_order = DFN_NAN;
      parent->cg.cyc.num = 0;
      parent->cg.cyc.head = parent;
      parent->cg.cyc.next = 0;
      if (ignore_direct_calls)
	{
	  find_call (parent, parent->addr, (parent + 1)->addr);
	}
    }
  for (pass = 1; ; pass++)
    {
      /*
       * Topologically order things.  If any node is unnumbered, number
       * it and any of its descendents.
       */
      for (dfn_init (), parent = symtab.base; parent < symtab.limit; parent++)
	{
	  if (parent->cg.top_order == DFN_NAN)
	    {
	      cg_dfn (parent);
	    }
	}

      /* link together nodes on the same cycle: */
      cycle_link ();
      /* if no cycles to break up, proceed */
      if (!eliminate_cycles)
	{
	  break;
	}
      if (pass == 1)
	{
	  printf ("\n\n%s %s\n%s %d:\n",
		  "The following arcs were deleted",
		  "from the propagation calculation",
		  "to reduce the maximum cycle size to", cyclethreshold);
	}
      if (cycleanalyze ())
	{
	  break;
	}
      free (cycle_header);
      num_cycles = 0;
      for (parent = symtab.base; parent < symtab.limit; parent++)
	{
	  parent->cg.top_order = DFN_NAN;
	  parent->cg.cyc.num = 0;
	  parent->cg.cyc.head = parent;
	  parent->cg.cyc.next = 0;
	}
    }
  if (pass > 1)
    {
      printf("\f\n");
    }

  /* sort the symbol table in reverse topological order: */
  top_sorted_syms = (Sym **) xmalloc (symtab.len * sizeof (Sym *));
  for (index = 0; index < symtab.len; ++index)
    {
      top_sorted_syms[index] = &symtab.base[index];
    }
  qsort (top_sorted_syms, symtab.len, sizeof (Sym *), cmp_topo);
  DBG (DFNDEBUG,
       printf ("[cg_assemble] topological sort listing\n");
       for (index = 0; index < symtab.len; ++index)
       {
       printf ("[cg_assemble] ");
       printf ("%d:", top_sorted_syms[index]->cg.top_order);
       print_name (top_sorted_syms[index]);
       printf ("\n");
       }
  );
  /*
   * Starting from the topological top, propagate print flags to
   * children.  also, calculate propagation fractions.  this happens
   * before time propagation since time propagation uses the
   * fractions.
   */
  propagate_flags (top_sorted_syms);

  /*
   * Starting from the topological bottom, propogate children times
   * up to parents.
   */
  cycle_time ();
  for (index = 0; index < symtab.len; ++index)
    {
      propagate_time (top_sorted_syms[index]);
    }

  free (top_sorted_syms);

  /*
   * Now, sort by CG.PROP.SELF + CG.PROP.CHILD.  Sorting both the regular
   * function names and cycle headers.
   */
  time_sorted_syms = (Sym **) xmalloc ((symtab.len + num_cycles) * sizeof (Sym *));
  for (index = 0; index < symtab.len; index++)
    {
      time_sorted_syms[index] = &symtab.base[index];
    }
  for (index = 1; index <= num_cycles; index++)
    {
      time_sorted_syms[symtab.len + index - 1] = &cycle_header[index];
    }
  qsort (time_sorted_syms, symtab.len + num_cycles, sizeof (Sym *),
	 cmp_total);
  for (index = 0; index < symtab.len + num_cycles; index++)
    {
      time_sorted_syms[index]->cg.index = index + 1;
    }
  return time_sorted_syms;
}
