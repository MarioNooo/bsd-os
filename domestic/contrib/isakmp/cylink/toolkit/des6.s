 /*
 * This cryptographic library is offered under the following license:
 * 
 *      "Cylink Corporation, through its wholly owned subsidiary Caro-Kann
 * Corporation, holds exclusive sublicensing rights to the following U.S.
 * patents owned by the Leland Stanford Junior University:
 *  
 *         Cryptographic Apparatus and Method
 *         ("Hellman-Diffie") ............................  No. 4,200,770 
 *  
 *         Public Key Cryptographic Apparatus
 *         and Method ("Hellman-Merkle") .................. No. 4,218, 582
 *  
 *         In order to promote the widespread use of these inventions from
 * Stanford University and adoption of the ISAKMP reference by the IETF
 * community, Cisco has acquired the right under its sublicense from Cylink 
to
 * offer the ISAKMP reference implementation to all third parties on a 
royalty
 * free basis.  This royalty free privilege is limited to use of the ISAKMP
 * reference implementation in accordance with proposed, pending or 
approved
 * IETF standards, and applies only when used with Diffie-Hellman key
 * exchange, the Digital Signature Standard, or any other public key
 * techniques which are publicly available for commercial use on a royalty
 * free basis.  Any use of the ISAKMP reference implementation which does 
not
 * satisfy these conditions and incorporates the practice of public key may
 * require a separate patent license to the Stanford Patents which must be
 * negotiated with Cylink's subsidiary, Caro-Kann Corporation."
 * 
 * Disclaimer of All Warranties  THIS SOFTWARE IS BEING PROVIDED "AS IS", 
 * WITHOUT ANY EXPRESS OR IMPLIED WARRANTY OF ANY KIND WHATSOEVER.
 * IN PARTICULAR, WITHOUT LIMITATION ON THE GENERALITY OF THE FOREGOING,  
 * CYLINK MAKES NO REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING 
 * THE MERCHANTABILITY OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR
 * PURPOSE.
 *
 * Cylink or its representatives shall not be liable for tort, indirect, 
 * special or consequential damages such as loss of profits or loss of 
goodwill 
 * from the use or inability to use the software for any purpose or for any 
 * reason whatsoever.
 *
 *******************************************************************
 * This cryptographic library incorporates the BigNum multiprecision 
 * integer math library by Colin Plumb.
 *
 * BigNum has been licensed by Cylink Corporation 
 * from Philip Zimmermann.
 *
 * For BigNum licensing information, please contact Philip Zimmermann
 * (prz@acm.org, +1 303 541-0140). 
 *******************************************************************
 * 
*/

! DES   Target = SPARC, C linkage, constant speed, minimize memory references
        .seg    "text"

 .ascii "(C) Copyright 1979, 1984-1994  Richard H. Gumpertz, Leawood, KS "

        .align  4
S1:
        .word   0x00404100,0x00000000,0x00004000,0x00404101
        .word   0x00404001,0x00004101,0x00000001,0x00004000
        .word   0x00000100,0x00404100,0x00404101,0x00000100
        .word   0x00400101,0x00404001,0x00400000,0x00000001
        .word   0x00000101,0x00400100,0x00400100,0x00004100
        .word   0x00004100,0x00404000,0x00404000,0x00400101
        .word   0x00004001,0x00400001,0x00400001,0x00004001
        .word   0x00000000,0x00000101,0x00004101,0x00400000
        .word   0x00004000,0x00404101,0x00000001,0x00404000
        .word   0x00404100,0x00400000,0x00400000,0x00000100
        .word   0x00404001,0x00004000,0x00004100,0x00400001
        .word   0x00000100,0x00000001,0x00400101,0x00004101
        .word   0x00404101,0x00004001,0x00404000,0x00400101
        .word   0x00400001,0x00000101,0x00004101,0x00404100
        .word   0x00000101,0x00400100,0x00400100,0x00000000
        .word   0x00004001,0x00004100,0x00000000,0x00404001
