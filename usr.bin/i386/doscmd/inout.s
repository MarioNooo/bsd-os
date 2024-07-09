	.section ".text"
	.type in,@function
	.align 2
	.global in
in:
	pushl %ebp
	movl %esp,%ebp
	movw 8(%ebp),%dx
	in %dx,%al
	movzbl %al,%eax
	leave
	ret

	.type out,@function
	.align 2
	.global out
out:
	pushl %ebp
	movl %esp,%ebp
	movw 8(%ebp),%dx
	movb 10(%ebp),%al
	out %al,%dx
	leave
	ret
