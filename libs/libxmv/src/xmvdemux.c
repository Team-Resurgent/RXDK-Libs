//------------------------------------------------------------------------------
// xmvdemux.c -- standalone Xbox XMV container demuxer (see xmvdemux.h).
//
// Reworked from FFmpeg's LGPL libavformat/xmv.c to walk a whole in-memory XMV
// file image. No libav* dependencies.
//------------------------------------------------------------------------------

#include "xmvdemux.h"

#define XMV_MIN_HEADER_SIZE 36

// Little-endian reads with bounds checking against the file image.
static uint16_t rl16(const XmvDemux *d, uint32_t off)
{
    if (off + 2 > d->file_size) return 0;
    return (uint16_t)(d->file[off] | (d->file[off + 1] << 8));
}
static uint32_t rl32(const XmvDemux *d, uint32_t off)
{
    if (off + 4 > d->file_size) return 0;
    return (uint32_t)d->file[off] | ((uint32_t)d->file[off + 1] << 8) |
           ((uint32_t)d->file[off + 2] << 16) | ((uint32_t)d->file[off + 3] << 24);
}

// Process the header of the packet at d->this_packet_offset: carve out the video
// data block and each audio track's data block. Mirrors xmv_process_packet_header.
static int xmv_process_packet_header(XmvDemux *d)
{
    uint32_t p = d->this_packet_offset;
    uint32_t vid;
    int i;

    if (p + 12 > d->file_size)
        return -1;

    // Next packet size.
    d->next_packet_size = rl32(d, p);
    p += 4;

    // Video header dword (the read is 8 bytes; only the first 4 are used).
    vid = rl32(d, p);
    p += 8;

    d->video_data_size    = vid & 0x007FFFFF;
    d->video_frame_count  = (vid >> 23) & 0xFF;
    // has_extradata == (byte3 & 0x80); byte3 is bits 24..31 of `vid`.
    {
        int has_extradata = (vid & 0x80000000u) != 0;

        if (d->video_data_size < d->audio_track_count * 4)
            return -1;
        d->video_data_size -= d->audio_track_count * 4;

        if (d->video_frame_count == 0)
            d->video_frame_count = 1;

        // Audio data-size dwords (one per track).
        for (i = 0; i < d->audio_track_count; i++) {
            uint32_t asz = rl32(d, p) & 0x007FFFFF;
            p += 4;

            if (asz == 0 && i != 0)
                asz = d->audio_data_size[i - 1];

            d->audio_data_size[i] = asz;

            // Carve audio into frame_count slices, aligned to block_align.
            if (d->video_frame_count) {
                uint32_t fs = asz / d->video_frame_count;
                uint32_t ba = d->audio[i].block_align ? d->audio[i].block_align : 1;
                fs -= fs % ba;
                d->audio_frame_size[i] = fs;
            } else {
                d->audio_frame_size[i] = asz;
            }
        }

        // Data offsets: video block, then each audio block.
        d->video_data_offset = p;
        p += d->video_data_size;
        for (i = 0; i < d->audio_track_count; i++) {
            d->audio_data_offset[i] = p;
            p += d->audio_data_size[i];
        }

        // Per-packet video extradata (4 bytes) precedes the frames. This is the
        // WMV2 sequence header (picture-coding options); capture it once.
        if (d->video_data_size > 0 && has_extradata) {
            if (!d->has_extradata && d->video_data_offset + 4 <= d->file_size) {
                d->video_extradata[0] = d->file[d->video_data_offset + 0];
                d->video_extradata[1] = d->file[d->video_data_offset + 1];
                d->video_extradata[2] = d->file[d->video_data_offset + 2];
                d->video_extradata[3] = d->file[d->video_data_offset + 3];
                d->has_extradata = 1;
            }
            d->video_data_size   -= 4;
            d->video_data_offset += 4;
        }
    }

    d->video_current_frame = 0;
    return 0;
}

// Advance to the next packet. Mirrors xmv_fetch_new_packet.
static int xmv_fetch_new_packet(XmvDemux *d)
{
    if (d->next_packet_size == 0) {
        d->eof = 1;
        return 0;
    }

    d->this_packet_offset = d->next_packet_offset;
    d->this_packet_size   = d->next_packet_size;

    if (d->this_packet_size < (uint32_t)(12 + d->audio_track_count * 4))
        return -1;
    if (d->this_packet_offset + d->this_packet_size > d->file_size + 0)
        ; // tolerate a short final packet; reads are individually bounds-checked

    if (xmv_process_packet_header(d) != 0)
        return -1;

    d->next_packet_offset = d->this_packet_offset + d->this_packet_size;
    return 0;
}

