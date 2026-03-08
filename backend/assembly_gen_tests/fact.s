	.text
	.globl	func
	.type	func, @function
func:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$16, %esp
	pushl	%ebx
.Lfunc_0:
	movl	$1, -16(%ebp)
	movl	$1, -12(%ebp)
	jmp	.Lfunc_1
.Lfunc_1:
	movl	-16(%ebp), %ebx
	movl	8(%ebp), %ecx
	movl	%ebx, %edx
	cmpl	%ecx, %edx
	setl	%al
	movzbl	%al, %edx
	movl	%edx, %eax
	cmpl	$0, %eax
	jne	.Lfunc_2
	jmp	.Lfunc_3
.Lfunc_2:
	movl	-16(%ebp), %ebx
	addl	$1, %ebx
	movl	%ebx, -16(%ebp)
	movl	-12(%ebp), %ebx
	movl	-16(%ebp), %ecx
	imull	%ecx, %ebx
	movl	%ebx, -12(%ebp)
	jmp	.Lfunc_1
.Lfunc_3:
	movl	-12(%ebp), %ebx
	movl	%ebx, %eax
	popl	%ebx
	leave
	ret