S2:
        .word   0x20042008,0x20002000,0x00002000,0x00042008
        .word   0x00040000,0x00000008,0x20040008,0x20002008
        .word   0x20000008,0x20042008,0x20042000,0x20000000
        .word   0x20002000,0x00040000,0x00000008,0x20040008
        .word   0x00042000,0x00040008,0x20002008,0x00000000
        .word   0x20000000,0x00002000,0x00042008,0x20040000
        .word   0x00040008,0x20000008,0x00000000,0x00042000
        .word   0x00002008,0x20042000,0x20040000,0x00002008
        .word   0x00000000,0x00042008,0x20040008,0x00040000
        .word   0x20002008,0x20040000,0x20042000,0x00002000
        .word   0x20040000,0x20002000,0x00000008,0x20042008
        .word   0x00042008,0x00000008,0x00002000,0x20000000
        .word   0x00002008,0x20042000,0x00040000,0x20000008
        .word   0x00040008,0x20002008,0x20000008,0x00040008
        .word   0x00042000,0x00000000,0x20002000,0x00002008
        .word   0x20000000,0x20040008,0x20042008,0x00042000
S3:
        .word   0x00000082,0x02008080,0x00000000,0x02008002
        .word   0x02000080,0x00000000,0x00008082,0x02000080
        .word   0x00008002,0x02000002,0x02000002,0x00008000
        .word   0x02008082,0x00008002,0x02008000,0x00000082
        .word   0x02000000,0x00000002,0x02008080,0x00000080
        .word   0x00008080,0x02008000,0x02008002,0x00008082
        .word   0x02000082,0x00008080,0x00008000,0x02000082
        .word   0x00000002,0x02008082,0x00000080,0x02000000
        .word   0x02008080,0x02000000,0x00008002,0x00000082
        .word   0x00008000,0x02008080,0x02000080,0x00000000
        .word   0x00000080,0x00008002,0x02008082,0x02000080
        .word   0x02000002,0x00000080,0x00000000,0x02008002
        .word   0x02000082,0x00008000,0x02000000,0x02008082
        .word   0x00000002,0x00008082,0x00008080,0x02000002
        .word   0x02008000,0x02000082,0x00000082,0x02008000
        .word   0x00008082,0x00000002,0x02008002,0x00008080
S4:
        .word   0x40200800,0x40000820,0x40000820,0x00000020
        .word   0x00200820,0x40200020,0x40200000,0x40000800
        .word   0x00000000,0x00200800,0x00200800,0x40200820
        .word   0x40000020,0x00000000,0x00200020,0x40200000
        .word   0x40000000,0x00000800,0x00200000,0x40200800
        .word   0x00000020,0x00200000,0x40000800,0x00000820
        .word   0x40200020,0x40000000,0x00000820,0x00200020
        .word   0x00000800,0x00200820,0x40200820,0x40000020
        .word   0x00200020,0x40200000,0x00200800,0x40200820
        .word   0x40000020,0x00000000,0x00000000,0x00200800
        .word   0x00000820,0x00200020,0x40200020,0x40000000
        .word   0x40200800,0x40000820,0x40000820,0x00000020
        .word   0x40200820,0x40000020,0x40000000,0x00000800
        .word   0x40200000,0x40000800,0x00200820,0x40200020
        .word   0x40000800,0x00000820,0x00200000,0x40200800
        .word   0x00000020,0x00200000,0x00000800,0x00200820
S5:
        .word   0x00000040,0x00820040,0x00820000,0x10800040
        .word   0x00020000,0x00000040,0x10000000,0x00820000
        .word   0x10020040,0x00020000,0x00800040,0x10020040
        .word   0x10800040,0x10820000,0x00020040,0x10000000
        .word   0x00800000,0x10020000,0x10020000,0x00000000
        .word   0x10000040,0x10820040,0x10820040,0x00800040
        .word   0x10820000,0x10000040,0x00000000,0x10800000
        .word   0x00820040,0x00800000,0x10800000,0x00020040
        .word   0x00020000,0x10800040,0x00000040,0x00800000
        .word   0x10000000,0x00820000,0x10800040,0x10020040
        .word   0x00800040,0x10000000,0x10820000,0x00820040
        .word   0x10020040,0x00000040,0x00800000,0x10820000
        .word   0x10820040,0x00020040,0x10800000,0x10820040
        .word   0x00820000,0x00000000,0x10020000,0x10800000
        .word   0x00020040,0x00800040,0x10000040,0x00020000
        .word   0x00000000,0x10020000,0x00820040,0x10000040
