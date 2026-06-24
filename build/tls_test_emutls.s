	.def	@feat.00;
	.scl	3;
	.type	0;
	.endef
	.globl	@feat.00
@feat.00 = 1
	.file	"tls_test.c"
	.def	_SetLastError;
	.scl	2;
	.type	32;
	.endef
	.text
	.globl	_SetLastError                   # -- Begin function SetLastError
	.p2align	4
_SetLastError:                          # @SetLastError
Lfunc_begin0:
	.cv_func_id 0
	.cv_file	1 "D:\\Git\\RXDK-LibsZig\\build\\tls_test.c" "F541540528F3C171113E55C3D1E86FA3" 1
	.cv_loc	0 1 3 0                         # build\tls_test.c:3:0
	.cfi_sections .debug_frame
	.cfi_startproc
	.cv_fpo_proc	_SetLastError 4
# %bb.0:
	pushl	%ebp
	.cfi_def_cfa_offset 8
	.cfi_offset %ebp, -8
	.cv_fpo_pushreg	%ebp
	movl	%esp, %ebp
	.cfi_def_cfa_register %ebp
	.cv_fpo_setframe	%ebp
	subl	$8, %esp
	.cv_fpo_stackalloc	8
	.cv_fpo_endprologue
	movl	8(%ebp), %eax
Ltmp0:
	movl	8(%ebp), %eax
	movl	%eax, -4(%ebp)                  # 4-byte Spill
	movl	%esp, %eax
	movl	$___emutls_v.XapiLastErrorCode, (%eax)
	calll	___emutls_get_address
	movl	-4(%ebp), %ecx                  # 4-byte Reload
	movl	%ecx, (%eax)
	addl	$8, %esp
	popl	%ebp
	retl
Ltmp1:
	.cv_fpo_endproc
Lfunc_end0:
	.cfi_endproc
                                        # -- End function
	.data
	.globl	___emutls_v.XapiLastErrorCode   # @__emutls_v.XapiLastErrorCode
	.p2align	2, 0x0
___emutls_v.XapiLastErrorCode:
	.long	4                               # 0x4
	.long	4                               # 0x4
	.long	0
	.long	0

	.section	.debug$S,"dr"
	.p2align	2, 0x0
	.long	4                               # Debug section magic
	.long	241
	.long	Ltmp3-Ltmp2                     # Subsection size
Ltmp2:
	.short	Ltmp5-Ltmp4                     # Record length
Ltmp4:
	.short	4353                            # Record kind: S_OBJNAME
	.long	0                               # Signature
	.asciz	"D:/Git/RXDK-LibsZig/.zig-cache/tmp/6e865a46f8b66905-tls_test.s" # Object name
	.p2align	2, 0x0
Ltmp5:
	.short	Ltmp7-Ltmp6                     # Record length
Ltmp6:
	.short	4412                            # Record kind: S_COMPILE3
	.long	0                               # Flags and language
	.short	7                               # CPUType
	.short	21                              # Frontend version
	.short	1
	.short	0
	.short	0
	.short	21010                           # Backend version
	.short	0
	.short	0
	.short	0
	.asciz	"clang version 21.1.0"          # Null-terminated compiler version string
	.p2align	2, 0x0
Ltmp7:
Ltmp3:
	.p2align	2, 0x0
	.cv_fpo_data	_SetLastError
	.long	241                             # Symbol subsection for SetLastError
	.long	Ltmp9-Ltmp8                     # Subsection size
Ltmp8:
	.short	Ltmp11-Ltmp10                   # Record length
Ltmp10:
	.short	4423                            # Record kind: S_GPROC32_ID
	.long	0                               # PtrParent
	.long	0                               # PtrEnd
	.long	0                               # PtrNext
	.long	Lfunc_end0-_SetLastError        # Code size
	.long	0                               # Offset after prologue
	.long	0                               # Offset before epilogue
	.long	4098                            # Function type index
	.secrel32	_SetLastError           # Function section relative address
	.secidx	_SetLastError                   # Function section index
	.byte	193                             # Flags
	.asciz	"SetLastError"                  # Function name
	.p2align	2, 0x0
Ltmp11:
	.short	Ltmp13-Ltmp12                   # Record length
Ltmp12:
	.short	4114                            # Record kind: S_FRAMEPROC
	.long	12                              # FrameSize
	.long	0                               # Padding
	.long	0                               # Offset of padding
	.long	0                               # Bytes of callee saved registers
	.long	0                               # Exception handler offset
	.short	0                               # Exception handler section
	.long	163840                          # Flags (defines frame register)
	.p2align	2, 0x0
