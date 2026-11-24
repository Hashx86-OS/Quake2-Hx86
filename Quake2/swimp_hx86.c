#include "engine/ref_soft/r_local.h"
#include <Hx86/Hx86.h>
#include <stdlib.h>

extern viddef_t vid;
extern unsigned d_8to24table[256];

static uint32_t *framebuffer = 0;
static int fb_width = 640;
static int fb_height = 480;

static uint32_t sys_palette[256];

void SWimp_BeginFrame(float camera_separation) {
    (void)camera_separation;
}

void SWimp_EndFrame(void) {
    static int framecount = 0;
    if (framecount++ % 60 == 0) {
        printf("[Quake2] SWimp_EndFrame called! framebuffer=0x%p, vid.buffer=0x%p\n", framebuffer, vid.buffer);
    }

    if (!framebuffer || !vid.buffer) return;

    pixel_t *src = (pixel_t *)vid.buffer;
    int srcW = vid.width;
    int srcH = vid.height;
    int srcRowBytes = vid.rowbytes;
    int destW = fb_width;
    int destH = fb_height;

    int non_black = 0;
    for (int y = 0; y < destH; y++) {
        int srcY = (y * srcH) / destH;
        int dstOff = y * destW;
        pixel_t *srcRow = src + srcY * srcRowBytes;
        for (int x = 0; x < destW; x++) {
            int srcX = (x * srcW) / destW;
            uint8_t color_idx = srcRow[srcX];
            if (color_idx != 0 && color_idx != 2) non_black++;
            framebuffer[dstOff + x] = sys_palette[color_idx];
        }
    }
    
    if (framecount % 60 == 0) {
        printf("[Quake2] Non-black pixels this frame: %d\n", non_black);
    }
}

int SWimp_Init(void *hInstance, void *wndProc) {
    (void)hInstance;
    (void)wndProc;

    FramebufferInfo fb = syscall_get_framebuffer();
    framebuffer = (uint32_t*)fb.buffer;
    fb_width = (int)fb.width;
    fb_height = (int)fb.height;

    return true;
}

void SWimp_SetPalette(const unsigned char *palette) {
    if (!palette) return;
    for (int i = 0; i < 256; i++) {
        unsigned char r = palette[i*4+0];
        unsigned char g = palette[i*4+1];
        unsigned char b = palette[i*4+2];
        sys_palette[i] = (255 << 24) | (r << 16) | (g << 8) | b;
    }
    printf("[Quake2] SWimp_SetPalette called! Color 255 is ARGB(255,%d,%d,%d)\n", (sys_palette[255]>>16)&0xFF, (sys_palette[255]>>8)&0xFF, sys_palette[255]&0xFF);
}

void SWimp_Shutdown(void) {
    if (vid.buffer) {
        free(vid.buffer);
        vid.buffer = NULL;
    }
}

rserr_t SWimp_SetMode(int *pwidth, int *pheight, int mode, qboolean fullscreen) {
    (void)fullscreen;

    if (!ri.Vid_GetModeInfo(pwidth, pheight, mode)) {
        ri.Con_Printf(PRINT_ALL, "SWimp_SetMode: invalid mode %d\n", mode);
        return rserr_invalid_mode;
    }

    if (vid.buffer) {
        free(vid.buffer);
        vid.buffer = NULL;
    }

    vid.width = *pwidth;
    vid.height = *pheight;
    vid.rowbytes = vid.width;

    vid.buffer = (pixel_t*)malloc(vid.rowbytes * vid.height);
    if (!vid.buffer) {
        ri.Sys_Error(ERR_FATAL, "SWimp_SetMode: Failed to allocate %d bytes for vid.buffer\n", vid.rowbytes * vid.height);
        return rserr_invalid_mode;
    }
    
    // Hashx86 Port: Zero the buffer so the unrendered areas (like console background) are black instead of garbage memory!
    memset(vid.buffer, 0, vid.rowbytes * vid.height);

    // CRITICAL: Notify the engine of the new window dimensions! Otherwise viddef.width and viddef.height remain 0 and nothing renders!
    ri.Vid_NewWindow(*pwidth, *pheight);

    return rserr_ok;
}

void SWimp_AppActivate(qboolean active) {
    (void)active;
}

