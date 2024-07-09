/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
**
** http://oss.sgi.com/projects/FreeB
**
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
**
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
**
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
*/

/*
 * pwlarc.h
 *
 * 2003/07/09 17:11:18 1.2
 * /master/core_contrib/xc/extras/ogl-sample/main/gfx/lib/glu/libnurbs/internals/pwlarc.h,v 1.2 2003/07/09 17:11:18 polk Exp
 */

#ifndef __glupwlarc_h_
#define __glupwlarc_h_

#include "myassert.h"
#include "nurbsconsts.h"

class TrimVertex;

class PwlArc : public PooledObj { /* a piecewise-linear arc */
public:
    TrimVertex *	pts;		/* sample points */
    int			npts;		/* number of sample points */
    long		type;		/* curve type */
    inline		PwlArc( void );
    inline		PwlArc( int, TrimVertex * );
    inline		PwlArc( int, TrimVertex *, long );
};

inline
PwlArc::PwlArc( void )
{
    type = N_P2D;
    pts = 0;
    npts = -1;
}

inline
PwlArc::PwlArc( int _npts, TrimVertex *_pts )
{
    pts = _pts;
    npts = _npts;
    type = N_P2D;
}

inline
PwlArc::PwlArc( int _npts, TrimVertex *_pts, long _type )
{
    pts = _pts;
    npts = _npts;
    type = _type;
}

#endif /* __glupwlarc_h_ */