Ltmp13:
	.short	Ltmp15-Ltmp14                   # Record length
Ltmp14:
	.short	4414                            # Record kind: S_LOCAL
	.long	34                              # TypeIndex
	.short	1                               # Flags
	.asciz	"x"
	.p2align	2, 0x0
Ltmp15:
	.cv_def_range	 Ltmp0 Ltmp1, frame_ptr_rel, 8
	.short	2                               # Record length
	.short	4431                            # Record kind: S_PROC_ID_END
Ltmp9:
	.p2align	2, 0x0
	.cv_linetable	0, _SetLastError, Lfunc_end0
	.long	241                             # Symbol subsection for globals
	.long	Ltmp17-Ltmp16                   # Subsection size
Ltmp16:
	.short	Ltmp19-Ltmp18                   # Record length
Ltmp18:
	.short	4371                            # Record kind: S_GTHREAD32
	.long	34                              # Type
	.secrel32	_XapiLastErrorCode      # DataOffset
	.secidx	_XapiLastErrorCode              # Segment
	.asciz	"XapiLastErrorCode"             # Name
	.p2align	2, 0x0
Ltmp19:
Ltmp17:
	.p2align	2, 0x0
	.long	241
	.long	Ltmp21-Ltmp20                   # Subsection size
Ltmp20:
	.short	Ltmp23-Ltmp22                   # Record length
Ltmp22:
	.short	4360                            # Record kind: S_UDT
	.long	34                              # Type
	.asciz	"DWORD"
	.p2align	2, 0x0
Ltmp23:
Ltmp21:
	.p2align	2, 0x0
	.cv_filechecksums                       # File index to string table offset subsection
	.cv_stringtable                         # String table
	.long	241
	.long	Ltmp25-Ltmp24                   # Subsection size
Ltmp24:
	.short	Ltmp27-Ltmp26                   # Record length
Ltmp26:
	.short	4428                            # Record kind: S_BUILDINFO
	.long	4104                            # LF_BUILDINFO index
	.p2align	2, 0x0
