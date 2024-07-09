/**************************************************************************
 * LPRng IFHP Filter
 * Copyright 1994-1999 Patrick Powell, San Diego, CA <papowell@astart.com>
 **************************************************************************/
/**** HEADER *****
open_device.h,v 1.3 2000/04/19 03:18:32 jch Exp
 **** HEADER *****/

#if !defined(_OPEN_DEVICE_H_)
#define  _OPEN_DEVICE_H_  1
/* PROTOTYPES */
void Open_device( char *device );
void Set_keepalive( int sock );
void Open_monitor( char *device );

#endif
