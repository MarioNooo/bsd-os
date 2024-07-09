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
 * curve.h
 *
 * 2003/07/09 17:11:14 1.2
 * /master/core_contrib/xc/extras/ogl-sample/main/gfx/lib/glu/libnurbs/internals/curve.h,v 1.2 2003/07/09 17:11:14 polk Exp
 */

#ifndef __glucurve_h_
#define __glucurve_h_

#include "types.h"
#include "defines.h"

class Mapdesc;
class Quilt;


class Curve {
public:
friend class Curvelist;
    			Curve( Quilt *, REAL, REAL, Curve * );
    			Curve( Curve&, REAL, Curve * );
    Curve *		next;
private:
    Mapdesc *		mapdesc;
    int			stride;
    int		        order;
    int			cullval;
    int			needsSampling;
    REAL		cpts[MAXORDER*MAXCOORDS];
    REAL		spts[MAXORDER*MAXCOORDS];
    REAL		stepsize;
    REAL		minstepsize;
    REAL		range[3];

    void		clamp( void );
    void		setstepsize( REAL );
    void		getstepsize( void );
    int			cullCheck( void );
    int			needsSamplingSubdivision( void );
};
#endif /* __glucurve_h_ */
