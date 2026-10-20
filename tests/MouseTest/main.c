// MouseTest: a hands-on check of XAPI's USB mouse support (XDEVICE_TYPE_DEBUG_MOUSE).
// Plug a HID mouse into any controller port (through a port-to-USB adapter), run
// this, and move, click and scroll it. It prints which ports XAPI sees gamepads,
// mice and keyboards on, then one line per new mouse report: dwPacketNumber,
// buttons, and the X/Y/wheel motion since the previous read. Motion that arrives
// faster than this 100 ms poll adds up and stops at -128/127, as in the XDK.
//
// Output goes through OutputDebugStringA, so it shows on xbWatson or any XBDM
// notification listener; the screen stays black.
#include <xtl.h>
#include <xkbd.h>
#include <stdarg.h>
#include <stdio.h>

static void Print(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    OutputDebugStringA(buf);
}

int main(void)
{
    HANDLE mice[4] = {0};
    DWORD packet[4] = {0};
    DWORD lastPads = ~0u, lastMice = ~0u, lastKeyboards = ~0u;

    XInitDevices(0, NULL);
    Print("MouseTest: move, click and scroll a USB mouse for the next 5 minutes\n");

    for (int tick = 0; tick < 3000; ++tick) {
        DWORD pads = XGetDevices(XDEVICE_TYPE_GAMEPAD);
        DWORD mouse = XGetDevices(XDEVICE_TYPE_DEBUG_MOUSE);
        DWORD keyboards = XGetDevices(XDEVICE_TYPE_DEBUG_KEYBOARD);

        // Report the ports on any change, and every 5 s in case a listener missed it.
        if (pads != lastPads || mouse != lastMice || keyboards != lastKeyboards || tick % 50 == 0) {
            Print("MouseTest: ports gamepads=0x%lx mice=0x%lx keyboards=0x%lx\n", pads, mouse, keyboards);
            lastPads = pads;
            lastMice = mouse;
            lastKeyboards = keyboards;
        }

        for (int port = 0; port < 4; ++port) {
            if ((mouse & (1u << port)) && !mice[port]) {
                mice[port] = XInputOpen(XDEVICE_TYPE_DEBUG_MOUSE, port, XDEVICE_NO_SLOT, NULL);
                packet[port] = 0;
                Print("MouseTest: port %d mouse %s\n", port + 1, mice[port] ? "opened" : "failed to open");
            } else if (!(mouse & (1u << port)) && mice[port]) {
                XInputClose(mice[port]);
                mice[port] = NULL;
                Print("MouseTest: port %d mouse removed\n", port + 1);
            }
            if (!mice[port]) {
                continue;
            }

            XINPUT_STATE state;
            ZeroMemory(&state, sizeof(state));
            if (XInputGetState(mice[port], &state) == ERROR_SUCCESS && state.dwPacketNumber != packet[port]) {
                packet[port] = state.dwPacketNumber;
                Print("MouseTest: port %d packet %lu buttons=0x%02x x=%d y=%d wheel=%d\n", port + 1,
                      state.dwPacketNumber, state.DebugMouse.bButtons, state.DebugMouse.cMickeysX,
                      state.DebugMouse.cMickeysY, state.DebugMouse.cWheel);
            }
        }
        Sleep(100);
    }

    Print("MouseTest: done\n");
    return 0;
}
