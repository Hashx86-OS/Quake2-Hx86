#include "engine/client/client.h"
#include <Hx86/Hx86.h>

static uint32_t *framebuffer = 0;
static int fb_width = 640;
static int fb_height = 480;
static InputState cachedInput;
static int inputCached = 0;
static int scancode_to_q2[128];

cvar_t *vid_gamma;
cvar_t *vid_ref;
cvar_t *vid_xpos;
cvar_t *vid_ypos;
cvar_t *vid_fullscreen;

viddef_t viddef;
qboolean reflib_active = false;

extern refexport_t re;

extern refexport_t GetRefAPI(refimport_t rimp);

typedef struct vidmode_s {
    const char *description;
    int width, height;
    int mode;
} vidmode_t;

vidmode_t vid_modes[] = {
    { "Mode 0: 320x240",   320, 240,   0 },
    { "Mode 1: 400x300",   400, 300,   1 },
    { "Mode 2: 512x384",   512, 384,   2 },
    { "Mode 3: 640x480",   640, 480,   3 },
    { "Mode 4: 800x600",   800, 600,   4 },
    { "Mode 5: 960x720",   960, 720,   5 },
    { "Mode 6: 1024x768",  1024, 768,  6 },
    { "Mode 7: 1152x864",  1152, 864,  7 },
    { "Mode 8: 1280x1024", 1280, 1024, 8 },
    { "Mode 9: 1600x1200", 1600, 1200, 9 }
};

#define VID_NUM_MODES (sizeof(vid_modes) / sizeof(vid_modes[0]))

static void BuildKeyMap(void) {
    for (int i = 0; i < 128; i++)
        scancode_to_q2[i] = -1;

    scancode_to_q2[0] = -1;
    scancode_to_q2[1] = 27;
    for (int k = 2; k <= 11; k++)
        scancode_to_q2[k] = '0' + (11 - k);
    scancode_to_q2[12] = '-';
    scancode_to_q2[13] = '=';
    scancode_to_q2[14] = 127;
    scancode_to_q2[15] = 9;
    scancode_to_q2[16] = 'q';
    scancode_to_q2[17] = 'w';
    scancode_to_q2[18] = 'e';
    scancode_to_q2[19] = 'r';
    scancode_to_q2[20] = 't';
    scancode_to_q2[21] = 'y';
    scancode_to_q2[22] = 'u';
    scancode_to_q2[23] = 'i';
    scancode_to_q2[24] = 'o';
    scancode_to_q2[25] = 'p';
    scancode_to_q2[26] = '[';
    scancode_to_q2[27] = ']';
    scancode_to_q2[28] = K_ENTER; // 13
    scancode_to_q2[29] = 133;
    scancode_to_q2[30] = 'a';
    scancode_to_q2[31] = 's';
    scancode_to_q2[32] = 'd';
    scancode_to_q2[33] = 'f';
    scancode_to_q2[34] = 'g';
    scancode_to_q2[35] = 'h';
    scancode_to_q2[36] = 'j';
    scancode_to_q2[37] = 'k';
    scancode_to_q2[38] = 'l';
    scancode_to_q2[39] = ';';
    scancode_to_q2[40] = '\'';
    scancode_to_q2[41] = '`';
    scancode_to_q2[42] = 134;
    scancode_to_q2[43] = '\\';
    scancode_to_q2[44] = 'z';
    scancode_to_q2[45] = 'x';
    scancode_to_q2[46] = 'c';
    scancode_to_q2[47] = 'v';
    scancode_to_q2[48] = 'b';
    scancode_to_q2[49] = 'n';
    scancode_to_q2[50] = 'm';
    scancode_to_q2[51] = ',';
    scancode_to_q2[52] = '.';
    scancode_to_q2[53] = '/';
    scancode_to_q2[54] = 134;
    scancode_to_q2[55] = '*';
    scancode_to_q2[56] = 132;
    scancode_to_q2[57] = ' ';
    scancode_to_q2[58] = 135;
    scancode_to_q2[59] = 136;
    scancode_to_q2[60] = 137;
    scancode_to_q2[61] = 138;
    scancode_to_q2[62] = 139;
    scancode_to_q2[63] = 140;
    scancode_to_q2[64] = 141;
    scancode_to_q2[65] = 142;
    scancode_to_q2[66] = 143;
    scancode_to_q2[67] = 144;
    scancode_to_q2[68] = 145;
    scancode_to_q2[69] = 146;
    scancode_to_q2[70] = 147;
    scancode_to_q2[71] = 148;
    scancode_to_q2[72] = 128; // Up arrow (ext scancode 0x48)
    scancode_to_q2[73] = 151;
    scancode_to_q2[74] = 128;
    scancode_to_q2[75] = 130; // Left arrow (ext scancode 0x4B)
    scancode_to_q2[76] = 152;
    scancode_to_q2[77] = 131; // Right arrow (ext scancode 0x4D)
    scancode_to_q2[78] = 130;
    scancode_to_q2[79] = 131;
    scancode_to_q2[80] = 129; // Down arrow (ext scancode 0x50)
    scancode_to_q2[81] = 131;
    scancode_to_q2[82] = 131;
    scancode_to_q2[83] = 130;
}