Ltmp27:
Ltmp25:
	.p2align	2, 0x0
	.section	.debug$T,"dr"
	.p2align	2, 0x0
	.long	4                               # Debug section magic
	# ArgList (0x1000)
	.short	0xa                             # Record length
	.short	0x1201                          # Record kind: LF_ARGLIST
	.long	0x1                             # NumArgs
	.long	0x22                            # Argument: unsigned long
	# Procedure (0x1001)
	.short	0xe                             # Record length
	.short	0x1008                          # Record kind: LF_PROCEDURE
	.long	0x3                             # ReturnType: void
	.byte	0x0                             # CallingConvention: NearC
	.byte	0x0                             # FunctionOptions
	.short	0x1                             # NumParameters
	.long	0x1000                          # ArgListType: (unsigned long)
	# FuncId (0x1002)
	.short	0x1a                            # Record length
	.short	0x1601                          # Record kind: LF_FUNC_ID
	.long	0x0                             # ParentScope
	.long	0x1001                          # FunctionType: void (unsigned long)
	.asciz	"SetLastError"                  # Name
	.byte	243
	.byte	242
	.byte	241
	# StringId (0x1003)
	.short	0x1a                            # Record length
	.short	0x1605                          # Record kind: LF_STRING_ID
	.long	0x0                             # Id
	.asciz	"D:/Git/RXDK-LibsZig"           # StringData
	# StringId (0x1004)
	.short	0x1a                            # Record length
	.short	0x1605                          # Record kind: LF_STRING_ID
	.long	0x0                             # Id
	.asciz	"build/tls_test.c"              # StringData
	.byte	243
	.byte	242
	.byte	241
	# StringId (0x1005)
	.short	0xa                             # Record length
	.short	0x1605                          # Record kind: LF_STRING_ID
	.long	0x0                             # Id
	.byte	0                               # StringData
	.byte	243
	.byte	242
	.byte	241
	# StringId (0x1006)
	.short	0x8e                            # Record length
	.short	0x1605                          # Record kind: LF_STRING_ID
	.long	0x0                             # Id
	.asciz	"C:/Users/eq2k/AppData/Local/Microsoft/WinGet/Packages/zig.zig_Microsoft.Winget.Source_8wekyb3d8bbwe/zig-x86_64-windows-0.16.0/zig.exe" # StringData
	.byte	242
	.byte	241
	# StringId (0x1007)
	.short	0x238e                          # Record length
	.short	0x1605                          # Record kind: LF_STRING_ID
	.long	0x0                             # Id
	.asciz	"\"-cc1\" \"-triple\" \"i386-unknown-windows-gnu\" \"-O0\" \"-S\" \"-disable-free\" \"-clear-ast-before-backend\" \"-disable-llvm-verifier\" \"-discard-value-names\" \"-mrelocation-model\" \"static\" \"-mframe-pointer=all\" \"-fmath-errno\" \"-ffp-contract=on\" \"-fno-rounding-math\" \"-mconstructor-aliases\" \"-mms-bitfields\" \"-ffreestanding\" \"-fno-sized-deallocation\" \"-fno-use-init-array\" \"-target-cpu\" \"pentium4\" \"-tune-cpu\" \"generic\" \"-gno-column-info\" \"-gcodeview\" \"-debug-info-kind=constructor\" \"-debugger-tuning=gdb\" \"-fdebug-compilation-dir=D:/Git/RXDK-LibsZig\" \"-fcoverage-compilation-dir=D:/Git/RXDK-LibsZig\" \"-nostdsysteminc\" \"-nobuiltininc\" \"-resource-dir\" \"C:/Users/eq2k/AppData/Local/Microsoft/WinGet/Packages/zig.zig_Microsoft.Winget.Source_8wekyb3d8bbwe/lib/clang/21\" \"-dependency-file\" \".zig-cache\\\\tmp\\\\6e865a46f8b66905-tls_test.s.d\" \"-MT\" \".zig-cache\\\\tmp\\\\6e865a46f8b66905-tls_test.s\" \"-sys-header-deps\" \"-MV\" \"-isystem\" \"C:\\\\Users\\\\eq2k\\\\AppData\\\\Local\\\\Microsoft\\\\WinGet\\\\Packages\\\\zig.zig_Microsoft.Winget.Source_8wekyb3d8bbwe\\\\zig-x86_64-windows-0.16.0\\\\lib\\\\include\" \"-isystem\" \"C:\\\\Users\\\\eq2k\\\\AppData\\\\Local\\\\Microsoft\\\\WinGet\\\\Packages\\\\zig.zig_Microsoft.Winget.Source_8wekyb3d8bbwe\\\\zig-x86_64-windows-0.16.0\\\\lib\\\\libc\\\\include\\\\x86-windows-gnu\" \"-isystem\" \"C:\\\\Users\\\\eq2k\\\\AppData\\\\Local\\\\Microsoft\\\\WinGet\\\\Packages\\\\zig.zig_Microsoft.Winget.Source_8wekyb3d8bbwe\\\\zig-x86_64-windows-0.16.0\\\\lib\\\\libc\\\\include\\\\generic-mingw\" \"-isystem\" \"C:\\\\Users\\\\eq2k\\\\AppData\\\\Local\\\\Microsoft\\\\WinGet\\\\Packages\\\\zig.zig_Microsoft.Winget.Source_8wekyb3d8bbwe\\\\zig-x86_64-windows-0.16.0\\\\lib\\\\libc\\\\include\\\\x86-windows-any\" \"-isystem\" \"C:\\\\Users\\\\eq2k\\\\AppData\\\\Local\\\\Microsoft\\\\WinGet\\\\Packages\\\\zig.zig_Microsoft.Winget.Source_8wekyb3d8bbwe\\\\zig-x86_64-windows-0.16.0\\\\lib\\\\libc\\\\include\\\\any-windows-any\" \"-D\" \"__MSVCRT_VERSION__=0xE00\" \"-D\" \"_WIN32_WINNT=0x0a00\" \"-Wno-pragma-pack\" \"-ferror-limit\" \"19\" \"-femulated-tls\" \"-stack-protector\" \"2\" \"-stack-protector-buffer-size\" \"4\" \"-fno-use-cxa-atexit\" \"-fms-extensions\" \"-fgnuc-version=4.2.1\" \"-fskip-odr-check-in-gmf\" \"-exception-model=dwarf\" \"-fno-spell-checking\" \"-target-cpu\" \"pentium4\" \"-target-feature\" \"-16bit-mode\" \"-target-feature\" \"+32bit-mode\" \"-target-feature\" \"-64bit\" \"-target-feature\" \"-adx\" \"-target-feature\" \"-aes\" \"-target-feature\" \"-allow-light-256-bit\" \"-target-feature\" \"-amx-avx512\" \"-target-feature\" \"-amx-bf16\" \"-target-feature\" \"-amx-complex\" \"-target-feature\" \"-amx-fp16\" \"-target-feature\" \"-amx-fp8\" \"-target-feature\" \"-amx-int8\" \"-target-feature\" \"-amx-movrs\" \"-target-feature\" \"-amx-tf32\" \"-target-feature\" \"-amx-tile\" \"-target-feature\" \"-amx-transpose\" \"-target-feature\" \"-avx\" \"-target-feature\" \"-avx10.1-512\" \"-target-feature\" \"-avx10.2-512\" \"-target-feature\" \"-avx2\" \"-target-feature\" \"-avx512bf16\" \"-target-feature\" \"-avx512bitalg\" \"-target-feature\" \"-avx512bw\" \"-target-feature\" \"-avx512cd\" \"-target-feature\" \"-avx512dq\" \"-target-feature\" \"-avx512f\" \"-target-feature\" \"-avx512fp16\" \"-target-feature\" \"-avx512ifma\" \"-target-feature\" \"-avx512vbmi\" \"-target-feature\" \"-avx512vbmi2\" \"-target-feature\" \"-avx512vl\" \"-target-feature\" \"-avx512vnni\" \"-target-feature\" \"-avx512vp2intersect\" \"-target-feature\" \"-avx512vpopcntdq\" \"-target-feature\" \"-avxifma\" \"-target-feature\" \"-avxneconvert\" \"-target-feature\" \"-avxvnni\" \"-target-feature\" \"-avxvnniint16\" \"-target-feature\" \"-avxvnniint8\" \"-target-feature\" \"-bmi\" \"-target-feature\" \"-bmi2\" \"-target-feature\" \"-branch-hint\" \"-target-feature\" \"-branchfusion\" \"-target-feature\" \"-ccmp\" \"-target-feature\" \"-cf\" \"-target-feature\" \"-cldemote\" \"-target-feature\" \"-clflushopt\" \"-target-feature\" \"-clwb\" \"-target-feature\" \"-clzero\" \"-target-feature\" \"+cmov\" \"-target-feature\" \"-cmpccxadd\" \"-target-feature\" \"-crc32\" \"-target-feature\" \"-cx16\" \"-target-feature\" \"+cx8\" \"-target-feature\" \"-egpr\" \"-target-feature\" \"-enqcmd\" \"-target-feature\" \"-ermsb\" \"-target-feature\" \"-evex512\" \"-target-feature\" \"-f16c\" \"-target-feature\" \"-false-deps-getmant\" \"-target-feature\" \"-false-deps-lzcnt-tzcnt\" \"-target-feature\" \"-false-deps-mulc\" \"-target-feature\" \"-false-deps-mullq\" \"-target-feature\" \"-false-deps-perm\" \"-target-feature\" \"-false-deps-popcnt\" \"-target-feature\" \"-false-deps-range\" \"-target-feature\" \"-fast-11bytenop\" \"-target-feature\" \"-fast-15bytenop\" \"-target-feature\" \"-fast-7bytenop\" \"-target-feature\" \"-fast-bextr\" \"-target-feature\" \"-fast-dpwssd\" \"-target-feature\" \"-fast-gather\" \"-target-feature\" \"-fast-hops\" \"-target-feature\" \"-fast-imm16\" \"-target-feature\" \"-fast-lzcnt\" \"-target-feature\" \"-fast-movbe\" \"-target-feature\" \"-fast-scalar-fsqrt\" \"-target-feature\" \"-fast-scalar-shift-masks\" \"-target-feature\" \"-fast-shld-rotate\" \"-target-feature\" \"-fast-variable-crosslane-shuffle\" \"-target-feature\" \"-fast-variable-perlane-shuffle\" \"-target-feature\" \"-fast-vector-fsqrt\" \"-target-feature\" \"-fast-vector-shift-masks\" \"-target-feature\" \"-faster-shift-than-shuffle\" \"-target-feature\" \"-fma\" \"-target-feature\" \"-fma4\" \"-target-feature\" \"-fsgsbase\" \"-target-feature\" \"-fsrm\" \"-target-feature\" \"+fxsr\" \"-target-feature\" \"-gfni\" \"-target-feature\" \"-harden-sls-ijmp\" \"-target-feature\" \"-harden-sls-ret\" \"-target-feature\" \"-hreset\" \"-target-feature\" \"-idivl-to-divb\" \"-target-feature\" \"-idivq-to-divl\" \"-target-feature\" \"-inline-asm-use-gpr32\" \"-target-feature\" \"-invpcid\" \"-target-feature\" \"-kl\" \"-target-feature\" \"-lea-sp\" \"-target-feature\" \"-lea-uses-ag\" \"-target-feature\" \"-lvi-cfi\" \"-target-feature\" \"-lvi-load-hardening\" \"-target-feature\" \"-lwp\" \"-target-feature\" \"-lzcnt\" \"-target-feature\" \"-macrofusion\" \"-target-feature\" \"+mmx\" \"-target-feature\" \"-movbe\" \"-target-feature\" \"-movdir64b\" \"-target-feature\" \"-movdiri\" \"-target-feature\" \"-movrs\" \"-target-feature\" \"-mwaitx\" \"-target-feature\" \"-ndd\" \"-target-feature\" \"-nf\" \"-target-feature\" \"-no-bypass-delay\" \"-target-feature\" \"-no-bypass-delay-blend\" \"-target-feature\" \"-no-bypass-delay-mov\" \"-target-feature\" \"-no-bypass-delay-shuffle\" \"-target-feature\" \"+nopl\" \"-target-feature\" \"-pad-short-functions\" \"-target-feature\" \"-pclmul\" \"-target-feature\" \"-pconfig\" \"-target-feature\" \"-pku\" \"-target-feature\" \"-popcnt\" \"-target-feature\" \"-ppx\" \"-target-feature\" \"-prefer-128-bit\" \"-target-feature\" \"-prefer-256-bit\" \"-target-feature\" \"-prefer-mask-registers\" \"-target-feature\" \"-prefer-movmsk-over-vtest\" \"-target-feature\" \"-prefer-no-gather\" \"-target-feature\" \"-prefer-no-scatter\" \"-target-feature\" \"-prefetchi\" \"-target-feature\" \"-prfchw\" \"-target-feature\" \"-ptwrite\" \"-target-feature\" \"-push2pop2\" \"-target-feature\" \"-raoint\" \"-target-feature\" \"-rdpid\" \"-target-feature\" \"-rdpru\" \"-target-feature\" \"-rdrnd\" \"-target-feature\" \"-rdseed\" \"-target-feature\" \"-retpoline\" \"-target-feature\" \"-retpoline-external-thunk\" \"-target-feature\" \"-retpoline-indirect-branches\" \"-target-feature\" \"-retpoline-indirect-calls\" \"-target-feature\" \"-rtm\" \"-target-feature\" \"-sahf\" \"-target-feature\" \"-sbb-dep-breaking\" \"-target-feature\" \"-serialize\" \"-target-feature\" \"-seses\" \"-target-feature\" \"-sgx\" \"-target-feature\" \"-sha\" \"-target-feature\" \"-sha512\" \"-target-feature\" \"-shstk\" \"-target-feature\" \"-slow-3ops-lea\" \"-target-feature\" \"-slow-incdec\" \"-target-feature\" \"-slow-lea\" \"-target-feature\" \"-slow-pmaddwd\" \"-target-feature\" \"-slow-pmulld\" \"-target-feature\" \"-slow-shld\" \"-target-feature\" \"-slow-two-mem-ops\" \"-target-feature\" \"+slow-unaligned-mem-16\" \"-target-feature\" \"-slow-unaligned-mem-32\" \"-target-feature\" \"-sm3\" \"-target-feature\" \"-sm4\" \"-target-feature\" \"+sse\" \"-target-feature\" \"+sse2\" \"-target-feature\" \"-sse3\" \"-target-feature\" \"-sse4.1\" \"-target-feature\" \"-sse4.2\" \"-target-feature\" \"-sse4a\" \"-target-feature\" \"-sse-unaligned-mem\" \"-target-feature\" \"-ssse3\" \"-target-feature\" \"-tagged-globals\" \"-target-feature\" \"-tbm\" \"-target-feature\" \"-tsxldtrk\" \"-target-feature\" \"-tuning-fast-imm-vector-shift\" \"-target-feature\" \"-uintr\" \"-target-feature\" \"-use-glm-div-sqrt-costs\" \"-target-feature\" \"-use-slm-arith-costs\" \"-target-feature\" \"-usermsr\" \"-target-feature\" \"-vaes\" \"-target-feature\" \"-vpclmulqdq\" \"-target-feature\" \"+vzeroupper\" \"-target-feature\" \"-waitpkg\" \"-target-feature\" \"-wbnoinvd\" \"-target-feature\" \"-widekl\" \"-target-feature\" \"+x87\" \"-target-feature\" \"-xop\" \"-target-feature\" \"-xsave\" \"-target-feature\" \"-xsavec\" \"-target-feature\" \"-xsaveopt\" \"-target-feature\" \"-xsaves\" \"-target-feature\" \"-zu\" \"-fsanitize=alignment,array-bounds,bool,builtin,enum,float-cast-overflow,integer-divide-by-zero,nonnull-attribute,null,pointer-overflow,return,returns-nonnull-attribute,shift-base,shift-exponent,signed-integer-overflow,unreachable,vla-bound\" \"-fsanitize-recover=alignment,array-bounds,bool,builtin,enum,float-cast-overflow,integer-divide-by-zero,nonnull-attribute,null,pointer-overflow,returns-nonnull-attribute,shift-base,shift-exponent,signed-integer-overflow,vla-bound\" \"-fsanitize-merge=alignment,array-bounds,bool,builtin,enum,float-cast-overflow,integer-divide-by-zero,nonnull-attribute,null,pointer-overflow,return,returns-nonnull-attribute,shift-base,shift-exponent,signed-integer-overflow,unreachable,vla-bound\" \"-fno-sanitize-memory-param-retval\" \"-fno-sanitize-address-use-odr-indicator\" \"-faddrsig\" \"-x\" \"c\" \"build\\\\tls_test.c\"" # StringData
	# BuildInfo (0x1008)
	.short	0x1a                            # Record length
	.short	0x1603                          # Record kind: LF_BUILDINFO
	.short	0x5                             # NumArgs
	.long	0x1003                          # Argument: D:/Git/RXDK-LibsZig
	.long	0x1006                          # Argument: C:/Users/eq2k/AppData/Local/Microsoft/WinGet/Packages/zig.zig_Microsoft.Winget.Source_8wekyb3d8bbwe/zig-x86_64-windows-0.16.0/zig.exe
	.long	0x1004                          # Argument: build/tls_test.c
	.long	0x1005                          # Argument
	.long	0x1007                          # Argument: "-cc1" "-triple" "i386-unknown-windows-gnu" "-O0" "-S" "-disable-free" "-clear-ast-before-backend" "-disable-llvm-verifier" "-discard-value-names" "-mrelocation-model" "static" "-mframe-pointer=all" "-fmath-errno" "-ffp-contract=on" "-fno-rounding-math" "-mconstructor-aliases" "-mms-bitfields" "-ffreestanding" "-fno-sized-deallocation" "-fno-use-init-array" "-target-cpu" "pentium4" "-tune-cpu" "generic" "-gno-column-info" "-gcodeview" "-debug-info-kind=constructor" "-debugger-tuning=gdb" "-fdebug-compilation-dir=D:/Git/RXDK-LibsZig" "-fcoverage-compilation-dir=D:/Git/RXDK-LibsZig" "-nostdsysteminc" "-nobuiltininc" "-resource-dir" "C:/Users/eq2k/AppData/Local/Microsoft/WinGet/Packages/zig.zig_Microsoft.Winget.Source_8wekyb3d8bbwe/lib/clang/21" "-dependency-file" ".zig-cache\\tmp\\6e865a46f8b66905-tls_test.s.d" "-MT" ".zig-cache\\tmp\\6e865a46f8b66905-tls_test.s" "-sys-header-deps" "-MV" "-isystem" "C:\\Users\\eq2k\\AppData\\Local\\Microsoft\\WinGet\\Packages\\zig.zig_Microsoft.Winget.Source_8wekyb3d8bbwe\\zig-x86_64-windows-0.16.0\\lib\\include" "-isystem" "C:\\Users\\eq2k\\AppData\\Local\\Microsoft\\WinGet\\Packages\\zig.zig_Microsoft.Winget.Source_8wekyb3d8bbwe\\zig-x86_64-windows-0.16.0\\lib\\libc\\include\\x86-windows-gnu" "-isystem" "C:\\Users\\eq2k\\AppData\\Local\\Microsoft\\WinGet\\Packages\\zig.zig_Microsoft.Winget.Source_8wekyb3d8bbwe\\zig-x86_64-windows-0.16.0\\lib\\libc\\include\\generic-mingw" "-isystem" "C:\\Users\\eq2k\\AppData\\Local\\Microsoft\\WinGet\\Packages\\zig.zig_Microsoft.Winget.Source_8wekyb3d8bbwe\\zig-x86_64-windows-0.16.0\\lib\\libc\\include\\x86-windows-any" "-isystem" "C:\\Users\\eq2k\\AppData\\Local\\Microsoft\\WinGet\\Packages\\zig.zig_Microsoft.Winget.Source_8wekyb3d8bbwe\\zig-x86_64-windows-0.16.0\\lib\\libc\\include\\any-windows-any" "-D" "__MSVCRT_VERSION__=0xE00" "-D" "_WIN32_WINNT=0x0a00" "-Wno-pragma-pack" "-ferror-limit" "19" "-femulated-tls" "-stack-protector" "2" "-stack-protector-buffer-size" "4" "-fno-use-cxa-atexit" "-fms-extensions" "-fgnuc-version=4.2.1" "-fskip-odr-check-in-gmf" "-exception-model=dwarf" "-fno-spell-checking" "-target-cpu" "pentium4" "-target-feature" "-16bit-mode" "-target-feature" "+32bit-mode" "-target-feature" "-64bit" "-target-feature" "-adx" "-target-feature" "-aes" "-target-feature" "-allow-light-256-bit" "-target-feature" "-amx-avx512" "-target-feature" "-amx-bf16" "-target-feature" "-amx-complex" "-target-feature" "-amx-fp16" "-target-feature" "-amx-fp8" "-target-feature" "-amx-int8" "-target-feature" "-amx-movrs" "-target-feature" "-amx-tf32" "-target-feature" "-amx-tile" "-target-feature" "-amx-transpose" "-target-feature" "-avx" "-target-feature" "-avx10.1-512" "-target-feature" "-avx10.2-512" "-target-feature" "-avx2" "-target-feature" "-avx512bf16" "-target-feature" "-avx512bitalg" "-target-feature" "-avx512bw" "-target-feature" "-avx512cd" "-target-feature" "-avx512dq" "-target-feature" "-avx512f" "-target-feature" "-avx512fp16" "-target-feature" "-avx512ifma" "-target-feature" "-avx512vbmi" "-target-feature" "-avx512vbmi2" "-target-feature" "-avx512vl" "-target-feature" "-avx512vnni" "-target-feature" "-avx512vp2intersect" "-target-feature" "-avx512vpopcntdq" "-target-feature" "-avxifma" "-target-feature" "-avxneconvert" "-target-feature" "-avxvnni" "-target-feature" "-avxvnniint16" "-target-feature" "-avxvnniint8" "-target-feature" "-bmi" "-target-feature" "-bmi2" "-target-feature" "-branch-hint" "-target-feature" "-branchfusion" "-target-feature" "-ccmp" "-target-feature" "-cf" "-target-feature" "-cldemote" "-target-feature" "-clflushopt" "-target-feature" "-clwb" "-target-feature" "-clzero" "-target-feature" "+cmov" "-target-feature" "-cmpccxadd" "-target-feature" "-crc32" "-target-feature" "-cx16" "-target-feature" "+cx8" "-target-feature" "-egpr" "-target-feature" "-enqcmd" "-target-feature" "-ermsb" "-target-feature" "-evex512" "-target-feature" "-f16c" "-target-feature" "-false-deps-getmant" "-target-feature" "-false-deps-lzcnt-tzcnt" "-target-feature" "-false-deps-mulc" "-target-feature" "-false-deps-mullq" "-target-feature" "-false-deps-perm" "-target-feature" "-false-deps-popcnt" "-target-feature" "-false-deps-range" "-target-feature" "-fast-11bytenop" "-target-feature" "-fast-15bytenop" "-target-feature" "-fast-7bytenop" "-target-feature" "-fast-bextr" "-target-feature" "-fast-dpwssd" "-target-feature" "-fast-gather" "-target-feature" "-fast-hops" "-target-feature" "-fast-imm16" "-target-feature" "-fast-lzcnt" "-target-feature" "-fast-movbe" "-target-feature" "-fast-scalar-fsqrt" "-target-feature" "-fast-scalar-shift-masks" "-target-feature" "-fast-shld-rotate" "-target-feature" "-fast-variable-crosslane-shuffle" "-target-feature" "-fast-variable-perlane-shuffle" "-target-feature" "-fast-vector-fsqrt" "-target-feature" "-fast-vector-shift-masks" "-target-feature" "-faster-shift-than-shuffle" "-target-feature" "-fma" "-target-feature" "-fma4" "-target-feature" "-fsgsbase" "-target-feature" "-fsrm" "-target-feature" "+fxsr" "-target-feature" "-gfni" "-target-feature" "-harden-sls-ijmp" "-target-feature" "-harden-sls-ret" "-target-feature" "-hreset" "-target-feature" "-idivl-to-divb" "-target-feature" "-idivq-to-divl" "-target-feature" "-inline-asm-use-gpr32" "-target-feature" "-invpcid" "-target-feature" "-kl" "-target-feature" "-lea-sp" "-target-feature" "-lea-uses-ag" "-target-feature" "-lvi-cfi" "-target-feature" "-lvi-load-hardening" "-target-feature" "-lwp" "-target-feature" "-lzcnt" "-target-feature" "-macrofusion" "-target-feature" "+mmx" "-target-feature" "-movbe" "-target-feature" "-movdir64b" "-target-feature" "-movdiri" "-target-feature" "-movrs" "-target-feature" "-mwaitx" "-target-feature" "-ndd" "-target-feature" "-nf" "-target-feature" "-no-bypass-delay" "-target-feature" "-no-bypass-delay-blend" "-target-feature" "-no-bypass-delay-mov" "-target-feature" "-no-bypass-delay-shuffle" "-target-feature" "+nopl" "-target-feature" "-pad-short-functions" "-target-feature" "-pclmul" "-target-feature" "-pconfig" "-target-feature" "-pku" "-target-feature" "-popcnt" "-target-feature" "-ppx" "-target-feature" "-prefer-128-bit" "-target-feature" "-prefer-256-bit" "-target-feature" "-prefer-mask-registers" "-target-feature" "-prefer-movmsk-over-vtest" "-target-feature" "-prefer-no-gather" "-target-feature" "-prefer-no-scatter" "-target-feature" "-prefetchi" "-target-feature" "-prfchw" "-target-feature" "-ptwrite" "-target-feature" "-push2pop2" "-target-feature" "-raoint" "-target-feature" "-rdpid" "-target-feature" "-rdpru" "-target-feature" "-rdrnd" "-target-feature" "-rdseed" "-target-feature" "-retpoline" "-target-feature" "-retpoline-external-thunk" "-target-feature" "-retpoline-indirect-branches" "-target-feature" "-retpoline-indirect-calls" "-target-feature" "-rtm" "-target-feature" "-sahf" "-target-feature" "-sbb-dep-breaking" "-target-feature" "-serialize" "-target-feature" "-seses" "-target-feature" "-sgx" "-target-feature" "-sha" "-target-feature" "-sha512" "-target-feature" "-shstk" "-target-feature" "-slow-3ops-lea" "-target-feature" "-slow-incdec" "-target-feature" "-slow-lea" "-target-feature" "-slow-pmaddwd" "-target-feature" "-slow-pmulld" "-target-feature" "-slow-shld" "-target-feature" "-slow-two-mem-ops" "-target-feature" "+slow-unaligned-mem-16" "-target-feature" "-slow-unaligned-mem-32" "-target-feature" "-sm3" "-target-feature" "-sm4" "-target-feature" "+sse" "-target-feature" "+sse2" "-target-feature" "-sse3" "-target-feature" "-sse4.1" "-target-feature" "-sse4.2" "-target-feature" "-sse4a" "-target-feature" "-sse-unaligned-mem" "-target-feature" "-ssse3" "-target-feature" "-tagged-globals" "-target-feature" "-tbm" "-target-feature" "-tsxldtrk" "-target-feature" "-tuning-fast-imm-vector-shift" "-target-feature" "-uintr" "-target-feature" "-use-glm-div-sqrt-costs" "-target-feature" "-use-slm-arith-costs" "-target-feature" "-usermsr" "-target-feature" "-vaes" "-target-feature" "-vpclmulqdq" "-target-feature" "+vzeroupper" "-target-feature" "-waitpkg" "-target-feature" "-wbnoinvd" "-target-feature" "-widekl" "-target-feature" "+x87" "-target-feature" "-xop" "-target-feature" "-xsave" "-target-feature" "-xsavec" "-target-feature" "-xsaveopt" "-target-feature" "-xsaves" "-target-feature" "-zu" "-fsanitize=alignment,array-bounds,bool,builtin,enum,float-cast-overflow,integer-divide-by-zero,nonnull-attribute,null,pointer-overflow,return,returns-nonnull-attribute,shift-base,shift-exponent,signed-integer-overflow,unreachable,vla-bound" "-fsanitize-recover=alignment,array-bounds,bool,builtin,enum,float-cast-overflow,integer-divide-by-zero,nonnull-attribute,null,pointer-overflow,returns-nonnull-attribute,shift-base,shift-exponent,signed-integer-overflow,vla-bound" "-fsanitize-merge=alignment,array-bounds,bool,builtin,enum,float-cast-overflow,integer-divide-by-zero,nonnull-attribute,null,pointer-overflow,return,returns-nonnull-attribute,shift-base,shift-exponent,signed-integer-overflow,unreachable,vla-bound" "-fno-sanitize-memory-param-retval" "-fno-sanitize-address-use-odr-indicator" "-faddrsig" "-x" "c" "build\\tls_test.c"
	.byte	242
	.byte	241
	.addrsig
