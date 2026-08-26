/*
 * 2026 - Team Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Reduced Block Command (RBC) definitions, pared down from the Windows 2000
 * scsi.h. The Xbox MU protocol is an extreme subset of the SCSI commands and
 * packs a little extra information into the READ_CAPACITY command beyond the
 * RBC specification; SCSI compatibility is not required. Only the relevant
 * definitions are kept.
 */

#pragma once
#define __RBC_H__


//
// Command Descriptor Block. Passed by SCSI controller chip over the SCSI bus
//

#include <pshpack1.h>
typedef union _CDB {
    
    //
    // Standard 10-byte CDB
    //

    struct _CDB10 {
        UCHAR OperationCode;
        UCHAR RelativeAddress : 1;
        UCHAR Reserved1 : 2;
        UCHAR ForceUnitAccess : 1;
        UCHAR DisablePageOut : 1;
        UCHAR LogicalUnitNumber : 3;
        union {
            struct {
                UCHAR LogicalBlockByte0;
                UCHAR LogicalBlockByte1;
                UCHAR LogicalBlockByte2;
                UCHAR LogicalBlockByte3;
            };
            ULONG LogicalBlock;
        };
        UCHAR Reserved2;
        union {
            struct {
                UCHAR TransferBlocksMsb;
                UCHAR TransferBlocksLsb;
            };
            USHORT TransferBlocks;
        };
        UCHAR Control;
    } CDB10, *PCDB10;

    //
    // Access as array of ULONGS or BYTES
    //

    ULONG AsUlong[4];
    UCHAR AsByte[16];

} CDB, *PCDB;
#include <poppack.h>

//
// SCSI CDB operation codes
//

#define SCSIOP_READ_CAPACITY       0x25
#define SCSIOP_READ                0x28
#define SCSIOP_WRITE               0x2A
#define SCSIOP_VERIFY              0x2F

//
// Read Capacity Data - returned in Big Endian format
//
// (CAVEAT! - this is not the structure defined by SCSI!!!)
// We added the LogicalBlocKPerMediaBlock by carving away
// 16 bits 

typedef struct _READ_CAPACITY_DATA {
    ULONG  LogicalBlockAddress;
    USHORT LogicalBlocksPerMediaBlock; 
    USHORT BytesPerLogicalBlock;
} READ_CAPACITY_DATA, *PREAD_CAPACITY_DATA;

