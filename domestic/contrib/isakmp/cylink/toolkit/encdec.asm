;/****************************************************************************
;*  FILENAME: encdec.asm   PRODUCT NAME: CRYPTOGRAPHIC TOOLKIT
;*
;*  FILE STATUS:
;*
;*  DESCRIPTION:  Intel 386 assembly fuction for Toolkit
;*
;*  PUBLIC FUNCTIONS:
;*
;*  PRIVATE FUNCTIONS:
;*
;*  REVISION  HISTORY:
;*
;****************************************************************************/

	.386p
	public _des_encrypt
	public 	_des_decrypt
	extrn	_fperm:byte
	extrn	_iperm:byte
	extrn	_sp:word
DGROUP	group	_DATA,_BSS
	assume	cs:ENCDEC_TEXT,ds:DGROUP
_DATA	segment word public use16 'DATA'
_DATA	ends
_BSS	segment word public use16 'BSS'
_BSS	ends
ENCDEC_TEXT	segment byte public use16 'CODE'
	assume	cs:ENCDEC_TEXT
;/****************************************************************************
;*  NAME:     void des_encrypt( uchar *keybuf,
;*      						char  *block )
;*
;*  DESCRIPTION:  In-place DES encryption
;*
;*  INPUTS:
;*	   PARAMETERS:
;*           uchar *keybuf		    Pointer to key array
;*			 char *block           Pointer to array
;*  OUTPUT:
;*	   PARAMETERS:
;*           char *block			Pointer to encrypted array
;*	   RETURN:
;*
;*  REVISION HISTORY:
;*
;*
;****************************************************************************/
_des_encrypt	proc	far
	mov	ax,ds
	nop
	inc	bp
	push	bp
	mov	bp,sp
	push	ds
	mov	ds,ax
	sub	sp,18
	push	si
	push	di
   ;	   permute(block,iperm,(char *)work);     /* Initial Permutation */

	mov     bx,word ptr [bp+12]
	mov     fs,bx
	mov     bx,seg _iperm
	mov     gs,bx

   ;	   /* Clear output block     */
   ;	   for (i=8, ob = outblock; i !=0; i--)
	mov	si,8
	lea     bx,word ptr [bp-10]
	jmp	short @58@282
@58@226:
   ;	      *ob++ = 0;
   ;
	mov	byte ptr ss:[bx],0
	inc     bx
	dec	si
