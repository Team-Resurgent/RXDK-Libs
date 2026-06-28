#include "bridge_usb.h"
/*++
    High-level Xbox debug-keyboard support -- the keystroke queue that turns the
    raw HID boot-keyboard reports (delivered by the XID input path through
    XID_pKeyboardServices->pfnNewData) into XINPUT_DEBUG_KEYSTROKE events that a
    title reads with XInputDebugGetKeystroke. Ported from the Xbox kernel source
    (private/ntos/dd/usb/xkbd/kbd.cpp) and built for SINGLE_KEYBOARD_ONLY (one
    shared queue; the hDevice argument to XInputDebugGetKeystroke is omitted).

    XInputGetState deliberately refuses keyboards (xidinp.cpp) -- keystrokes only
    come through this queue. The XID driver already calls our pfnOpen/Close/
    Remove/NewData hooks (xid.cpp); InitKeyboardQueue installs the service table.
--*/

#define _XAPI_
extern "C" {
#include <ntos.h>
}
#include <ntrtl.h>
#include <nturtl.h>
#include <xtl.h>
#include <xboxp.h>

#define MODULE_POOL_TAG '_DBK'
#include <debug.h>
DEFINE_USB_DEBUG_FUNCTIONS("KBD");

#include <usb.h>
#include "xid.h"
#include <xkbd.h>
#include <kbd.h>

//--------------------------------------------------------------------
//  Local Structure Definitions
//--------------------------------------------------------------------
typedef struct _XID_KEYBOARD_INSTANCE
{
    HANDLE                  hDevice;
    XINPUT_KEYBOARD         LastPacket;
    XINPUT_KEYBOARD_LEDS    KeyboardLeds;
    XINPUT_DEBUG_KEYSTROKE  *KeyQueue;
    ULONG                   QueueReadPos;
    ULONG                   QueueWritePos;
    XINPUT_DEBUG_KEYSTROKE  RepeatKeystroke;
    DWORD                   LastRepeatTick;
} XID_KEYBOARD_INSTANCE, *PXID_KEYBOARD_INSTANCE;

typedef struct _XID_KEYBOARD_STATE
{
    XID_KEYBOARD_INSTANCE             Keyboards[1];   // SINGLE_KEYBOARD_ONLY
    XINPUT_DEBUG_KEYQUEUE_PARAMETERS  QueueParameters;
} XID_KEYBOARD_STATE, *PXID_KEYBOARD_STATE;

static XID_KEYBOARD_STATE XID_KeyboardState = {0};

//--------------------------------------------------------------------
// Keyboard Services Service Table for XID
//--------------------------------------------------------------------
static void XID_KeyboardOpen(HANDLE hDevice);
static void XID_KeyboardClose(HANDLE hDevice);
static void XID_KeyboardRemove(HANDLE hDevice);
static void XID_KeyboardNewData(HANDLE hDevice, XINPUT_KEYBOARD *pPacket);

static XID_KEYBOARD_SERVICES XID_KeyboardServices =
{
    XID_KeyboardOpen,
    XID_KeyboardClose,
    XID_KeyboardRemove,
    XID_KeyboardNewData
};

//--------------------------------------------------------------------
// Local Utility Functions
//--------------------------------------------------------------------
static BOOL XID_KeyboardInitQueue(int iInstance, DWORD dwQueueLength);
static void XID_KeyboardReset(int iInstance);
static void XID_KeyboardUpdate(int iInstance, XINPUT_KEYBOARD *pPacket);
static XINPUT_DEBUG_KEYSTROKE *XID_KeyboardQueueGetWritePos(int iQueueIndex);
static XINPUT_DEBUG_KEYSTROKE *XID_KeyboardQueueIncrementWritePos(int iQueueIndex);
static VOID XID_KeyboardQueueHidKeystroke(UCHAR HidUsage, UCHAR Flags, PXINPUT_DEBUG_KEYSTROKE pKeystroke);

