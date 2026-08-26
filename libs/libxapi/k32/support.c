#include "bridge_k32.h"
/*
 * Copyright (C) 2026 Team-Resurgent
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Part of RXDK - see LICENSE.md for the full GNU GPL v3.
 */

/*
 * Parameter-conversion helpers shared by the base APIs: XapiFormatObjectAttributes
 * builds an NT OBJECT_ATTRIBUTES from a Win32 name, and XapiFormatTimeOut turns a
 * millisecond timeout into the NT relative-time LARGE_INTEGER form.
 */

#include "basedll.h"

POBJECT_ATTRIBUTES
XapiFormatObjectAttributes(
    OUT POBJECT_ATTRIBUTES ObjectAttributes,
    OUT POBJECT_STRING ObjectName,
    IN PCOSTR lpName
    )

/*++

Routine Description:

    This function transforms a Win32 security attributes structure into
    an NT object attributes structure.  It returns the address of the
    resulting structure (or NULL if SecurityAttributes was not
    specified).

Arguments:

    ObjectAttributes - Returns an initialized NT object attributes structure.

    ObjectName - Returns an initialized OBJECT_STRING structure.

    lpName - Supplies the name of the object relative to the
        ObWin32NamedObjectsDirectory() object directory.

Return Value:

    NON-NULL - Returns the ObjectAttributes value.  The structure is
        properly initialized by this function.

--*/

{
    RtlInitObjectString(ObjectName, lpName);

    InitializeObjectAttributes(
        ObjectAttributes,
        ObjectName,
        OBJ_OPENIF,
        ObWin32NamedObjectsDirectory(),
        NULL
        );

    return ObjectAttributes;
}

PLARGE_INTEGER
XapiFormatTimeOut(
    OUT PLARGE_INTEGER TimeOut,
    IN DWORD Milliseconds
    )

/*++

Routine Description:

    This function translates a Win32 style timeout to an NT relative
    timeout value.

Arguments:

    TimeOut - Returns an initialized NT timeout value that is equivalent
         to the Milliseconds parameter.

    Milliseconds - Supplies the timeout value in milliseconds.  A value
         of -1 indicates indefinite timeout.

Return Value:


    NULL - A value of null should be used to mimic the behavior of the
        specified Milliseconds parameter.

    NON-NULL - Returns the TimeOut value.  The structure is properly
        initialized by this function.

--*/

{
    if ( (LONG) Milliseconds == -1 ) {
        return( NULL );
        }
    TimeOut->QuadPart = UInt32x32To64( Milliseconds, 10000 );
    TimeOut->QuadPart *= -1;
    return TimeOut;
}
