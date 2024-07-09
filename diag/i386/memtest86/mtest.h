/* mtest.h - MemTest-86 defines */

/* Copyright 1995,  Chris Brady
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without fee
 * is granted provided that the above copyright notice appears in all copies.
 * It is provided "as is" without express or implied warranty.
 */ 

#define SETUPSECS	1		/* Number of setup sectors */
#define BOOTSEG		0x07c0		/* Segment adrs for inital boot */
#define INITSEG		0x9000		/* Segment adrs for relocated boot */
#define SETUPSEG	INITSEG+0x20	/* Segment adrs for relocated setup */
#define TSTLOAD		0x1000		/* Segment adrs for load of test */
#define TESTADR		0x0100		/* Final adrs for the test code */
#define TESTSEG		TESTADR/0x10	/* Final segment adrs for test code */
#define TSTSIZE		0x2000		/* Size of the test code */
#define KERNEL_CS	0x10		/* 32 bit segment adrs for code */
#define KERNEL_DS	0x18		/* 32 bit segment adrs for data */
#define START_ADR	0x0e00		/* Starting adrs for testing */