//--------------------------------------------------------------------
//  HID usage -> VK_ / ASCII translation tables (indexed by HID usage).
//--------------------------------------------------------------------
static UCHAR HidToVK_Table[] =
{
/*ZERO*/          '0',          /*RETURN*/        VK_RETURN,    /*ESCAPE*/    VK_ESCAPE,
/*BACKSPACE*/     VK_BACK,      /*TAB*/           VK_TAB,       /*SPACEBAR*/  VK_SPACE,
/*MINUS*/         VK_OEM_MINUS, /*EQUALS*/        VK_OEM_PLUS,  /*OPEN_BRACE*/VK_OEM_4,
/*CLOSE_BRACE*/   VK_OEM_6,     /*BACKSLASH*/     VK_OEM_5,     /*NON_US_TILDE*/VK_OEM_3,
/*COLON*/         VK_OEM_1,     /*QUOTE*/         VK_OEM_7,     /*TILDE*/     VK_OEM_3,
/*COMMA*/         VK_OEM_COMMA, /*PERIOD*/        VK_OEM_PERIOD,/*QUESTION*/  VK_OEM_2,
/*CAPS_LOCK*/     VK_CAPITAL,
/*F1*/  VK_F1,  /*F2*/  VK_F2,  /*F3*/  VK_F3,  /*F4*/  VK_F4,  /*F5*/  VK_F5,  /*F6*/  VK_F6,
/*F7*/  VK_F7,  /*F8*/  VK_F8,  /*F9*/  VK_F9,  /*F10*/ VK_F10, /*F11*/ VK_F11, /*F12*/ VK_F12,
/*PRINT_SCREEN*/  VK_PRINT,     /*SCROLL_LOCK*/   VK_SCROLL,    /*PAUSE*/     VK_PAUSE,
/*INSERT*/        VK_INSERT,    /*HOME*/          VK_HOME,      /*PAGE_UP*/   VK_PRIOR,
/*DELETE*/        VK_DELETE,    /*END*/           VK_END,       /*PAGE_DOWN*/ VK_NEXT,
/*RIGHT_ARROW*/   VK_RIGHT,     /*LEFT_ARROW*/    VK_LEFT,      /*DOWN_ARROW*/VK_DOWN,
/*UP_ARROW*/      VK_UP,        /*NUM_LOCK*/      VK_NUMLOCK,   /*KP_BACKSLASH*/VK_DIVIDE,
/*KP_ASTERICK*/   VK_MULTIPLY,  /*KP_MINUS*/      VK_SUBTRACT,  /*KP_PLUS*/   VK_ADD,
/*KP_ENTER*/      VK_SEPARATOR, /*KP_ONE*/        VK_NUMPAD1,   /*KP_TWO*/    VK_NUMPAD2,
/*KP_THREE*/      VK_NUMPAD3,   /*KP_FOUR*/       VK_NUMPAD4,   /*KP_FIVE*/   VK_NUMPAD5,
/*KP_SIX*/        VK_NUMPAD6,   /*KP_SEVEN*/      VK_NUMPAD7,   /*KP_EIGHT*/  VK_NUMPAD8,
/*KP_NINE*/       VK_NUMPAD9,   /*KP_ZERO*/       VK_NUMPAD0,   /*KP_DECIMAL*/VK_DECIMAL,
/*NON_US_BACKSLASH*/VK_OEM_5,   /*APPLICATION*/   VK_APPS,      /*POWER*/     VK_SLEEP,
/*KP_EQUALS*/     VK_OEM_NEC_EQUAL,
/*F13*/ VK_F13, /*F14*/ VK_F14, /*F15*/ VK_F15, /*F16*/ VK_F16, /*F17*/ VK_F17, /*F18*/ VK_F18,
/*F19*/ VK_F19, /*F20*/ VK_F20, /*F21*/ VK_F21, /*F22*/ VK_F22, /*F23*/ VK_F23, /*F24*/ VK_F24,
/*EXECUTE*/ VK_EXECUTE, /*HELP*/ VK_HELP, /*MENU*/ VK_MENU, /*SELECT*/ VK_SELECT,
/*STOP*/ VK_BROWSER_STOP, /*AGAIN*/ 0, /*UNDO*/ 0, /*CUT*/ 0, /*COPY*/ 0, /*PASTE*/ 0,
/*FIND*/ VK_BROWSER_SEARCH, /*MUTE*/ VK_VOLUME_MUTE, /*VOLUME_UP*/ VK_VOLUME_UP,
/*VOLUME_DOWN*/ VK_VOLUME_DOWN, /*LOCKING_CAPS*/ 0, /*LOCKING_NUM*/ 0, /*LOCKING_SCROLL*/ 0,
/*KP_COMMA*/ VK_DECIMAL, /*KP_EQUALS_AS400*/ VK_OEM_NEC_EQUAL,
/*INTERNATIONAL1*/ 0, /*INTERNATIONAL2*/ 0, /*INTERNATIONAL3*/ 0, /*INTERNATIONAL4*/ 0,
/*INTERNATIONAL5*/ 0, /*INTERNATIONAL6*/ 0, /*INTERNATIONAL7*/ 0, /*INTERNATIONAL8*/ 0,
/*INTERNATIONAL9*/ 0, /*LANG1*/ 0, /*LANG2*/ VK_HANJA, /*LANG3*/ VK_KANA,
/*LANG4*/ 0, /*LANG5*/ 0, /*LANG6*/ 0, /*LANG7*/ 0, /*LANG8*/ 0, /*LANG9*/ 0
};

