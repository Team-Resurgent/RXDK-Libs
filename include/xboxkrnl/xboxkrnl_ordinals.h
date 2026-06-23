#ifndef RXDK_XBOXKRNL_ORDINALS_H
#define RXDK_XBOXKRNL_ORDINALS_H

#define RXDK_ORDINAL_AvGetSavedDataAddress 1
#define RXDK_ORDINAL_AvSendTVEncoderOption 2
#define RXDK_ORDINAL_AvSetDisplayMode 3
#define RXDK_ORDINAL_AvSetSavedDataAddress 4
#define RXDK_ORDINAL_DbgBreakPoint 5
#define RXDK_ORDINAL_DbgBreakPointWithStatus 6
#define RXDK_ORDINAL_DbgLoadImageSymbols 7
#define RXDK_ORDINAL_DbgPrint 8
#define RXDK_ORDINAL_HalReadSMCTrayState 9
#define RXDK_ORDINAL_DbgPrompt 10
#define RXDK_ORDINAL_DbgUnLoadImageSymbols 11
#define RXDK_ORDINAL_ExAcquireReadWriteLockExclusive 12
#define RXDK_ORDINAL_ExAcquireReadWriteLockShared 13
#define RXDK_ORDINAL_ExAllocatePool 14
#define RXDK_ORDINAL_ExAllocatePoolWithTag 15
#define RXDK_ORDINAL_ExEventObjectType 16 /* DATA */
#define RXDK_ORDINAL_ExFreePool 17
#define RXDK_ORDINAL_ExInitializeReadWriteLock 18
#define RXDK_ORDINAL_ExInterlockedAddLargeInteger 19
#define RXDK_ORDINAL_ExInterlockedAddLargeStatistic 20
#define RXDK_ORDINAL_ExInterlockedCompareExchange64 21
#define RXDK_ORDINAL_ExMutantObjectType 22 /* DATA */
#define RXDK_ORDINAL_ExQueryPoolBlockSize 23
#define RXDK_ORDINAL_ExQueryNonVolatileSetting 24
#define RXDK_ORDINAL_ExReadWriteRefurbInfo 25
#define RXDK_ORDINAL_ExRaiseException 26
#define RXDK_ORDINAL_ExRaiseStatus 27
#define RXDK_ORDINAL_ExReleaseReadWriteLock 28
#define RXDK_ORDINAL_ExSaveNonVolatileSetting 29
#define RXDK_ORDINAL_ExSemaphoreObjectType 30 /* DATA */
#define RXDK_ORDINAL_ExTimerObjectType 31 /* DATA */
#define RXDK_ORDINAL_ExfInterlockedInsertHeadList 32
#define RXDK_ORDINAL_ExfInterlockedInsertTailList 33
#define RXDK_ORDINAL_ExfInterlockedRemoveHeadList 34
#define RXDK_ORDINAL_FscGetCacheSize 35
#define RXDK_ORDINAL_FscInvalidateIdleBlocks 36
#define RXDK_ORDINAL_FscSetCacheSize 37
#define RXDK_ORDINAL_HalClearSoftwareInterrupt 38
#define RXDK_ORDINAL_HalDisableSystemInterrupt 39
#define RXDK_ORDINAL_HalDiskCachePartitionCount 40 /* DATA */
#define RXDK_ORDINAL_HalDiskModelNumber 41 /* DATA */
#define RXDK_ORDINAL_HalDiskSerialNumber 42 /* DATA */
#define RXDK_ORDINAL_HalEnableSystemInterrupt 43
#define RXDK_ORDINAL_HalGetInterruptVector 44
#define RXDK_ORDINAL_HalReadSMBusValue 45
#define RXDK_ORDINAL_HalReadWritePCISpace 46
#define RXDK_ORDINAL_HalRegisterShutdownNotification 47
#define RXDK_ORDINAL_HalRequestSoftwareInterrupt 48
#define RXDK_ORDINAL_HalReturnToFirmware 49
#define RXDK_ORDINAL_HalWriteSMBusValue 50
#define RXDK_ORDINAL_InterlockedCompareExchange 51
#define RXDK_ORDINAL_InterlockedDecrement 52
#define RXDK_ORDINAL_InterlockedIncrement 53
#define RXDK_ORDINAL_InterlockedExchange 54
#define RXDK_ORDINAL_InterlockedExchangeAdd 55
#define RXDK_ORDINAL_InterlockedFlushSList 56
#define RXDK_ORDINAL_InterlockedPopEntrySList 57
#define RXDK_ORDINAL_InterlockedPushEntrySList 58
#define RXDK_ORDINAL_IoAllocateIrp 59
#define RXDK_ORDINAL_IoBuildAsynchronousFsdRequest 60
#define RXDK_ORDINAL_IoBuildDeviceIoControlRequest 61
#define RXDK_ORDINAL_IoBuildSynchronousFsdRequest 62
#define RXDK_ORDINAL_IoCheckShareAccess 63
#define RXDK_ORDINAL_IoCompletionObjectType 64 /* DATA */
#define RXDK_ORDINAL_IoCreateDevice 65
#define RXDK_ORDINAL_IoCreateFile 66
#define RXDK_ORDINAL_IoCreateSymbolicLink 67
#define RXDK_ORDINAL_IoDeleteDevice 68
#define RXDK_ORDINAL_IoDeleteSymbolicLink 69
#define RXDK_ORDINAL_IoDeviceObjectType 70 /* DATA */
#define RXDK_ORDINAL_IoFileObjectType 71 /* DATA */
#define RXDK_ORDINAL_IoFreeIrp 72
#define RXDK_ORDINAL_IoInitializeIrp 73
#define RXDK_ORDINAL_IoInvalidDeviceRequest 74
#define RXDK_ORDINAL_IoQueryFileInformation 75
#define RXDK_ORDINAL_IoQueryVolumeInformation 76
#define RXDK_ORDINAL_IoQueueThreadIrp 77
#define RXDK_ORDINAL_IoRemoveShareAccess 78
#define RXDK_ORDINAL_IoSetIoCompletion 79
#define RXDK_ORDINAL_IoSetShareAccess 80
#define RXDK_ORDINAL_IoStartNextPacket 81
#define RXDK_ORDINAL_IoStartNextPacketByKey 82
#define RXDK_ORDINAL_IoStartPacket 83
#define RXDK_ORDINAL_IoSynchronousDeviceIoControlRequest 84
#define RXDK_ORDINAL_IoSynchronousFsdRequest 85
#define RXDK_ORDINAL_IofCallDriver 86
#define RXDK_ORDINAL_IofCompleteRequest 87
#define RXDK_ORDINAL_KdDebuggerEnabled 88 /* DATA */
#define RXDK_ORDINAL_KdDebuggerNotPresent 89 /* DATA */
#define RXDK_ORDINAL_IoDismountVolume 90
#define RXDK_ORDINAL_IoDismountVolumeByName 91
#define RXDK_ORDINAL_KeAlertResumeThread 92
#define RXDK_ORDINAL_KeAlertThread 93
#define RXDK_ORDINAL_KeBoostPriorityThread 94
#define RXDK_ORDINAL_KeBugCheck 95
#define RXDK_ORDINAL_KeBugCheckEx 96
#define RXDK_ORDINAL_KeCancelTimer 97
#define RXDK_ORDINAL_KeConnectInterrupt 98
#define RXDK_ORDINAL_KeDelayExecutionThread 99
#define RXDK_ORDINAL_KeDisconnectInterrupt 100
#define RXDK_ORDINAL_KeEnterCriticalRegion 101
#define RXDK_ORDINAL_MmGlobalData 102 /* DATA */
#define RXDK_ORDINAL_KeGetCurrentIrql 103
#define RXDK_ORDINAL_KeGetCurrentThread 104
#define RXDK_ORDINAL_KeInitializeApc 105
#define RXDK_ORDINAL_KeInitializeDeviceQueue 106
#define RXDK_ORDINAL_KeInitializeDpc 107
#define RXDK_ORDINAL_KeInitializeEvent 108
#define RXDK_ORDINAL_KeInitializeInterrupt 109
#define RXDK_ORDINAL_KeInitializeMutant 110
#define RXDK_ORDINAL_KeInitializeQueue 111
#define RXDK_ORDINAL_KeInitializeSemaphore 112
#define RXDK_ORDINAL_KeInitializeTimerEx 113
#define RXDK_ORDINAL_KeInsertByKeyDeviceQueue 114
#define RXDK_ORDINAL_KeInsertDeviceQueue 115
#define RXDK_ORDINAL_KeInsertHeadQueue 116
#define RXDK_ORDINAL_KeInsertQueue 117
#define RXDK_ORDINAL_KeInsertQueueApc 118
#define RXDK_ORDINAL_KeInsertQueueDpc 119
#define RXDK_ORDINAL_KeInterruptTime 120 /* DATA */
#define RXDK_ORDINAL_KeIsExecutingDpc 121
#define RXDK_ORDINAL_KeLeaveCriticalRegion 122
#define RXDK_ORDINAL_KePulseEvent 123
#define RXDK_ORDINAL_KeQueryBasePriorityThread 124
#define RXDK_ORDINAL_KeQueryInterruptTime 125
#define RXDK_ORDINAL_KeQueryPerformanceCounter 126
#define RXDK_ORDINAL_KeQueryPerformanceFrequency 127
#define RXDK_ORDINAL_KeQuerySystemTime 128
#define RXDK_ORDINAL_KeRaiseIrqlToDpcLevel 129
#define RXDK_ORDINAL_KeRaiseIrqlToSynchLevel 130
#define RXDK_ORDINAL_KeReleaseMutant 131
#define RXDK_ORDINAL_KeReleaseSemaphore 132
#define RXDK_ORDINAL_KeRemoveByKeyDeviceQueue 133
#define RXDK_ORDINAL_KeRemoveDeviceQueue 134
#define RXDK_ORDINAL_KeRemoveEntryDeviceQueue 135
#define RXDK_ORDINAL_KeRemoveQueue 136
#define RXDK_ORDINAL_KeRemoveQueueDpc 137
#define RXDK_ORDINAL_KeResetEvent 138
#define RXDK_ORDINAL_KeRestoreFloatingPointState 139
#define RXDK_ORDINAL_KeResumeThread 140
#define RXDK_ORDINAL_KeRundownQueue 141
#define RXDK_ORDINAL_KeSaveFloatingPointState 142
#define RXDK_ORDINAL_KeSetBasePriorityThread 143
#define RXDK_ORDINAL_KeSetDisableBoostThread 144
#define RXDK_ORDINAL_KeSetEvent 145
#define RXDK_ORDINAL_KeSetEventBoostPriority 146
#define RXDK_ORDINAL_KeSetPriorityProcess 147
#define RXDK_ORDINAL_KeSetPriorityThread 148
#define RXDK_ORDINAL_KeSetTimer 149
#define RXDK_ORDINAL_KeSetTimerEx 150
#define RXDK_ORDINAL_KeStallExecutionProcessor 151
#define RXDK_ORDINAL_KeSuspendThread 152
#define RXDK_ORDINAL_KeSynchronizeExecution 153
#define RXDK_ORDINAL_KeSystemTime 154 /* DATA */
#define RXDK_ORDINAL_KeTestAlertThread 155
#define RXDK_ORDINAL_KeTickCount 156 /* DATA */
#define RXDK_ORDINAL_KeTimeIncrement 157 /* DATA */
#define RXDK_ORDINAL_KeWaitForMultipleObjects 158
#define RXDK_ORDINAL_KeWaitForSingleObject 159
#define RXDK_ORDINAL_KfRaiseIrql 160
#define RXDK_ORDINAL_KfLowerIrql 161
#define RXDK_ORDINAL_KiBugCheckData 162 /* DATA */
#define RXDK_ORDINAL_KiUnlockDispatcherDatabase 163
#define RXDK_ORDINAL_LaunchDataPage 164 /* DATA */
#define RXDK_ORDINAL_MmAllocateContiguousMemory 165
#define RXDK_ORDINAL_MmAllocateContiguousMemoryEx 166
#define RXDK_ORDINAL_MmAllocateSystemMemory 167
#define RXDK_ORDINAL_MmClaimGpuInstanceMemory 168
#define RXDK_ORDINAL_MmCreateKernelStack 169
#define RXDK_ORDINAL_MmDeleteKernelStack 170
#define RXDK_ORDINAL_MmFreeContiguousMemory 171
#define RXDK_ORDINAL_MmFreeSystemMemory 172
#define RXDK_ORDINAL_MmGetPhysicalAddress 173
#define RXDK_ORDINAL_MmIsAddressValid 174
#define RXDK_ORDINAL_MmLockUnlockBufferPages 175
#define RXDK_ORDINAL_MmLockUnlockPhysicalPage 176
#define RXDK_ORDINAL_MmMapIoSpace 177
#define RXDK_ORDINAL_MmPersistContiguousMemory 178
#define RXDK_ORDINAL_MmQueryAddressProtect 179
#define RXDK_ORDINAL_MmQueryAllocationSize 180
#define RXDK_ORDINAL_MmQueryStatistics 181
#define RXDK_ORDINAL_MmSetAddressProtect 182
#define RXDK_ORDINAL_MmUnmapIoSpace 183
#define RXDK_ORDINAL_NtAllocateVirtualMemory 184
#define RXDK_ORDINAL_NtCancelTimer 185
#define RXDK_ORDINAL_NtClearEvent 186
#define RXDK_ORDINAL_NtClose 187
#define RXDK_ORDINAL_NtCreateDirectoryObject 188
#define RXDK_ORDINAL_NtCreateEvent 189
#define RXDK_ORDINAL_NtCreateFile 190
#define RXDK_ORDINAL_NtCreateIoCompletion 191
#define RXDK_ORDINAL_NtCreateMutant 192
#define RXDK_ORDINAL_NtCreateSemaphore 193
#define RXDK_ORDINAL_NtCreateTimer 194
#define RXDK_ORDINAL_NtDeleteFile 195
#define RXDK_ORDINAL_NtDeviceIoControlFile 196
#define RXDK_ORDINAL_NtDuplicateObject 197
#define RXDK_ORDINAL_NtFlushBuffersFile 198
#define RXDK_ORDINAL_NtFreeVirtualMemory 199
#define RXDK_ORDINAL_NtFsControlFile 200
#define RXDK_ORDINAL_NtOpenDirectoryObject 201
#define RXDK_ORDINAL_NtOpenFile 202
#define RXDK_ORDINAL_NtOpenSymbolicLinkObject 203
#define RXDK_ORDINAL_NtProtectVirtualMemory 204
#define RXDK_ORDINAL_NtPulseEvent 205
#define RXDK_ORDINAL_NtQueueApcThread 206
#define RXDK_ORDINAL_NtQueryDirectoryFile 207
#define RXDK_ORDINAL_NtQueryDirectoryObject 208
#define RXDK_ORDINAL_NtQueryEvent 209
#define RXDK_ORDINAL_NtQueryFullAttributesFile 210
#define RXDK_ORDINAL_NtQueryInformationFile 211
#define RXDK_ORDINAL_NtQueryIoCompletion 212
#define RXDK_ORDINAL_NtQueryMutant 213
#define RXDK_ORDINAL_NtQuerySemaphore 214
#define RXDK_ORDINAL_NtQuerySymbolicLinkObject 215
#define RXDK_ORDINAL_NtQueryTimer 216
#define RXDK_ORDINAL_NtQueryVirtualMemory 217
#define RXDK_ORDINAL_NtQueryVolumeInformationFile 218
#define RXDK_ORDINAL_NtReadFile 219
#define RXDK_ORDINAL_NtReadFileScatter 220
#define RXDK_ORDINAL_NtReleaseMutant 221
#define RXDK_ORDINAL_NtReleaseSemaphore 222
#define RXDK_ORDINAL_NtRemoveIoCompletion 223
#define RXDK_ORDINAL_NtResumeThread 224
#define RXDK_ORDINAL_NtSetEvent 225
#define RXDK_ORDINAL_NtSetInformationFile 226
#define RXDK_ORDINAL_NtSetIoCompletion 227
#define RXDK_ORDINAL_NtSetSystemTime 228
#define RXDK_ORDINAL_NtSetTimerEx 229
#define RXDK_ORDINAL_NtSignalAndWaitForSingleObjectEx 230
#define RXDK_ORDINAL_NtSuspendThread 231
#define RXDK_ORDINAL_NtUserIoApcDispatcher 232
#define RXDK_ORDINAL_NtWaitForSingleObject 233
#define RXDK_ORDINAL_NtWaitForSingleObjectEx 234
#define RXDK_ORDINAL_NtWaitForMultipleObjectsEx 235
#define RXDK_ORDINAL_NtWriteFile 236
#define RXDK_ORDINAL_NtWriteFileGather 237
#define RXDK_ORDINAL_NtYieldExecution 238
#define RXDK_ORDINAL_ObCreateObject 239
#define RXDK_ORDINAL_ObDirectoryObjectType 240 /* DATA */
#define RXDK_ORDINAL_ObInsertObject 241
#define RXDK_ORDINAL_ObMakeTemporaryObject 242
#define RXDK_ORDINAL_ObOpenObjectByName 243
#define RXDK_ORDINAL_ObOpenObjectByPointer 244
#define RXDK_ORDINAL_ObpObjectHandleTable 245 /* DATA */
#define RXDK_ORDINAL_ObReferenceObjectByHandle 246
#define RXDK_ORDINAL_ObReferenceObjectByName 247
#define RXDK_ORDINAL_ObReferenceObjectByPointer 248
#define RXDK_ORDINAL_ObSymbolicLinkObjectType 249 /* DATA */
#define RXDK_ORDINAL_ObfDereferenceObject 250
#define RXDK_ORDINAL_ObfReferenceObject 251
#define RXDK_ORDINAL_PhyGetLinkState 252
#define RXDK_ORDINAL_PhyInitialize 253
#define RXDK_ORDINAL_PsCreateSystemThread 254
#define RXDK_ORDINAL_PsCreateSystemThreadEx 255
#define RXDK_ORDINAL_PsQueryStatistics 256
#define RXDK_ORDINAL_PsSetCreateThreadNotifyRoutine 257
#define RXDK_ORDINAL_PsTerminateSystemThread 258
#define RXDK_ORDINAL_PsThreadObjectType 259 /* DATA */
#define RXDK_ORDINAL_RtlAnsiStringToUnicodeString 260
#define RXDK_ORDINAL_RtlAppendStringToString 261
#define RXDK_ORDINAL_RtlAppendUnicodeStringToString 262
#define RXDK_ORDINAL_RtlAppendUnicodeToString 263
#define RXDK_ORDINAL_RtlAssert 264
#define RXDK_ORDINAL_RtlCaptureContext 265
#define RXDK_ORDINAL_RtlCaptureStackBackTrace 266
#define RXDK_ORDINAL_RtlCharToInteger 267
#define RXDK_ORDINAL_RtlCompareMemory 268
#define RXDK_ORDINAL_RtlCompareMemoryUlong 269
#define RXDK_ORDINAL_RtlCompareString 270
#define RXDK_ORDINAL_RtlCompareUnicodeString 271
#define RXDK_ORDINAL_RtlCopyString 272
#define RXDK_ORDINAL_RtlCopyUnicodeString 273
#define RXDK_ORDINAL_RtlCreateUnicodeString 274
#define RXDK_ORDINAL_RtlDowncaseUnicodeChar 275
#define RXDK_ORDINAL_RtlDowncaseUnicodeString 276
#define RXDK_ORDINAL_RtlEnterCriticalSection 277
#define RXDK_ORDINAL_RtlEnterCriticalSectionAndRegion 278
#define RXDK_ORDINAL_RtlEqualString 279
#define RXDK_ORDINAL_RtlEqualUnicodeString 280
#define RXDK_ORDINAL_RtlExtendedIntegerMultiply 281
#define RXDK_ORDINAL_RtlExtendedLargeIntegerDivide 282
#define RXDK_ORDINAL_RtlExtendedMagicDivide 283
#define RXDK_ORDINAL_RtlFillMemory 284
#define RXDK_ORDINAL_RtlFillMemoryUlong 285
#define RXDK_ORDINAL_RtlFreeAnsiString 286
#define RXDK_ORDINAL_RtlFreeUnicodeString 287
#define RXDK_ORDINAL_RtlGetCallersAddress 288
#define RXDK_ORDINAL_RtlInitAnsiString 289
#define RXDK_ORDINAL_RtlInitUnicodeString 290
#define RXDK_ORDINAL_RtlInitializeCriticalSection 291
#define RXDK_ORDINAL_RtlIntegerToChar 292
#define RXDK_ORDINAL_RtlIntegerToUnicodeString 293
#define RXDK_ORDINAL_RtlLeaveCriticalSection 294
#define RXDK_ORDINAL_RtlLeaveCriticalSectionAndRegion 295
#define RXDK_ORDINAL_RtlLowerChar 296
#define RXDK_ORDINAL_RtlMapGenericMask 297
#define RXDK_ORDINAL_RtlMoveMemory 298
#define RXDK_ORDINAL_RtlMultiByteToUnicodeN 299
#define RXDK_ORDINAL_RtlMultiByteToUnicodeSize 300
#define RXDK_ORDINAL_RtlNtStatusToDosError 301
#define RXDK_ORDINAL_RtlRaiseException 302
#define RXDK_ORDINAL_RtlRaiseStatus 303
#define RXDK_ORDINAL_RtlTimeFieldsToTime 304
#define RXDK_ORDINAL_RtlTimeToTimeFields 305
#define RXDK_ORDINAL_RtlTryEnterCriticalSection 306
#define RXDK_ORDINAL_RtlUlongByteSwap 307
#define RXDK_ORDINAL_RtlUnicodeStringToAnsiString 308
#define RXDK_ORDINAL_RtlUnicodeStringToInteger 309
#define RXDK_ORDINAL_RtlUnicodeToMultiByteN 310
#define RXDK_ORDINAL_RtlUnicodeToMultiByteSize 311
#define RXDK_ORDINAL_RtlUnwind 312
#define RXDK_ORDINAL_RtlUpcaseUnicodeChar 313
#define RXDK_ORDINAL_RtlUpcaseUnicodeString 314
#define RXDK_ORDINAL_RtlUpcaseUnicodeToMultiByteN 315
#define RXDK_ORDINAL_RtlUpperChar 316
#define RXDK_ORDINAL_RtlUpperString 317
#define RXDK_ORDINAL_RtlUshortByteSwap 318
#define RXDK_ORDINAL_RtlWalkFrameChain 319
#define RXDK_ORDINAL_RtlZeroMemory 320
#define RXDK_ORDINAL_XboxEEPROMKey 321 /* DATA */
#define RXDK_ORDINAL_XboxHardwareInfo 322 /* DATA */
#define RXDK_ORDINAL_XboxHDKey 323 /* DATA */
#define RXDK_ORDINAL_XboxKrnlVersion 324 /* DATA */
#define RXDK_ORDINAL_XboxSignatureKey 325 /* DATA */
#define RXDK_ORDINAL_XeImageFileName 326 /* DATA */
#define RXDK_ORDINAL_XeLoadSection 327
#define RXDK_ORDINAL_XeUnloadSection 328
#define RXDK_ORDINAL_READ_PORT_BUFFER_UCHAR 329
#define RXDK_ORDINAL_READ_PORT_BUFFER_USHORT 330
#define RXDK_ORDINAL_READ_PORT_BUFFER_ULONG 331
#define RXDK_ORDINAL_WRITE_PORT_BUFFER_UCHAR 332
#define RXDK_ORDINAL_WRITE_PORT_BUFFER_USHORT 333
#define RXDK_ORDINAL_WRITE_PORT_BUFFER_ULONG 334
#define RXDK_ORDINAL_XcSHAInit 335
#define RXDK_ORDINAL_XcSHAUpdate 336
#define RXDK_ORDINAL_XcSHAFinal 337
#define RXDK_ORDINAL_XcRC4Key 338
#define RXDK_ORDINAL_XcRC4Crypt 339
#define RXDK_ORDINAL_XcHMAC 340
#define RXDK_ORDINAL_XcPKEncPublic 341
#define RXDK_ORDINAL_XcPKDecPrivate 342
#define RXDK_ORDINAL_XcPKGetKeyLen 343
#define RXDK_ORDINAL_XcVerifyPKCS1Signature 344
#define RXDK_ORDINAL_XcModExp 345
#define RXDK_ORDINAL_XcDESKeyParity 346
#define RXDK_ORDINAL_XcKeyTable 347
#define RXDK_ORDINAL_XcBlockCrypt 348
#define RXDK_ORDINAL_XcBlockCryptCBC 349
#define RXDK_ORDINAL_XcCryptService 350
#define RXDK_ORDINAL_XcUpdateCrypto 351
#define RXDK_ORDINAL_RtlRip 352
#define RXDK_ORDINAL_XboxLANKey 353 /* DATA */
#define RXDK_ORDINAL_XboxAlternateSignatureKeys 354 /* DATA */
#define RXDK_ORDINAL_XePublicKeyData 355 /* DATA */
#define RXDK_ORDINAL_HalBootSMCVideoMode 356 /* DATA */
#define RXDK_ORDINAL_IdexChannelObject 357 /* DATA */
#define RXDK_ORDINAL_HalIsResetOrShutdownPending 358
#define RXDK_ORDINAL_IoMarkIrpMustComplete 359
#define RXDK_ORDINAL_HalInitiateShutdown 360
#define RXDK_ORDINAL_HalEnableSecureTrayEject 365
#define RXDK_ORDINAL_HalWriteSMCScratchRegister 366
#define RXDK_ORDINAL_MmDbgAllocateMemory 374
#define RXDK_ORDINAL_MmDbgFreeMemory 375
#define RXDK_ORDINAL_MmDbgQueryAvailablePages 376
#define RXDK_ORDINAL_MmDbgReleaseAddress 377
#define RXDK_ORDINAL_MmDbgWriteCheck 378

#endif
