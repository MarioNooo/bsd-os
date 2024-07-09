/*
 * Copyright (c) 2003 QUALCOMM Incorporated.  All rights reserved.
 * The license.txt file specifies the terms and conditions for
 * modifications and redistribution.
 *
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 */

/*
 *  Current version of Qpopper
 */

#define VERS_NUM          "4.0.5"

#ifdef KERBEROS
#  ifdef KRB4
#    define VERS_SUF1     "-krb-IV"
#  endif /* KRB4 */

#  ifdef KRB5
#    define VERS_SUF1     "-krb-V"
#  endif /* KRB5 */

#else /* not KERBEROS */
#  define VERS_SUF1       ""
#endif /* KERBEROS */

#ifdef _DEBUG
#  define VERS_SUF2       "_DEBUG"
#else
#  define VERS_SUF2       ""
#endif

#define VERSION         VERS_NUM VERS_SUF1 VERS_SUF2      