static UCHAR HidModifierToVK_Table[] =
{
    VK_LCONTROL, VK_LSHIFT, VK_LMENU, VK_LWIN,
    VK_RCONTROL, VK_RSHIFT, VK_RMENU, VK_RWIN
};

static UCHAR HidSymToAscii_Table[] =
{
/*ZERO*/ '0', /*RETURN*/ '\n', /*ESCAPE*/ 27, /*BACKSPACE*/ '\b', /*TAB*/ '\t',
/*SPACEBAR*/ ' ', /*MINUS*/ '-', /*EQUALS*/ '=', /*OPEN_BRACE*/ '[', /*CLOSE_BRACE*/ ']',
/*BACKSLASH*/ '\\', /*NON_US_TILDE*/ '~', /*COLON*/ ';', /*QUOTE*/ '\'', /*TILDE*/ '`',
/*COMMA*/ ',', /*PERIOD*/ '.', /*QUESTION*/ '/'
};

static UCHAR HidSymToAsciiShift_Table[] =
{
/*ONE*/ '!', /*TWO*/ '@', /*THREE*/ '#', /*FOUR*/ '$', /*FIVE*/ '%', /*SIX*/ '^',
/*SEVEN*/ '&', /*EIGHT*/ '*', /*NINE*/ '(', /*ZERO*/ ')', /*RETURN*/ '\n', /*ESCAPE*/ 27,
/*BACKSPACE*/ '\b', /*TAB*/ '\t', /*SPACEBAR*/ ' ', /*MINUS*/ '_', /*EQUALS*/ '+',
/*OPEN_BRACE*/ '{', /*CLOSE_BRACE*/ '}', /*BACKSLASH*/ '|', /*NON_US_TILDE*/ '~',
/*COLON*/ ':', /*QUOTE*/ '\"', /*TILDE*/ '~', /*COMMA*/ '<', /*PERIOD*/ '>', /*QUESTION*/ '?'
};

static UCHAR HidNumPadToAscii[] =
{
/*KP_BACKSLASH*/ '/', /*KP_ASTERICK*/ '*', /*KP_MINUS*/ '-', /*KP_PLUS*/ '+', /*KP_ENTER*/ '\n',
/*KP_ONE*/ 0, /*KP_TWO*/ 0, /*KP_THREE*/ 0, /*KP_FOUR*/ 0, /*KP_FIVE*/ 0, /*KP_SIX*/ 0,
/*KP_SEVEN*/ 0, /*KP_EIGHT*/ 0, /*KP_NINE*/ 0, /*KP_ZERO*/ 0, /*KP_DECIMAL*/ 127
};

static UCHAR HidNumPadShiftToAscii[] =
{
/*KP_BACKSLASH*/ '/', /*KP_ASTERICK*/ '*', /*KP_MINUS*/ '-', /*KP_PLUS*/ '+', /*KP_ENTER*/ '\n',
/*KP_ONE*/ '1', /*KP_TWO*/ '2', /*KP_THREE*/ '3', /*KP_FOUR*/ '4', /*KP_FIVE*/ '5',
/*KP_SIX*/ '6', /*KP_SEVEN*/ '7', /*KP_EIGHT*/ '8', /*KP_NINE*/ '9', /*KP_ZERO*/ '0',
/*KP_DECIMAL*/ '.'
};

