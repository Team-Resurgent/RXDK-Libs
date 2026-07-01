	.text
	.globl _start
_start:
	call	xbox_runtime_init
	call	xbox_start_main
	ret
