	.text
	.globl	func
	.type	func, @function
func:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$4, %esp
	pushl	%ebx
.Lfunc_0:
	movl	8(%ebp), %eax
	popl	%ebx
	leave
	ret
