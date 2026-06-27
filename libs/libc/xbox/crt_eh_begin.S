/* Marks the start of the merged .eh_frame for libunwind baremetal
   discovery (findUnwindSections reads &__eh_frame_start). Linked FIRST.
   i386 COFF mangles C __eh_frame_start to ___eh_frame_start. */
	.section .eh_frame,"dr"
	.globl ___eh_frame_start
___eh_frame_start:
