	.text
	.section	.note.GNU-stack,"",@progbits
	.globl	func
	.type	func, @function
func:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$20, %esp
	pushl	%ebx
.Lfunc_0:
	movl	$1, 8(%ebp)
	movl	$1, 8(%ebp)
	jmp	.Lfunc_2
.Lfunc_1:
	movl	8(%ebp), %ebx
	movl	%ebx, %eax
	popl	%ebx
	leave
	ret
