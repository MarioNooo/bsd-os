/*	BSDI personality.x,v 1.3 1999/03/11 22:54:40 prb Exp	*/

/*
 * The personality() syscall transform.
 */

/*
 * The parameter is more complex than this, but we only care about this case.
 */
cookie int linux_pers_t {
	PERS_LINUX	0;
};

int personality(PERS_LINUX) = 0;
int personality(linux_pers_t p) = EINVAL;