//====================================================================
//  Public API
//====================================================================
XBOXAPI
DWORD
__attribute__((__stdcall__))
XInputDebugInitKeyboardQueue(
    IN PXINPUT_DEBUG_KEYQUEUE_PARAMETERS pParameters OPTIONAL
    )
{
    BOOL fSuccess;

    if (pParameters) {
        RtlCopyMemory(&XID_KeyboardState.QueueParameters, pParameters,
                      sizeof(XINPUT_DEBUG_KEYQUEUE_PARAMETERS));
    } else {
        XID_KeyboardState.QueueParameters.dwFlags =
            XINPUT_DEBUG_KEYQUEUE_FLAG_KEYDOWN | XINPUT_DEBUG_KEYQUEUE_FLAG_KEYREPEAT;
        XID_KeyboardState.QueueParameters.dwQueueSize = 40;
        XID_KeyboardState.QueueParameters.dwRepeatDelay = 400;
        XID_KeyboardState.QueueParameters.dwRepeatInterval = 100;
    }

    DWORD dwQueueLength =
        XID_KeyboardState.QueueParameters.dwQueueSize * sizeof(XINPUT_DEBUG_KEYSTROKE);
    fSuccess = XID_KeyboardInitQueue(0, dwQueueLength);

    if (fSuccess) {
        // Install the service hook so the XID input path feeds us reports.
        XID_pKeyboardServices = &XID_KeyboardServices;
        return ERROR_SUCCESS;
    }
    return ERROR_OUTOFMEMORY;
}

XBOXAPI
DWORD
__attribute__((__stdcall__))
XInputDebugGetKeystroke(
    OUT PXINPUT_DEBUG_KEYSTROKE pKeystroke
    )
{
    const int iQueueIndex = 0;   // SINGLE_KEYBOARD_ONLY
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwReadPos;
    KIRQL oldIrql;
    DWORD dwTickCount;

    oldIrql = KeRaiseIrqlToDpcLevel();

    dwTickCount = GetTickCount();
    if (dwTickCount == 0) dwTickCount = 1;
    dwReadPos = XID_KeyboardState.Keyboards[iQueueIndex].QueueReadPos;
    if (dwReadPos == XID_KeyboardState.Keyboards[iQueueIndex].QueueWritePos) {
        //
        //  Queue is empty -- handle auto-repeat of the held key.
        //
        DWORD dwTickCountDiff = dwTickCount - XID_KeyboardState.Keyboards[iQueueIndex].LastRepeatTick;
        if (dwTickCountDiff > 2000) dwTickCountDiff = 0;
        if ((XID_KeyboardState.QueueParameters.dwFlags & XINPUT_DEBUG_KEYQUEUE_FLAG_KEYREPEAT) &&
            XID_KeyboardState.Keyboards[iQueueIndex].LastRepeatTick &&
            (dwTickCountDiff > XID_KeyboardState.QueueParameters.dwRepeatInterval)) {
            RtlCopyMemory(pKeystroke,
                          &XID_KeyboardState.Keyboards[iQueueIndex].RepeatKeystroke,
                          sizeof(XINPUT_DEBUG_KEYSTROKE));
            XID_KeyboardState.Keyboards[iQueueIndex].LastRepeatTick = dwTickCount;
        } else {
            dwError = ERROR_HANDLE_EOF;
        }
        goto ExitXInputGetKeyStroke;
    }

    RtlCopyMemory(pKeystroke,
                  &XID_KeyboardState.Keyboards[iQueueIndex].KeyQueue[dwReadPos],
                  sizeof(XINPUT_DEBUG_KEYSTROKE));

    if ((XID_KeyboardState.QueueParameters.dwFlags & XINPUT_DEBUG_KEYQUEUE_FLAG_KEYREPEAT) &&
        !(pKeystroke->Flags & XINPUT_DEBUG_KEYSTROKE_FLAG_KEYUP)) {
        RtlCopyMemory(&XID_KeyboardState.Keyboards[iQueueIndex].RepeatKeystroke,
                      &XID_KeyboardState.Keyboards[iQueueIndex].KeyQueue[dwReadPos],
                      sizeof(XINPUT_DEBUG_KEYSTROKE));
        XID_KeyboardState.Keyboards[iQueueIndex].RepeatKeystroke.Flags |= XINPUT_DEBUG_KEYSTROKE_FLAG_REPEAT;
        dwTickCount += XID_KeyboardState.QueueParameters.dwRepeatDelay -
                       XID_KeyboardState.QueueParameters.dwRepeatInterval;
        XID_KeyboardState.Keyboards[iQueueIndex].LastRepeatTick = dwTickCount;
    }

    XID_KeyboardState.Keyboards[iQueueIndex].QueueReadPos =
        (dwReadPos + 1) % XID_KeyboardState.QueueParameters.dwQueueSize;

ExitXInputGetKeyStroke:
    KeLowerIrql(oldIrql);
    return dwError;
}