S6:
        .word   0x08000004,0x08100000,0x00001000,0x08101004
        .word   0x08100000,0x00000004,0x08101004,0x00100000
        .word   0x08001000,0x00101004,0x00100000,0x08000004
        .word   0x00100004,0x08001000,0x08000000,0x00001004
        .word   0x00000000,0x00100004,0x08001004,0x00001000
        .word   0x00101000,0x08001004,0x00000004,0x08100004
        .word   0x08100004,0x00000000,0x00101004,0x08101000
        .word   0x00001004,0x00101000,0x08101000,0x08000000
        .word   0x08001000,0x00000004,0x08100004,0x00101000
        .word   0x08101004,0x00100000,0x00001004,0x08000004
        .word   0x00100000,0x08001000,0x08000000,0x00001004
        .word   0x08000004,0x08101004,0x00101000,0x08100000
        .word   0x00101004,0x08101000,0x00000000,0x08100004
        .word   0x00000004,0x00001000,0x08100000,0x00101004
        .word   0x00001000,0x00100004,0x08001004,0x00000000
        .word   0x08101000,0x08000000,0x00100004,0x08001004
S7:
        .word   0x00080000,0x81080000,0x81000200,0x00000000
        .word   0x00000200,0x81000200,0x80080200,0x01080200
        .word   0x81080200,0x00080000,0x00000000,0x81000000
        .word   0x80000000,0x01000000,0x81080000,0x80000200
        .word   0x01000200,0x80080200,0x80080000,0x01000200
        .word   0x81000000,0x01080000,0x01080200,0x80080000
        .word   0x01080000,0x00000200,0x80000200,0x81080200
        .word   0x00080200,0x80000000,0x01000000,0x00080200
        .word   0x01000000,0x00080200,0x00080000,0x81000200
        .word   0x81000200,0x81080000,0x81080000,0x80000000
        .word   0x80080000,0x01000000,0x01000200,0x00080000
        .word   0x01080200,0x80000200,0x80080200,0x01080200
        .word   0x80000200,0x81000000,0x81080200,0x01080000
        .word   0x00080200,0x00000000,0x80000000,0x81080200
        .word   0x00000000,0x80080200,0x01080000,0x00000200
        .word   0x81000000,0x01000200,0x00000200,0x80080000
S8:
        .word   0x04000410,0x00000400,0x00010000,0x04010410
        .word   0x04000000,0x04000410,0x00000010,0x04000000
        .word   0x00010010,0x04010000,0x04010410,0x00010400
        .word   0x04010400,0x00010410,0x00000400,0x00000010
        .word   0x04010000,0x04000010,0x04000400,0x00000410
        .word   0x00010400,0x00010010,0x04010010,0x04010400
        .word   0x00000410,0x00000000,0x00000000,0x04010010
        .word   0x04000010,0x04000400,0x00010410,0x00010000
        .word   0x00010410,0x00010000,0x04010400,0x00000400
        .word   0x00000010,0x04010010,0x00000400,0x00010410
        .word   0x04000400,0x00000010,0x04000010,0x04010000
        .word   0x04010010,0x04000000,0x00010000,0x04000410
        .word   0x00000000,0x04010410,0x00010010,0x04000010
        .word   0x04010000,0x04000400,0x04000410,0x00000000
        .word   0x04010410,0x00010400,0x00010400,0x00000410
        .word   0x00000410,0x00010010,0x04000000,0x04010400

        .global Encrypt
Encrypt:
        save    %sp,-64,%sp
        ld      [%i1],%g2
        sub     %i0,128,%i3
        set     S7,%g4
        ld      [%i1+4],%g3
        srl     %g2,4,%o7
        sll     %g2,28,%g2
        or      %o7,%g2,%g2
        xor     %g2,%g3,%g1
        set     0x0F0F0F0F,%o0
        and     %g1,%o0,%g1
        xor     %g1,%g2,%g2
        xor     %g1,%g3,%g3
        srl     %g2,12,%o7
        sll     %g2,20,%g2
        or      %o7,%g2,%g2
        xor     %g2,%g3,%g1
        set     0x0FFFF0000,%o1
        andn    %g1,%o1,%g1
        xor     %g1,%g2,%g2
        xor     %g1,%g3,%g3
        srl     %g2,14,%o7
        sll     %g2,18,%g2
        or      %o7,%g2,%g2
        xor     %g2,%g3,%g1
        set     0x33333333,%o2
        andn    %g1,%o2,%g1
        xor     %g1,%g2,%g2
        xor     %g1,%g3,%g3
        srl     %g2,26,%o7
        sll     %g2,6,%g2
        or      %o7,%g2,%g2
        xor     %g2,%g3,%g1
        set     0x00FF00FF,%o3
        andn    %g1,%o3,%g1
        xor     %g1,%g2,%g2
        xor     %g1,%g3,%g3
        srl     %g2,9,%o7
        sll     %g2,23,%g2
        or      %o7,%g2,%g2
        xor     %g2,%g3,%g1
        set     0x55555555,%o4
        and     %g1,%o4,%g1
        xor     %g1,%g2,%g2
        xor     %g1,%g3,%g3
        srl     %g3,1,%o7
        sll     %g3,31,%g3
        or      %o7,%g3,%g3
