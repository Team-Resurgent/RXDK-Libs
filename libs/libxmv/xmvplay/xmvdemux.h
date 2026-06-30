//------------------------------------------------------------------------------
// xmvdemux.h -- standalone Xbox XMV container demuxer.
//
// Parses the retail XMV ("xobX") container: a 36-byte file header + per-track
// audio descriptors, followed by packets. Each packet holds a run of WMV2 video
// frames plus one slice of each audio track. Modeled on FFmpeg's LGPL
// libavformat/xmv.c (the authoritative open-source reference for the format);
// reworked to operate on a whole in-memory file image with no libav* deps.
//
// The video elementary stream is WMV2 (AV_CODEC_ID_WMV2); per-frame data is
// stored as little-endian dwords and must be dword-byte-reversed before being fed
// to a big-endian-bitstream WMV2 decoder (see XmvDemuxNextVideoFrame).
//------------------------------------------------------------------------------

#ifndef RXDK_XMVDEMUX_H
#define RXDK_XMVDEMUX_H

#include <stdint.h>

#define XMV_MAX_AUDIO_TRACKS 8

typedef struct XmvAudioDesc {
    uint16_t compression;     // WAVE_FORMAT tag (1 = PCM)
    uint16_t channels;
    uint32_t sample_rate;
    uint16_t bits_per_sample;
    uint16_t flags;           // XMV_AUDIO_ADPCM51_* (5.1 ADPCM grouping)
    uint32_t block_align;     // 36 * channels (XMV ADPCM block); PCM uses it too
} XmvAudioDesc;

typedef struct XmvDemux {
    // Whole file image (caller-owned, must outlive the demuxer).
    const uint8_t *file;
    uint32_t       file_size;

    // Global header.
    uint32_t width;
    uint32_t height;
    uint32_t duration_ms;
    uint16_t audio_track_count;
    XmvAudioDesc audio[XMV_MAX_AUDIO_TRACKS];

    // WMV2 sequence-header extradata (the 4-byte per-packet block flagged by the
    // video header's has_extradata bit). Holds the picture-coding options
    // (mspel/loop-filter/variable-transform/xintra8/...). Captured from the first
    // packet that carries it.
    uint8_t  video_extradata[4];
    int      has_extradata;

    // Packet iteration.
    uint32_t this_packet_size;
    uint32_t next_packet_size;
    uint32_t this_packet_offset;
    uint32_t next_packet_offset;

    // Current packet: video.
    uint32_t video_data_size;
    uint32_t video_data_offset;
    uint32_t video_frame_count;
    uint32_t video_current_frame;
    uint32_t video_pts_ms;        // running PTS (ms) of the current frame

    // Current packet: audio (per track).
    uint32_t audio_data_size[XMV_MAX_AUDIO_TRACKS];
    uint32_t audio_data_offset[XMV_MAX_AUDIO_TRACKS];
    uint32_t audio_frame_size[XMV_MAX_AUDIO_TRACKS];

    int eof;

    // Scratch for the dword-byte-reversed video frame (sized to the max packet).
    uint8_t *frame_scratch;
    uint32_t frame_scratch_size;
} XmvDemux;

// Parse the header of an in-memory XMV file. Returns 0 on success, <0 on error.
// `scratch`/`scratch_size` is a caller-provided buffer (>= max packet size) used
// to hold the byte-reversed video frame returned by XmvDemuxNextVideoFrame.
int XmvDemuxOpen(XmvDemux *d, const uint8_t *file, uint32_t file_size,
                 uint8_t *scratch, uint32_t scratch_size);

// Advance to the next video frame. On success returns 1 and fills *data/*size
// with the dword-byte-reversed WMV2 frame, *keyframe (1 if an I-frame) and
// *pts_ms. Returns 0 at end of stream, <0 on error. Drives packet fetching.
int XmvDemuxNextVideoFrame(XmvDemux *d, const uint8_t **data, uint32_t *size,
                           int *keyframe, uint32_t *pts_ms);

// Get the audio slice for the current video frame on `track`. Returns 1 with
// *data/*size (raw, e.g. PCM), 0 if none, <0 on error. Call after each
// XmvDemuxNextVideoFrame, before the next one.
int XmvDemuxAudioFrame(XmvDemux *d, int track,
                       const uint8_t **data, uint32_t *size);

#endif // RXDK_XMVDEMUX_H