@58@282:
	or	si,si
	jne	short @58@226
   ;
   ;
   ;	   ib = inblock;
   ;
	mov	eax,dword ptr [bp+10]
	mov	dword ptr [bp-18],eax
   ;
   ;	   for (j = 0; j < 16; j += 2, ib++) { /* for each input nibble */
   ;
	mov	cx,offset _iperm
	xor	dx,dx
	jmp	short @58@478
@58@338:
   ;
   ;	      p = perm[j][(*ib >> 4) & 017];
   ;
	mov	bx,word ptr [bp-18]
	mov	al,byte ptr fs:[bx]
	and	ax,240
	shr	ax,1
	add	ax,dx
	add	ax,cx
	mov	si,ax
   ;
   ;	      q = perm[j + 1][*ib & 017];
   ;
	mov	al,byte ptr fs:[bx]
	and	ax,15
	shl	ax,3
	add	dx,128
	add	ax,dx
	add	ax,cx
	mov	di,ax
   ;
   ;	      ob = outblock;
   ;
	lea	bx,word ptr [bp-10]
   ;
   ;	      for (i =8; i !=0; i--){   /* and each output byte */
   ;
	add	dx,8
	jmp	short @58@422
@58@366:
   ;
   ;		 *ob++ |= *p++ | *q++;  /* OR the masks together*/
   ;
	mov	al,byte ptr gs:[si]
	or	al,byte ptr gs:[di]
	or	byte ptr ss:[bx],al
	inc	si
	inc	di
	inc     bx
	dec	dx
@58@422:
	cmp	dl,128
	jne	short @58@366
	add     dx,128
	inc	word ptr [bp-18]
@58@478:
	cmp	dx,2048
	jge short	@@584
	jmp	@58@338
@@584:
@58@506:

	lea	bx,word ptr [bp-10]
   ;
   ;	    ctmp = cp[3];
   ;
	mov	cl,byte ptr ss:[bx+3]
   ;
   ;	    cp[3] = cp[0];
   ;
	mov	al,byte ptr ss:[bx]
	mov	byte ptr ss:[bx+3],al
   ;
   ;	    cp[0] = ctmp;
   ;
	mov	byte ptr ss:[bx],cl
   ;
   ;
   ;	    ctmp = cp[2];
   ;
	mov	cl,byte ptr ss:[bx+2]
   ;
   ;	    cp[2] = cp[1];
   ;
	mov	al,byte ptr ss:[bx+1]
	mov	byte ptr ss:[bx+2],al
   ;
   ;	    cp[1] = ctmp;
   ;
	mov	byte ptr ss:[bx+1],cl
   ;
   ;
   ;
   ;	   work[1] = bswapcfb1(work[1]);
   ;
	lea	bx,word ptr [bp-6]
   ;
   ;	    ctmp = cp[3];
   ;
	mov	cl,byte ptr ss:[bx+3]
   ;
   ;	    cp[3] = cp[0];
   ;
	mov	al,byte ptr ss:[bx]
	mov	byte ptr ss:[bx+3],al
   ;
   ;	    cp[0] = ctmp;
   ;
	mov	byte ptr ss:[bx],cl
   ;
   ;
   ;	    ctmp = cp[2];
   ;
	mov	cl,byte ptr ss:[bx+2]
   ;
   ;	    cp[2] = cp[1];
   ;
	mov	al,byte ptr ss:[bx+1]
	mov	byte ptr ss:[bx+2],al
   ;
   ;	    cp[1] = ctmp;
   ;
	mov	byte ptr ss:[bx+1],cl

   ;	   /* Do the 16 rounds */
   ;	   for (i=0; i<16; i++)
   ;
	xor	di,di
	jmp	@18@114
@18@58:
   ;
   ;	      roundcfb1(keybuf,i,work);
   ;


   ;	   if(num & 1){
   ;
	test	di,1
	jne short	@@58
	jmp	@48@254
@@58:
	mov	edx,dword ptr [bp-10]
   ;	     rval = 0;
   ;
	mov     eax, 0
   ;
   ;	     rval |= sp[0][((rt >>26 ) ^ keybuf->kn[num][0]) & 0x3f];
   ;
	ror     edx,27

	mov     cx,di

	shl	cx,3
	les	si,dword ptr [bp+6]
	add	si,cx
	mov	bl,byte ptr es:[si]
	mov	bh,0
	xor	bl,dl
	shl	bl,2
	mov     cx,seg _sp
	mov     fs,cx
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	     rval |= sp[1][((rt >>22 ) ^ keybuf->kn[num][1]) & 0x3f];
   ;

	rol     edx,4
	mov	bl,byte ptr es:[si+1]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	     rval |= sp[2][((rt >>18 ) ^ keybuf->kn[num][2]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+2]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	     rval |= sp[3][((rt >>14 ) ^ keybuf->kn[num][3]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+3]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	     rval |= sp[4][((rt >>10 ) ^ keybuf->kn[num][4]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+4]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	     rval |= sp[5][((rt >> 6 ) ^ keybuf->kn[num][5]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+5]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	     rval |= sp[6][((rt >> 2 ) ^ keybuf->kn[num][6]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+6]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	     rval |= sp[7][(rt ^ keybuf->kn[num][7]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+7]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	     block[1] ^= rval;
   ;
	xor	dword ptr [bp-6],eax
   ;
	jmp	@48@450
@48@254:
	mov	edx,dword ptr [bp-6]

	mov   	eax, 0
   ;
   ;	      rval |= sp[0][((rt >>26 ) ^ keybuf->kn[num][0]) & 0x3f];
   ;
	ror     edx,27

	mov     cx,di

	shl	cx,3
	les	si,dword ptr [bp+6]
	add	si,cx
	mov	bl,byte ptr es:[si]
	mov	bh,0
	xor	bl,dl
	shl	bl,2
	mov     cx,seg _sp
	mov     fs,cx
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	      rval |= sp[1][((rt >>22 ) ^ keybuf->kn[num][1]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+1]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	      rval |= sp[2][((rt >>18 ) ^ keybuf->kn[num][2]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+2]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	      rval |= sp[3][((rt >>14 ) ^ keybuf->kn[num][3]) & 0x3f];
   ;
	rol	edx,4
	mov	bl,byte ptr es:[si+3]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	      rval |= sp[4][((rt >>10 ) ^ keybuf->kn[num][4]) & 0x3f];
   ;
	rol	edx,4
	mov	bl,byte ptr es:[si+4]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	      rval |= sp[5][((rt >> 6 ) ^ keybuf->kn[num][5]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+5]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	      rval |= sp[6][((rt >> 2 ) ^ keybuf->kn[num][6]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+6]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]

   ;
   ;	      rval |= sp[7][(rt ^ keybuf->kn[num][7]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+7]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	      block[0] ^= rval;
   ;
	xor	dword ptr [bp-10],eax
@48@450:
   ;
	inc	di
@18@114:
	cmp	di,16
	jl	@18@58
   ;	   /* Left/right half swap */
   ;	   tmp = work[0];
   ;
	mov	eax,dword ptr [bp-10]
	mov	dword ptr [bp-14],eax
   ;
   ;	   work[0] = work[1];
   ;
	mov	eax,dword ptr [bp-6]
	mov	dword ptr [bp-10],eax
   ;
   ;	   work[1] = tmp;
   ;
	mov	eax,dword ptr [bp-14]
	mov	dword ptr [bp-6],eax
   ;
   ;
   ;	   work[0] = byteswap(work[0]);
   ;

	lea	bx,word ptr [bp-10]
   ;
   ;	    ctmp = cp[3];
   ;
	mov	cl,byte ptr ss:[bx+3]
   ;
   ;	    cp[3] = cp[0];
   ;
	mov	al,byte ptr ss:[bx]
	mov	byte ptr ss:[bx+3],al
   ;
   ;	    cp[0] = ctmp;
   ;
	mov	byte ptr ss:[bx],cl
   ;
   ;
   ;	    ctmp = cp[2];
   ;
	mov	cl,byte ptr ss:[bx+2]
   ;
   ;	    cp[2] = cp[1];
   ;
	mov	al,byte ptr ss:[bx+1]
	mov	byte ptr ss:[bx+2],al
   ;
   ;	    cp[1] = ctmp;
   ;
	mov	byte ptr ss:[bx+1],cl
   ;
   ;
   ;
   ;	   work[1] = bswapcfb1(work[1]);
   ;
	lea	bx,word ptr [bp-6]
   ;
   ;	    ctmp = cp[3];
   ;
	mov	cl,byte ptr ss:[bx+3]
   ;
   ;	    cp[3] = cp[0];
   ;
	mov	al,byte ptr ss:[bx]
	mov	byte ptr ss:[bx+3],al
   ;
   ;	    cp[0] = ctmp;
   ;
	mov	byte ptr ss:[bx],cl
   ;
   ;
   ;	    ctmp = cp[2];
   ;
	mov	cl,byte ptr ss:[bx+2]
   ;
   ;	    cp[2] = cp[1];
   ;
	mov	al,byte ptr ss:[bx+1]
	mov	byte ptr ss:[bx+2],al
   ;
   ;	    cp[1] = ctmp;
   ;
	mov	byte ptr ss:[bx+1],cl


   ;	   work[1] = byteswap(work[1]);
   ;	   permute((char *)work,fperm,block);  /* Inverse initial permutation */
   ;

	mov     bx,word ptr [bp+12]
	mov     fs,bx
	mov     bx,seg _fperm
	mov     gs,bx

   ;	   /* Clear output block     */
   ;	   for (i=8, ob = outblock; i !=0; i--)
   ;
	mov	si,8
	mov     bx,word ptr [bp+10]
	jmp	short @581@282
@581@226:
   ;
   ;	      *ob++ = 0;
   ;
	mov	byte ptr fs:[bx],0
	inc     bx
	dec	si
@581@282:
	or	si,si
	jne	short @581@226
   ;	   ib = inblock;
   ;
	lea	ax,word ptr [bp-10]
	mov	word ptr [bp-18],ax
   ;
   ;	   for (j = 0; j < 16; j += 2, ib++) { /* for each input nibble */
   ;
	mov	cx,offset _fperm
	xor	dx,dx
	jmp	short @581@478
@581@338:
   ;
   ;	      p = perm[j][(*ib >> 4) & 017];
   ;
	mov	bx,word ptr [bp-18]
	mov	al,byte ptr ss:[bx]
	and	ax,240
	shr	ax,1
	add	ax,dx
	add	ax,cx
	mov	si,ax
   ;
   ;	      q = perm[j + 1][*ib & 017];
   ;
	mov	al,byte ptr ss:[bx]
	and	ax,15
	shl	ax,3
	add	dx,128
	add	ax,dx
	add	ax,cx
	mov	di,ax
   ;
   ;	      ob = outblock;
	mov	bx,word ptr [bp+10]
   ;	      for (i =8; i !=0; i--){   /* and each output byte */
   ;
	add	dx,8
	jmp	short @581@422
@581@366:
   ;
   ;		 *ob++ |= *p++ | *q++;  /* OR the masks together*/
   ;
	mov	al,byte ptr gs:[si]
	or	al,byte ptr gs:[di]
	or	byte ptr fs:[bx],al
	inc	si
	inc	di
	inc     bx
	dec	dx
@581@422:
	cmp	dl,128
	jne	short @581@366
	add     dx,128
	inc	word ptr [bp-18]
@581@478:
	cmp	dx,2048
	jge short	@@5814
	jmp	@581@338
@@5814:
@581@506:
	pop	di
	pop	si
	lea	sp,word ptr [bp-2]
	pop	ds
	pop	bp
	dec	bp
	ret
_des_encrypt	endp
;/****************************************************************************
;*  NAME:     void des_decrypt( uchar *keybuf,
;*      						char  *block )
;*
;*  DESCRIPTION:  In-place DES encryption
;*
;*  INPUTS:
;*	   PARAMETERS:
;*           uchar *keybuf		   Pointer to key array
;*			 char *block           Pointer to encrypted array
;*  OUTPUT:
;*	   PARAMETERS:
;*           char *block			Pointer to result array
;*	   RETURN:
;*
;*  REVISION HISTORY:
;*
;*
;****************************************************************************/
_des_decrypt	proc	far
	mov	ax,ds
	nop
	inc	bp
	push	bp
	mov	bp,sp
	push	ds
	mov	ds,ax
	sub	sp,18
	push	si
	push	di
   ;	   permute(block,iperm,(char *)work);     /* Initial Permutation */
   ;

	mov     bx,word ptr [bp+12]
	mov     fs,bx
	mov     bx,seg _iperm
	mov     gs,bx

   ;	   /* Clear output block     */
   ;	   for (i=8, ob = outblock; i !=0; i--)
   ;
	mov	si,8
	lea     bx,word ptr [bp-10]
	jmp	short @58@d282
@58@d226:
   ;
   ;	      *ob++ = 0;
   ;
	mov	byte ptr ss:[bx],0
	inc     bx
	dec	si
@58@d282:
	or	si,si
	jne	short @58@d226
   ;
   ;
   ;	   ib = inblock;
   ;
	mov	eax,dword ptr [bp+10]
	mov	dword ptr [bp-18],eax
   ;
   ;	   for (j = 0; j < 16; j += 2, ib++) { /* for each input nibble */
   ;
	mov	cx,offset _iperm
	xor	dx,dx
	jmp	short @58@d478
@58@d338:
   ;
   ;	      p = perm[j][(*ib >> 4) & 017];
   ;
	mov	bx,word ptr [bp-18]
	mov	al,byte ptr fs:[bx]
	and	ax,240
	shr	ax,1
	add	ax,dx
	add	ax,cx
	mov	si,ax
   ;
   ;	      q = perm[j + 1][*ib & 017];
   ;
	mov	al,byte ptr fs:[bx]
	and	ax,15
	shl	ax,3
	add	dx,128
	add	ax,dx
	add	ax,cx
	mov	di,ax
   ;
   ;	      ob = outblock;
   ;
	lea	bx,word ptr [bp-10]
   ;
   ;	      for (i =8; i !=0; i--){   /* and each output byte */
   ;
	add	dx,8
	jmp	short @58@d422
@58@d366:
   ;
   ;		 *ob++ |= *p++ | *q++;  /* OR the masks together*/
   ;
	mov	al,byte ptr gs:[si]
	or	al,byte ptr gs:[di]
	or	byte ptr ss:[bx],al
	inc	si
	inc	di
	inc     bx
	dec	dx
@58@d422:
	cmp	dl,128
	jne	short @58@d366
	add     dx,128
	inc	word ptr [bp-18]
@58@d478:
	cmp	dx,2048
	jge short	@@d584
	jmp	@58@d338
@@d584:
@58@d506:
   ;	    cp = (char *)work;
   ;
	lea	bx,word ptr [bp-10]
   ;
   ;	    ctmp = cp[3];
   ;
	mov	cl,byte ptr ss:[bx+3]
   ;
   ;	    cp[3] = cp[0];
   ;
	mov	al,byte ptr ss:[bx]
	mov	byte ptr ss:[bx+3],al
   ;
   ;	    cp[0] = ctmp;
   ;
	mov	byte ptr ss:[bx],cl
   ;
   ;
   ;	    ctmp = cp[2];
   ;
	mov	cl,byte ptr ss:[bx+2]
   ;
   ;	    cp[2] = cp[1];
   ;
	mov	al,byte ptr ss:[bx+1]
	mov	byte ptr ss:[bx+2],al
   ;
   ;	    cp[1] = ctmp;
   ;
	mov	byte ptr ss:[bx+1],cl
   ;
   ;
   ;
   ;	   work[1] = bswapcfb1(work[1]);
   ;
	lea	bx,word ptr [bp-6]
   ;
   ;	    ctmp = cp[3];
   ;
	mov	cl,byte ptr ss:[bx+3]
   ;
   ;	    cp[3] = cp[0];
   ;
	mov	al,byte ptr ss:[bx]
	mov	byte ptr ss:[bx+3],al
   ;
   ;	    cp[0] = ctmp;
   ;
	mov	byte ptr ss:[bx],cl
   ;
   ;
   ;	    ctmp = cp[2];
   ;
	mov	cl,byte ptr ss:[bx+2]
   ;
   ;	    cp[2] = cp[1];
   ;
	mov	al,byte ptr ss:[bx+1]
	mov	byte ptr ss:[bx+2],al
   ;
   ;	    cp[1] = ctmp;
   ;
	mov	byte ptr ss:[bx+1],cl
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;	   /* Left/right half swap */
   ;	   tmp = work[0];
   ;
	mov	eax,dword ptr [bp-10]
	mov	dword ptr [bp-14],eax
   ;
   ;	   work[0] = work[1];
   ;
	mov	eax,dword ptr [bp-6]
	mov	dword ptr [bp-10],eax
   ;
   ;	   work[1] = tmp;
   ;
	mov	eax,dword ptr [bp-14]
	mov	dword ptr [bp-6],eax
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;	   /* Do the 16 rounds */
   ;	//   for (i=0; i<16; i++)
   ;       for (i=15; i>=0; i--)
	mov  	di,15
	jmp	@18@d114
@18@d58:
   ;
   ;	      roundcfb1(keybuf,i,work);
   ;


   ;	   if(num & 1){
   ;
	test	di,1
	jne short	@@58d
	jmp	@48@d254
@@58d:
	mov	edx,dword ptr [bp-10]
   ;
   ;	     rval = 0;
   ;
	mov     eax, 0
   ;
   ;	     rval |= sp[0][((rt >>26 ) ^ keybuf->kn[num][0]) & 0x3f];
   ;
	ror     edx,27

	mov     cx,di

	shl	cx,3
	les	si,dword ptr [bp+6]
	add	si,cx
	mov	bl,byte ptr es:[si]
	mov	bh,0
	xor	bl,dl
	shl	bl,2
	mov     cx,seg _sp
	mov     fs,cx
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	     rval |= sp[1][((rt >>22 ) ^ keybuf->kn[num][1]) & 0x3f];
   ;

	rol     edx,4
	mov	bl,byte ptr es:[si+1]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	     rval |= sp[2][((rt >>18 ) ^ keybuf->kn[num][2]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+2]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	     rval |= sp[3][((rt >>14 ) ^ keybuf->kn[num][3]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+3]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	     rval |= sp[4][((rt >>10 ) ^ keybuf->kn[num][4]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+4]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	     rval |= sp[5][((rt >> 6 ) ^ keybuf->kn[num][5]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+5]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	     rval |= sp[6][((rt >> 2 ) ^ keybuf->kn[num][6]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+6]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	     rval |= sp[7][(rt ^ keybuf->kn[num][7]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+7]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	     block[1] ^= rval;
   ;
	xor	dword ptr [bp-6],eax
   ;
   ;	   } else {
   ;
	jmp	@48@d450
@48@d254:
	mov	edx,dword ptr [bp-6]

	mov   	eax, 0
   ;
   ;	      rval |= sp[0][((rt >>26 ) ^ keybuf->kn[num][0]) & 0x3f];
   ;
	ror     edx,27

	mov     cx,di

	shl	cx,3
	les	si,dword ptr [bp+6]
	add	si,cx
	mov	bl,byte ptr es:[si]
	mov	bh,0
	xor	bl,dl
	shl	bl,2
	mov     cx,seg _sp
	mov     fs,cx
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	      rval |= sp[1][((rt >>22 ) ^ keybuf->kn[num][1]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+1]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	      rval |= sp[2][((rt >>18 ) ^ keybuf->kn[num][2]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+2]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	      rval |= sp[3][((rt >>14 ) ^ keybuf->kn[num][3]) & 0x3f];
   ;
	rol	edx,4
	mov	bl,byte ptr es:[si+3]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	      rval |= sp[4][((rt >>10 ) ^ keybuf->kn[num][4]) & 0x3f];
   ;
	rol	edx,4
	mov	bl,byte ptr es:[si+4]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	      rval |= sp[5][((rt >> 6 ) ^ keybuf->kn[num][5]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+5]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	      rval |= sp[6][((rt >> 2 ) ^ keybuf->kn[num][6]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+6]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]

   ;
   ;	      rval |= sp[7][(rt ^ keybuf->kn[num][7]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr es:[si+7]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr fs:_sp[bx]
   ;
   ;	      block[0] ^= rval;
   ;
	xor	dword ptr [bp-10],eax
@48@d450:
	dec	di
@18@d114:
	cmp	di,0
	jge	@18@d58
   ;	   work[0] = byteswap(work[0]);
   ;

	lea	bx,word ptr [bp-10]
   ;
   ;	    ctmp = cp[3];
   ;
	mov	cl,byte ptr ss:[bx+3]
   ;
   ;	    cp[3] = cp[0];
   ;
	mov	al,byte ptr ss:[bx]
	mov	byte ptr ss:[bx+3],al
   ;
   ;	    cp[0] = ctmp;
   ;
	mov	byte ptr ss:[bx],cl
   ;
   ;
   ;	    ctmp = cp[2];
   ;
	mov	cl,byte ptr ss:[bx+2]
   ;
   ;	    cp[2] = cp[1];
   ;
	mov	al,byte ptr ss:[bx+1]
	mov	byte ptr ss:[bx+2],al
   ;
   ;	    cp[1] = ctmp;
   ;
	mov	byte ptr ss:[bx+1],cl
   ;
   ;
   ;
   ;	   work[1] = bswapcfb1(work[1]);
   ;
	lea	bx,word ptr [bp-6]
   ;
   ;	    ctmp = cp[3];
   ;
	mov	cl,byte ptr ss:[bx+3]
   ;
   ;	    cp[3] = cp[0];
   ;
	mov	al,byte ptr ss:[bx]
	mov	byte ptr ss:[bx+3],al
   ;
   ;	    cp[0] = ctmp;
   ;
	mov	byte ptr ss:[bx],cl
   ;
   ;
   ;	    ctmp = cp[2];
   ;
	mov	cl,byte ptr ss:[bx+2]
   ;
   ;	    cp[2] = cp[1];
   ;
	mov	al,byte ptr ss:[bx+1]
	mov	byte ptr ss:[bx+2],al
   ;
   ;	    cp[1] = ctmp;
   ;
	mov	byte ptr ss:[bx+1],cl

   ;	   permute((char *)work,fperm,block);  /* Inverse initial permutation */
   ;

	mov     bx,word ptr [bp+12]
	mov     fs,bx
	mov     bx,seg _fperm
	mov     gs,bx

   ;	   /* Clear output block     */
   ;	   for (i=8, ob = outblock; i !=0; i--)
   ;
	mov	si,8
	mov     bx,word ptr [bp+10]
	jmp	short @581@d282
@581@d226:
   ;
   ;	      *ob++ = 0;
   ;
	mov	byte ptr fs:[bx],0
	inc     bx
	dec	si
@581@d282:
	or	si,si
	jne	short @581@d226
   ;
   ;
   ;	   ib = inblock;
   ;
	lea	ax,word ptr [bp-10]
	mov	word ptr [bp-18],ax
   ;
   ;	   for (j = 0; j < 16; j += 2, ib++) { /* for each input nibble */
   ;
	mov	cx,offset _fperm
	xor	dx,dx
	jmp	short @581@d478
@581@d338:
   ;
   ;	      p = perm[j][(*ib >> 4) & 017];
   ;
	mov	bx,word ptr [bp-18]
	mov	al,byte ptr ss:[bx]
	and	ax,240
	shr	ax,1
	add	ax,dx
	add	ax,cx
	mov	si,ax
   ;
   ;	      q = perm[j + 1][*ib & 017];
   ;
	mov	al,byte ptr ss:[bx]
	and	ax,15
	shl	ax,3
	add	dx,128
	add	ax,dx
	add	ax,cx
	mov	di,ax
   ;
   ;	      ob = outblock;
   ;
	mov	bx,word ptr [bp+10]
   ;
   ;	      for (i =8; i !=0; i--){   /* and each output byte */
   ;
	add	dx,8
	jmp	short @581@d422
@581@d366:
   ;
   ;		 *ob++ |= *p++ | *q++;  /* OR the masks together*/
   ;
	mov	al,byte ptr gs:[si]
	or	al,byte ptr gs:[di]
	or	byte ptr fs:[bx],al
	inc	si
	inc	di
	inc     bx
	dec	dx
@581@d422:
	cmp	dl,128
	jne	short @581@d366
	add     dx,128
	inc	word ptr [bp-18]
@581@d478:
	cmp	dx,2048
	jge short	@@d5814
	jmp	@581@d338
@@d5814:
@581@d506:
	pop	di
	pop	si
	lea	sp,word ptr [bp-2]
	pop	ds
	pop	bp
	dec	bp
	ret
_des_decrypt	endp

ENCDEC_TEXT	ends
_DATA	segment word public use16 'DATA'
_DATA	ends

ENCDEC_TEXT	segment byte public use16 'CODE'
ENCDEC_TEXT	ends
	end