LOOP1:
        ldd     [%i3+128],%i4
        add     %i3,16,%i3
        xor     %i4,%g3,%i4
        and     %i4,0x0FC,%g1
        ld      [%g4+%g1],%g1
        srl     %i4,8,%i4
        and     %i4,0x0FC,%o7
        add     %o7,-512,%o7
        ld      [%g4+%o7],%o7
        xor     %g2,%g1,%g2
        srl     %i4,8,%i4
        and     %i4,0x0FC,%g1
        add     %g1,-1024,%g1
        ld      [%g4+%g1],%g1
        xor     %g2,%o7,%g2
        srl     %i4,8,%i4
        and     %i4,0x0FC,%o7
        add     %o7,-1536,%o7
        ld      [%g4+%o7],%o7
        xor     %g2,%g1,%g2
        srl     %g3,28,%i4
        sll     %g3,4,%g1
        or      %g1,%i4,%g1
        xor     %i5,%g1,%i5
        and     %i5,0x0FC,%g1
        add     %g1,256,%g1
        ld      [%g4+%g1],%g1
        xor     %g2,%o7,%g2
        srl     %i5,8,%i5
        and     %i5,0x0FC,%o7
        add     %o7,-256,%o7
        ld      [%g4+%o7],%o7
        xor     %g2,%g1,%g2
        srl     %i5,8,%i5
        and     %i5,0x0FC,%g1
        add     %g1,-768,%g1
        ld      [%g4+%g1],%g1
        xor     %g2,%o7,%g2
        srl     %i5,8,%i5
        and     %i5,0x0FC,%o7
        add     %o7,-1280,%o7
        ld      [%g4+%o7],%o7
        xor     %g2,%g1,%g2
        ldd     [%i3+120],%i4
        cmp     %i3,%i0
        xor     %g2,%o7,%g2
        xor     %i4,%g2,%i4
        and     %i4,0x0FC,%g1
        ld      [%g4+%g1],%g1
        srl     %i4,8,%i4
        and     %i4,0x0FC,%o7
        add     %o7,-512,%o7
        ld      [%g4+%o7],%o7
        xor     %g3,%g1,%g3
        srl     %i4,8,%i4
        and     %i4,0x0FC,%g1
        add     %g1,-1024,%g1
        ld      [%g4+%g1],%g1
        xor     %g3,%o7,%g3
        srl     %i4,8,%i4
        and     %i4,0x0FC,%o7
        add     %o7,-1536,%o7
        ld      [%g4+%o7],%o7
        xor     %g3,%g1,%g3
        srl     %g2,28,%i4
        sll     %g2,4,%g1
        or      %g1,%i4,%g1
        xor     %i5,%g1,%i5
        and     %i5,0x0FC,%g1
        add     %g1,256,%g1
        ld      [%g4+%g1],%g1
        xor     %g3,%o7,%g3
        srl     %i5,8,%i5
        and     %i5,0x0FC,%o7
        add     %o7,-256,%o7
        ld      [%g4+%o7],%o7
        xor     %g3,%g1,%g3
        srl     %i5,8,%i5
        and     %i5,0x0FC,%g1
        add     %g1,-768,%g1
        ld      [%g4+%g1],%g1
        xor     %g3,%o7,%g3
        srl     %i5,8,%i5
        and     %i5,0x0FC,%o7
        add     %o7,-1280,%o7
        ld      [%g4+%o7],%o7
        xor     %g3,%g1,%g3
        bne     LOOP1
        xor     %g3,%o7,%g3
