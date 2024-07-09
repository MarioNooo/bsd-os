/*	BSDI math.h,v 1.1 1995/07/10 18:21:17 donn Exp	*/

#ifndef	_SCO_MATH_H_
#define	_SCO_MATH_H_

#define	HUGE_VAL	(1e250*1e250)

#define	M_E		2.7182818284590452354	/* e */
#define	M_LOG2E		1.4426950408889634074	/* log 2e */
#define	M_LOG10E	0.43429448190325182765	/* log 10e */
#define	M_LN2		0.69314718055994530942	/* log e2 */
#define	M_LN10		2.30258509299404568402	/* log e10 */
#define	M_PI		3.14159265358979323846	/* pi */
#define	M_PI_2		1.57079632679489661923	/* pi/2 */
#define	M_PI_4		0.78539816339744830962	/* pi/4 */
#define	M_1_PI		0.31830988618379067154	/* 1/pi */
#define	M_2_PI		0.63661977236758134308	/* 2/pi */
#define	M_2_SQRTPI	1.12837916709551257390	/* 2/sqrt(pi) */
#define	M_SQRT2		1.41421356237309504880	/* sqrt(2) */
#define	M_SQRT1_2	0.70710678118654752440	/* 1/sqrt(2) */

/* prototypes required by POSIX.1 */
#include <sys/cdefs.h>

double acos __P((double));
double asin __P((double));
double atan __P((double));
double atan2 __P((double, double));
double ceil __P((double));
double cos __P((double));
double cosh __P((double));
double exp __P((double));
double fabs __P((double));
double floor __P((double));
double fmod __P((double, double));
double frexp __P((double, int *));;
double ldexp __P((double, int));
double log __P((double));
double log10 __P((double));
double modf __P((double, double *));;
double pow __P((double, double));
double sin __P((double));
double sinh __P((double));
double sqrt __P((double));
double tan __P((double));
double tanh __P((double));

#endif /* _SCO_MATH_H_ */