//====================================================================
//  XID service hooks (called by the XID input path in xid.cpp)
//====================================================================
static void XID_KeyboardOpen(HANDLE hDevice)
{
    if (NULL == XID_KeyboardState.Keyboards[0].hDevice) {
        XID_KeyboardState.Keyboards[0].hDevice = hDevice;
        XID_KeyboardReset(0);
    }
}

static void XID_KeyboardClose(HANDLE hDevice)
{
    if (hDevice == XID_KeyboardState.Keyboards[0].hDevice) {
        XID_KeyboardState.Keyboards[0].hDevice = NULL;
    }
}

static void XID_KeyboardRemove(HANDLE hDevice)
{
    XID_KeyboardClose(hDevice);
}

static void XID_KeyboardNewData(HANDLE hDevice, XINPUT_KEYBOARD *pPacket)
{
    if (hDevice == XID_KeyboardState.Keyboards[0].hDevice) {
        XID_KeyboardUpdate(0, pPacket);
    }
}

//====================================================================
//  Queue + report processing
//====================================================================
static BOOL XID_KeyboardInitQueue(int iInstance, DWORD dwQueueLength)
{
    XID_KeyboardState.Keyboards[iInstance].KeyQueue =
        (PXINPUT_DEBUG_KEYSTROKE)ExAllocatePoolWithTag(dwQueueLength, 'drbk');
    if (XID_KeyboardState.Keyboards[iInstance].KeyQueue) {
        XID_KeyboardState.Keyboards[iInstance].QueueReadPos = 0;
        XID_KeyboardState.Keyboards[iInstance].QueueWritePos = 0;
        XID_KeyboardState.Keyboards[iInstance].LastRepeatTick = 0;
        return TRUE;
    }
    return FALSE;
}

static void XID_KeyboardReset(int iInstance)
{
    XID_KeyboardState.Keyboards[iInstance].KeyboardLeds.LedStates = 0;
    RtlZeroMemory(&XID_KeyboardState.Keyboards[iInstance].LastPacket, sizeof(XINPUT_KEYBOARD));
}