EXIT1:
        addcc   %g2,%g2,%g0
        addx    %g2,%g2,%g2
        xor     %g3,%g2,%g1
        andn    %g1,%o4,%g1
        xor     %g1,%g3,%g3
        xor     %g1,%g2,%g2
        srl     %g2,23,%o7
        sll     %g2,9,%g2
        or      %o7,%g2,%g2
        xor     %g2,%g3,%g1
        andn    %g1,%o3,%g1
        xor     %g1,%g2,%g2
        xor     %g1,%g3,%g3
        srl     %g2,6,%o7
        sll     %g2,26,%g2
        or      %o7,%g2,%g2
        xor     %g2,%g3,%g1
        andn    %g1,%o2,%g1
        xor     %g1,%g2,%g2
        xor     %g1,%g3,%g3
        srl     %g2,18,%o7
        sll     %g2,14,%g2
        or      %o7,%g2,%g2
        xor     %g2,%g3,%g1
        andn    %g1,%o1,%g1
        xor     %g1,%g2,%g2
        xor     %g1,%g3,%g3
        srl     %g2,20,%o7
        sll     %g2,12,%g2
        or      %o7,%g2,%g2
        xor     %g2,%g3,%g1
        and     %g1,%o0,%g1
        xor     %g1,%g2,%g2
        xor     %g1,%g3,%g3
        srl     %g2,28,%o7
        sll     %g2,4,%g2
        or      %o7,%g2,%g2
        st      %g2,[%i2]
        st      %g3,[%i2+4]
        ret
        restore

        .global Decrypt
Decrypt:
        save    %sp,-64,%sp
        ld      [%i1],%g2
        add     %i0,128,%i3
        set     S7,%g4
        ld      [%i1+4],%g3
        srl     %g2,4,%o7
        sll     %g2,28,%g2
        or      %o7,%g2,%g2
        xor     %g2,%g3,%g1
        set     0x0F0F0F0F,%o0
        and     %g1,%o0,%g1
        xor     %g1,%g2,%g2
        xor     %g1,%g3,%g3
        srl     %g2,12,%o7
        sll     %g2,20,%g2
        or      %o7,%g2,%g2
        xor     %g2,%g3,%g1
        set     0x0FFFF0000,%o1
        andn    %g1,%o1,%g1
        xor     %g1,%g2,%g2
        xor     %g1,%g3,%g3
        srl     %g2,14,%o7
        sll     %g2,18,%g2
        or      %o7,%g2,%g2
        xor     %g2,%g3,%g1
        set     0x33333333,%o2
        andn    %g1,%o2,%g1
        xor     %g1,%g2,%g2
        xor     %g1,%g3,%g3
        srl     %g2,26,%o7
        sll     %g2,6,%g2
        or      %o7,%g2,%g2
        xor     %g2,%g3,%g1
        set     0x00FF00FF,%o3
        andn    %g1,%o3,%g1
        xor     %g1,%g2,%g2
        xor     %g1,%g3,%g3
        srl     %g2,9,%o7
        sll     %g2,23,%g2
        or      %o7,%g2,%g2
        xor     %g2,%g3,%g1
        set     0x55555555,%o4
        and     %g1,%o4,%g1
        xor     %g1,%g2,%g2
        xor     %g1,%g3,%g3
        srl     %g3,1,%o7
        sll     %g3,31,%g3
        or      %o7,%g3,%g3
