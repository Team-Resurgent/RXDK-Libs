// libxvoice source manifest -- the XDK-5849 voice library (xvoice.lib): the XHV
// high-level voice-chat engine (xhv.h) + the low-level voice XMO/codec API
// (xvoice.h) + the XDEVICE_TYPE_VOICE_* device tables.
//
// UNLIKE the other RXDK libs this is NOT a leak port: the May-2020 leak has no
// xvoice implementation anywhere (private/genx/directx/xvoice holds only an
// older public header; the codec + USB audio driver shipped binary-only in the
// retail lib). This is a fresh RXDK implementation of the 5849 public C
// surface: engine/talker bookkeeping is real, while everything that needs the
// communicator USB audio driver or the SC03/WMAVoice codecs reports the same
// failure retail produces with no communicator inserted. See the notes at the
// top of src/xhv.cpp and src/xvoice.cpp.
//
// xhv.cpp and xvoice.cpp are separate TUs by header design: xhv.h and xvoice.h
// #error if both are included in one translation unit.

pub const Slice = struct {
    name: []const u8,
    sources: []const []const u8,
    is_cpp: bool,
};

const X = "libs/libxvoice";

pub const cpp_sources = [_][]const u8{
    X ++ "/src/xhv.cpp",
    X ++ "/src/xvoice.cpp",
};

pub const slices = [_]Slice{
    .{ .name = "xvoice-cpp", .is_cpp = true, .sources = &cpp_sources },
};
