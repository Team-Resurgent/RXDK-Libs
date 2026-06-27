	.text
	.globl _xbox_image_anchor
	.globl _start
_xbox_image_anchor:
_start:
	call	xbox_runtime_init
	call	xbox_start_main
	ret
