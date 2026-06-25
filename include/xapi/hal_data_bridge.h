#ifndef RXDK_XAPI_HAL_DATA_BRIDGE_H
#define RXDK_XAPI_HAL_DATA_BRIDGE_H

/*
 * xboxkrnl DATA imports: the XBE IAT slot holds a pointer to the kernel variable,
 * not the value itself. Vendor xAPI uses HalDiskCachePartitionCount as ULONG.
 */

static __inline ULONG RxdkReadHalDiskCachePartitionCount(void)
{
    return *(PULONG)(void *)&HalDiskCachePartitionCount;
}

#undef HalDiskCachePartitionCount
#define HalDiskCachePartitionCount RxdkReadHalDiskCachePartitionCount()

#endif