LOOP2:
        ldd     [%i3-8],%i4
        sub     %i3,16,%i3
        xor     %i4,%g3,%i4
        and     %i4,0x0FC,%g1
        ld      [%g4+%g1],%g1
        srl     %i4,8,%i4
        and     %i4,0x0FC,%o7
        add     %o7,-512,%o7
        ld      [%g4+%o7],%o7
        xor     %g2,%g1,%g2
        srl     %i4,8,%i4
        and     %i4,0x0FC,%g1
        add     %g1,-1024,%g1
        ld      [%g4+%g1],%g1
        xor     %g2,%o7,%g2
        srl     %i4,8,%i4
        and     %i4,0x0FC,%o7
        add     %o7,-1536,%o7
        ld      [%g4+%o7],%o7
        xor     %g2,%g1,%g2
        srl     %g3,28,%i4
        sll     %g3,4,%g1
        or      %g1,%i4,%g1
        xor     %i5,%g1,%i5
        and     %i5,0x0FC,%g1
        add     %g1,256,%g1
        ld      [%g4+%g1],%g1
        xor     %g2,%o7,%g2
        srl     %i5,8,%i5
        and     %i5,0x0FC,%o7
        add     %o7,-256,%o7
        ld      [%g4+%o7],%o7
        xor     %g2,%g1,%g2
        srl     %i5,8,%i5
        and     %i5,0x0FC,%g1
        add     %g1,-768,%g1
        ld      [%g4+%g1],%g1
        xor     %g2,%o7,%g2
        srl     %i5,8,%i5
        and     %i5,0x0FC,%o7
        add     %o7,-1280,%o7
        ld      [%g4+%o7],%o7
        xor     %g2,%g1,%g2
        ldd     [%i3],%i4
        cmp     %i3,%i0
        xor     %g2,%o7,%g2
        xor     %i4,%g2,%i4
        and     %i4,0x0FC,%g1
        ld      [%g4+%g1],%g1
        srl     %i4,8,%i4
        and     %i4,0x0FC,%o7
        add     %o7,-512,%o7
        ld      [%g4+%o7],%o7
        xor     %g3,%g1,%g3
        srl     %i4,8,%i4
        and     %i4,0x0FC,%g1
        add     %g1,-1024,%g1
        ld      [%g4+%g1],%g1
        xor     %g3,%o7,%g3
        srl     %i4,8,%i4
        and     %i4,0x0FC,%o7
        add     %o7,-1536,%o7
        ld      [%g4+%o7],%o7
        xor     %g3,%g1,%g3
        srl     %g2,28,%i4
        sll     %g2,4,%g1
        or      %g1,%i4,%g1
        xor     %i5,%g1,%i5
        and     %i5,0x0FC,%g1
        add     %g1,256,%g1
        ld      [%g4+%g1],%g1
        xor     %g3,%o7,%g3
        srl     %i5,8,%i5
        and     %i5,0x0FC,%o7
        add     %o7,-256,%o7
        ld      [%g4+%o7],%o7
        xor     %g3,%g1,%g3
        srl     %i5,8,%i5
        and     %i5,0x0FC,%g1
        add     %g1,-768,%g1
        ld      [%g4+%g1],%g1
        xor     %g3,%o7,%g3
        srl     %i5,8,%i5
        and     %i5,0x0FC,%o7
        add     %o7,-1280,%o7
        ld      [%g4+%o7],%o7
        xor     %g3,%g1,%g3
        bne     LOOP2
        xor     %g3,%o7,%g3
EXIT2:
        addcc   %g2,%g2,%g0
        addx    %g2,%g2,%g2
        xor     %g3,%g2,%g1
        andn    %g1,%o4,%g1
        xor     %g1,%g3,%g3
        xor     %g1,%g2,%g2
        srl     %g2,23,%o7
        sll     %g2,9,%g2
        or      %o7,%g2,%g2
        xor     %g2,%g3,%g1
        andn    %g1,%o3,%g1
        xor     %g1,%g2,%g2
        xor     %g1,%g3,%g3
        srl     %g2,6,%o7
        sll     %g2,26,%g2
        or      %o7,%g2,%g2
        xor     %g2,%g3,%g1
        andn    %g1,%o2,%g1
        xor     %g1,%g2,%g2
        xor     %g1,%g3,%g3
        srl     %g2,18,%o7
        sll     %g2,14,%g2
        or      %o7,%g2,%g2
        xor     %g2,%g3,%g1
        andn    %g1,%o1,%g1
        xor     %g1,%g2,%g2
        xor     %g1,%g3,%g3
        srl     %g2,20,%o7
        sll     %g2,12,%g2
        or      %o7,%g2,%g2
        xor     %g2,%g3,%g1
        and     %g1,%o0,%g1
        xor     %g1,%g2,%g2
        xor     %g1,%g3,%g3
        srl     %g2,28,%o7
        sll     %g2,4,%g2
        or      %o7,%g2,%g2
        st      %g2,[%i2]
        st      %g3,[%i2+4]
        ret
        restore

        .global ExpandKey
ExpandKey:
        save    %sp,-64,%sp
        ld      [%i1],%g2
        ld      [%i1+4],%g3
        srl     %g3,28,%o7
        sll     %g3,4,%g3
        or      %o7,%g3,%g3
        xor     %g3,%g2,%g1
        set     0x1F1F1F1F,%o0
        andn    %g1,%o0,%g1
        xor     %g1,%g3,%g3
        xor     %g1,%g2,%g2
        srl     %g3,4,%o7
        sll     %g3,28,%g3
        or      %o7,%g3,%g3
        srl     %g3,18,%g1
        xor     %g1,%g3,%g1
        set     0x00003333,%o1
        and     %g1,%o1,%g1
        xor     %g3,%g1,%g3
        sll     %g1,18,%g1
        xor     %g3,%g1,%g3
        srl     %g3,9,%g1
        xor     %g1,%g3,%g1
        set     0x00550055,%o2
        and     %g1,%o2,%g1
        xor     %g3,%g1,%g3
        sll     %g1,9,%g1
        xor     %g3,%g1,%g3
        sll     %g3,24,%g1
        srl     %g3,24,%o7
        or      %g1,%o7,%g1
        sll     %g3,8,%o7
        sll     %g3,16,%g3
        srl     %o7,24,%o7
        srl     %g3,24,%g3
        sll     %o7,8,%o7
        sll     %g3,16,%g3
        or      %g1,%o7,%g1
        or      %g1,%g3,%g3
        srl     %g2,18,%g1
        xor     %g1,%g2,%g1
        and     %g1,%o1,%g1
        xor     %g2,%g1,%g2
        sll     %g1,18,%g1
        xor     %g2,%g1,%g2
        srl     %g2,9,%g1
        xor     %g1,%g2,%g1
        and     %g1,%o2,%g1
        xor     %g2,%g1,%g2
        sll     %g1,9,%g1
        xor     %g2,%g1,%g2
        srl     %g2,24,%o7
        sll     %g2,8,%g2
        or      %o7,%g2,%g2
        set     0x81030000,%g5
