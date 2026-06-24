/* RXDK naked SwitchToFiber — KeGetCurrentThread()->TlsData + image offset. */
	.intel_syntax noprefix
	.text
	.equ	RXDK_KTHREAD_TlsData, 40
	.extern _KeGetCurrentThread@0
	.extern _rxdk_xapi_current_fiber_tls_disp
	.global _SwitchToFiber@4
	.global _SwitchToFiber
_SwitchToFiber@4:
_SwitchToFiber:
	mov	eax, dword ptr [esp + 4]
	push	ebp
	push	esi
	push	edi
	push	ebx
	push	dword ptr fs:[0]
	push	eax
	call	_KeGetCurrentThread@0
	mov	edx, dword ptr [eax + RXDK_KTHREAD_TlsData]
	add	edx, 4
	pop	eax
	mov	edi, dword ptr _rxdk_xapi_current_fiber_tls_disp
	mov	ecx, dword ptr [edx + edi]
	mov	dword ptr [ecx + 12], esp
	mov	esp, dword ptr [eax + 12]
	mov	dword ptr [edx + edi], eax
	pop	dword ptr fs:[0]
	pop	ebx
	pop	edi
	pop	esi
	pop	ebp
	ret	4
