/*
 * Copyright (c) 2000-2001 Sendmail, Inc. and its suppliers.
 *	All rights reserved.
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the sendmail distribution.
 *
 *	test.h,v 1.3 2003/09/17 21:19:19 polk Exp
 */

/*
**  Abstractions for writing a libsm test program.
*/

#ifndef SM_TEST_H
# define SM_TEST_H

# include <sm/gen.h>

# if defined(__STDC__) || defined(__cplusplus)
#  define SM_TEST(cond) sm_test(cond, #cond, __FILE__, __LINE__)
# else /* defined(__STDC__) || defined(__cplusplus) */
#  define SM_TEST(cond) sm_test(cond, "cond", __FILE__, __LINE__)
# endif /* defined(__STDC__) || defined(__cplusplus) */

extern int SmTestIndex;
extern int SmTestNumErrors;

extern void
sm_test_begin __P((
	int _argc,
	char **_argv,
	char *_testname));

extern bool
sm_test __P((
	bool _success,
	char *_expr,
	char *_filename,
	int _lineno));

extern int
sm_test_end __P((void));

#endif /* ! SM_TEST_H */
