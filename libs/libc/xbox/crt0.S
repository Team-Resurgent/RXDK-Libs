	.text
	.globl _xbox_image_anchor
	.globl _start
_xbox_image_anchor:
_start:
	call	xbox_runtime_init
	subl	$12, %esp
	call	main
	addl	$12, %esp
	call	xbox_halt
