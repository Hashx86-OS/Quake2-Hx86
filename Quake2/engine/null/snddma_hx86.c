/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// snddma_hx86.c -- Hashx86 streaming audio backend.
//
// The game keeps a small ring that snd_mix.c paints into. SNDDMA_Submit()
// copies newly painted frames into a kernel-owned ring through Hsys_audioWrite,
// and SNDDMA_GetDMAPos() reports the mixer's consumed byte position in the
// units snd_dma.c expects (interleaved samples).

#include "../client/client.h"
#include "../client/snd_loc.h"

#include <Hx86/Hsyscalls/syscalls.h>

#define SND_HX86_RATE 44100
#define SND_HX86_FRAMES 16384
#define SND_HX86_CHANNELS 2
#define SND_HX86_SAMPLE_BYTES 2
#define SND_HX86_FRAME_BYTES (SND_HX86_CHANNELS * SND_HX86_SAMPLE_BYTES)

static byte snd_ring[SND_HX86_FRAMES * SND_HX86_FRAME_BYTES];
static int32_t snd_stream = -1;
static uint32_t snd_submitted_frames;

qboolean SNDDMA_Init(void) {
    if (snd_stream >= 0) return true;

    memset(&dma, 0, sizeof(dma));
    dma.channels = SND_HX86_CHANNELS;
    dma.samplebits = SND_HX86_SAMPLE_BYTES * 8;
    dma.speed = SND_HX86_RATE;
    dma.samples = SND_HX86_FRAMES * SND_HX86_CHANNELS;
    dma.submission_chunk = 1;
    dma.buffer = snd_ring;

    snd_stream = syscall_audio_open(dma.speed, dma.channels, dma.samplebits);
    if (snd_stream < 0) {
        dma.buffer = NULL;
        return false;
    }

    snd_submitted_frames = 0;
    memset(snd_ring, 0, sizeof(snd_ring));
    printf("[SND] Hx86 audio: %d Hz, %d channels, %d-bit\n", dma.speed, dma.channels,
           dma.samplebits);
    return true;
}

int SNDDMA_GetDMAPos(void) {
    if (snd_stream < 0 || !dma.buffer) return 0;

    int32_t position = syscall_audio_get_position(snd_stream);
    if (position < 0) return dma.samplepos;

    dma.samplepos = ((uint32_t)position / SND_HX86_SAMPLE_BYTES) % dma.samples;
    return dma.samplepos;
}

void SNDDMA_Shutdown(void) {
    if (snd_stream >= 0) {
        syscall_audio_close(snd_stream);
        snd_stream = -1;
    }
    dma.buffer = NULL;
    snd_submitted_frames = 0;
}

void SNDDMA_BeginPainting(void) {}

void SNDDMA_Submit(void) {
    if (snd_stream < 0 || !dma.buffer) return;

    uint32_t end = (uint32_t)paintedtime;
    if (end < snd_submitted_frames) {
        // paintedtime was reset (level change or long pause).
        snd_submitted_frames = end;
    }

    while (snd_submitted_frames < end) {
        uint32_t ring_pos = snd_submitted_frames & (SND_HX86_FRAMES - 1);
        uint32_t count = end - snd_submitted_frames;
        uint32_t until_end = SND_HX86_FRAMES - ring_pos;
        if (count > until_end) count = until_end;

        uint32_t bytes = count * SND_HX86_FRAME_BYTES;
        int32_t written = syscall_audio_write(
            snd_stream, dma.buffer + ring_pos * SND_HX86_FRAME_BYTES, bytes);
        if (written <= 0) break;

        uint32_t frames = (uint32_t)written / SND_HX86_FRAME_BYTES;
        snd_submitted_frames += frames;
        if (frames < count) break;
    }
}