static void XID_KeyboardUpdate(int iInstance, XINPUT_KEYBOARD *pPacket)
{
    BYTE oldModifiers, newModifiers;
    XINPUT_KEYBOARD DownKeys;
    XINPUT_KEYBOARD UpKeys;
    int index, iNewKeyIndex, iOldKeyIndex;
    UCHAR keyStrokeFlags;
    const int iQueueIndex = iInstance;   // per-keyboard queue (single keyboard)

    oldModifiers = XID_KeyboardState.Keyboards[iInstance].LastPacket.Modifiers;
    newModifiers = pPacket->Modifiers;

    UpKeys.Modifiers = oldModifiers & ~newModifiers;
    DownKeys.Modifiers = ~oldModifiers & newModifiers;

    // Stop key repeat if the modifier set changed.
    if (oldModifiers != newModifiers) {
        XID_KeyboardState.Keyboards[iQueueIndex].LastRepeatTick = 0;
    }

    // Keys newly pressed (present now, absent in the last packet).
    for (iNewKeyIndex = 0; iNewKeyIndex < 6; iNewKeyIndex++) {
        BOOL fAddKey = FALSE;
        UCHAR NewKey = pPacket->Keys[iNewKeyIndex];
        if (NewKey > HID_USAGE_INDEX_KEYBOARD_UNDEFINED) {
            fAddKey = TRUE;
            for (iOldKeyIndex = 0; iOldKeyIndex < 6; iOldKeyIndex++) {
                if (NewKey == XID_KeyboardState.Keyboards[iInstance].LastPacket.Keys[iOldKeyIndex]) {
                    fAddKey = FALSE;
                    break;
                }
            }
        }
        if (fAddKey) {
            XID_KeyboardState.Keyboards[iQueueIndex].LastRepeatTick = 0;
            DownKeys.Keys[iNewKeyIndex] = NewKey;
        } else {
            DownKeys.Keys[iNewKeyIndex] = 0;
        }
    }

    // Keys newly released (present in the last packet, absent now).
    for (iOldKeyIndex = 0; iOldKeyIndex < 6; iOldKeyIndex++) {
        UCHAR OldKey = XID_KeyboardState.Keyboards[iInstance].LastPacket.Keys[iOldKeyIndex];
        BOOL fAddKey = FALSE;
        if (OldKey > HID_USAGE_INDEX_KEYBOARD_UNDEFINED) {
            fAddKey = TRUE;
            for (iNewKeyIndex = 0; iNewKeyIndex < 6; iNewKeyIndex++) {
                if (OldKey == pPacket->Keys[iNewKeyIndex]) {
                    fAddKey = FALSE;
                    break;
                }
            }
        }
        if (fAddKey) {
            XID_KeyboardState.Keyboards[iQueueIndex].LastRepeatTick = 0;
            UpKeys.Keys[iOldKeyIndex] = OldKey;
        } else {
            UpKeys.Keys[iOldKeyIndex] = 0;
        }
    }

    // Latch the new packet as the baseline for next time.
    RtlCopyMemory(&XID_KeyboardState.Keyboards[iInstance].LastPacket, pPacket, sizeof(XINPUT_KEYBOARD));

    // Toggle the lock LEDs on their key-down.
    BOOL fLedStateChange = FALSE;
    for (index = 0; index < 6; index++) {
        switch (DownKeys.Keys[index]) {
        case HID_USAGE_INDEX_KEYBOARD_SCROLL_LOCK:
            XID_KeyboardState.Keyboards[iQueueIndex].KeyboardLeds.LedStates ^= HID_KEYBOARDLED_MASK_SCROLL_LOCK;
            fLedStateChange = TRUE;
            break;
        case HID_USAGE_INDEX_KEYPAD_NUM_LOCK:
            XID_KeyboardState.Keyboards[iQueueIndex].KeyboardLeds.LedStates ^= HID_KEYBOARDLED_MASK_NUM_LOCK;
            fLedStateChange = TRUE;
            break;
        case HID_USAGE_INDEX_KEYBOARD_CAPS_LOCK:
            XID_KeyboardState.Keyboards[iQueueIndex].KeyboardLeds.LedStates ^= HID_KEYBOARDLED_MASK_CAPS_LOCK;
            fLedStateChange = TRUE;
            break;
        }
    }
    (void)fLedStateChange;   // LED write-back to the device is not implemented (as in the XDK).

    //
    //  Queue the events.
    //
    XINPUT_DEBUG_KEYSTROKE *pKeystrokeBuffer = XID_KeyboardQueueGetWritePos(iQueueIndex);
    // Ctrl/Shift/Alt flags: fold the left/right modifier nibbles together.
    keyStrokeFlags = (UCHAR)((newModifiers | (newModifiers >> 4)) & 0x07);
    if (HID_KEYBOARDLED_MASK_SCROLL_LOCK & XID_KeyboardState.Keyboards[iQueueIndex].KeyboardLeds.LedStates)
        keyStrokeFlags |= XINPUT_DEBUG_KEYSTROKE_FLAG_SCROLLLOCK;
    if (HID_KEYBOARDLED_MASK_NUM_LOCK & XID_KeyboardState.Keyboards[iQueueIndex].KeyboardLeds.LedStates)
        keyStrokeFlags |= XINPUT_DEBUG_KEYSTROKE_FLAG_NUMLOCK;
    if (HID_KEYBOARDLED_MASK_CAPS_LOCK & XID_KeyboardState.Keyboards[iQueueIndex].KeyboardLeds.LedStates)
        keyStrokeFlags |= XINPUT_DEBUG_KEYSTROKE_FLAG_CAPSLOCK;

    // Modifier key events.
    if (!(XINPUT_DEBUG_KEYQUEUE_FLAG_ASCII_ONLY & XID_KeyboardState.QueueParameters.dwFlags)) {
        int i, mask;
        BOOL fSet;
        for (i = 0, mask = 1; i < 8; i++, mask <<= 1) {
            fSet = FALSE;
            if (XINPUT_DEBUG_KEYQUEUE_FLAG_KEYUP & XID_KeyboardState.QueueParameters.dwFlags) {
                if (UpKeys.Modifiers & mask) {
                    pKeystrokeBuffer->Flags = XINPUT_DEBUG_KEYSTROKE_FLAG_KEYUP;
                    fSet = TRUE;
                }
            }
            if (XINPUT_DEBUG_KEYQUEUE_FLAG_KEYDOWN & XID_KeyboardState.QueueParameters.dwFlags) {
                if (DownKeys.Modifiers & mask) {
                    pKeystrokeBuffer->Flags = 0;
                    fSet = TRUE;
                }
            }
            if (fSet) {
                pKeystrokeBuffer->Flags |= keyStrokeFlags;
                pKeystrokeBuffer->VirtualKey = HidModifierToVK_Table[i];
                pKeystrokeBuffer->Ascii = 0;
                pKeystrokeBuffer = XID_KeyboardQueueIncrementWritePos(iQueueIndex);
            }
        }
    }

    // Normal key-up events.
    if (XINPUT_DEBUG_KEYQUEUE_FLAG_KEYUP & XID_KeyboardState.QueueParameters.dwFlags) {
        for (iNewKeyIndex = 0; iNewKeyIndex < 6; iNewKeyIndex++) {
            if (UpKeys.Keys[iNewKeyIndex] > HID_USAGE_INDEX_KEYBOARD_UNDEFINED) {
                XID_KeyboardQueueHidKeystroke(UpKeys.Keys[iNewKeyIndex],
                                              (UCHAR)(XINPUT_DEBUG_KEYSTROKE_FLAG_KEYUP | keyStrokeFlags),
                                              pKeystrokeBuffer);
                if (!(XINPUT_DEBUG_KEYQUEUE_FLAG_ASCII_ONLY & XID_KeyboardState.QueueParameters.dwFlags) ||
                    pKeystrokeBuffer->Ascii) {
                    pKeystrokeBuffer = XID_KeyboardQueueIncrementWritePos(iQueueIndex);
                }
            }
        }
    }

    // Normal key-down events.
    if (XINPUT_DEBUG_KEYQUEUE_FLAG_KEYDOWN & XID_KeyboardState.QueueParameters.dwFlags) {
        for (iNewKeyIndex = 0; iNewKeyIndex < 6; iNewKeyIndex++) {
            if (DownKeys.Keys[iNewKeyIndex] > HID_USAGE_INDEX_KEYBOARD_UNDEFINED) {
                XID_KeyboardQueueHidKeystroke(DownKeys.Keys[iNewKeyIndex],
                                              keyStrokeFlags,
                                              pKeystrokeBuffer);
                if (!(XINPUT_DEBUG_KEYQUEUE_FLAG_ASCII_ONLY & XID_KeyboardState.QueueParameters.dwFlags) ||
                    pKeystrokeBuffer->Ascii) {
                    pKeystrokeBuffer = XID_KeyboardQueueIncrementWritePos(iQueueIndex);
                }
            }
        }
    }
}