#define MAXPRINTMSG 4096

void VID_Printf(int print_level, char *fmt, ...) {
    va_list argptr;
    char msg[MAXPRINTMSG];
    va_start(argptr, fmt);
    vsprintf(msg, fmt, argptr);
    va_end(argptr);
    if (print_level == PRINT_ALL)
        Com_Printf("%s", msg);
    else
        Com_DPrintf("%s", msg);
}

void VID_Error(int err_level, char *fmt, ...) {
    va_list argptr;
    char msg[MAXPRINTMSG];
    va_start(argptr, fmt);
    vsprintf(msg, fmt, argptr);
    va_end(argptr);
    Com_Error(err_level, "%s", msg);
}

void VID_Restart_f(void) {
    vid_ref->modified = true;
}

qboolean VID_GetModeInfo(int *width, int *height, int mode) {
    if (mode < 0 || mode >= VID_NUM_MODES)
        return false;
    *width  = vid_modes[mode].width;
    *height = vid_modes[mode].height;
    return true;
}

void VID_NewWindow(int width, int height) {
    viddef.width  = width;
    viddef.height = height;
}

void VID_FreeReflib(void) {
    memset(&re, 0, sizeof(re));
    reflib_active = false;
}

qboolean VID_LoadRefresh(char *name) {
    refimport_t ri;

    if (reflib_active) {
        re.Shutdown();
        VID_FreeReflib();
    }

    Com_Printf("------- Loading %s -------\n", "ref_soft (static)");

    memset(&ri, 0, sizeof(ri));
    ri.Sys_Error = VID_Error;
    ri.Cmd_AddCommand = Cmd_AddCommand;
    ri.Cmd_RemoveCommand = Cmd_RemoveCommand;
    ri.Cmd_Argc = Cmd_Argc;
    ri.Cmd_Argv = Cmd_Argv;
    ri.Cmd_ExecuteText = Cbuf_ExecuteText;
    ri.Con_Printf = VID_Printf;
    ri.FS_LoadFile = FS_LoadFile;
    ri.FS_FreeFile = FS_FreeFile;
    ri.FS_Gamedir = FS_Gamedir;
    ri.Cvar_Get = Cvar_Get;
    ri.Cvar_Set = Cvar_Set;
    ri.Cvar_SetValue = Cvar_SetValue;
    ri.Vid_GetModeInfo = VID_GetModeInfo;
    ri.Vid_MenuInit = NULL;
    ri.Vid_NewWindow = VID_NewWindow;

    re = GetRefAPI(ri);

    if (re.api_version != API_VERSION) {
        Com_Printf("ref_soft has wrong api version (%i, expected %i)\n", re.api_version, API_VERSION);
        return false;
    }

    if (!re.Init(NULL, NULL)) {
        re.Shutdown();
        return false;
    }

    Com_Printf("------------------------------------\n");
    reflib_active = true;
    return true;
}

void VID_CheckChanges(void) {
    if (vid_ref->modified) {
        char *name = vid_ref->string;
        if (!VID_LoadRefresh(name)) {
            if (name != vid_ref->string) {
                Cvar_Set("vid_ref", vid_ref->string);
            }
        }
        vid_ref->modified = false;
    }
}

