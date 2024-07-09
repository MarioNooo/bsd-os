/*	BSDI sharedlib.h,v 1.1 1997/06/10 23:27:49 donn Exp	*/

/*
 * Linux compatibility.
 * Some of these are just guesses that need to be confirmed later.
 */

#define	MAJOR_MASK	0xffff0000
#define	MINOR_MASK	0x0000ffff

struct libentry {
	char *name;
	caddr_t addr;
	int vers;
	char *avers;
};
