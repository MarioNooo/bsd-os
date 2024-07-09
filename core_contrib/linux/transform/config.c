/*	BSDI config.c,v 1.3 2000/12/08 02:59:56 donn Exp	*/

/*
 * OS-specific code.
 */

/* The foreign OS's typedefs and defines have these prefixes.  */
const char foreign[] = "linux_";
const char FOREIGN[] = "LINUX_";

/* Native prefixes may appear on global symbols, so they need underscores.  */
const char native[] = "__bsdi_";
const char NATIVE[] = "__BSDI_";

/* Prefix for kernel vs. library versions of syscalls.  */
const char kernel[] = "__kernel_";
const char library[] = "__library_";