static XINPUT_DEBUG_KEYSTROKE *XID_KeyboardQueueGetWritePos(int iQueueIndex)
{
    return &XID_KeyboardState.Keyboards[iQueueIndex].KeyQueue[
        XID_KeyboardState.Keyboards[iQueueIndex].QueueWritePos];
}

static XINPUT_DEBUG_KEYSTROKE *XID_KeyboardQueueIncrementWritePos(int iQueueIndex)
{
    ULONG ulWritePos = XID_KeyboardState.Keyboards[iQueueIndex].QueueWritePos;
    ulWritePos = (ulWritePos + 1) % XID_KeyboardState.QueueParameters.dwQueueSize;

    // On a full queue, just don't advance (drop the oldest-unread strategy: keep write).
    if (XID_KeyboardState.Keyboards[iQueueIndex].QueueReadPos == ulWritePos) {
        ulWritePos = XID_KeyboardState.Keyboards[iQueueIndex].QueueWritePos;
    } else {
        XID_KeyboardState.Keyboards[iQueueIndex].QueueWritePos = ulWritePos;
    }
    return &XID_KeyboardState.Keyboards[iQueueIndex].KeyQueue[ulWritePos];
}

static VOID XID_KeyboardQueueHidKeystroke(UCHAR HidUsage, UCHAR Flags, PXINPUT_DEBUG_KEYSTROKE pKeystroke)
{
    UCHAR Shift = (XINPUT_DEBUG_KEYSTROKE_FLAG_SHIFT & Flags) ? 1 : 0;
    UCHAR CapsLock = (XINPUT_DEBUG_KEYSTROKE_FLAG_CAPSLOCK & Flags) ? 1 : 0;
    UCHAR NumLock = (XINPUT_DEBUG_KEYSTROKE_FLAG_NUMLOCK & Flags) ? 1 : 0;
    ASSERT(HidUsage > HID_USAGE_INDEX_KEYBOARD_UNDEFINED);
    pKeystroke->Flags = Flags;
    pKeystroke->Ascii = 0;

    //  A-Z range: VK_A..VK_Z == 'A'..'Z'.
    if ((HidUsage >= HID_USAGE_INDEX_KEYBOARD_aA) && (HidUsage <= HID_USAGE_INDEX_KEYBOARD_zZ)) {
        pKeystroke->VirtualKey = (BYTE)(HidUsage + ('A' - HID_USAGE_INDEX_KEYBOARD_aA));
        if (!(XINPUT_DEBUG_KEYSTROKE_FLAG_ALT & Flags)) {
            if (XINPUT_DEBUG_KEYSTROKE_FLAG_CTRL & Flags) {
                if (!Shift) {
                    pKeystroke->Ascii = (CHAR)(HidUsage - (HID_USAGE_INDEX_KEYBOARD_aA - 1)); // ^A..^Z
                }
            } else if (Shift ^ CapsLock) {
                pKeystroke->Ascii = (CHAR)(HidUsage + ('A' - HID_USAGE_INDEX_KEYBOARD_aA));
            } else {
                pKeystroke->Ascii = (CHAR)(HidUsage + ('a' - HID_USAGE_INDEX_KEYBOARD_aA));
            }
        }
    }
    //  1-9 range.
    else if ((HidUsage >= HID_USAGE_INDEX_KEYBOARD_ONE) && (HidUsage <= HID_USAGE_INDEX_KEYBOARD_NINE)) {
        pKeystroke->VirtualKey = (BYTE)(HidUsage + ('1' - HID_USAGE_INDEX_KEYBOARD_ONE));
        if (Shift) {
            pKeystroke->Ascii = (CHAR)HidSymToAsciiShift_Table[HidUsage - HID_USAGE_INDEX_KEYBOARD_ONE];
        } else {
            pKeystroke->Ascii = (CHAR)pKeystroke->VirtualKey;
        }
    }
    //  Everything else via the lookup table (+ symbol/numpad ASCII sub-ranges).
    else {
        pKeystroke->VirtualKey = HidToVK_Table[HidUsage - HID_USAGE_INDEX_KEYBOARD_ZERO];
        if (HidUsage <= HID_USAGE_INDEX_KEYBOARD_QUESTION) {
            if (Shift) {
                pKeystroke->Ascii = (CHAR)HidSymToAsciiShift_Table[HidUsage - HID_USAGE_INDEX_KEYBOARD_ONE];
            } else {
                pKeystroke->Ascii = (CHAR)HidSymToAscii_Table[HidUsage - HID_USAGE_INDEX_KEYBOARD_ZERO];
            }
        } else if ((HidUsage >= HID_USAGE_INDEX_KEYPAD_BACKSLASH) &&
                   (HidUsage <= HID_USAGE_INDEX_KEYPAD_DECIMAL)) {
            if (Shift ^ NumLock) {
                pKeystroke->Ascii = (CHAR)HidNumPadShiftToAscii[HidUsage - HID_USAGE_INDEX_KEYPAD_BACKSLASH];
            } else {
                pKeystroke->Ascii = (CHAR)HidNumPadToAscii[HidUsage - HID_USAGE_INDEX_KEYPAD_BACKSLASH];
            }
        }
    }
}
