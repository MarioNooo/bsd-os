/*	BSDI libc-dummy.c,v 1.2 2001/01/23 04:01:46 donn Exp	*/

/*
 * The dummy C library has one main job.
 *
 * It loads the emulation library and the GNU C library as auxiliary filters.
 * This forces the proper search ordering between the two libraries,
 * and guarantees that we'll get that ordering even if some code asks
 * for 'libc.so.6' as a dependency, or even using dlopen().
 *
 * The following function exists just so that the library has some text in it.
 */

void __bsdi_dummy_C_library() { }
