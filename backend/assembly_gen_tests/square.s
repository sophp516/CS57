	.text
	.globl	func
	.type	func, @function
func:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$12, %esp
	pushl	%ebx
.Lfunc_0:
	movl	8(%ebp), %ebx
	movl	8(%ebp), %ecx
	imull	%ecx, %ebx
	movl	%ebx, -12(%ebp)
	movl	-12(%ebp), %ebx
	movl	%ebx, %eax
	popl	%ebx
	leave
	ret