int XmvDemuxOpen(XmvDemux *d, const uint8_t *file, uint32_t file_size,
                 uint8_t *scratch, uint32_t scratch_size)
{
    uint32_t this_packet_size;
    uint32_t hdr;
    int i;

    for (i = 0; i < (int)sizeof(*d); i++) ((uint8_t *)d)[i] = 0;
    d->file = file;
    d->file_size = file_size;
    d->frame_scratch = scratch;
    d->frame_scratch_size = scratch_size;

    if (file_size < XMV_MIN_HEADER_SIZE)
        return -1;

    // 0: next_packet_size (unused), 4: this_packet_size, 8: max_packet_size.
    this_packet_size = rl32(d, 4);

    // 12: "xobX" magic.
    if (!(file[12] == 'x' && file[13] == 'o' && file[14] == 'b' && file[15] == 'X'))
        return -2;

    // 16: file version (1 = original Xbox; FFmpeg also sees 2/4). Accept 1..4.
    {
        uint32_t ver = rl32(d, 16);
        if (ver == 0 || ver > 4)
            return -3;
    }

    d->width       = rl32(d, 20);
    d->height      = rl32(d, 24);
    d->duration_ms = rl32(d, 28);

    d->audio_track_count = rl16(d, 32);
    if (d->audio_track_count > XMV_MAX_AUDIO_TRACKS)
        return -4;
    // 34: 2 bytes padding.

    hdr = XMV_MIN_HEADER_SIZE; // 36
    for (i = 0; i < d->audio_track_count; i++) {
        d->audio[i].compression     = rl16(d, hdr + 0);
        d->audio[i].channels        = rl16(d, hdr + 2);
        d->audio[i].sample_rate     = rl32(d, hdr + 4);
        d->audio[i].bits_per_sample = rl16(d, hdr + 8);
        d->audio[i].flags           = rl16(d, hdr + 10);
        d->audio[i].block_align     = 36u * (d->audio[i].channels ? d->audio[i].channels : 1);
        hdr += 12;
    }

    // First packet body begins right after the header; its total size is the
    // file-header this_packet_size minus the header we just consumed.
    d->next_packet_offset = hdr;
    if (this_packet_size < hdr)
        return -5;
    d->next_packet_size = this_packet_size - hdr;
    d->video_pts_ms = 0;

    // Prime the first packet.
    if (xmv_fetch_new_packet(d) != 0)
        return -6;

    // Probe the (constant) inter-frame interval -> fps, by walking a throwaway
    // copy of the iteration state until the first non-zero PTS delta. duration_ms
    // is unreliable for this (it can include trailing audio), so we derive the
    // rate from the bitstream's own frame timestamps.
    {
        XmvDemux tmp = *d;
        const uint8_t *fd; uint32_t fsz, pts, prev = 0; int kf, n = 0;
        d->frame_duration_ms = 0;
        while (n++ < 64 && XmvDemuxNextVideoFrame(&tmp, &fd, &fsz, &kf, &pts) == 1) {
            if (pts > prev) { d->frame_duration_ms = pts - prev; break; }
            prev = pts;
        }
        d->fps = d->frame_duration_ms
            ? (1000u + d->frame_duration_ms / 2) / d->frame_duration_ms : 0;
    }

    return 0;
}

int XmvDemuxNextVideoFrame(XmvDemux *d, const uint8_t **data, uint32_t *size,
                           int *keyframe, uint32_t *pts_ms)
{
    uint32_t fh, frame_size, ts;
    uint32_t off, i;

    // Exhausted the current packet's frames? Fetch the next packet.
    while (d->video_current_frame >= d->video_frame_count) {
        if (d->eof)
            return 0;
        if (xmv_fetch_new_packet(d) != 0)
            return -1;
        if (d->eof)
            return 0;
    }

    off = d->video_data_offset;
    if (off + 4 > d->file_size)
        return 0;

    fh = rl32(d, off);
    frame_size = (fh & 0x1FFFF) * 4 + 4;
    ts = fh >> 17;

    if ((uint64_t)off + 4 + frame_size > d->file_size)
        return -1;
    if (frame_size + 4 > d->video_data_size + 4)
        return -1;
    if (frame_size > d->frame_scratch_size)
        return -1;

    // Copy the frame body (after the 4-byte frame header), reversing each dword
    // (XMV stores the WMV2 bitstream as LE dwords; the decoder wants BE order).
    {
        const uint8_t *src = d->file + off + 4;
        uint8_t *dst = d->frame_scratch;
        for (i = 0; i + 4 <= frame_size; i += 4) {
            dst[i + 0] = src[i + 3];
            dst[i + 1] = src[i + 2];
            dst[i + 2] = src[i + 1];
            dst[i + 3] = src[i + 0];
        }
        for (; i < frame_size; i++)   // tail (frame_size is *4+4 so always /4, but be safe)
            dst[i] = src[i];
    }

    if (keyframe)
        *keyframe = (d->frame_scratch[0] & 0x80) ? 0 : 1;

    d->video_pts_ms += ts;
    if (pts_ms)
        *pts_ms = d->video_pts_ms;

    *data = d->frame_scratch;
    *size = frame_size;

    // Advance within the packet.
    d->video_data_size   -= frame_size + 4;
    d->video_data_offset += frame_size + 4;
    d->video_current_frame++;
    return 1;
}

int XmvDemuxAudioFrame(XmvDemux *d, int track,
                       const uint8_t **data, uint32_t *size)
{
    uint32_t sz;

    if (track < 0 || track >= d->audio_track_count)
        return 0;
    if (d->audio_data_size[track] == 0)
        return 0;

    // Last frame of the packet gets the remainder; others get one slice.
    if (d->video_current_frame < d->video_frame_count) {
        sz = d->audio_frame_size[track];
        if (sz > d->audio_data_size[track])
            sz = d->audio_data_size[track];
    } else {
        sz = d->audio_data_size[track];
    }
    if (sz == 0)
        return 0;
    if (d->audio_data_offset[track] + sz > d->file_size)
        return -1;

    *data = d->file + d->audio_data_offset[track];
    *size = sz;

    d->audio_data_size[track]   -= sz;
    d->audio_data_offset[track] += sz;
    return 1;
}
