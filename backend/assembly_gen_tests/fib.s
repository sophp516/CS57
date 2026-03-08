	.text
	.globl	func
	.type	func, @function
func:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	pushl	%ebx
.Lfunc_0:
	movl	$1, -12(%ebp)
	movl	$1, -16(%ebp)
	movl	$0, -20(%ebp)
	jmp	.Lfunc_1
.Lfunc_1:
	movl	-20(%ebp), %ebx
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
	movl	-12(%ebp), %ebx
	pushl	%ecx
	pushl	%edx
	pushl	%ebx
	call	print
	addl	$4, %esp
	popl	%edx
	popl	%ecx
	movl	-20(%ebp), %ebx
	addl	$1, %ebx
	movl	%ebx, -20(%ebp)
	movl	-16(%ebp), %ebx
	movl	%ebx, -24(%ebp)
	movl	-12(%ebp), %ebx
	movl	-16(%ebp), %ecx
	addl	%ecx, %ebx
	movl	%ebx, -16(%ebp)
	movl	-24(%ebp), %ebx
	movl	%ebx, -12(%ebp)
	jmp	.Lfunc_1
.Lfunc_3:
	movl	$1, %eax
	popl	%ebx
	leave
	ret
