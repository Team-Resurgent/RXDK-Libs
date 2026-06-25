/* stdcall entry points: lld references _Name; clang emits _Name@0. */
	.intel_syntax noprefix
	.text
	.global _XapiApplyKernelPatches@0
	.global _XapiApplyKernelPatches
	.extern _xapi_runtime_init
_XapiApplyKernelPatches@0:
_XapiApplyKernelPatches:
	call	_xapi_runtime_init
	ret