void VID_Init(void) {
    FramebufferInfo fb = syscall_get_framebuffer();
    framebuffer = (uint32_t*)fb.buffer;
    fb_width = (int)fb.width;
    fb_height = (int)fb.height;

    printf("[Quake2] Framebuffer: %dx%d @ 0x%x\n", fb_width, fb_height, (unsigned int)fb.buffer);

    BuildKeyMap();

    vid_ref = Cvar_Get("vid_ref", "soft", CVAR_ARCHIVE);
    vid_xpos = Cvar_Get("vid_xpos", "0", CVAR_ARCHIVE);
    vid_ypos = Cvar_Get("vid_ypos", "0", CVAR_ARCHIVE);
    vid_fullscreen = Cvar_Get("vid_fullscreen", "0", CVAR_ARCHIVE);
    vid_gamma = Cvar_Get("vid_gamma", "1", CVAR_ARCHIVE);

    Cmd_AddCommand("vid_restart", VID_Restart_f);

    VID_LoadRefresh("ref_soft.so");
    if (vid_ref) vid_ref->modified = false;
}

void VID_Shutdown(void) {
    if (reflib_active) {
        re.Shutdown();
        VID_FreeReflib();
    }
}

void IN_Init(void) {
}

void IN_Shutdown(void) {
}

void Do_Key_Event(int key, qboolean down) {
    Key_Event(key, down, 0);
}

void IN_Commands(void) {
    if (!inputCached) { syscall_get_input(&cachedInput); inputCached = 1; }

    static int mouse_oldbuttonstate = 0;
    int mouse_buttonstate = cachedInput.mouseButtons;
    for (int i = 0; i < 3; i++) {
        if ((mouse_buttonstate & (1 << i)) && !(mouse_oldbuttonstate & (1 << i)))
            Key_Event(K_MOUSE1 + i, true, 0);
        if (!(mouse_buttonstate & (1 << i)) && (mouse_oldbuttonstate & (1 << i)))
            Key_Event(K_MOUSE1 + i, false, 0);
    }
    mouse_oldbuttonstate = mouse_buttonstate;
}

void IN_Move(usercmd_t *cmd) {
    if (!inputCached) { syscall_get_input(&cachedInput); inputCached = 1; }

    int mx = cachedInput.mouseDX;
    int my = cachedInput.mouseDY;

    if (!mx && !my)
        return;

    float mx_f = (float)mx * sensitivity->value;
    float my_f = (float)my * sensitivity->value;

    // Mouse X → yaw (look left/right), or sidemove if strafe held
    if (in_strafe.state & 1)
        cmd->sidemove += m_side->value * mx_f;
    else
        cl.viewangles[YAW] -= m_yaw->value * mx_f;

    // Mouse Y → pitch (look up/down) if freelook, or forwardmove
    if (freelook->value)
        cl.viewangles[PITCH] += m_pitch->value * my_f;
    else
        cmd->forwardmove -= m_forward->value * my_f;

    // Clamp pitch
    if (cl.viewangles[PITCH] > 80)
        cl.viewangles[PITCH] = 80;
    if (cl.viewangles[PITCH] < -70)
        cl.viewangles[PITCH] = -70;
}

void IN_Frame(void) {
    if (!inputCached) { syscall_get_input(&cachedInput); inputCached = 1; }

    static uint8_t prevKeyStates[128];
    for (int i = 0; i < 128; i++) {
        if (cachedInput.keyStates[i] != prevKeyStates[i]) {
            int q2Key = scancode_to_q2[i];
            if (q2Key >= 0) {
                Key_Event(q2Key, cachedInput.keyStates[i], 0);
            }
        }
    }
    memcpy(prevKeyStates, cachedInput.keyStates, 128);
    inputCached = 0; // Clear cache so the next frame will poll input again
}

void IN_Activate(qboolean active) {
    (void)active;
}

void VID_MenuInit(void) {
}

const char *VID_MenuKey(int key) {
    (void)key;
    return NULL;
}

void VID_MenuDraw(void) {
}
