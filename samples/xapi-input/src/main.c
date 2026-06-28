// xAPI input monitor -- exercises the XID input device types on kit hardware.
//
// Polls for connection/disconnection of controllers, IR remotes, mice, and
// keyboards (XGetDeviceChanges), opens each as it appears (XInputOpen), and
// debug-prints every button / movement event. Runs forever; watch the output
// with xbWatson or the deploy log and press buttons / move the mouse.
//
// Reading keyboard *keystrokes* needs the XInputDebug*Keyboard queue API (a
// separate gap); keyboard connect/disconnect is reported here, and keystroke
// printing is wired in once that API lands.

#include "common.h"   // xapi.h + trace helpers
#include <xkbd.h>     // XDEVICE_TYPE_DEBUG_KEYBOARD

// IR remote isn't in the public headers; declare it like the reference title.
extern XPP_DEVICE_TYPE XDEVICE_TYPE_IR_REMOTE_TABLE;
#define XDEVICE_TYPE_IR_REMOTE (&XDEVICE_TYPE_IR_REMOTE_TABLE)

// IR remote report (see input_manager.cpp): XInputGetState fills this small
// layout for an IR device. firstEvent==0 means "no event this poll".
typedef struct _XINPUT_STATEEX {
    DWORD dwPacketNumber;
    BYTE  wButtons;
    BYTE  region;
    BYTE  counter;
    BYTE  firstEvent;
} XINPUT_STATEEX;

#define INPUT_MAX_PORTS 4

static const char *kGamepadDigital[] = {
    "DPAD_UP", "DPAD_DOWN", "DPAD_LEFT", "DPAD_RIGHT",
    "START", "BACK", "LEFT_THUMB", "RIGHT_THUMB",
};
static const WORD kGamepadDigitalBits[] = {
    XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT,
    XINPUT_GAMEPAD_START, XINPUT_GAMEPAD_BACK, XINPUT_GAMEPAD_LEFT_THUMB, XINPUT_GAMEPAD_RIGHT_THUMB,
};
static const char *kGamepadAnalog[] = {
    "A", "B", "X", "Y", "BLACK", "WHITE", "LEFT_TRIGGER", "RIGHT_TRIGGER",
};
static const char *kMouseButtons[] = { "LEFT", "RIGHT", "MIDDLE", "X1", "X2" };
static const BYTE kMouseBits[] = {
    XINPUT_DEBUG_MOUSE_LEFT_BUTTON, XINPUT_DEBUG_MOUSE_RIGHT_BUTTON, XINPUT_DEBUG_MOUSE_MIDDLE_BUTTON,
    XINPUT_DEBUG_MOUSE_XBUTTON1, XINPUT_DEBUG_MOUSE_XBUTTON2,
};

#define ANALOG_THRESHOLD 48 // press detection for the 8-bit analog buttons

static HANDLE g_pad[INPUT_MAX_PORTS];
static HANDLE g_mouse[INPUT_MAX_PORTS];
static HANDLE g_ir[INPUT_MAX_PORTS];
static HANDLE g_kbd[INPUT_MAX_PORTS];

static WORD g_padPrevDigital[INPUT_MAX_PORTS];
static BYTE g_padPrevAnalog[INPUT_MAX_PORTS][8];
static BYTE g_mousePrevButtons[INPUT_MAX_PORTS];
static DWORD g_irPrevPacket[INPUT_MAX_PORTS];

// Open/close devices of one type as they are inserted/removed, logging each.
static void process_changes(PXPP_DEVICE_TYPE type, const char *name,
                            HANDLE handles[INPUT_MAX_PORTS],
                            PXINPUT_POLLING_PARAMETERS polling)
{
    DWORD insertions = 0;
    DWORD removals = 0;

    if (XGetDeviceChanges(type, &insertions, &removals) != TRUE) {
        return;
    }

    for (int port = 0; port < INPUT_MAX_PORTS; ++port) {
        if (insertions & (1u << port)) {
            handles[port] = XInputOpen(type, port, XDEVICE_NO_SLOT, polling);
            DbgPrint("input: %s connected port=%d%s\n", name, port,
                     handles[port] ? "" : " (open FAILED)");
        }
        if (removals & (1u << port)) {
            if (handles[port]) {
                XInputClose(handles[port]);
                handles[port] = NULL;
            }
            DbgPrint("input: %s disconnected port=%d\n", name, port);
        }
    }
}