LOOP3:
        sll     %g3,14,%g1
        addcc   %g1,%g1,%g0
        sll     %g3,17,%g1
        addx    %i4,%i4,%i4
        addcc   %g1,%g1,%g0
        sll     %g3,11,%g1
        addx    %i4,%i4,%i4
        addcc   %g1,%g1,%g0
        sll     %g3,24,%g1
        addx    %i4,%i4,%i4
        addcc   %g1,%g1,%g0
        sll     %g3,1,%g1
        addx    %i4,%i4,%i4
        addcc   %g1,%g1,%g0
        sll     %g3,5,%g1
        addx    %i4,%i4,%i4
        addcc   %g1,%g1,%g0
        addx    %i4,%i4,%i4
        sll     %i4,2,%i4
        sll     %g3,23,%g1
        addcc   %g1,%g1,%g0
        sll     %g3,19,%g1
        addx    %i4,%i4,%i4
        addcc   %g1,%g1,%g0
        sll     %g3,12,%g1
        addx    %i4,%i4,%i4
        addcc   %g1,%g1,%g0
        sll     %g3,4,%g1
        addx    %i4,%i4,%i4
        addcc   %g1,%g1,%g0
        sll     %g3,26,%g1
        addx    %i4,%i4,%i4
        addcc   %g1,%g1,%g0
        sll     %g3,8,%g1
        addx    %i4,%i4,%i4
        addcc   %g1,%g1,%g0
        addx    %i4,%i4,%i4
        sll     %i4,2,%i4
        sll     %g2,13,%g1
        addcc   %g1,%g1,%g0
        sll     %g2,24,%g1
        addx    %i4,%i4,%i4
        addcc   %g1,%g1,%g0
        sll     %g2,3,%g1
        addx    %i4,%i4,%i4
        addcc   %g1,%g1,%g0
        sll     %g2,9,%g1
        addx    %i4,%i4,%i4
        addcc   %g1,%g1,%g0
        sll     %g2,19,%g1
        addx    %i4,%i4,%i4
        addcc   %g1,%g1,%g0
        sll     %g2,27,%g1
        addx    %i4,%i4,%i4
        addcc   %g1,%g1,%g0
        addx    %i4,%i4,%i4
        sll     %i4,2,%i4
        add     %i4,2,%i4
        sll     %g2,16,%g1
        addcc   %g1,%g1,%g0
        sll     %g2,21,%g1
        addx    %i4,%i4,%i4
        addcc   %g1,%g1,%g0
        sll     %g2,11,%g1
        addx    %i4,%i4,%i4
        addcc   %g1,%g1,%g0
        addx    %i4,%i4,%i4
        addcc   %g2,%g2,%g0
        sll     %g2,6,%g1
        addx    %i4,%i4,%i4
        addcc   %g1,%g1,%g0
        sll     %g2,25,%g1
        addx    %i4,%i4,%i4
        addcc   %g1,%g1,%g0
        addx    %i4,%i4,%i4
        sll     %i4,2,%i4
        sll     %g3,3,%g1
        addcc   %g1,%g1,%g0
        addx    %i5,%i5,%i5
        addcc   %g3,%g3,%g0
        sll     %g3,15,%g1
        addx    %i5,%i5,%i5
        addcc   %g1,%g1,%g0
        sll     %g3,6,%g1
        addx    %i5,%i5,%i5
        addcc   %g1,%g1,%g0
        sll     %g3,21,%g1
        addx    %i5,%i5,%i5
        addcc   %g1,%g1,%g0
        sll     %g3,10,%g1
        addx    %i5,%i5,%i5
        addcc   %g1,%g1,%g0
        addx    %i5,%i5,%i5
        sll     %i5,2,%i5
        add     %i5,1,%i5
        sll     %g3,16,%g1
        addcc   %g1,%g1,%g0
        sll     %g3,7,%g1
        addx    %i5,%i5,%i5
        addcc   %g1,%g1,%g0
        sll     %g3,27,%g1
        addx    %i5,%i5,%i5
        addcc   %g1,%g1,%g0
        sll     %g3,20,%g1
        addx    %i5,%i5,%i5
        addcc   %g1,%g1,%g0
        sll     %g3,13,%g1
        addx    %i5,%i5,%i5
        addcc   %g1,%g1,%g0
        sll     %g3,2,%g1
        addx    %i5,%i5,%i5
        addcc   %g1,%g1,%g0
        addx    %i5,%i5,%i5
        sll     %i5,2,%i5
        sll     %g2,2,%g1
        addcc   %g1,%g1,%g0
        sll     %g2,12,%g1
        addx    %i5,%i5,%i5
        addcc   %g1,%g1,%g0
        sll     %g2,23,%g1
        addx    %i5,%i5,%i5
        addcc   %g1,%g1,%g0
        sll     %g2,17,%g1
        addx    %i5,%i5,%i5
        addcc   %g1,%g1,%g0
        sll     %g2,5,%g1
        addx    %i5,%i5,%i5
        addcc   %g1,%g1,%g0
        sll     %g2,20,%g1
        addx    %i5,%i5,%i5
        addcc   %g1,%g1,%g0
        addx    %i5,%i5,%i5
        sll     %i5,2,%i5
        add     %i5,3,%i5
        sll     %g2,18,%g1
        addcc   %g1,%g1,%g0
        sll     %g2,14,%g1
        addx    %i5,%i5,%i5
        addcc   %g1,%g1,%g0
        sll     %g2,22,%g1
        addx    %i5,%i5,%i5
        addcc   %g1,%g1,%g0
        sll     %g2,8,%g1
        addx    %i5,%i5,%i5
        addcc   %g1,%g1,%g0
        sll     %g2,1,%g1
        addx    %i5,%i5,%i5
        addcc   %g1,%g1,%g0
        sll     %g2,4,%g1
        addx    %i5,%i5,%i5
        addcc   %g1,%g1,%g0
        addx    %i5,%i5,%i5
        sll     %i5,2,%i5
        std     %i4,[%i0]
        srl     %g3,28,%g1
        addcc   %g5,%g5,%g5
        andn    %g3,0x0F,%g3
        subx    %g0,-2,%o7
        or      %g3,%g1,%g3
        srl     %g2,28,%g1
        add     %i0,8,%i0
        andn    %g2,0x0F,%g2
        sll     %g3,%o7,%g3
        or      %g2,%g1,%g2
        bne     LOOP3
        sll     %g2,%o7,%g2
EXIT3:
        ret
        restore
