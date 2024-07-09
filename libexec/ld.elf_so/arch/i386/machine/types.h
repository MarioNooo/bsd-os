/*	BSDI types.h,v 1.2 2003/08/22 16:42:34 polk Exp	*/

/*
 * Mate the c99 type system of NetBSD to our old type headers.
 */

#ifndef _MACHINE_TYPES_H_
#define	_MACHINE_TYPES_H_ 1

#include <inttypes.h>

typedef	unsigned long	vm_offset_t;
typedef	unsigned long	vm_size_t;

typedef	__uint8_t	u_int8_t;
typedef	__uint16_t	u_int16_t;
typedef	__uint32_t	u_int32_t;
typedef	__uint64_t	u_int64_t;

typedef	__uint16_t	u_int16m_t;
typedef	__uint32_t	u_int32m_t;
typedef	__uint64_t	u_int64m_t;

#endif