static void poll_gamepads(void)
{
    for (int port = 0; port < INPUT_MAX_PORTS; ++port) {
        XINPUT_STATE state;
        if (!g_pad[port] || XInputGetState(g_pad[port], &state) != ERROR_SUCCESS) {
            continue;
        }

        WORD digital = state.Gamepad.wButtons;
        WORD changed = (WORD)(digital ^ g_padPrevDigital[port]);
        for (int i = 0; i < 8; ++i) {
            if (changed & kGamepadDigitalBits[i]) {
                DbgPrint("input: pad%d %s %s\n", port, kGamepadDigital[i],
                         (digital & kGamepadDigitalBits[i]) ? "down" : "up");
            }
        }
        g_padPrevDigital[port] = digital;

        for (int i = 0; i < 8; ++i) {
            BYTE now = state.Gamepad.bAnalogButtons[i];
            BYTE prev = g_padPrevAnalog[port][i];
            BOOL nowDown = now >= ANALOG_THRESHOLD;
            BOOL prevDown = prev >= ANALOG_THRESHOLD;
            if (nowDown != prevDown) {
                DbgPrint("input: pad%d %s %s (val=%u)\n", port, kGamepadAnalog[i],
                         nowDown ? "down" : "up", (unsigned)now);
            }
            g_padPrevAnalog[port][i] = now;
        }
    }
}

static void poll_mice(void)
{
    for (int port = 0; port < INPUT_MAX_PORTS; ++port) {
        XINPUT_STATE state;
        if (!g_mouse[port] || XInputGetState(g_mouse[port], &state) != ERROR_SUCCESS) {
            continue;
        }

        BYTE buttons = state.DebugMouse.bButtons;
        BYTE changed = (BYTE)(buttons ^ g_mousePrevButtons[port]);
        for (int i = 0; i < 5; ++i) {
            if (changed & kMouseBits[i]) {
                DbgPrint("input: mouse%d %s %s\n", port, kMouseButtons[i],
                         (buttons & kMouseBits[i]) ? "down" : "up");
            }
        }
        g_mousePrevButtons[port] = buttons;

        int dx = state.DebugMouse.cMickeysX;
        int dy = state.DebugMouse.cMickeysY;
        int dz = state.DebugMouse.cWheel;
        if (dx || dy || dz) {
            DbgPrint("input: mouse%d move dx=%d dy=%d wheel=%d\n", port, dx, dy, dz);
        }
    }
}

static void poll_ir(void)
{
    for (int port = 0; port < INPUT_MAX_PORTS; ++port) {
        XINPUT_STATEEX state;
        if (!g_ir[port] ||
            XInputGetState(g_ir[port], (PXINPUT_STATE)&state) != ERROR_SUCCESS ||
            state.firstEvent == 0) {
            continue;
        }
        if (state.dwPacketNumber != g_irPrevPacket[port]) {
            DbgPrint("input: remote%d button code=%u\n", port, (unsigned)state.wButtons);
            g_irPrevPacket[port] = state.dwPacketNumber;
        }
    }
}

int main(void)
{
    XINPUT_POLLING_PARAMETERS kbdPolling;

    XInitDevices(0, NULL);

    xapi_smoke_trace_line("input monitor start");
    DbgPrint("input: press buttons / move mouse / aim the remote; connect & disconnect devices.\n");

    kbdPolling.fAutoPoll = TRUE;
    kbdPolling.fInterruptOut = TRUE;
    kbdPolling.bInputInterval = 32;
    kbdPolling.bOutputInterval = 32;
    kbdPolling.ReservedMBZ1 = 0;
    kbdPolling.ReservedMBZ2 = 0;

    for (;;) {
        process_changes(XDEVICE_TYPE_GAMEPAD, "pad", g_pad, NULL);
        process_changes(XDEVICE_TYPE_IR_REMOTE, "remote", g_ir, NULL);
        process_changes(XDEVICE_TYPE_DEBUG_MOUSE, "mouse", g_mouse, NULL);
        process_changes(XDEVICE_TYPE_DEBUG_KEYBOARD, "keyboard", g_kbd, &kbdPolling);

        poll_gamepads();
        poll_mice();
        poll_ir();
        // Keyboard keystroke printing is added once the XInputDebug*Keyboard
        // queue API is implemented; connect/disconnect is reported above.

        Sleep(16); // ~60 Hz
    }

    return 0;
}
