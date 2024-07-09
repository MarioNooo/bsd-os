/*
 * Copyright (c) 1993,1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI emulate.h,v 2.2 1997/11/01 04:50:14 donn Exp
 */

/*
 * Emulator-wide definitions.
 */

#define	DEFAULT_STACKSIZE	8192

/*
 * A utility macro for transform.c.
 * Skip our return PC and the syscall stub frame return PC.
 * Note that we use 1-based addressing.
 */
#define	ARG(stack,n,type)	((type)((int *)stack)[(n)+2])

/*
 * Structure declarations.
 */

/*
 * This structure controls bit translations between flag words.
 * x_common is a mask of bits that are common between the two formats.
 * x_map is a vector of pairs of bitmasks, where the first element
 * in each pair holds a bit that translates into the second element.
 * The x_map vector is zero-terminated.
 */
struct xbits {
	unsigned long		x_common;
	const unsigned long	*x_map;
};

/*
 * Global variables.
 */

extern vm_offset_t emulate_stack;
extern vm_offset_t emulate_break_low;
extern vm_offset_t emulate_break_high;

#ifdef DEBUG
extern int debug;
#define	DEBUG_SYSCALLS		0x01
#define	DEBUG_SEMAPHORES	0x02
#define	DEBUG_FILEMAP		0x04
#define	DEBUG_BREAKPOINT	0x08
#endif

/*
 * Function prototypes.
 */

void bsd_emulate_glue __P((void));
void emulate_glue __P((void));
void emulate_init_break __P((void));
#ifdef RUNSCO
void emulate_launch __P((vm_offset_t, vm_offset_t));
void emulate_start __P((int, vm_offset_t, vm_offset_t, vm_offset_t));
void program_init_break __P((vm_offset_t));
#else
int emulate_start __P((void));
void program_init_break __P((void));
#endif
int nosys(int);

/*
 * Support routines.
 */

void *smalloc __P((size_t));
void sfree __P((void *));

/*
 * Support for the syscall switch.
 */
extern int (*low_syscall[])();
extern int (*high_syscall[])();

extern int low_syscall_max;
extern int high_syscall_min;
extern int high_syscall_max;
