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

	.386
FLAT group ENCDEC_TEXT	

	public _des_encrypt
	public 	_des_decrypt
	extrn	_fperm:byte
	extrn	_iperm:byte
	extrn	_sp:dword

	assume	cs:FLAT, ss:FLAT, ds:FLAT

ENCDEC_TEXT	segment para public use32 'CODE'
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
_des_encrypt	proc	near
	push	ebp
	mov	ebp,esp
	sub	esp,16
	push	esi
	push	edi
	push    ebx
	push    ecx
	push    edx
   ;	   permute(block,iperm,(char *)work);     /* Initial Permutation */


   ;	   /* Clear output block     */
   ;	   for (i=8, ob = outblock; i !=0; i--)
	mov	esi,8
	lea	ebx,[ebp-8]
@58@226:
   ;	      *ob++ = 0;
   ;
	mov	byte ptr [ebx],0
	inc     ebx
	dec	esi
@58@282:
	or	esi,esi
	jne	short @58@226
   ;
   ;
   ;	   ib = inblock;
   ;
	mov	eax, [ebp+12]
	mov	dword ptr [ebp-16],eax
   ;
   ;	   for (j = 0; j < 16; j += 2, ib++) { /* for each input nibble */
   ;
	mov	ecx,offset FLAT: _iperm 
	xor	edx,edx
	jmp	short @58@478
@58@338:
   ;
   ;	      p = perm[j][(*ib >> 4) & 017];
   ;
	mov	ebx,[ebp-16]
	mov	al,byte ptr [ebx]
	and	eax,240
	shr	eax,1
	add	eax,edx
	add	eax,ecx
	mov	esi,eax
   ;
   ;	      q = perm[j + 1][*ib & 017];
   ;
	mov	al,byte ptr [ebx]
	and	eax,15
	shl	eax,3
	add	edx,128
	add	eax,edx
	add	eax,ecx
	mov	edi,eax
   ;
   ;	      ob = outblock;
   ;
	lea	ebx,[ebp-8]
   ;
   ;	      for (i =8; i !=0; i--){   /* and each output byte */
   ;
	add	edx,8
	jmp	short @58@422
@58@366:
   ;
   ;		 *ob++ |= *p++ | *q++;  /* OR the masks together*/
   ;
	mov	al,byte ptr [esi]
	or	al,byte ptr [edi]
	or	byte ptr [ebx],al
	inc	esi
	inc	edi
	inc     ebx
	dec	edx
@58@422:
	cmp	dl,128
	jne	short @58@366
	add     dx,128
	inc	dword ptr [ebp-16]
@58@478:
	cmp	dx,2048
	jge short	@@584
	jmp	@58@338
@@584:
@58@506:

	mov	ebx,[ebp-8]
	ror	bx,8
	ror	ebx,16
	ror	bx,8
	mov	dword ptr[ebp-8],ebx
   ;
   ;
   ;	   work[1] = bswapcfb1(work[1]);
   ;
	
	mov	ebx,[ebp-4]
	ror	bx,8
	ror	ebx,16
	ror	bx,8
	mov	dword ptr[ebp-4],ebx

   ;	   /* Do the 16 rounds */
   ;	   for (i=0; i<16; i++)
   ;
	xor	edi,edi
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
	mov	edx,[ebp-8]
   ;	     rval = 0;
   ;
	mov     eax, 0
   ;
   ;	     rval |= sp[0][((rt >>26 ) ^ keybuf->kn[num][0]) & 0x3f];
   ;
	ror     edx,27

	movzx	ecx,di   ;;;;;

	shl	ecx,3
	mov	esi,[ebp+8]
	add	esi,ecx
	mov	ebx,0    ;;;;;
	mov	bl,byte ptr[esi]
	xor	bl,dl
	shl	bl,2
	or	eax,_sp[ebx]
   ;
   ;	     rval |= sp[1][((rt >>22 ) ^ keybuf->kn[num][1]) & 0x3f];
   ;

	rol     edx,4
	mov	bl,byte ptr [esi+1]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,_sp[ebx]
   ;
   ;	     rval |= sp[2][((rt >>18 ) ^ keybuf->kn[num][2]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+2]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,_sp[ebx]
   ;
   ;	     rval |= sp[3][((rt >>14 ) ^ keybuf->kn[num][3]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+3]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,_sp[ebx]
   ;
   ;	     rval |= sp[4][((rt >>10 ) ^ keybuf->kn[num][4]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+4]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax, _sp[ebx]
   ;
   ;	     rval |= sp[5][((rt >> 6 ) ^ keybuf->kn[num][5]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+5]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,_sp[ebx]
   ;
   ;	     rval |= sp[6][((rt >> 2 ) ^ keybuf->kn[num][6]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+6]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax, _sp[ebx]
   ;
   ;	     rval |= sp[7][(rt ^ keybuf->kn[num][7]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+7]
	inc	bh
	xor	bl,dl
	shl	bl,2
 	or	eax, _sp[ebx]
   ;
   ;	     block[1] ^= rval;
   ;
	xor	dword ptr[ebp-4],eax
   ;
	jmp	@48@450
@48@254:
	mov	edx,dword ptr [ebp-4]

	mov   	eax, 0
   ;
   ;	      rval |= sp[0][((rt >>26 ) ^ keybuf->kn[num][0]) & 0x3f];
   ;
	ror     edx,27

	mov     ecx,edi

	shl	ecx,3
	mov	esi,[ebp+8]
	add	esi,ecx
	mov	ebx,0  ;;
	mov	bl,byte ptr [esi]
	xor	bl,dl
	shl	bl,2
	or	eax, _sp[ebx]
   ;
   ;	      rval |= sp[1][((rt >>22 ) ^ keybuf->kn[num][1]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+1]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr _sp[ebx]
   ;
   ;	      rval |= sp[2][((rt >>18 ) ^ keybuf->kn[num][2]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+2]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax, _sp[ebx]
   ;
   ;	      rval |= sp[3][((rt >>14 ) ^ keybuf->kn[num][3]) & 0x3f];
   ;
	rol	edx,4
	mov	bl,byte ptr [esi+3]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr _sp[ebx]
   ;
   ;	      rval |= sp[4][((rt >>10 ) ^ keybuf->kn[num][4]) & 0x3f];
   ;
	rol	edx,4
	mov	bl,byte ptr [esi+4]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr _sp[ebx]
   ;
   ;	      rval |= sp[5][((rt >> 6 ) ^ keybuf->kn[num][5]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+5]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr _sp[ebx]
   ;
   ;	      rval |= sp[6][((rt >> 2 ) ^ keybuf->kn[num][6]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+6]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr _sp[ebx]

   ;
   ;	      rval |= sp[7][(rt ^ keybuf->kn[num][7]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+7]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,dword ptr _sp[ebx]
   ;
   ;	      block[0] ^= rval;
   ;
	xor	dword ptr [ebp-8],eax
@48@450:
   ;
	inc	di
@18@114:
	cmp	di,16
	jl	@18@58
   ;	   /* Left/right half swap */
   ;	   tmp = work[0];
   ;
	mov	eax,[ebp-8]
	mov	dword ptr [ebp-12],eax
   ;
   ;	   work[0] = work[1];
   ;
	mov	eax,[ebp-4]
	mov	dword ptr [ebp-8],eax
   ;
   ;	   work[1] = tmp;
   ;
	mov	eax,[ebp-12]
	mov	dword ptr [ebp-4],eax
   ;
   ;
   ;	   work[0] = byteswap(work[0]);
   ;

	mov	ebx,[ebp-8]
	ror	bx,8
	ror	ebx,16
	ror	bx,8
	mov	dword ptr[ebp-8],ebx
   ;
   ;	   work[1] = bswapcfb1(work[1]);
   ;
	mov	ebx,[ebp-4]
	ror	bx,8
	ror	ebx,16
	ror	bx,8
	mov	dword ptr[ebp-4],ebx

   ;	   work[1] = byteswap(work[1]);
   ;	   permute((char *)work,fperm,block);  /* Inverse initial permutation */
   ;


   ;	   /* Clear output block     */
   ;	   for (i=8, ob = outblock; i !=0; i--)
   ;
	mov	esi,8
	mov     ebx,[ebp+12]
	jmp	short @581@282
@581@226:
   ;
   ;	      *ob++ = 0;
   ;
	mov	byte ptr [ebx],0
	inc     bx
	dec	si
@581@282:
	or	si,si
	jne	short @581@226
   ;	   ib = inblock;
   ;
	lea	eax,[ebp-8]
	mov	dword ptr [ebp-16],eax
   ;
   ;	   for (j = 0; j < 16; j += 2, ib++) { /* for each input nibble */
   ;
	mov	ecx,offset FLAT: _fperm
	xor	edx,edx
	jmp	short @581@478
@581@338:
   ;
   ;	      p = perm[j][(*ib >> 4) & 017];
   ;
	mov	ebx,[ebp-16]
	mov	al,byte ptr [ebx]
	and	eax,240
	shr	eax,1
	add	eax,edx
	add	eax,ecx
	mov	esi,eax
   ;
   ;	      q = perm[j + 1][*ib & 017];
   ;
	mov	al,byte ptr [ebx]
	and	eax,15
	shl	eax,3
	add	edx,128
	add	eax,edx
	add	eax,ecx
	mov	edi,eax
   ;
   ;	      ob = outblock;
	mov	ebx,[ebp+12]
   ;	      for (i =8; i !=0; i--){   /* and each output byte */
   ;
	add	edx,8
	jmp	short @581@422
@581@366:
   ;
   ;		 *ob++ |= *p++ | *q++;  /* OR the masks together*/
   ;
	mov	al,byte ptr [esi]
	or	al,byte ptr [edi]
	or	byte ptr [ebx],al
	inc	si
	inc	di
	inc     bx
	dec	dx
@581@422:
	cmp	dl,128
	jne	short @581@366
	add     edx,128
	inc	dword ptr [ebp-16]
@581@478:
	cmp	dx,2048
	jge short	@@5814
	jmp	@581@338
@@5814:
@581@506:
	pop     edx
	pop     ecx
	pop     ebx
	pop	edi
	pop	esi
	mov	esp,ebp
	pop	ebp
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
_des_decrypt	proc	near
	push	ebp
	mov	ebp,esp
	sub	esp,16
	push	esi
	push	edi
	push    ebx
	push    ecx
	push    edx
   ;	   permute(block,iperm,(char *)work);     /* Initial Permutation */
   ;


   ;	   /* Clear output block     */
   ;	   for (i=8, ob = outblock; i !=0; i--)
   ;
	mov	esi,8
	lea     ebx, [ebp-8]
@58@d226:
   ;
   ;	      *ob++ = 0;
   ;
	mov	byte ptr [ebx],0
	inc     ebx
	dec	esi
@58@d282:
	or	esi,esi
	jne	short @58@d226
   ;
   ;
   ;	   ib = inblock;
   ;
	mov	eax,[ebp+10+2]
	mov	dword ptr [ebp-16],eax
   ;
   ;	   for (j = 0; j < 16; j += 2, ib++) { /* for each input nibble */
   ;
	mov	ecx,offset FLAT: _iperm
	xor	edx,edx
	jmp	short @58@d478
@58@d338:
   ;
   ;	      p = perm[j][(*ib >> 4) & 017];
   ;
	mov	ebx,[ebp-16]
	mov	al,byte ptr [ebx]
	and	eax,240
	shr	eax,1
	add	eax,edx
	add	eax,ecx
	mov	esi,eax
   ;
   ;	      q = perm[j + 1][*ib & 017];
   ;
	mov	al,byte ptr [ebx]
	and	eax,15
	shl	eax,3
	add	edx,128
	add	eax,edx
	add	eax,ecx
	mov	edi,eax
   ;
   ;	      ob = outblock;
   ;
	lea	ebx,[ebp-8]
   ;
   ;	      for (i =8; i !=0; i--){   /* and each output byte */
   ;
	add	edx,8
	jmp	short @58@d422
@58@d366:
   ;
   ;		 *ob++ |= *p++ | *q++;  /* OR the masks together*/
   ;
	mov	al,byte ptr [esi]
	or	al,byte ptr [edi]
	or	byte ptr [ebx],al
	inc	esi
	inc	edi
	inc     ebx
	dec	edx
@58@d422:
	cmp	dl,128
	jne	short @58@d366
	add     edx,128
	inc	dword ptr [ebp-16]
@58@d478:
	cmp	dx,2048
	jge short	@@d584
	jmp	@58@d338
@@d584:
@58@d506:
   ;	    cp = (char *)work;
   ;
	mov	ebx,[ebp-8]
	ror	bx,8
	ror	ebx,16
	ror	bx,8
	mov	dword ptr[ebp-8],ebx
	mov	ebx,[ebp-4]
	ror	bx,8
	ror	ebx,16
	ror	bx,8	
	mov	dword ptr[ebp-4],ebx

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;	   /* Left/right half swap */
   ;	   tmp = work[0];
   ;
	mov	eax,[ebp-8]
	mov	dword ptr [ebp-12],eax
   ;
   ;	   work[0] = work[1];
   ;
	mov	eax,[ebp-4]
	mov	dword ptr [ebp-8],eax
   ;
   ;	   work[1] = tmp;
   ;
	mov	eax,[ebp-12]
	mov	dword ptr [ebp-4],eax
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
   ;	   /* Do the 16 rounds */
   ;	//   for (i=0; i<16; i++)
   ;       for (i=15; i>=0; i--)
	mov  	edi,15
	jmp	@18@d114
@18@d58:
   ;
   ;	      roundcfb1(keybuf,i,work);
   ;


   ;	   if(num & 1){
   ;
	test	edi,1
	jne short	@@58d
	jmp	@48@d254
@@58d:
	mov	edx,[ebp-8]
   ;
   ;	     rval = 0;
   ;
	mov     eax, 0
   ;
   ;	     rval |= sp[0][((rt >>26 ) ^ keybuf->kn[num][0]) & 0x3f];
   ;
	ror     edx,27

	mov     ecx,edi

	shl	ecx,3
	mov	esi,[ebp+8]
	add	esi,ecx
	movzx	ebx,byte ptr [esi]
	xor	bl,dl
	shl	bl,2
	or	eax,_sp[ebx]
   ;
   ;	     rval |= sp[1][((rt >>22 ) ^ keybuf->kn[num][1]) & 0x3f];
   ;

	rol     edx,4
	mov	bl,byte ptr [esi+1]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,_sp[ebx]
   ;
   ;	     rval |= sp[2][((rt >>18 ) ^ keybuf->kn[num][2]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+2]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,_sp[ebx]
   ;
   ;	     rval |= sp[3][((rt >>14 ) ^ keybuf->kn[num][3]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+3]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,_sp[ebx]
   ;
   ;	     rval |= sp[4][((rt >>10 ) ^ keybuf->kn[num][4]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+4]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,_sp[ebx]
   ;
   ;	     rval |= sp[5][((rt >> 6 ) ^ keybuf->kn[num][5]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+5]
	inc	bh
	xor	bl,dl
	shl	bl,2
   	or	eax,_sp[ebx]

   ;	     rval |= sp[6][((rt >> 2 ) ^ keybuf->kn[num][6]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+6]
	inc	bh
	xor	bl,dl
	shl	bl,2
 	or	eax,_sp[ebx]
  ;
   ;	     rval |= sp[7][(rt ^ keybuf->kn[num][7]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+7]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,_sp[ebx]
   ;
   ;	     block[1] ^= rval;
   ;
	xor	dword ptr [ebp-4],eax
   ;
   ;	   } else {
   ;
	jmp	@48@d450
@48@d254:
	mov	edx,[ebp-4]

	mov   	eax, 0
   ;
   ;	      rval |= sp[0][((rt >>26 ) ^ keybuf->kn[num][0]) & 0x3f];
   ;
	ror     edx,27

	mov     ecx,edi

	shl	ecx,3
	mov	esi,[ebp+8]
	add	esi,ecx
	movzx	ebx,byte ptr [esi]
	xor	bl,dl
	shl	bl,2
   	or	eax,_sp[ebx]

   ;	      rval |= sp[1][((rt >>22 ) ^ keybuf->kn[num][1]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+1]
	inc	bh
	xor	bl,dl
	shl	bl,2
   	or	eax,_sp[ebx]

   ;	      rval |= sp[2][((rt >>18 ) ^ keybuf->kn[num][2]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+2]
	inc	bh
	xor	bl,dl
	shl	bl,2
  	or	eax,_sp[ebx]

   ;	      rval |= sp[3][((rt >>14 ) ^ keybuf->kn[num][3]) & 0x3f];
   ;
	rol	edx,4
	mov	bl,byte ptr [esi+3]
	inc	bh
	xor	bl,dl
	shl	bl,2
   	or	eax,_sp[ebx]

   ;	      rval |= sp[4][((rt >>10 ) ^ keybuf->kn[num][4]) & 0x3f];
   ;
	rol	edx,4
	mov	bl,byte ptr [esi+4]
	inc	bh
	xor	bl,dl
	shl	bl,2
   	or	eax,_sp[ebx]

   ;	      rval |= sp[5][((rt >> 6 ) ^ keybuf->kn[num][5]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+5]
	inc	bh
	xor	bl,dl
	shl	bl,2
   	or	eax,_sp[ebx]

   ;	      rval |= sp[6][((rt >> 2 ) ^ keybuf->kn[num][6]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+6]
	inc	bh
	xor	bl,dl
	shl	bl,2
	or	eax,_sp[ebx]

   ;
   ;	      rval |= sp[7][(rt ^ keybuf->kn[num][7]) & 0x3f];
   ;
	rol     edx,4
	mov	bl,byte ptr [esi+7]
	inc	bh
	xor	bl,dl
	shl	bl,2
   	or	eax,_sp[ebx]

   ;	      block[0] ^= rval;
   ;
	xor	dword ptr [ebp-8],eax
@48@d450:
	dec	di
@18@d114:
	cmp	di,0
	jge	@18@d58
   ;	   work[0] = byteswap(work[0]);
   ;

	mov	ebx,[ebp-8]
	ror	bx,8
	ror	ebx,16
	ror	bx,8
	mov	dword ptr[ebp-8],ebx
   ;	   work[1] = bswapcfb1(work[1]);
   ;
	mov	ebx,[ebp-4]
	ror	bx,8
	ror	ebx,16
	ror	bx,8
	mov	dword ptr[ebp-4],ebx
   ;	   permute((char *)work,fperm,block);  /* Inverse initial permutation */
   ;


   ;	   /* Clear output block     */
   ;	   for (i=8, ob = outblock; i !=0; i--)
   ;
	mov	esi,8
	mov     ebx,[ebp+12]
	jmp	short @581@d282
@581@d226:
   ;
   ;	      *ob++ = 0;
   ;
	mov	byte ptr [ebx],0
	inc     ebx
	dec	esi
@581@d282:
	or	esi,esi
	jne	short @581@d226
   ;
   ;
   ;	   ib = inblock;
   ;
	lea	eax,[ebp-8]
	mov	dword ptr [ebp-16],eax
   ;
   ;	   for (j = 0; j < 16; j += 2, ib++) { /* for each input nibble */
   ;
	mov	ecx,offset FLAT:_fperm
	xor	edx,edx
	jmp	short @581@d478
@581@d338:
   ;
   ;	      p = perm[j][(*ib >> 4) & 017];
   ;
	mov	ebx,[ebp-16]
	mov	al,byte ptr [ebx]
	and	eax,240
	shr	eax,1
	add	eax,edx
	add	eax,ecx
	mov	esi,eax
   ;
   ;	      q = perm[j + 1][*ib & 017];
   ;
	mov	al,byte ptr [ebx]
	and	eax,15
	shl	eax,3
	add	edx,128
	add	eax,edx
	add	eax,ecx
	mov	edi,eax
   ;
   ;	      ob = outblock;
   ;
	mov	ebx,[ebp+12]
   ;
   ;	      for (i =8; i !=0; i--){   /* and each output byte */
   ;
	add	edx,8
	jmp	short @581@d422
@581@d366:
   ;
   ;		 *ob++ |= *p++ | *q++;  /* OR the masks together*/
   ;
	mov	al,byte ptr [esi]
	or	al,byte ptr [edi]
	or	byte ptr [ebx],al
	inc	esi
	inc	edi
	inc     ebx
	dec	edx
@581@d422:
	cmp	dl,128
	jne	short @581@d366
	add     dx,128
	inc	dword ptr [ebp-16]
@581@d478:
	cmp	dx,2048
	jge short	@@d5814
	jmp	@581@d338
@@d5814:
@581@d506:
	pop     edx
	pop     ecx
	pop     ebx
	pop	edi
	pop	esi
	mov	esp,ebp
	pop	ebp
	ret
_des_decrypt	endp


ENCDEC_TEXT	ends
	end



