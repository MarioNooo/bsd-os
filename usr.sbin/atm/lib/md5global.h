/*
 *
 * ===================================
 * HARP  |  Host ATM Research Platform
 * ===================================
 *
 *
 * Copyright (c) 1996-1998, Network Computing Services, Inc.
 * All rights reserved.
 *
 *      @(#) md5global.h,v 1.1 1998/07/09 21:46:21 johnc Exp
 *
 */

/*
 * User Space Library Functions
 * ----------------------------
 *
 * RSAREF header file
 *
 */

/*
 * GLOBAL.H - RSAREF types and constants
 */

/*
 * PROTOTYPES should be set to one if and only if the compiler supports
 * function argument prototyping.
 * The following makes PROTOTYPES default to 0 if it has not already
 * been defined with C compiler flags.
 */
#ifndef PROTOTYPES
#define PROTOTYPES 0
#endif

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT2 defines a two byte word */
typedef unsigned short int UINT2;

/* UINT4 defines a four byte word */
typedef unsigned long int UINT4;

/* PROTO_LIST is defined depending on how PROTOTYPES is defined above.
If using PROTOTYPES, then PROTO_LIST returns the list, otherwise it
  returns an empty list.
 */
#if PROTOTYPES
#define PROTO_LIST(list) list
#else
#define PROTO_LIST(list) ()
#endif
